#ifndef MMM_TEXTUREINFO_HPP
#define MMM_TEXTUREINFO_HPP

#include <atomic>
#include <cstdint>
#include <glm/glm.hpp>

// 描述单个纹理的渲染信息
struct TextureInfo {
    // 它所属的图集数组的GPU ID
    uint32_t gl_texture_array_id;
    // 它在数组中的层索引
    uint32_t layer_index;
    // UV缩放
    glm::vec2 uv_scale;
    // UV偏移
    glm::vec2 uv_offset;
    // 原始尺寸
    glm::vec2 origin_size;
    // 是否是字符
    bool is_char{false};
};

// 描述一个图集数组
// (一个桶)
struct AtlasGroup {
    uint32_t gl_id{0};
    uint32_t bucket_width{0};
    uint32_t bucket_height{0};
    uint32_t layer_count{0};
    std::atomic<uint32_t> uploaded_layers{0};
};

#endif  // MMM_TEXTUREINFO_HPP
