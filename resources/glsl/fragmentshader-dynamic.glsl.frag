#version 450 core

// 渲染管线前传递来的颜色
in vec4 fill_color;

// 当前绘制形状的边界矩形尺寸
in vec2 bound_size;

// 渲染管线前传递来的采样器数据
in float texture_policy;
in float texture_id;
in vec2 texture_uv;

// 纹理池相关uniform

// 指定纹理池使用方式
// 0-不使用纹理
// 1-单独使用采样器
// 2-使用同尺寸纹理采样器数组
uniform int texture_pool_usage;

// 1-使用纹理图集
uniform int useatlas;

// 使用方式为单独或纹理图集
uniform sampler2D samplers[16];

// 使用方式为纹理采样器数组
uniform sampler2DArray samplerarray;
// 当前采样器数组的首纹理id偏移
// (使用时用texture_id-arraystartoffset)
// 结果就是使用的纹理在samplerarray的层级
uniform int arraystartoffset;

// 纹理模式掩码
const int MASK_COMPLEMENT = 0x0F00;
const int MASK_ALIGN = 0x00F0;
const int MASK_FILL = 0x000F;

// 纹理补充模式
// 使用填充色
const int FILL_COLOR = 0x0100;
// 重复贴图
const int REPEAT_TEXTURE = 0x0200;

// 纹理对齐模式
// 对齐左下角
const int ALIGN_LEFT_BOTTOM = 0x0010;
// 对齐右下角
const int ALIGN_RIGHT_BOTTOM = 0x0020;
// 对齐左上角
const int ALIGN_LEFT_TOP = 0x0030;
// 对齐右上角
const int ALIGN_RIGHT_TOP = 0x0040;
// 对齐中心
const int ALIGN_CENTER = 0x0050;

// 纹理填充模式
// 填充,直接塞入(比例不一致会变形)
const int FILL = 0x0000;
// 缩放, 直接塞入(比例不一致会变形)
const int KEEP = 0x0001;
// 裁切
// 裁切--比例不一致会保证不变形的前提下裁剪一部分
// 缩放并平铺
// (选择会导致丢失像素最少的一边为基准裁剪)
// 保证最大可视度
const int SCALLING_AND_TILE = 0x0002;
// 缩放并裁切 (强制指定以宽为基准)
const int SCALLING_BASE_WIDTH_AND_CUT = 0x0003;
// 缩放并裁切 (强制指定以高为基准)
const int SCALLING_BASE_HEIGHT_AND_CUT = 0x0004;

// 渲染颜色结果(向后传输着色结果)
out vec4 FragColor;

void main() {
  // 取出纹理模式
  int texture_comolement_mode = int(texture_policy) & MASK_COMPLEMENT;
  int texture_align_mode = int(texture_policy) & MASK_ALIGN;
  int texture_fill_mode = int(texture_policy) & MASK_FILL;

  // 计算最终uv
  vec2 final_uv;
  // 判断是否保持比例
  bool keepratio = false;
  if (texture_fill_mode >= KEEP) {
    keepratio = true;
  }
  if (useatlas == 1) {
    // 使用纹理集
    // TODO 实现纹理集uv计算
  } else {
    // 直接使用纹理
    if (keepratio) {
      // 保持纹理比例
      vec2 texsize = textureSize(samplers[int(texture_id) % 16], 0);
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
    } else {
      // 不保持纹理比例
      // 直接使用原uv
      final_uv = texture_uv;
    }
  }

  // 区分纹理池类型
  switch (texture_pool_usage) {
    case 0: {
      // 不使用纹理池
      // 直接使用填充颜色
      FragColor = fill_color;
      break;
    };
    case 1: {
      // 根据补充方式处理超出纹理范围的情况
      switch (texture_comolement_mode) {
        case REPEAT_TEXTURE: {
          // 重复纹理模式
          FragColor = texture(samplers[int(texture_id) % 16], final_uv);
          break;
        }
        case FILL_COLOR: {
          if (final_uv.x < 0.0 || final_uv.x > 1.0 || final_uv.y < 0.0 ||
              final_uv.y > 1.0) {
            // 超出部分使用填充色
            FragColor = fill_color;
          } else {
            // 未超出部分使用纹理采样
            FragColor = texture(samplers[int(texture_id) % 16], final_uv);
          }
          break;
        }
      }
      break;
    }
    case 2: {
      // 2-使用同尺寸纹理采样器数组
      if (useatlas == 1) {
        // 使用纹理集
        // TODO 实现纹理集数组采样
      } else {
      }
      break;
    }
  }
}