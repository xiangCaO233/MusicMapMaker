#ifndef MMM_RENDERER2D_HPP
#define MMM_RENDERER2D_HPP

#include <qopenglshaderprogram.h>

#include <mutex>
#include <render/RenderCommand.hpp>
#include <render/quad/QuadData.hpp>
#include <render/texture/TexturePool.hpp>

class GLCanvas;
class Renderer2D {
   public:
    explicit Renderer2D(GLCanvas* canvas);
    ~Renderer2D();

    // 直接访问着色器
    QOpenGLShaderProgram* shader() const { return shader_program; }

    // 设置投影矩阵
    void set_projection(const QMatrix4x4& projection);

    // 更新需要更新的资源等等
    void update();

    // 提交渲染指令
    void commit(const RenderCommand& command);

    // 添加纹理目录
    void add_texture_from_path(const std::string& path);

    // 结束
    void finalize();

    // 执行渲染
    void render();

    // 扩充矩形实例缓冲区
    void expandQuadDataBuffer();

    // 访问纹理池
    const std::unique_ptr<TexturePool>& texture_pool() const {
        return texturepool;
    }

   private:
    // 纹理池
    std::unique_ptr<TexturePool> texturepool;

    // gl函数上下文
    QOpenGLFunctions_4_1_Core* glf;
    // 着色器
    QOpenGLShaderProgram* shader_program;

    // 渲染指令序列
    std::vector<RenderCommand> command_list;
    std::vector<QuadData> quad_datas;

    std::vector<RenderBatch> command_batch;
    std::mutex command_mtx;

    // 最大矩形数量
    uint32_t max_quadcount{8192};

    // gl资源
    uint32_t instance_dataAO{0};
    uint32_t instance_dataBO{0};

    // 从指定实例位置开始更新顶点数组指针
    void update_attribptrFromInstance(size_t instance_index);
};

#endif  // MMM_RENDERER2D_HPP
