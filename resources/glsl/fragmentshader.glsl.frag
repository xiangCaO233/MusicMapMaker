#version 450 core

// 当前绘制形状的边界矩形尺寸
in vec2 bound_size;
// 当前绘制形状的边界矩形圆角半径
in float radius;

// 渲染管线前传递来的颜色
in vec4 fill_color;

// 渲染管线前传递来的采样器数据
in float texture_policy;
in float texture_id;
in vec2 texture_uv;

// 纹理池相关数据
// 图集子纹理元数据结构
struct AtlasSubMeta {
  // 使用的纹理在图集中的x位置
  vec2 position;
  // 使用的纹理的尺寸
  vec2 size;
};
// 以上16b,4f

// 图集元数据结构
struct AtlasMeta {
  // 使用单独-图集方式时
  // 映射实际采样的纹理单元
  // 使用采样器数组-图集方式时
  // 映射的实际采样的纹理数组层级
  int unit_index;
  // 子纹理数量
  float sub_count;
  // 图集的总尺寸
  vec2 size;
  // 以上16b,4f

  // 子纹理元数据集
  AtlasSubMeta sub_metas[512];
};

// 是否使用纹理
uniform int use_texture;

// 当前纹理池存储的全部纹理图集对应的纹理图集元数据组
uniform sampler2DArray atlas_meta_buffer_array;
/*
每个AtlasMeta存储在纹理数组的一个layer中
每个layer尺寸为足够存储2052个float的2D纹理
使用RGBA32F格式(每个纹素4个float)
纹理尺寸:
2052 floats / 4 = 513纹素 →
32×16=512 + 1 →
使用32×17=544纹素
*/

// 纹理采样器数组
uniform sampler2DArray samplerarray;

// 纹理模式掩码
const uint MASK_EFFECT = 0xF000;
const uint MASK_COMPLEMENT = 0x0F00;
const uint MASK_ALIGN = 0x00F0;
const uint MASK_FILL = 0x000F;

// 特效
// 无特效
const uint NONE = 0x0000;
// 模糊
const uint BLUR = 0x1000;
// 灰度
const uint GRAYSCALE = 0x2000;
// 发光
const uint GLOWING = 0x3000;
// 半透明
const uint HALF_TRANSPARENT = 0x4000;

// 纹理补充模式
// 使用填充色
const uint FILL_COLOR = 0x0100;
// 重复贴图
const uint REPEAT_TEXTURE = 0x0200;

// 纹理对齐模式
// 对齐左下角
const uint ALIGN_LEFT_BOTTOM = 0x0010;
// 对齐右下角
const uint ALIGN_RIGHT_BOTTOM = 0x0020;
// 对齐左上角
const uint ALIGN_LEFT_TOP = 0x0030;
// 对齐右上角
const uint ALIGN_RIGHT_TOP = 0x0040;
// 对齐中心
const uint ALIGN_CENTER = 0x0050;

// 纹理填充模式
// 填充,直接塞入(比例不一致会变形)
const uint FILL = 0x0000;
// 缩放, 直接塞入(比例不一致会变形)
const uint KEEP = 0x0001;
// 裁切
// 裁切--比例不一致会保证不变形的前提下裁剪一部分
// 缩放并平铺
// (选择会导致丢失像素最少的一边为基准裁剪)
// 保证最大可视度
const uint SCALLING_AND_TILE = 0x0002;
// 缩放并裁切 (强制指定以宽为基准)
const uint SCALLING_BASE_WIDTH_AND_CUT = 0x0003;
// 缩放并裁切 (强制指定以高为基准)
const uint SCALLING_BASE_HEIGHT_AND_CUT = 0x0004;

// 渲染颜色结果(向后传输着色结果)
out vec4 FragColor;

// 纹理集头部数据
struct AtlasMetaHeader {
  int unit_index;
  float sub_count;
  vec2 size;
};

// 获取AtlasMeta头部信息
AtlasMetaHeader getAtlasMetaHeader(int atlasIndex) {
  // 第一个纹素-4f
  /*
   *int unit_index;
   *子纹理数量--索引:1
   *float sub_count;
   *图集的总尺寸--索引:2,3
   *vec2 size;
   */
  vec4 headerData =
      texelFetch(atlas_meta_buffer_array, ivec3(0, 0, atlasIndex), 0);

  AtlasMetaHeader header;
  header.unit_index = int(headerData.r);
  header.sub_count = headerData.g;
  header.size = headerData.ba;
  return header;
}

// 获取指定AtlasMeta中的子纹理数据
AtlasSubMeta getAtlasSubMeta(int atlasIndex, int subIndex) {
  ivec2 texCoord;
  texCoord.x = (subIndex + 1) % 32;  // +1跳过头部
  texCoord.y = (subIndex + 1) / 32;

  /*
   *使用的纹理在图集中的x位置
   *vec2 position;
   *使用的纹理的尺寸
   *vec2 size;
   */
  vec4 subData =
      texelFetch(atlas_meta_buffer_array, ivec3(texCoord, atlasIndex), 0);
  AtlasSubMeta meta;
  meta.position = subData.rg;
  meta.size = subData.ba;

  return meta;
}

void main() {
  // 采样颜色
  vec4 texture_color;
  // 效果选项
  uint texture_effect = uint(texture_policy) & MASK_EFFECT;

  // 未使用纹理--直接填充指定颜色
  if (use_texture == 0) {
    texture_color = fill_color / 255.0;
  } else {
    // 取出纹理模式
    uint texture_comolement_mode = uint(texture_policy) & MASK_COMPLEMENT;
    uint texture_align_mode = uint(texture_policy) & MASK_ALIGN;
    uint texture_fill_mode = uint(texture_policy) & MASK_FILL;

    // 计算最终uv
    vec2 final_uv;
    // 判断是否保持比例
    bool keepratio = (texture_fill_mode >= KEEP);

    // 取出纹理集索引和子纹理的集内索引
    int atlasIndex = (int(texture_id) >> 16) & 0xFF;
    int subIndex = int(texture_id) & 0xFFFF;

    // 区分是否保持原图比例
    if (keepratio) {
      vec2 texsize;
      // 获取绑定纹理集中子纹理的元数据--尺寸
      texsize = getAtlasSubMeta(atlasIndex, subIndex).size;

      // 计算归一化理论最终uv
      if (texture_fill_mode == KEEP) {
        // 保持原纹理尺寸-不缩放
        // 计算实际显示的UV范围（基于纹理尺寸和矩形尺寸的比例）
        vec2 uvScale = max(bound_size / texsize, 1.0);
        vec2 uvOffset = vec2(0.0);
        // 根据对齐方式计算偏移
        switch (texture_align_mode) {
          case ALIGN_LEFT_TOP:
            // 左上 - 无需偏移
            break;
          case ALIGN_LEFT_BOTTOM:
            // 左下
            uvOffset.y = 1.0 - uvScale.y;
            break;
          case ALIGN_RIGHT_TOP:
            // 右上
            uvOffset.x = 1.0 - uvScale.x;
            break;
          case ALIGN_RIGHT_BOTTOM:
            // 右下
            uvOffset = 1.0 - uvScale;
            break;
          case ALIGN_CENTER:
            // 中心
            uvOffset = (1.0 - uvScale) * 0.5;
            break;
        }
        // 应用最终UV坐标
        final_uv = uvOffset + texture_uv * uvScale;

      } else {
        // 不保持原纹理尺寸-缩放
        // 计算矩形和纹理的宽高比
        float rectAspect = bound_size.x / bound_size.y;
        float texAspect = texsize.x / texsize.y;
        // 计算UV缩放因子
        vec2 scale = vec2(1.0);
        if (texture_fill_mode == SCALLING_AND_TILE) {
          // 平铺模式-选能铺满矩形的比例缩放
          if (texAspect > rectAspect) {
            // 纹理比矩形宽
            scale.x = rectAspect / texAspect;
          } else {
            // 纹理比矩形高
            scale.y = texAspect / rectAspect;
          }
        } else {
          // 非平铺模式-按要求选比例缩放
          if (texture_fill_mode == SCALLING_BASE_HEIGHT_AND_CUT) {
            // 以矩形高为基准(高不变)
            scale.x = rectAspect / texAspect;
          } else if (texture_fill_mode == SCALLING_BASE_WIDTH_AND_CUT) {
            // 以矩形宽为基准(宽不变)
            scale.y = texAspect / rectAspect;
          }
        }
        // 根据基准点调整UV坐标
        vec2 anchorOffset = vec2(0.0);
        switch (texture_align_mode) {
          case ALIGN_LEFT_TOP:
            // 左上
            anchorOffset = vec2(0.0, 0.0);
            break;
          case ALIGN_LEFT_BOTTOM:
            // 左下
            anchorOffset = vec2(0.0, 1.0 - scale.y);
            break;
          case ALIGN_RIGHT_TOP:
            // 右上
            anchorOffset = vec2(1.0 - scale.x, 0.0);
            break;
          case ALIGN_RIGHT_BOTTOM:
            // 右下
            anchorOffset = vec2(1.0 - scale.x, 1.0 - scale.y);
            break;
          case ALIGN_CENTER:
            // 中心(0)
            anchorOffset = (1.0 - scale) * 0.5;
        }
        // 应用缩放和偏移到最终uv坐标
        final_uv = anchorOffset + texture_uv * scale;
      }

      // 将归一化final_uv再转化为纹理集内uv
      // 已计算过:final_uv
      // 子纹理尺寸:texsize

      // 获取纹理集头数据--纹理集的总尺寸
      vec2 atlas_size = getAtlasMetaHeader(atlasIndex).size;
      // 获取绑定纹理集中子纹理的元数据--纹理集内位置(左上角的像素偏移)
      vec2 sub_image_pos = getAtlasSubMeta(atlasIndex, subIndex).position;

      // FragColor = texture(samplerarray, vec3(final_uv, atlasIndex));
      // FragColor = vec4(atlas_size.x / 8192.0, 0, 0, 1);
      // return;

      // 转化为纹理集内uv
      // 计算子纹理在纹理集中的UV范围
      vec2 sub_uv_min = sub_image_pos / atlas_size;
      vec2 sub_uv_max = (sub_image_pos + texsize) / atlas_size;

      // 区分模式--重采样/应用填充色
      if (texture_comolement_mode == REPEAT_TEXTURE) {
        // 平铺模式 - 使用fract重复纹理
        final_uv = fract(final_uv);
      } else if (texture_comolement_mode == FILL_COLOR) {
        // 填充模式
        if (final_uv.x < 0.0 || final_uv.x > 1.0 || final_uv.y < 0.0 ||
            final_uv.y > 1.0) {
          // 超出部分使用填充色
          FragColor = fill_color / 255.0;
          return;
        } else {
          // 未超出部分使用纹理采样
          final_uv = clamp(final_uv, 0.0, 1.0);
        }
      }
      // 将uv映射到子纹理的UV空间
      final_uv = sub_uv_min + final_uv * (sub_uv_max - sub_uv_min);
    } else {
      // 不保持纹理比例--拉伸(可能变形)

      // TODO 实现不保持原图比例的纹理集uv计算
      // 获取纹理集头数据--纹理集的总尺寸
      vec2 atlas_size = getAtlasMetaHeader(atlasIndex).size;
      // 获取绑定纹理集中子纹理的元数据--纹理集内位置(左上角的像素偏移)
      AtlasSubMeta sub_image_meta = getAtlasSubMeta(atlasIndex, subIndex);
      // 转化为纹理集内uv
      // 计算子纹理在纹理集中的UV范围
      vec2 sub_uv_min = sub_image_meta.position / atlas_size;
      vec2 sub_uv_max =
          (sub_image_meta.position + sub_image_meta.size) / atlas_size;
      // 映射到子纹理的UV空间
      final_uv = sub_uv_min + texture_uv * (sub_uv_max - sub_uv_min);
    }
    // 区分纹理池类型
    // 2-使用同尺寸纹理采样器数组
    // 根据补充方式处理超出纹理范围的情况
    switch (texture_comolement_mode) {
      case REPEAT_TEXTURE: {
        // 重复纹理模式
        texture_color = texture(samplerarray, vec3(final_uv, atlasIndex));
        break;
      }
      case FILL_COLOR: {
        if (final_uv.x < 0.0 || final_uv.x > 1.0 || final_uv.y < 0.0 ||
            final_uv.y > 1.0) {
          // 超出部分使用填充色
          texture_color = fill_color / 255.0;
        } else {
          // 未超出部分使用纹理采样
          texture_color = texture(samplerarray, vec3(final_uv, atlasIndex));
        }
        break;
      }
    }
  }

  // ---- 圆角遮罩部分 ----
  vec2 pos = texture_uv * 2.0 - 1.0;
  float trueradius = mod(radius, 1.0);
  float rx = 1.0 - trueradius;
  float ry = 1.0 - trueradius;

  float dx = max(abs(pos.x) - rx, 0.0);
  float dy = max(abs(pos.y) - ry, 0.0);

  float dist = sqrt(dx * dx + dy * dy);
  float effectiveRadius = max(trueradius, 0.0001);
  float alpha = clamp(1.0 - dist / effectiveRadius, 0.0, 1.0);

  if (radius > 1) {
    // 启用淡入淡出
    texture_color = vec4(texture_color.rgb, texture_color.a * alpha);
  } else {
    // 禁用淡入淡出
    if (alpha == 0) {
      discard;
    }
  }

  // TODO(xiang 2025-04-16): 实现特效
  switch (texture_effect) {
    case NONE: {
      // 不使用特效
      FragColor = texture_color;
      break;
    }
    case BLUR: {
      // 实现模糊特效
      FragColor = texture_color;
      break;
    }
    case GRAYSCALE: {
      // 实现灰度处理
      FragColor = texture_color;
      break;
    }
    case GLOWING: {
      // 实现发光特效
      FragColor = texture_color;
      break;
    }
    case HALF_TRANSPARENT: {
      // 实现1/4半透明特效
      FragColor = vec4(texture_color.rgb, texture_color.a * 0.5);
      break;
    }
    default:
      FragColor = texture_color;
  }
}
