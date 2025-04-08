#version 450 core

// 渲染管线前传递来的颜色
in vec4 fill_color;

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

// 使用纹理数组时的采样器数组偏移地址
uniform int texture_array_offset;

// 纹理元数据
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

// 渲染颜色结果(向后传输着色结果)
out vec4 FragColor;

void main() {
  switch (texture_pool_usage) {
    case 0: {
      FragColor = fill_color;
      break;
    };
    case 1: {
      // 使用单独采样器
      if (useatlas == 1) {
        // 使用纹理集
      } else {
        // 直接使用纹理
        FragColor = texture(samplers[int(texture_id) % 16], texture_uv);
        // FragColor = texture(samplers[8], texture_uv);
        // FragColor = vec4(texture_id / 100.0, 0, 0, 1);
      }
      break;
    }
  }
}
