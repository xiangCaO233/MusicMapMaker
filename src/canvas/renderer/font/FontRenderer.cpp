#include "FontRenderer.h"

#include <freetype/freetype.h>

#include <memory>
#include <string>
#include <utility>

#include "../../../log/colorful-log.h"
#include "../../GLCanvas.h"

// 用于包装 OpenGL 调用并检查错误
#define GLCALL(func)                                       \
  func;                                                    \
  {                                                        \
    XLogger::glcalls++;                                    \
    GLenum error = cvs->glGetError();                      \
    if (error != GL_NO_ERROR) {                            \
      XERROR("在[" + std::string(#func) +                  \
             "]发生OpenGL错误: " + std::to_string(error)); \
    }                                                      \
  }

// 当前字形id
uint32_t FontRenderer::current_glyph_id = 0;
// ftlibrary库
FT_Library FontRenderer::ft;
// 每层最大尺寸
const uint32_t FontRenderer::layer_size;
// 最大层数
const uint32_t FontRenderer::layer_count;

// ftlibrary库加载标识
bool FontRenderer::is_ft_library_loaded = false;

// 字体池数量
int FontRenderer::frenderer_count = 0;

FontRenderer::FontRenderer(GLCanvas* canvas,
                           std::shared_ptr<Shader> font_shader)
    : AbstractRenderer(canvas, font_shader, -1, 4096) {
  font_shader->use();
  frenderer_count++;
  // 检查字体库是否已初始化
  if (!is_ft_library_loaded) {
    // @return:
    //   FreeType error code.  0~means success.
    // FreeType函数在出现错误时将返回一个非零的整数值
    if (FT_Init_FreeType(&ft)) {
      XCRITICAL("FreeType初始化失败");
      return;
    } else {
      is_ft_library_loaded = true;
      XINFO("FreeType初始化成功");
    }
  }
  // 生成字体渲染器的实例缓冲对象
  GLCALL(cvs->glGenBuffers(1, &fInstanceVBO));
  GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, fInstanceVBO));
  /*
   *layout(location = 2) in vec2 character_pos;
   *layout(location = 3) in vec2 character_size;
   *layout(location = 4) in float character_rotation;
   *layout(location = 5) in float character_texture_layer;
   *layout(location = 6) in vec4 character_color;
   *
   ****character_uv[7] 会占用连续的 locations 7~10
   *layout(location = 7) in vec2 character_uvs[4];
   */

  auto stride = (2 + 2 + 1 + 1 + 4 + 2 * 4) * sizeof(float);
  // 位置信息
  // 描述location2 缓冲位置0~1float为float类型数据--位置信息(用vec2接收)
  GLCALL(cvs->glEnableVertexAttribArray(2));
  GLCALL(cvs->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, nullptr));
  // 每个实例变化一次
  GLCALL(cvs->glVertexAttribDivisor(2, 1));

  // 尺寸信息
  // 描述location3 缓冲位置2~3float为float类型数据--尺寸信息(用vec2接收)
  GLCALL(cvs->glEnableVertexAttribArray(3));
  GLCALL(cvs->glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride,
                                    (void*)(2 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(3, 1));

  // 旋转角度
  // 描述location4 缓冲位置4~4float为float类型数据--旋转角度(用float接收)
  GLCALL(cvs->glEnableVertexAttribArray(4));
  GLCALL(cvs->glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride,
                                    (void*)(4 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(4, 1));

  // 纹理集层数索引
  // 描述location5 缓冲位置5~5float为float类型数据--纹理集层数索引(用float接收)
  GLCALL(cvs->glEnableVertexAttribArray(5));
  GLCALL(cvs->glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, stride,
                                    (void*)(5 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(5, 1));

  // 字符填充色
  // 描述location6 缓冲位置6~9float为vec4类型数据--文本填充色(用vec4接收)
  GLCALL(cvs->glEnableVertexAttribArray(6));
  GLCALL(cvs->glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride,
                                    (void*)(6 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(6, 1));

  // 字符纹理集uv
  for (int i = 0; i < 4; i++) {
    // 描述location7+i
    // 缓冲位置10+2*i ~ 10+2*i+1float为vec2类型数据-- uv(用vec2接收)
    GLCALL(cvs->glEnableVertexAttribArray(7 + i));
    GLCALL(cvs->glVertexAttribPointer(7 + i, 2, GL_FLOAT, GL_FALSE, stride,
                                      (void*)((10 + 2 * i) * sizeof(float))));
    GLCALL(cvs->glVertexAttribDivisor(7 + i, 1));
  }

  GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER, (max_shape_count * stride), nullptr,
                           GL_DYNAMIC_DRAW));

  // 创建纹理数组
  GLCALL(cvs->glGenTextures(1, &glyphs_texture_array));
  // 激活纹理单元
  GLCALL(cvs->glActiveTexture(GL_TEXTURE0 + 13));
  GLCALL(cvs->glBindTexture(GL_TEXTURE_2D_ARRAY, glyphs_texture_array));
#ifdef __APPLE__
#else
  // 分配存储空间--8位灰度图
  GLCALL(cvs->glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R8, layer_size,
                             layer_size, layer_count));
#endif  //__APPLE__
  GLCALL(cvs->glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

  // 设置纹理参数
  GLCALL(
      cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GLCALL(
      cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
  GLCALL(
      cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  GLCALL(
      cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  free_layers.emplace_back(current_max_layer_index);
  // 首次设置
  font_shader->unuse();
}

FontRenderer::~FontRenderer() {
  frenderer_count--;
  if (frenderer_count == 0 && is_ft_library_loaded) {
    // 纹理池已全部释放
    FT_Done_FreeType(ft);
    // 恢复标识
    is_ft_library_loaded = false;
  }
}

// 加载字体
int FontRenderer::load_font(const char* font_path) {
  // 载入字体
  FT_Face face;
  FT_Error ret = FT_New_Face(ft, font_path, 0, &face);
  auto it = ft_faces.try_emplace(face->family_name, std::move(face)).first;

  if (ret != 0) {
    XERROR("加载字体" + std::string(font_path) + "失败");
    return ret;
  } else {
    XINFO("加载字体" + std::string(font_path) + "成功");

    // 载入常用字符
    XINFO("正在生成ascii字符缓存");
    // 创建ascii字符串
    std::u32string asciistr(U"a");
    for (char32_t c = 32; c <= 127; c++) asciistr.append(1, c);

    // 创建cjk字符串
    // std::u16string cjkstr(u"");
    // for (char32_t c = u'\u4e00'; c <= u'\u5e00'; c++) cjkstr.append(1, c);

    // 检查载入
    check_u32string(asciistr, 48, it->second);
    // check_u32string(cjkstr, 48, it->second);

    XINFO("正在生成常用中文字符缓存");

    return ret;
  }
}

// 检查载入字符串
void FontRenderer::check_u32string(const std::u32string& str,
                                   uint32_t font_size, FT_Face& face) {
  // 获取字符包
  // 尝试创建字体包映射
  auto& pack = font_packs_mapping.try_emplace(face->family_name)
                   .first->second.try_emplace(font_size)
                   .first->second;
  pack.font_size = font_size;

  int32_t free_layer_index{-1};
  // 获取空闲层
  if (free_layers.empty()) {
    // 无空闲，新建层
    current_max_layer_index++;
    free_layers.emplace_back(current_max_layer_index);
    free_layer_index = current_max_layer_index;
  } else {
    // 有空闲，使用此层
    free_layer_index = free_layers.back();
  }
  bind();
  FT_Set_Pixel_Sizes(face, 0, font_size);
  // 当前写入位置
  int currentX = 0;
  int currentY = 0;
  int maxRowHeight = 0;
  for (const auto& c : str) {
    // 加载字符的字形
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
      XERROR("加载字符" + std::to_string(c) + "位图失败");
      continue;
    } else {
      CharacterGlyph character;
      FT_Bitmap& bitmap = face->glyph->bitmap;

      // 检查是否需要换行
      if (currentX + bitmap.width > layer_size) {
        currentX = 0;
        currentY += maxRowHeight;
        maxRowHeight = 0;
      }
      // 更新行高（考虑下伸部分）
      int glyphHeight = bitmap.rows;
      int glyphDescend =
          (face->glyph->metrics.height - face->glyph->metrics.horiBearingY) /
          64;
      maxRowHeight = std::max(maxRowHeight, glyphHeight + glyphDescend);

      // 上传字符位图到纹理

      GLCALL(cvs->glActiveTexture(GL_TEXTURE0 + 13));
      GLCALL(cvs->glBindTexture(GL_TEXTURE_2D_ARRAY, glyphs_texture_array));
      GLCALL(cvs->glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, currentX, currentY,
                                  free_layer_index, bitmap.width, bitmap.rows,
                                  1, GL_RED, GL_UNSIGNED_BYTE, bitmap.buffer));

      character.pos_in_atlas.setX(currentX);
      character.pos_in_atlas.setY(currentY);

      character.size.setWidth(bitmap.width);
      character.size.setHeight(bitmap.rows);

      character.glyph_id = free_layer_index;
      character.xadvance = face->glyph->advance.x;

      character.bearing.setX(face->glyph->metrics.horiBearingX / 64);
      character.bearing.setY(face->glyph->metrics.horiBearingY / 64);

      // 放入字符集
      pack.character_set.try_emplace(c, character);
      // 移动写入位置
      currentX += (bitmap.width + 2);
    }
  }

  unbind();
}

// 同步数据
void FontRenderer::synchronize_data(InstanceDataType data_type,
                                    size_t instance_index, void* data) {
  switch (data_type) {
    case InstanceDataType::POSITION: {
      auto pos = static_cast<QVector2D*>(data);
      if (position_data.empty() || position_data.size() <= instance_index) {
        // XWARN("添加位置数据");
        position_data.push_back(*pos +
                                QVector2D(current_character_position, 0));
        synchronize_update_mark(instance_index);
      } else {
        if (*pos != position_data.at(instance_index)) {
          // 位置数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此实例位置数据
          position_data[instance_index] =
              *pos + QVector2D(current_character_position, 0);
        }
      }
      break;
    }
    case InstanceDataType::ROTATION: {
      auto rotation = static_cast<float*>(data);
      if (rotation_data.empty() || rotation_data.size() <= instance_index) {
        // XWARN("添加角度数据");
        rotation_data.push_back(*rotation);
        synchronize_update_mark(instance_index);
      } else {
        if (*rotation != rotation_data.at(instance_index)) {
          // 角度数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此实例角度数据
          rotation_data[instance_index] = *rotation;
        }
      }
      break;
    }
    case InstanceDataType::FILL_COLOR: {
      auto fill_color = static_cast<QVector4D*>(data);
      if (fill_color_data.empty() || fill_color_data.size() <= instance_index) {
        // XWARN("添加填充颜色数据");
        fill_color_data.push_back(*fill_color);
        synchronize_update_mark(instance_index);
      } else {
        if (*fill_color != fill_color_data.at(instance_index)) {
          // 填充颜色数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此实例填充颜色数据
          fill_color_data[instance_index] = *fill_color;
        }
      }
      break;
    }
    case InstanceDataType::TEXT: {
      auto text = static_cast<Text*>(data);
      // TODO(xiang 2025-04-12): 查询size,uv,id
      bool query_failed{false};
      auto packs_it = font_packs_mapping.find(text->font_family);
      if (packs_it == font_packs_mapping.end()) {
        XCRITICAL("不存在字体[" + std::string(text->font_family) + "]");
        query_failed = true;
      }
      auto pack_it = packs_it->second.find(text->font_size);
      if (pack_it == packs_it->second.end()) {
        XCRITICAL("不存在[" + std::to_string(text->font_size) + "]号字体包");
        query_failed = true;
      }

      auto character_it = pack_it->second.character_set.find(text->character);
      if (character_it == pack_it->second.character_set.end()) {
        XCRITICAL("不存在[" +
                  std::string(reinterpret_cast<const char*>(&text->character)) +
                  "]字符数据");
        query_failed = true;
      }

      CharacterGlyph glyph;
      if (query_failed) {
        glyph.pos_in_atlas = {0, 0};
        glyph.size = {0, 0};
        glyph.glyph_id = 0;
      } else {
        glyph = character_it->second;
        /*
         *（单位：1/64像素）
         *（单位：1/64像素）
         *（单位：1/64像素）
         *（单位：1/64像素）
         *（单位：1/64像素）
         *（单位：1/64像素）
         *（单位：1/64像素）
         */
        current_character_position += glyph.xadvance / 64;
      }

      // id
      auto glyph_id = glyph.glyph_id;
      if (glyph_id_data.empty() || glyph_id_data.size() <= instance_index) {
        // XWARN("添加字符id数据");
        glyph_id_data.push_back(glyph_id);
        synchronize_update_mark(instance_index);
      } else {
        if (glyph_id != glyph_id_data.at(instance_index)) {
          // 字符id数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此实例字符id数据
          glyph_id_data[instance_index] = glyph_id;
        }
      }
      // size
      auto size = QVector2D(glyph.size.width(), glyph.size.height());
      if (size_data.empty() || size_data.size() <= instance_index) {
        // XWARN("添加尺寸数据");
        size_data.push_back(size);
        synchronize_update_mark(instance_index);
      } else {
        if (size != size_data.at(instance_index)) {
          // 尺寸数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此实例尺寸数据
          size_data[instance_index] = size;
        }
      }
      // 更新位置
      position_data[instance_index].setY(position_data[instance_index].y() -
                                         glyph.bearing.y());

      // uv
      CharacterUVSet uvset;
      uvset.p1.setX(float(glyph.pos_in_atlas.x()));
      uvset.p1.setY(float(glyph.pos_in_atlas.y()));
      uvset.p2.setX(float(glyph.pos_in_atlas.x() + glyph.size.width()));
      uvset.p2.setY(float(glyph.pos_in_atlas.y()));
      uvset.p3.setX(float(glyph.pos_in_atlas.x() + glyph.size.width()));
      uvset.p3.setY(float(glyph.pos_in_atlas.y() + glyph.size.height()));
      uvset.p4.setX(float(glyph.pos_in_atlas.x()));
      uvset.p4.setY(float(glyph.pos_in_atlas.y() + glyph.size.height()));

      if (uvset_data.empty() || uvset_data.size() <= instance_index) {
        // XWARN("同步字符uv集数据");
        uvset_data.push_back(uvset);
        synchronize_update_mark(instance_index);
      } else {
        if (uvset != uvset_data.at(instance_index)) {
          // 字符uv集数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此实例字符uv集数据
          uvset_data[instance_index] = uvset;
        }
      }
      break;
    }
    default:
      break;
  }
}

// 同步更新位置标记
void FontRenderer::synchronize_update_mark(size_t instance_index) {
  // 同步更新标记
  if (update_list.empty()) {
    // 空,创建新的更新标记
    update_list.emplace_back(instance_index, 1);
  } else {
    // 非空
    // 确保不是同一对象的不同属性变化
    if (update_list.back().first != instance_index) {
      // 检查是否连续上一更新标记
      if (update_list.back().first + update_list.back().second ==
          instance_index) {
        // 连续--增加更新数量
        update_list.back().second++;
      } else {
        if (update_list.back().first + update_list.back().second - 1 !=
            instance_index) {
          // 确保未记录过
          // 不连续,创建新的更新标记
          update_list.emplace_back(instance_index, 1);
        }
      }
    }
  }
}

// 重置更新内容
void FontRenderer::reset_update() {
  update_list.clear();
  current_character_position = 0;
}
// 绑定渲染器
void FontRenderer::bind() {
  AbstractRenderer::bind();
  if (need_update_sampler_location) {
    // 需要更新
    shader->set_sampler("glyph_atlas_array", 13);
  }
}

// 更新gpu数据
void FontRenderer::update_gpu_memory() {
  // 绑定实例缓冲区
  GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, fInstanceVBO));

  for (const auto& [instance_start_index, instance_count] : update_list) {
    // 构建内存块
    std::vector<float> memory_block(instance_count *
                                    (2 + 2 + 1 + 1 + 4 + 2 * 4));
    for (int i = instance_start_index;
         i < instance_start_index + instance_count; i++) {
      //// 图形位置数据
      memory_block[(i - instance_start_index) * 18] = position_data[i].x();
      memory_block[(i - instance_start_index) * 18 + 1] = position_data[i].y();
      //// 图形尺寸
      memory_block[(i - instance_start_index) * 18 + 2] = size_data[i].x();
      memory_block[(i - instance_start_index) * 18 + 3] = size_data[i].y();
      //// 旋转角度
      memory_block[(i - instance_start_index) * 18 + 4] = rotation_data[i];
      //// 纹理集层数索引
      memory_block[(i - instance_start_index) * 18 + 5] = glyph_id_data[i];
      //// 填充颜色
      memory_block[(i - instance_start_index) * 18 + 6] =
          fill_color_data[i].x();
      memory_block[(i - instance_start_index) * 18 + 7] =
          fill_color_data[i].y();
      memory_block[(i - instance_start_index) * 18 + 8] =
          fill_color_data[i].z();
      memory_block[(i - instance_start_index) * 18 + 9] =
          fill_color_data[i].w();
      // 纹理uv集
      auto& uvset = uvset_data[i];
      memory_block[(i - instance_start_index) * 18 + 10] = uvset.p1.x();
      memory_block[(i - instance_start_index) * 18 + 11] = uvset.p1.y();

      memory_block[(i - instance_start_index) * 18 + 12] = uvset.p2.x();
      memory_block[(i - instance_start_index) * 18 + 13] = uvset.p2.y();

      memory_block[(i - instance_start_index) * 18 + 14] = uvset.p3.x();
      memory_block[(i - instance_start_index) * 18 + 15] = uvset.p3.y();

      memory_block[(i - instance_start_index) * 18 + 16] = uvset.p4.x();
      memory_block[(i - instance_start_index) * 18 + 17] = uvset.p4.y();
    }
    // 上传内存块到显存
    GLCALL(cvs->glBufferSubData(
        GL_ARRAY_BUFFER,
        (instance_start_index * ((2 + 2 + 1 + 1 + 4 + 2 * 4)) * sizeof(float)),
        memory_block.size() * sizeof(float), memory_block.data()));
  }
}
