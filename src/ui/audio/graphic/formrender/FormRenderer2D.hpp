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

#include "ice/manage/AudioBuffer.hpp"

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

   private:
    void initShaders();
    void initGeometry();

    // 父窗体引用
    AudioGraphicWidget *parent_widget{nullptr};

    // 波形图资源
    // 波形图着色器
    QOpenGLShaderProgram *waveformShader{nullptr};
    // 波形图顶点缓冲区
    QOpenGLBuffer waveformVBO;
    // 波形图顶点数组
    QOpenGLVertexArrayObject waveformVAO;
    // 当前波形图范围的对应数据集
    ice::AudioBuffer wav_buffer;
    QVector<QVector4D> channel_colors{
        QVector4D(0.0f, 0.75f, 1.0f, 0.4f),  // 明亮的青蓝色 (Bright Cyan-Blue)
        QVector4D(0.1f, 1.0f, 0.5f, 0.4f)  // 鲜明的石灰绿 (Vibrant Lime Green)
    };
};

#endif  // MMM_FORMRENDERER2D_HPP
