#version 450 core

// 投影矩阵
uniform mat4 projection_mat;
uniform float texture_pool_type;

// 顶点数据
layout(location = 0) in vec3 vpos;
layout(location = 1) in vec2 vuv;

// 矩形数据
layout(location = 2) in vec2 shape_pos;
layout(location = 3) in vec2 shape_size;
layout(location = 4) in float shape_rotation;
layout(location = 5) in float shape_texture_policy;
layout(location = 6) in float shape_texture_id;
layout(location = 7) in vec4 shape_fillcolor;

// 1-使用纹理图集
uniform int useatlas;

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
// 缩放, 直接塞入(比例不一致会变形)
const int FILL = 0x0001;
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
// 缩放并保持比例，留出一部分空白保证不变形放下完整图形
const int SCALLING_AND_KEEP_RATIO = 0x0005;

// 纹理集元数据
struct TextureMeta {
  float woffset;
  float hoffset;
  float width;
  float height;
};

// 定义 UBO，大小固定为最大纹理集子纹理数
layout(std140) uniform TextureMetaBuffer {
  float atlas_width;
  float atlas_height;
  int sub_image_count;
  TextureMeta textureMetas[1024];
};

// 采样器数据
// 填充颜色
out vec4 fill_color;
// 图形纹理填充策略
out float texture_policy;
// 图形纹理id
out float texture_id;
// 预处理图形纹理uv
out vec2 texture_uv;
// 当前绘制形状的边界矩形尺寸
out vec2 bound_size;

void main() {
  // 缩放矩形到指定大小
  vec2 scaled_pos = vpos.xy * shape_size * 0.5;

  // 旋转矩形
  float angle_rad = radians(shape_rotation);
  float cos_angle = cos(angle_rad);
  float sin_angle = sin(angle_rad);
  mat2 rotation_matrix = mat2(cos_angle, -sin_angle, sin_angle, cos_angle);
  vec2 rotated_pos = rotation_matrix * scaled_pos;

  // 传递数据
  texture_id = shape_texture_id;
  texture_policy = shape_texture_policy;
  fill_color = shape_fillcolor;
  bound_size = shape_size;

  // 平移到指定位置
  vec2 final_pos = rotated_pos + shape_pos + shape_size / 2;

  // 应用视图和投影矩阵
  vec4 glpos = projection_mat * vec4(final_pos, 0.0, 1.0);
  gl_Position = vec4(glpos.x - 1.0, glpos.y + 1.0, glpos.zw);

  // 取出模式
  int texture_comolement_mode = int(shape_texture_policy) & MASK_COMPLEMENT;
  int texture_align_mode = int(shape_texture_policy) & MASK_ALIGN;
  int texture_fill_mode = int(shape_texture_policy) & MASK_FILL;

  // TODO 实现纹理预处理
  if (useatlas != 1) {
    // 不使用纹理集
    switch (texture_fill_mode) {
      case FILL: {
        break;
      }
      case SCALLING_AND_TILE: {
        break;
      }
    }
  } else {
    // 使用纹理集
  }

  // 传递纹理uv
  texture_uv = vuv;
}
