#include "Atlas.h"

#include "../Texture.h"

// 构造Atlas
Atlas::Atlas(uint32_t width, uint32_t height)
    : atlas_width(width), atlas_height(height) {}

// 析构Atlas
Atlas::~Atlas() = default;

// 是否已满
bool Atlas::isfull() { return sub_textures.size() >= max_subimage_count; }

// 向纹理集添加纹理
bool Atlas::add_texture(std::shared_ptr<TextureInstace> &texture) {
    if (isfull()) return false;

    if (current_y + texture->height > atlas_height) return false;

    // 尝试当前行放置
    if (current_x + texture->width > atlas_width) {
        // 换行处理
        current_y += current_line_height;  // 使用当前行最高高度
        current_x = 0;
        current_line_height = 0;

        // 检查换行后是否有足够空间
        if (current_y + texture->height > atlas_height) {
            return false;
        }
    }

    // 检查当前行高度是否足够
    if (texture->height > current_line_height) {
        // 如果增加行高后超出图集高度，则失败
        if (current_y + texture->height > atlas_height) {
            return false;
        }
        current_line_height = texture->height;
    }

    // 成功添加映射
    texture->woffset = current_x;
    texture->hoffset = current_y;
    current_x += texture->width;
    texture->texture_id = sub_textures.size();
    sub_textures.emplace_back(texture);

    return true;
}
