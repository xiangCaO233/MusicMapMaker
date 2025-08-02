#ifndef MMM_FORMRENDERER2D_HPP
#define MMM_FORMRENDERER2D_HPP

#include <qcontainerfwd.h>
#include <qvectornd.h>

#include <QMutex>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <audio/graphic/formrender/SpectrogramGenerator.hpp>
#include <ice/manage/AudioBuffer.hpp>
#include <span>
#include <vector>

class AudioGraphicWidget;
enum class GraphType;

class FormRenderer2D : public QOpenGLFunctions_4_1_Core {
   public:
    // 构造FormRenderer2D
    explicit FormRenderer2D(AudioGraphicWidget *parent);

    // 析构FormRenderer2D
    ~FormRenderer2D() override;

    // 窗口尺寸变化时调用
    void resize(int width, int height);

    // 主渲染循环
    void render(GraphType type, const QMatrix4x4 &projection,
                const QMatrix4x4 &view);

    ice::AudioBuffer &wav() { return wav_buffer; }

    std::vector<std::span<const float>> &span() { return wav_span; }

    void updateSpectrogramTexture();

    inline void set_live(bool flag) { liveGraph = flag; }

   private:
    void initShaders();
    void initGeometry();

    // 父窗体引用
    AudioGraphicWidget *parent_widget{nullptr};

    // 实时同步处理
    bool liveGraph{false};

    // 当前范围的数据
    ice::AudioBuffer wav_buffer;
    std::vector<std::span<const float>> wav_span;

    // 波形图资源
    // 波形图着色器
    QOpenGLShaderProgram *waveformShader{nullptr};

    // 波形图顶点缓冲区
    QOpenGLBuffer waveformVBO;

    // 波形图顶点数组
    QOpenGLVertexArrayObject waveformVAO;
    QVector<QVector4D> channel_colors{QVector4D(0.0f, 0.75f, 1.0f, 0.4f),
                                      QVector4D(0.98f, 0.495f, 0.f, 0.4f)};

    // 频谱图资源
    QOpenGLShaderProgram *spectroShader{nullptr};
    // 频谱图顶点缓冲区
    QOpenGLBuffer spectroVBO;
    // 频谱图顶点数组
    QOpenGLVertexArrayObject spectroVAO;

    std::unique_ptr<QOpenGLTexture> spectroTexLeft;
    std::unique_ptr<QOpenGLTexture> spectroTexRight;
};

#endif  // MMM_FORMRENDERER2D_HPP
