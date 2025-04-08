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

// 渲染颜色结果(向后传输着色结果)
out vec4 FragColor;

void main() {
	// 取出纹理模式
	int texture_comolement_mode = int(texture_policy) & MASK_COMPLEMENT;
	int texture_align_mode = int(texture_policy) & MASK_ALIGN;
	int texture_fill_mode = int(texture_policy) & MASK_FILL;
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
