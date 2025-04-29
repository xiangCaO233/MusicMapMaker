#include "MTexturePool.h"

#include <fmt/format.h>

#include <memory>
#include <string>
#include <vector>

#include "../../GLCanvas.h"
#include "colorful-log.h"

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

// 驱动信息
// 最大支持连续的采样器数
uint32_t MTexturePool::max_sampler_consecutive_count;

// 最大支持总采样器数
uint32_t MTexturePool::max_total_sampler_count;

// 最大采样器层数
uint32_t MTexturePool::max_texture_layer;

// cpu同步的显卡信息
// 着色器使用了的纹理数组
std::vector<uint32_t> MTexturePool::samplerarrays;

// 初始化驱动信息
void MTexturePool::init_driver_info(uint32_t texture_layer,
                                    uint32_t total_sampler_count,
                                    uint32_t sampler_consecutive_count) {
  max_texture_layer = texture_layer;
  max_total_sampler_count = total_sampler_count;
  max_sampler_consecutive_count = sampler_consecutive_count;

  // 留一个给元数据
  for (int i = 0; i < sampler_consecutive_count - 1; i++) {
    // 每层初始化一个未使用采样器标识-1
    samplerarrays.emplace_back(-1);
  }
}
// 获取空闲纹理单元
size_t MTexturePool::get_free_sampler_unit() {
  for (int i = 0; i < samplerarrays.size(); i++) {
    if (samplerarrays[i] == -1) {
      // 空闲纹理单元
      return i;
    }
  }
  return -1;
}

// 构造MTexturePool
MTexturePool::MTexturePool(GLCanvas *canvas, uint32_t layer_width,
                           uint32_t layer_height)
    : cvs(canvas), layerw(layer_width), layerh(layer_height) {
  // 创建纹理数组
  GLCALL(cvs->glGenTextures(1, &current_gl_texture_array_handle));
  sampler_unit = get_free_sampler_unit();
  if (sampler_unit != -1) {
    // 激活纹理单元
    GLCALL(cvs->glActiveTexture(GL_TEXTURE0 + sampler_unit));
    // 绑定纹理到选中的纹理单元
    GLCALL(cvs->glBindTexture(GL_TEXTURE_2D_ARRAY,
                              current_gl_texture_array_handle));
    // glTexImage3D(GL_TEXTURE_3D,
    //              // mipmap 级别
    //              0,
    //              // 内部格式
    //              GL_RGBA, layer_width, layer_height, current_max_layer,
    //              // 必须为0
    //              0,
    //              // 数据格式
    //              GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
#ifdef __APPLE__
#else
    // 分配存储空间-先16层
    GLCALL(cvs->glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, layer_width,
                               layer_height, current_max_layer));
#endif  //__APPLE__

    // 设置纹理参数
    GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER,
                                GL_NEAREST));
    GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER,
                                GL_NEAREST));
    GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,
                                GL_REPEAT));
    GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,
                                GL_REPEAT));

    // 新建层对应的元数据组
    GLCALL(cvs->glGenTextures(1, &atlasMetaTextureArray));
    // 激活最后一个纹理单元
    GLCALL(
        cvs->glActiveTexture(GL_TEXTURE0 + max_sampler_consecutive_count - 1));
    // 绑定元数据采样器到最后一个纹理单元
    GLCALL(cvs->glBindTexture(GL_TEXTURE_2D_ARRAY, atlasMetaTextureArray));

    /*
    图集子纹理元数据
    struct AtlasSubMeta {
      使用的纹理在图集中的x位置
      vec2 position;
      使用的纹理的尺寸
      vec2 size;
    };

    图集元数据
    struct AtlasMeta {
      使用单独-图集方式时
      映射实际采样的纹理单元
      使用采样器数组-图集方式时
      映射的实际采样的纹理数组层级--索引:0
      int unit_index;
      子纹理数量--索引:1
      float sub_count;
      图集的总尺寸--索引:2,3
      vec2 size;
      子纹理元数据集--索引头:4
      AtlasSubMeta sub_metas[512];
    };
    */
    /*
    每个AtlasMeta存储在纹理数组的一个layer中
    每个layer尺寸为足够存储2052个float的2D纹理
    使用RGBA32F格式(每个纹素4个float)
    纹理尺寸:
    2052 floats / 4 = 513纹素 →
    32×16=512 + 1 →
    使用32×17=544纹素
    */
#ifdef __APPLE__
#else
    // 创建32宽×17高×N层的RGBA32F纹理数组
    GLCALL(cvs->glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, 32, 17,
                             current_max_layer, 0, GL_RGBA, GL_FLOAT, nullptr));
#endif  //__APPLE__

    // 设置纹理参数
    GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER,
                                GL_NEAREST));
    GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER,
                                GL_NEAREST));
    GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,
                                GL_REPEAT));
    GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,
                                GL_REPEAT));
  }

  // 上传每层的纹理集头数据---等效纹素1x1
  for (int i = 0; i < current_max_layer; i++) {
    std::vector<float> atlas_meta_head = {float(i), 512.0f, float(layer_width),
                                          float(layer_height)};
    GLCALL(cvs->glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, 1, 1, 1,
                                GL_RGBA, GL_FLOAT, atlas_meta_head.data()));
  }
}

// 析构MTexturePool
MTexturePool::~MTexturePool() = default;

// 判满
bool MTexturePool::isfull() { return layers.size() > current_max_layer; }

void draw_texture_debug_border(uint8_t *data, int width, int height,
                               int stride = 4) {
  auto set_pixel = [&](int x, int y, uint8_t r, uint8_t g, uint8_t b,
                       uint8_t a = 255) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    int index = (y * width + x) * stride;
    data[index + 0] = r;
    data[index + 1] = g;
    data[index + 2] = b;
    data[index + 3] = a;
  };

  // 上下边框
  for (int x = 0; x < width; ++x) {
    set_pixel(x, 0, 255, 0, 0);           // 顶部红线
    set_pixel(x, height - 1, 0, 255, 0);  // 底部绿线
  }

  // 左右边框
  for (int y = 0; y < height; ++y) {
    set_pixel(0, y, 0, 0, 255);            // 左侧蓝线
    set_pixel(width - 1, y, 255, 255, 0);  // 右侧黄线
  }
}

// 添加纹理
bool MTexturePool::load_texture(std::shared_ptr<TextureInstace> &texture) {
  bool need_new_layer{true};
  std::shared_ptr<Atlas> atlas;
  uint32_t layer_index;
  if (!layers.empty()) {
    for (int i = 0; i < layers.size(); i++) {
      if (!layers[i]->isfull()) {
        // 有空闲层,不必新建层
        need_new_layer = false;
        atlas = layers[i];
        layer_index = i;
      }
    }
  }

  // 执行新建
  if (need_new_layer) {
    // 新建层
    atlas = std::make_shared<Atlas>(layerw, layerh);
    layers.emplace_back(atlas);
    layer_index = layers.size() - 1;
  }
  if (texture->width == -1) return false;

  // 载入纹理到纹理集
  auto res = atlas->add_texture(texture);
  if (!res) {
    // 放不下
    auto newatlas = std::make_shared<Atlas>(layerw, layerh);
    layers.emplace_back(newatlas);
    atlas = newatlas;
    layer_index = layers.size() - 1;

    res = atlas->add_texture(texture);
    if (!res) {
      XERROR("载入纹理[" + texture->name + "]失败");
      return false;
    }
  }
  texture->texture_layer = layer_index;

  // 按纹理打包结果上传到采样器对应位置
  // 激活纹理单元
  GLCALL(cvs->glActiveTexture(GL_TEXTURE0 + sampler_unit));
  // 绑定纹理到选中的纹理单元
  GLCALL(
      cvs->glBindTexture(GL_TEXTURE_2D_ARRAY, current_gl_texture_array_handle));
  // draw_texture_debug_border(texture->data, texture->width, texture->height);
  GLCALL(cvs->glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, texture->woffset,
                              texture->hoffset, layer_index, texture->width,
                              texture->height, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                              texture->data));
  // 更新纹理id--符合层索引和子纹理id
  texture->texture_id = (layer_index << 16) | texture->texture_id;
  // XINFO("纹理:[" + std::to_string(texture->texture_id) + "]" + texture->name
  // +
  //       "已上传到gpu");

  // 上传纹理元数据到元数据集采样器对应位置

  // 使用RGBA32F格式(每个纹素4个float)
  // 图集子纹理元数据布局

  // OpenGL 默认假设纹理数据在内存中是 按行连续排列 的。例如，一个 2×2 的
  // GL_RGBA 纹理，内存布局如下：
  // 纹素0：R0,G0,B0,A0, 纹素1：R1,G1,B1,A1,  // 第一行
  // 纹素2：R2,G2,B2,A2, 纹素3：R3,G3,B3,A3   // 第二行......

  // 图集元数据布局
  // struct AtlasMeta {
  //   使用单独-图集方式时
  //   映射实际采样的纹理单元
  //   使用采样器数组-图集方式时
  //   映射的实际采样的纹理数组层级--索引:0
  //   int unit_index;
  //   子纹理数量--索引:1
  //   float sub_count;
  //   图集的总尺寸--索引:2,3
  //   vec2 size;
  // 以上为固定纹素1
  // 首先x偏移int unit_index,float sub_count,vec2 size
  // 共4浮点,1纹素
  //
  //   子纹理元数据集--索引头:4
  //   AtlasSubMeta sub_metas[512];
  // };
  // 总513纹素
  // 实际创建32宽×17高×N层的RGBA32F纹理数组
  // struct AtlasSubMeta {
  //   使用的纹理在图集中的x位置
  //   vec2 position;
  //   使用的纹理的尺寸
  //   vec2 size;
  // };
  // 每个子纹理元数据为：
  // vec2 position,vec2 size
  // 共2vec2,4浮点，1纹素
  // 当前纹理在纹理集的索引为：(刚刚添加到子纹理列表最后)
  // auto subindex = atlas->sub_textures.size() - 1
  // 当前纹理的xoffset为------------>
  // (subindex + 1) % 32;
  // 当前纹理的yoffset为------------>
  // (subindex + 1) / 32;
  // 创建元数据
  std::vector<float> metadata = {float(texture->woffset),
                                 float(texture->hoffset), float(texture->width),
                                 float(texture->height)};

  // 上传子纹理元数据---等效纹素1x1
  auto subindex = atlas->sub_textures.size() - 1;
  // 激活最后一个纹理单元
  GLCALL(cvs->glActiveTexture(GL_TEXTURE0 + max_sampler_consecutive_count - 1));
  // 绑定元数据采样器到最后一个纹理单元
  GLCALL(cvs->glBindTexture(GL_TEXTURE_2D_ARRAY, atlasMetaTextureArray));
  GLCALL(cvs->glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, (subindex + 1) % 32,
                              (subindex + 1) / 32, layer_index, 1, 1, 1,
                              GL_RGBA, GL_FLOAT, metadata.data()));
  XINFO("纹理元数据:[atlaslayer:" + std::to_string(layer_index) +
        ",woff:" + std::to_string(texture->woffset) +
        ",hoff:" + std::to_string(texture->hoffset) +
        ",w:" + std::to_string(texture->width) + ",h:" +
        std::to_string(texture->height) + "]" + texture->name + "添加到纹理集");

  // 成功添加映射
  texture_map.try_emplace(texture->name, texture);
  need_update_sampler_location = true;
  return true;
}

// 使用纹理池
void MTexturePool::use(const std::shared_ptr<MTexturePool> &pool_reference,
                       std::shared_ptr<AbstractRenderer> &renderer_context) {
  bool need_update{false};
  // 绑定纹理池
  if (renderer_context->current_use_pool != pool_reference) {
    // 纹理池变了需要更新
    need_update = true;
  }
  if (need_update_sampler_location) {
    // 尺寸变了,sampler的unoform 需要更新
    need_update = true;
    need_update_sampler_location = false;
  }

  if (need_update) {
    // 绑定纹理采样器数组到当前选中的纹理采样器单元
    // 激活纹理单元
    renderer_context->current_use_pool = pool_reference;
    GLCALL(cvs->glActiveTexture(GL_TEXTURE0 + sampler_unit));
    // 绑定纹理到选中的纹理单元
    GLCALL(cvs->glBindTexture(GL_TEXTURE_2D_ARRAY,
                              current_gl_texture_array_handle));
    // 更新sampler uniform
    renderer_context->shader->set_sampler("samplerarray", sampler_unit);

    // 更新纹理集meta数据
    GLCALL(
        cvs->glActiveTexture(GL_TEXTURE0 + max_sampler_consecutive_count - 1));
    // 绑定纹理到选中的纹理单元
    GLCALL(cvs->glBindTexture(GL_TEXTURE_2D_ARRAY, atlasMetaTextureArray));
    // 更新sampler uniform
    renderer_context->shader->set_sampler("atlas_meta_buffer_array",
                                          max_sampler_consecutive_count - 1);
  }
}
