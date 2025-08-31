#ifndef MMM_RENDERER2D_HPP
#define MMM_RENDERER2D_HPP

#include <QObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <array>
#include <glm/fwd.hpp>
#include <mmm/project/TextureLoadCallback.hpp>
#include <mutex>
#include <render/command/GPUData.hpp>
#include <render/command/RenderCommand.hpp>
#include <render/texture/TexMode.hpp>
#include <render/texture/TexturePool.hpp>
#include <render/texture/font/FontPool.hpp>

class GLCanvas;
class Renderer2D : public QObject, public TextureLoadCallback {
    Q_OBJECT
   public:
    explicit Renderer2D(GLCanvas* canvas);
    ~Renderer2D() override;

    // 回调实现表
    // 需要载入纹理
    void need_loadtexture_dir(std::string_view texdir) override;
    // 需要卸载纹理
    void need_unloadtexture_dir(std::string_view texdir) override;
    // 获取信息
    TextureInfo getTextureInfo(std::string_view texname) override;

    // 设置投影矩阵
    void update_viewport(glm::vec2 view, glm::vec2 phisical_viewport);

    // 提交渲染指令
    void commit(const QuadCommand& command);
    void commit(const MeshCommand& command);
    void commit(const PrimitiveCommand& command);
    void commit(const CurveCommand& command);

    // 更新需要更新的资源等等
    void update();

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

    // 访问纹理池
    const std::unique_ptr<TexturePool>& texture_pool() const {
        return texturepool;
    }

    // 访问字体池
    const std::unique_ptr<FontPool>& font_pool() const { return fontpool; }

   signals:
    // 需要更新纹理信息信号
    void needUpdateTexinfo();

   private:
    // 启用调试线框
    bool draw_wireframe{false};

    // 尺寸
    glm::vec2 viewport;
    // 物理尺寸
    glm::vec2 phisical_viewport;
    bool update_view{true};

    // 纹理池
    std::unique_ptr<TexturePool> texturepool;

    // 字体池
    std::unique_ptr<FontPool> fontpool;

    // gl上下文
    GLCanvas* cvs;

    // 模糊处理迭代次数
    uint32_t blur_iteration_count{8};

    // 主渲染FBO
    QOpenGLFramebufferObject* main_fbo{nullptr};
    // 高斯模糊的乒乓FBO
    QOpenGLFramebufferObject* blur_fbo_A{nullptr};
    QOpenGLFramebufferObject* blur_fbo_B{nullptr};

    // 后期处理着色器程序
    // 发光附加着色器
    QOpenGLShaderProgram* glow_extract_shader;
    // 高斯模糊着色器
    QOpenGLShaderProgram* gaussian_blur_shader;
    // 混合着色器
    QOpenGLShaderProgram* composite_shader;

    // 着色器
    QOpenGLShaderProgram* quad_shader_program;
    QOpenGLShaderProgram* primitive_shader_program;
    QOpenGLShaderProgram* curve_shader_program;
    QOpenGLShaderProgram* mesh_shader_program;

    // 渲染指令序列
    // 统一的、保证顺序的索引表
    std::vector<CommandHandle> all_command_handles;
    // 分离存储不同渲染指令
    std::vector<QuadCommand> quad_command_list;
    std::vector<PrimitiveCommand> primitive_command_list;
    std::vector<CurveCommand> curve_command_list;
    std::vector<MeshCommand> mesh_command_list;
    RenderCommand nullCmd{};

    // 初始化资源
    void initShader(QOpenGLShaderProgram*& shader, const char* debug_name,
                    const char* vertex_shader_source_path,
                    const char* fragment_shader_source_path,
                    const char* geometry_shader_source_path = nullptr) const;
    void initShaderUBO(QOpenGLShaderProgram*& shader, const char* debug_name,
                       const char* name_in_shader, uint32_t ubo_index);

    // 初始化通用着色器/gl资源
    void initQuadShader();
    void initQuadObjectBuffers();
    void initMeshShader();
    void initMeshObjectBuffers();
    void initPrimitiveShader();
    void initPrimitiveObjectBuffers();
    void initCurveShader();
    void initCurveObjectBuffers();
    // 初始化后期着色器/gl资源
    void initAfterEffectShaders();
    void initAfterEffectObjectBuffers();
    // 初始化蒙版ubo
    void initMaskUBO();

    // 扩充矩形实例缓冲区
    void expandQuadDataBuffer();
    // 扩充网格缓冲区
    void expandMeshDataBuffer();
    // 扩充图元缓冲区
    void expandPrimitiveDataBuffer();
    // 扩充曲线缓冲区
    void expandCurveDataBuffer();

    // 更新fbo
    void update_fbo();

    // 后期处理
    void afterEffect();

    // 混合着色
    void composite();

    // 辅助函数，通过句柄获取 RenderCommand 的引用
    const RenderCommand& get_command_from_handle(const CommandHandle& handle);

    // 使用指定gpu实例
    QOpenGLShaderProgram* useShader(CommandType type);

    uint32_t useVAO(CommandType type) const;

    // 批绘制
    void drawBatch(const RenderBatch& batch, QOpenGLShaderProgram* shader,
                   bool wireframe) const;
    // 更新gpu数据
    void update_gpudata();

    // gpu数据预缓存
    std::vector<PrimitiveData> quad_datas;
    std::vector<PointsData> mesh_datas;
    std::vector<PrimitiveData> primitive_datas;
    std::vector<PrimitiveData> curve_datas;

    // 当前网格的总顶点数
    size_t current_mesh_vertex_count{0};
    // 当前曲线的总顶点数
    size_t current_curve_vertex_count{0};

    std::vector<RenderBatch> command_batchs;
    std::mutex command_mtx;

    // 最大矩形数量
    uint32_t max_quadcount{8192};
    // 最大图元数量
    uint32_t max_primitivecount{16384};
    // 最大网格顶点数量
    uint32_t max_mesh_vertexcount{32768};

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
    uint32_t quad_instance_dataAO{0};
    uint32_t quad_instance_dataBO{0};
    uint32_t mesh_dataAO{0};
    uint32_t mesh_dataBO{0};
    uint32_t primitive_dataAO{0};
    uint32_t primitive_dataBO{0};
    uint32_t curve_dataAO{0};
    uint32_t curve_dataBO{0};
    uint32_t fullScreenAO{0};

    // 手动管理纹理附件
    uint32_t glow_mask_texture_id;

    // 从指定矩形实例位置开始更新矩形顶点数组指针
    void updateQuadAttribptrFromInstance(size_t instance_index) const;
};

#endif  // MMM_RENDERER2D_HPP
