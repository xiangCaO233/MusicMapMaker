#include <canvas/render/texture/TexturePool.hpp>
#include <filesystem>
#include <format>
#include <mutex>
#include <util/glcheck.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// 构造TexturePool
TexturePool::TexturePool(QOpenGLFunctions_4_1_Core* gl_functions)
    : glf(gl_functions) {
    // 查询最大支持多层纹理的最大层数
    // 查询硬件支持
    GLCALL_V(glf->glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS,
                                &max_texture_array_layers),
             glf);
    qDebug() << "多层纹理最大层数: "
             << std::to_string(max_texture_array_layers);

    // 查询纹理采样器最大连续数量
    int32_t max_fragment_samplers;
    GLCALL_V(
        glf->glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_fragment_samplers),
        glf);
    qDebug() << "纹理采样器最大连续数量: "
             << std::to_string(max_fragment_samplers);
    if (max_fragment_samplers > 16) {
        max_fragment_samplers = 16;
    }

    // 查询纹理采样器最大数量
    int32_t max_combined_samplers;
    GLCALL_V(glf->glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
                                &max_combined_samplers),
             glf);
    qDebug() << "纹理采样器最大数量: " << std::to_string(max_combined_samplers);
}

// 析构TexturePool
TexturePool::~TexturePool() {
    is_running = false;
    // 唤醒所有可能在等待的线程，让他们退出
    cv.notify_all();
    clear();
}

// 定义分箱尺寸集
// const std::vector<uint32_t> BUCKET_SIZES = {8,    16,   32,   64,    128,
//                                             256,  512,  1024, 2048,  3072,
//                                             4096, 6144, 8192, 12288, 16384};
const std::vector<uint32_t> BUCKET_SIZES = {512,  1024, 2048,  3072, 4096,
                                            6144, 8192, 12288, 16384};
// const std::vector<uint32_t> BUCKET_SIZES = {2048, 3072,  4096, 6144,
//                                             8192, 12288, 16384};

// 辅助函数：找到能容纳n的最小的2的幂次桶尺寸
static uint32_t getBucketSize(uint32_t n) {
    for (uint32_t size : BUCKET_SIZES) {
        if (n <= size) return size;
    }
    // 如果纹理尺寸超过预设的最大桶，则放入一个特殊的“超大”桶
    // 这里简单返回最大的桶，也可以做特殊处理
    return BUCKET_SIZES.back();
}

// TexturePool.cpp
void TexturePool::clear() {
    // 停止并等待所有正在进行的加载任务（如果有的话）
    // 这个实现比较简单，更复杂的需要更精细的线程同步
    // 确保在调用clear之前，所有异步任务已经完成或被取消

    std::lock_guard<std::mutex> lock(info_mutex);
    std::lock_guard<std::mutex> queue_lock(queue_mutex);

    // 清理GPU资源
    std::unordered_set<uint32_t> deleted_ids;
    for (const auto& [path, info] : texture_infos) {
        if (!deleted_ids.contains(info.gl_texture_array_id)) {
            GLCALL_V(glf->glDeleteTextures(1, &info.gl_texture_array_id), glf);
            deleted_ids.insert(info.gl_texture_array_id);
        }
    }

    // 清理所有CPU端的数据结构
    texture_infos.clear();

    // 清空待上传队列，以防万一
    std::queue<LoadedImageData> empty_queue;
    upload_queue.swap(empty_queue);

    qDebug() << "TexturePool: All resources cleared.";
}

// 其他线程调用来请求添加新路径纹理
void TexturePool::request_new_directory(const std::string& dir) {
    auto path = std::filesystem::path(dir);
    if (!std::filesystem::exists(path)) {
        qDebug() << "[" << dir << "] 不存在";
        return;
    } else {
        std::lock_guard<std::mutex> lock(rebuild_mutex);
        new_texture_paths.clear();
        for (const auto& [loaded_path, _] : texture_infos) {
            new_texture_paths.insert(loaded_path);
        }
        for (auto it = std::filesystem::recursive_directory_iterator(path);
             it != std::filesystem::recursive_directory_iterator(); ++it) {
            auto filename = it->path().generic_string();
            if (!new_texture_paths.contains(filename) &&
                (filename.ends_with("png") || filename.ends_with("jpg"))) {
                qDebug() << "查到需要加载的纹理[" << filename << "]";
                new_texture_paths.insert(filename);
            }
        }
        rebuild_requested = true;
        need_update.store(true);
    }
}

// 其他线程调用来请求添加新路径纹理
void TexturePool::request_remove_directory(const std::string& dir) {
    auto path = std::filesystem::path(dir);
    if (!std::filesystem::exists(path)) {
        qDebug() << "[" << dir << "] 不存在";
        return;
    } else {
        std::lock_guard<std::mutex> lock(rebuild_mutex);
        new_texture_paths.clear();
        for (const auto& [loaded_path, _] : texture_infos) {
            new_texture_paths.insert(loaded_path);
        }
        for (auto it = std::filesystem::recursive_directory_iterator(path);
             it != std::filesystem::recursive_directory_iterator(); ++it) {
            auto filename = it->path().generic_string();
            if (auto path_it = new_texture_paths.find(filename);
                path_it != new_texture_paths.end()) {
                new_texture_paths.erase(path_it);
            }
        }
        rebuild_requested = true;
        need_update.store(true);
    }
}

// 主渲染循环调用来检查并执行重建
void TexturePool::processUpdateDirRequest() {
    // 必须 在拥有激活GL上下文的线程中调用
    std::unordered_set<std::string, StringHash, std::equal_to<>> paths_to_load;

    {
        std::lock_guard<std::mutex> lock(rebuild_mutex);
        if (!rebuild_requested) {
            return;  // 没有请求，直接返回
        }
        paths_to_load = std::move(new_texture_paths);
        rebuild_requested = false;
    }

    // 安全地在渲染线程中执行重建
    buildFromManifest(paths_to_load);
}

// 移除一个路径的纹理
void TexturePool::remove_directory(const std::string& dir) {
    auto path = std::filesystem::path(dir);
    if (!std::filesystem::exists(path)) {
        qDebug() << "[" << dir << "] 不存在";
        return;
    } else {
        std::unordered_set<std::string, StringHash, std::equal_to<>> paths;
        for (const auto& [loaded_path, _] : texture_infos) {
            paths.insert(loaded_path);
        }
        for (auto it = std::filesystem::recursive_directory_iterator(path);
             it != std::filesystem::recursive_directory_iterator(); ++it) {
            auto filename = it->path().generic_string();
            if (auto path_it = paths.find(filename); path_it != paths.end()) {
                paths.erase(path_it);
            }
        }
        buildFromManifest(paths);
    }
}

// 从一个路径加载
void TexturePool::add_directory(const std::string& dir) {
    auto path = std::filesystem::path(dir);
    if (!std::filesystem::exists(path)) {
        qDebug() << "[" << dir << "] 不存在";
        return;
    } else {
        std::unordered_set<std::string, StringHash, std::equal_to<>> paths;
        for (const auto& [loaded_path, _] : texture_infos) {
            paths.insert(loaded_path);
        }
        for (auto it = std::filesystem::recursive_directory_iterator(path);
             it != std::filesystem::recursive_directory_iterator(); ++it) {
            auto filename = it->path().generic_string();
            if (!paths.contains(filename) && filename.ends_with("png") ||
                filename.ends_with("jpg")) {
                qDebug() << "查到需要加载的纹理[" << filename << "]";
                paths.insert(filename);
            }
        }
        buildFromManifest(paths);
    }
}

void TexturePool::buildFromManifest(
    const std::unordered_set<std::string, StringHash, std::equal_to<>>&
        texture_paths) {
    clear();

    // 扫描所有纹理并进行分箱
    // key: "Bucket_256x256", value: {path1, path2, ...}
    std::unordered_map<std::string, std::vector<std::string>, StringHash,
                       std::equal_to<>>
        buckets;

    int w;
    int h;
    int c;
    for (const auto& path : texture_paths) {
        if (stbi_info(path.c_str(), &w, &h, &c)) {
            uint32_t bucket_w = getBucketSize(w);
            uint32_t bucket_h = getBucketSize(h);
            std::string bucket_key =
                std::format("Bucket_{}x{}", bucket_w, bucket_h);
            buckets[bucket_key].push_back(path);
        } else {
            qWarning() << "TexturePool: stbi_info failed for path:"
                       << QString::fromStdString(path);
        }
    }

    // 为每个桶(可能分裂成多个部分)创建AtlasGroup和GPU资源
    // 局部变量，最后再移交
    std::unordered_map<std::string, AtlasGroup, StringHash, std::equal_to<>>
        atlas_groups;

    for (const auto& [bucket_key, paths_in_bucket] : buckets) {
        uint32_t bucket_w;
        uint32_t bucket_h;
        sscanf(bucket_key.c_str(), "Bucket_%ux%u", &bucket_w, &bucket_h);

        // 处理桶溢出，按max_layers_per_array分割
        for (size_t i = 0; i < paths_in_bucket.size();
             i += max_texture_array_layers) {
            auto chunk_begin = paths_in_bucket.begin() + i;
            auto chunk_end = paths_in_bucket.begin() +
                             std::min(i + (size_t)max_texture_array_layers,
                                      paths_in_bucket.size());
            std::vector<std::string> chunk_paths(chunk_begin, chunk_end);

            int part_index = i / max_texture_array_layers;
            std::string group_key =
                bucket_key + "_Part" + std::to_string(part_index);

            // 创建并配置AtlasGroup
            auto& group = atlas_groups[group_key];
            group.bucket_width = bucket_w;
            group.bucket_height = bucket_h;
            group.layer_count = chunk_paths.size();

            // 在主线程创建GPU资源
            GLCALL_V(glf->glGenTextures(1, &group.gl_id), glf);
            GLCALL_V(glf->glBindTexture(GL_TEXTURE_2D_ARRAY, group.gl_id), glf);
            GLCALL_V(glf->glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
                                       group.bucket_width, group.bucket_height,
                                       group.layer_count, 0, GL_RGBA,
                                       GL_UNSIGNED_BYTE, nullptr),
                     glf);

            // 设置纹理参数
            GLCALL_V(glf->glTexParameteri(GL_TEXTURE_2D_ARRAY,
                                          GL_TEXTURE_MIN_FILTER, GL_LINEAR),
                     glf);
            GLCALL_V(glf->glTexParameteri(GL_TEXTURE_2D_ARRAY,
                                          GL_TEXTURE_MAG_FILTER, GL_LINEAR),
                     glf);
            GLCALL_V(
                glf->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,
                                     GL_MIRRORED_REPEAT),
                glf);
            GLCALL_V(
                glf->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,
                                     GL_MIRRORED_REPEAT),
                glf);

            // 预计算TextureInfo并派发异步加载任务
            for (size_t layer_idx = 0; layer_idx < chunk_paths.size();
                 ++layer_idx) {
                const std::string& path = chunk_paths[layer_idx];

                stbi_info(path.c_str(), &w, &h, &c);

                TextureInfo info;
                info.gl_texture_array_id = group.gl_id;
                info.layer_index = layer_idx;
                info.uv_scale = glm::vec2((float)w / group.bucket_width,
                                          (float)h / group.bucket_height);
                info.uv_offset = glm::vec2(0.0f, 0.0f);

                info.origin_size = glm::vec2(w, h);
                info.group_size = {group.bucket_width, group.bucket_height};

                // 派发加载任务
                threadpool.enqueue([this, path, info]() {
                    if (!is_running) return;

                    int load_w;
                    int load_h;
                    int load_c;
                    unsigned char* data =
                        stbi_load(path.c_str(), &load_w, &load_h, &load_c, 4);

                    if (data) {
                        LoadedImageData image_data{path, load_w, load_h, 4,
                                                   data};

                        {
                            std::lock_guard<std::mutex> lock(queue_mutex);
                            upload_queue.push(image_data);
                            need_update.store(true);
                        }
                        // 在此之前，已经有了最终的渲染信息，所以先存起来
                        {
                            std::lock_guard<std::mutex> lock(info_mutex);
                            texture_infos[path] = info;
                        }
                        cv.notify_one();
                    } else {
                        qWarning() << "TexturePool: stbi_load failed for path:"
                                   << QString::fromStdString(path);
                    }
                });
            }
        }

        for (const auto& [_, group] : atlas_groups) {
            auto& internal_group =
                groups.try_emplace(group.gl_id).first->second;
            internal_group.gl_id = group.gl_id;
            internal_group.layer_count = group.layer_count;
            internal_group.bucket_width = group.bucket_width;
            internal_group.bucket_height = group.bucket_height;
            internal_group.uploaded_layers.store(group.uploaded_layers);
        }
    }
}

// 从主线程调用，处理已从磁盘加载完成的纹理，将其上传到GPU
void TexturePool::processUploadQueue() {
    std::queue<LoadedImageData> to_process;

    // 将待处理任务快速移出,减少锁的持有时间
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

        // 上传到GPU后，CPU上的数据就可以释放了
        stbi_image_free(imageData.data);
        to_process.pop();
    }
}

void TexturePool::uploadToGpu(const LoadedImageData& data) {
    // 从已存储的info中获取GPU ID和层索引
    auto info_opt = get(data.path);
    if (!info_opt) {
        qWarning() << "TexturePool: No texture info found for uploading path:"
                   << QString::fromStdString(data.path);
        return;
    }

    const TextureInfo& info = *info_opt;

    // 绑定对应的图集数组
    GLCALL_V(glf->glBindTexture(GL_TEXTURE_2D_ARRAY, info.gl_texture_array_id),
             glf);

    // 将数据上传到指定的层
    // 因为是RGBA8，所以对齐是4字节，通常不需要特殊处理glPixelStorei
    GLCALL_V(
        glf->glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                             0,     // Mipmap level
                             0, 0,  // xoffset, yoffset (总是放在左上角)
                             info.layer_index,  // zoffset (the layer index)
                             data.width,        // 纹理自身的宽度
                             data.height,       // 纹理自身的高度
                             1,        // depth (always 1 for a single layer)
                             GL_RGBA,  // format of the pixel data
                             GL_UNSIGNED_BYTE,  // type of the pixel data
                             data.data          // pointer to the data
                             ),
        glf);

    qDebug() << "Uploaded" << QString::fromStdString(data.path) << "["
             << data.width << "x" << data.height << "]" << "to array"
             << info.gl_texture_array_id << "layer" << info.layer_index
             << ", layersize:[" << groups[info.gl_texture_array_id].bucket_width
             << "x" << groups[info.gl_texture_array_id].bucket_height << "]";
}

// 获取纹理信息以供渲染器使用
std::optional<TextureInfo> TexturePool::get(std::string_view path) const {
    std::lock_guard<std::mutex> lock(info_mutex);
    if (auto it = texture_infos.find(path); it != texture_infos.end()) {
        return it->second;
    }
    return std::nullopt;
}
