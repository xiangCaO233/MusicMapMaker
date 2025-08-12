#ifndef MMM_RENDERER2D_HPP
#define MMM_RENDERER2D_HPP

#include <qopenglshaderprogram.h>

#include <array>
#include <glm/fwd.hpp>
#include <mmm/project/TextureLoadCallback.hpp>
#include <mutex>
#include <render/QuadData.hpp>
#include <render/RenderCommand.hpp>
#include <render/texture/TexMode.hpp>
#include <render/texture/TexturePool.hpp>
#include <render/texture/font/FontPool.hpp>

class GLCanvas;
class Renderer2D : public TextureLoadCallback {
   public:
    explicit Renderer2D(GLCanvas* canvas);
    ~Renderer2D();

    // 需要载入纹理
    void need_loadtexture_dir(std::string_view texdir) override;
    // 需要卸载纹理
    void need_unloadtexture_dir(std::string_view texdir) override;

    // 直接访问着色器
    QOpenGLShaderProgram* shader() const { return shader_program; }

    // 设置投影矩阵
    void update_viewport(glm::vec2 view);

    // 更新需要更新的资源等等
    void update();

    // 提交渲染指令
    void commit(const RenderCommand& command);

    // 新建蒙版
    void newMask(glm::vec4 rect, glm::vec4 effectParams, MaskEffect effect);

    // 添加纹理目录
    void add_texture_from_path(const std::string& path);
    void request_texture_from_path(const std::string& path);

    // 移除纹理目录
    void remove_texture_from_path(const std::string& path);
    void request_remove_texture_from_path(const std::string& path);

    // 添加字体
    void add_font_from_path(const std::string& path, bool is_qrc = false);

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

    // 访问字体池
    const std::unique_ptr<FontPool>& font_pool() const { return fontpool; }

   private:
    // 启用调试线框
    bool draw_wireframe{false};

    // 尺寸
    glm::vec2 viewport;
    bool update_view{true};

    // 纹理池
    std::unique_ptr<TexturePool> texturepool;

    // 字体池
    std::unique_ptr<FontPool> fontpool;

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

    // 蒙版ubo句柄
    uint32_t mask_uBO;
    bool update_ubo{false};

    // gpu对应的蒙版结构体
    struct MaskLayer_STD140 {
        // 蒙版位置
        // {left, top, right, bottom}
        glm::vec4 rect;
        // 效果参数
        glm::vec4 effectParams;
        // 效果
        MaskEffect effect;
        // 满足std140的4N对齐规则，填充
        std::array<uint32_t, 3> padding;
    };

    // 最大的蒙版层数
    const uint32_t MAX_MASK_LAYERS = 16;
    std::vector<MaskLayer_STD140> mask_stack_cpu;

    // gl资源
    uint32_t instance_dataAO{0};
    uint32_t instance_dataBO{0};

    // 从指定实例位置开始更新顶点数组指针
    void update_attribptrFromInstance(size_t instance_index);
};

#endif  // MMM_RENDERER2D_HPP
