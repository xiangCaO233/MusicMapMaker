#ifndef MMM_TEXTUREPOOL_HPP
#define MMM_TEXTUREPOOL_HPP

#include <qopenglfunctions_4_1_core.h>

#include <canvas/render/texture/TextureInfo.hpp>
#include <cstdint>
#include <ice/thread/ThreadPool.hpp>
#include <string>

class TexturePool {
   public:
    // 构造TexturePool
    explicit TexturePool(QOpenGLFunctions_4_1_Core* gl_functions);
    // 析构TexturePool
    virtual ~TexturePool();

    // 禁止拷贝和赋值
    TexturePool(const TexturePool&) = delete;
    TexturePool& operator=(const TexturePool&) = delete;

    // 从一个包含所有纹理路径的清单文件构建池
    void buildFromManifest(const std::vector<std::string>& texture_paths);

    // 从主线程调用，处理已从磁盘加载完成的纹理，将其上传到GPU
    void processUploadQueue();

    // 获取纹理信息以供渲染器使用
    std::optional<TextureInfo> get(const std::string& path) const;

   private:
    QOpenGLFunctions_4_1_Core* glf;
    // 载入的纹理原始数据
    struct LoadedImageData {
        std::string path;
        int width;
        int height;
        int channels;
        // 原始像素数据
        unsigned char* data;
    };

    void uploadToGpu(const LoadedImageData& data);

    // 异步载入纹理或分配大显存的线程池
    ice::ThreadPool threadpool{4};
    std::atomic<bool> is_running{true};

    // 生产者-消费者队列，用于在工作线程和主线程间传递数据
    std::queue<LoadedImageData> upload_queue;
    mutable std::mutex queue_mutex;
    std::condition_variable cv;

    struct StringHash {
        // 这个标签用于开启透明性
        using is_transparent = void;
        [[nodiscard]] size_t operator()(const char* txt) const {
            return std::hash<std::string_view>{}(txt);
        }
        [[nodiscard]] size_t operator()(std::string_view txt) const {
            return std::hash<std::string_view>{}(txt);
        }
        [[nodiscard]] size_t operator()(const std::string& txt) const {
            return std::hash<std::string>{}(txt);
        }
    };

    // 存储所有纹理的信息，从路径映射到具体信息
    std::unordered_map<std::string, TextureInfo, StringHash, std::equal_to<>>
        texture_info;
    mutable std::mutex info_mutex;

    // 纹理数组(Texture Array)成员
    int32_t max_texture_array_layers{0};
};

#endif  // MMM_TEXTUREPOOL_HPP
