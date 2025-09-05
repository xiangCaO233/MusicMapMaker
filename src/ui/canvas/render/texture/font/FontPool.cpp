#include <ft2build.h>

#include <QFile>
#include <render/texture/font/FontPool.hpp>
#include <util/glcheck.hpp>
#include <vector>
// 包含FreeType的头文件
#include FT_FREETYPE_H

// 构造FontPool
FontPool::FontPool(QOpenGLFunctions_4_1_Core* gl_functions)
    : glf(gl_functions) {
    // 查询硬件支持
    GLCALL(
        glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_texture_array_layers),
        glf);
}

// 析构FontPool
FontPool::~FontPool() {
    is_running = false;
    cv.notify_all();

    // clear负责释放GPU资源
    clear();
}

// 清理资源
void FontPool::clear() {
    std::lock_guard<std::mutex> lock(info_mutex);

    // 1. 清理GPU资源
    for (const auto& [id, group] : groups) {
        GLCALL(glf->glDeleteTextures(1, &group.gl_id), glf);
    }

    // 2. 清理所有CPU端的数据结构
    character_infos.clear();
    groups.clear();

    // 重置打包状态
    current_atlas_id = 0;
    current_layer_index = 0;
    cursor_x = 0;
    cursor_y = 0;
    current_line_height = 0;

    qDebug() << "FontPool: All resources cleared.";
}

// 载入字体
void FontPool::load_font(std::string_view font_path, bool is_qrc) {
    if (character_infos.contains(font_path)) {
        return;
    }

    std::vector<uint32_t> font_sizes{8, 12, 16, 24, 36, 48};
    // 为每个需要的字号启动一个异步加载任务
    for (const auto& font_size_to_load : font_sizes) {
        threadpool.enqueue([this, font_path_str = std::string(font_path),
                            is_qrc, font_size_to_load]() {
            if (!is_running) return;
            FT_Library thread_local_library;
            if (FT_Init_FreeType(&thread_local_library)) {
                qWarning() << "FontPool: Could not initialize FreeType library "
                              "in thread.";
                return;
            }
            // --- FreeType 工作线程逻辑 ---
            FT_Face face;
            bool success = false;

            // --- 根据 is_qrc 标志选择加载方式 ---
            if (is_qrc) {
                // --- 路径 A: 从Qt资源系统 (qrc) 加载 ---
                QFile font_file(QString::fromStdString(font_path_str));
                if (font_file.open(QIODevice::ReadOnly)) {
                    auto font_data = font_file.readAll();
                    font_file.close();

                    if (FT_New_Memory_Face(thread_local_library,
                                           reinterpret_cast<const FT_Byte*>(
                                               font_data.constData()),
                                           font_data.size(), 0,
                                           &face) ==
                        0) {  // FreeType成功时返回0
                        success = true;
                        // qDebug() << "读取qrc成功";
                    }
                }
            } else {
                // --- 路径 B: 从文件系统加载 ---
                if (FT_New_Face(thread_local_library, font_path_str.c_str(), 0,
                                &face) == 0) {  // FreeType成功时返回0
                    success = true;
                }
            }

            // --- 检查加载结果并继续 ---
            if (!success) {
                qWarning() << "FontPool: Failed to load font:"
                           << (is_qrc ? "(qrc) " : "(file) ")
                           << QString::fromStdString(font_path_str);
                return;
            } else {
                // qDebug() << "读取字体文件成功";
            }

            // 1. 设置像素大小
            FT_Set_Pixel_Sizes(face, 0, font_size_to_load);

            // 2. 遍历所有需要预渲染的字符 (cjkstr)
            for (char32_t c : cjkstr) {
                // 3. 加载字符字形
                if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                    qWarning() << "FontPool: Failed to load glyph for char:"
                               << (uint)c;
                    continue;
                } else {
                    // 成功加载了字形
                    // qDebug()
                    //     << "加载字形" << c << "[" <<
                    //     face->glyph->bitmap.width
                    //     << "x" << face->glyph->bitmap.rows << "]";
                }

                // FreeType渲染出的位图是8位的灰度图 (alpha-only)
                // 我们需要将它转换为RGBA格式以上传
                const auto bitmap = &face->glyph->bitmap;

                // 需要拷贝一份，FreeType的bitmap->buffer在下一次FT_Load_Char后会失效
                int width = bitmap->width;
                int height = bitmap->rows;
                auto* single_channel_data = new unsigned char[width * height];

                // 直接拷贝单通道数据
                // [修正] 逐行拷贝，正确处理源的pitch和目标的紧凑布局
                for (int y = 0; y < height; ++y) {
                    memcpy(
                        single_channel_data + y * width,  // 目标地址：紧凑布局
                        bitmap->buffer +
                            y * bitmap->pitch,  // 源地址：带pitch的布局
                        width                   // 拷贝数量：只有有效宽度
                    );
                }
                // for (int y = 0; y < height; ++y) {
                //     memcpy(single_channel_data + y * width,
                //            bitmap->buffer + y * bitmap->pitch, width);
                // }

                // 将加载好的数据放入待上传队列
                LoadedImageData imageData{face->family_name, font_size_to_load,
                                          c, width, height, single_channel_data,
                                          // 还需要传递 bearing 和 advance
                                          glm::ivec2(face->glyph->bitmap_left,
                                                     face->glyph->bitmap_top),
                                          (uint32_t)face->glyph->advance.x};
                {
                    std::lock_guard<std::mutex> lock(queue_mutex);
                    upload_queue.push(imageData);
                }
            }

            // 释放FreeType的face对象
            FT_Done_Face(face);
            // 通知主线程有数据了
            cv.notify_one();
        });
        need_update.store(true);
    }
}

void FontPool::processUploadQueue() {
    std::queue<LoadedImageData> to_process;

    // 将待处理任务快速移出，减少锁的持有时间
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        if (upload_queue.empty()) {
            need_update.store(false);
            return;
        }
        to_process.swap(upload_queue);
    }

    // 在主线程中逐个上传
    while (!to_process.empty()) {
        LoadedImageData& imageData = to_process.front();
        uploadToGpu(imageData);

        // 不再需要CPU上的原始像素数据了
        // 释放cpu端字符bitmap数据
        delete[] imageData.data;

        to_process.pop();
    }
}

void FontPool::uploadToGpu(const LoadedImageData& data) {
    // 如果没有任何字形数据，则跳过
    if (data.width == 0 || data.height == 0) {
        // 存储一个“空”字形的信息，这样下次就不会再尝试加载它
        // (这部分逻辑可以根据需要添加)
        return;
    }

    // --- 步骤 1: 检查是否需要创建新的图集数组 ---
    if (current_atlas_id == 0) {
        // 这是上传的第一个字形，需要创建第一个图集数组
        uint32_t new_atlas_id;
        GLCALL(glf->glGenTextures(1, &new_atlas_id), glf);
        GLCALL(glf->glBindTexture(GL_TEXTURE_2D_ARRAY, new_atlas_id), glf);
        GLCALL(glf->glPixelStorei(GL_UNPACK_ALIGNMENT, 1), glf);
        // 使用单通道格式 GL_R8
        GLCALL(glf->glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, LAYER_SIZE.x,
                                 LAYER_SIZE.y, max_texture_array_layers, 0,
                                 GL_RED, GL_UNSIGNED_BYTE, nullptr),
               glf);

        // 设置纹理参数
        GLCALL(glf->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER,
                                    GL_LINEAR),
               glf);
        GLCALL(glf->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER,
                                    GL_LINEAR),
               glf);
        GLCALL(glf->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,
                                    GL_CLAMP_TO_EDGE),
               glf);
        GLCALL(glf->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,
                                    GL_CLAMP_TO_EDGE),
               glf);

        current_atlas_id = new_atlas_id;

        // 记录这个新的图集组信息
        groups[new_atlas_id].gl_id = new_atlas_id;
        groups[new_atlas_id].bucket_width = LAYER_SIZE.x;
        groups[new_atlas_id].bucket_height = LAYER_SIZE.y;
    }

    // --- 步骤 2: 检查当前位置是否能放下新字形 (书架算法) ---
    if (cursor_x + data.width > LAYER_SIZE.x) {
        // 当前行放不下了，换行
        cursor_x = 0;
        cursor_y += current_line_height;
        current_line_height = 0;
    }

    if (cursor_y + data.height > LAYER_SIZE.y) {
        // 当前层也放不下了，切换到下一层
        current_layer_index++;
        cursor_x = 0;
        cursor_y = 0;
        current_line_height = 0;
    }

    if (current_layer_index >= max_texture_array_layers) {
        // 当前图集数组也满了！需要创建一个新的图集数组
        qWarning()
            << "FontPool: Texture array is full! Need to create a new one.";
        current_atlas_id = 0;  // 强制在下一轮重新创建
        // 重新尝试上传这个字形
        uploadToGpu(data);
        return;
    }

    // 上传字形位图到GPU
    GLCALL(glf->glBindTexture(GL_TEXTURE_2D_ARRAY, current_atlas_id), glf);
    // 上传数据时，源格式为 GL_RED
    // 因为数据源 (single_channel_data) 只有一个通道
    GLCALL(glf->glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, cursor_x, cursor_y,
                                current_layer_index, data.width, data.height, 1,
                                GL_RED, GL_UNSIGNED_BYTE, data.data),
           glf);

    // 计算并存储该字形的完整信息
    CharacterGlyph glyph;

    // 填充纹理信息
    glyph.c = data.c;
    glyph.character_texinfo.is_char = true;
    glyph.character_texinfo.gl_texture_array_id = current_atlas_id;
    glyph.character_texinfo.layer_index = current_layer_index;

    // 计算UV偏移和缩放
    glyph.character_texinfo.uv_offset =
        glm::vec2(cursor_x / LAYER_SIZE.x, cursor_y / LAYER_SIZE.y);
    glyph.character_texinfo.uv_scale =
        glm::vec2(data.width / LAYER_SIZE.x, data.height / LAYER_SIZE.y);
    glyph.character_texinfo.origin_size.x = data.width;
    glyph.character_texinfo.origin_size.y = data.height;

    // 填充字形度量信息
    glyph.bearing = data.bearing;
    glyph.xadvance = data.xadvance;
    // qDebug() << "字符" << glyph.c << "放在"
    //          << "[" << glyph.character_texinfo.uv_offset.x << ","
    //          << glyph.character_texinfo.uv_offset.y << "]";

    // 将最终的Glyph信息存入缓存map中
    {
        std::lock_guard<std::mutex> lock(info_mutex);

        // 获取或创建字体家族的map
        auto& size_map = character_infos[data.family];

        // 检查特定字号的CharacterPack是否存在
        auto it = size_map.find(data.size);
        if (it == size_map.end()) {
            // 如果不存在，创建一个新的CharacterPack并初始化它
            CharacterPack new_pack;
            new_pack.font_size = data.size;
            // 将新的pack插入map
            it = size_map.try_emplace(data.size, new_pack).first;
        }

        // 添加字形
        it->second.character_set[data.c] = glyph;
    }

    // 更新光标位置
    // 光标向右移动
    cursor_x += data.width + 2;
    // 更新当前行的高度（取本行所有字形的最大高度）
    if (data.height > current_line_height) {
        current_line_height = data.height + 2;
    }
}

std::optional<CharacterGlyph> FontPool::get(std::string_view family,
                                            size_t font_size,
                                            char32_t character) const {
    std::lock_guard<std::mutex> lock(info_mutex);

    auto family_it = character_infos.find(family.data());
    if (family_it != character_infos.end()) {
        auto size_it = family_it->second.find(font_size);
        if (size_it != family_it->second.end()) {
            auto char_it = size_it->second.character_set.find(character);
            if (char_it != size_it->second.character_set.end()) {
                return char_it->second;
            }
        }
    }
    return std::nullopt;
}
