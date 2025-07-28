#include <audio/graphic/AudioGraphicWidget.h>

#include <QFile>
#include <audio/graphic/formrender/FormRenderer2D.hpp>

// 修正后的 glCallImpl 函数
template <typename Func>
auto glCallImpl(Func func, const char* funcStr) {
    // 对 lambda 本身的返回类型进行判断
    if constexpr (std::is_void_v<decltype(func())>) {
        func();
        // 先获取错误码，再进行判断
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            // 使用 hex 格式打印，更容易识别标准 OpenGL 错误码
            qDebug() << "OpenGL Error in [" << funcStr << "]:" << error;
        }
    } else {
        auto&& result = func();
        // 先获取错误码，再进行判断
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            qDebug() << "OpenGL Error in [" << funcStr << "]:" << error;
        }
        return std::forward<decltype(result)>(result);
    }
}
// 用于包装 OpenGL 调用并检查错误
#define GLCALL(func) glCallImpl([&]() { return func; }, #func)

// 构造FormRenderer2D
FormRenderer2D::FormRenderer2D(AudioGraphicWidget* parent)
    : parent_widget(parent) {
    initializeOpenGLFunctions();
    // VBO 使用动态绘制，因为它会频繁更新
    waveformVBO.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    initShaders();
    initGeometry();
}

FormRenderer2D::~FormRenderer2D() {
    // QOpenGL* 对象会自动清理
}

void FormRenderer2D::resize(int width, int height) {
    // 上传到uniform
    // waveformShader->setUniformValue("formwidth", width);
    // waveformShader->setUniformValue("formheight", height);
}

void FormRenderer2D::initShaders() {
    waveformShader = new QOpenGLShaderProgram();
    // 从资源qrc加载
    QFile wave_vert_source(":/glsl/audio_wavgraph_vshader.glsl.vert");
    QFile wave_frag_source(":/glsl/audio_wavgraph_fshader.glsl.frag");
    // 检查文件是否成功打开
    if (!wave_vert_source.open(QIODevice::ReadOnly | QIODevice::Text)) {
        auto errormsg = wave_vert_source.errorString();
        auto errorstr = errormsg.toStdString();
        qDebug() << "Failed to open vertex source file:" << errorstr;
    }
    if (!wave_frag_source.open(QIODevice::ReadOnly | QIODevice::Text)) {
        auto errormsg = wave_frag_source.errorString();
        auto errorstr = errormsg.toStdString();
        qDebug() << "Failed to open vertex source file:" << errorstr;
    }
    // 用QTextStream读取内容
    QTextStream vertin(&wave_vert_source);
    QTextStream fragin(&wave_frag_source);

    auto wavvertex_shader_qstr = vertin.readAll();
    auto wavfragment_shader_qstr = fragin.readAll();

    // 关闭文件
    wave_vert_source.close();
    wave_frag_source.close();

    if (!waveformShader->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                                 wavvertex_shader_qstr)) {
        qCritical() << "Waveform Vertex Shader compilation failed:"
                    << waveformShader->log();
    }
    if (!waveformShader->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                                 wavfragment_shader_qstr)) {
        qCritical() << "Waveform Fragment Shader compilation failed:"
                    << waveformShader->log();
    }
    if (!waveformShader->link()) {
        qCritical() << "Waveform Shader link failed:" << waveformShader->log();
    }
}

void FormRenderer2D::initGeometry() {
    // Waveform VAO/VBO setup
    waveformVAO.create();
    QOpenGLVertexArrayObject::Binder vaoBinder(&waveformVAO);

    waveformVBO.create();
    waveformVBO.bind();
    // 暂时不分配数据，等 updateWaveformBuffer 时再分配
    // m_waveformVBO.allocate(nullptr, 0);

    // 设置顶点属性
    // location = 0, 每个顶点1个float, 类型是float, 不归一化, 步长为0, 偏移为0
    GLCALL(glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, nullptr));
    GLCALL(glEnableVertexAttribArray(0));

    waveformVBO.release();
}

void FormRenderer2D::render(GraphType type, const QMatrix4x4& projection,
                            const QMatrix4x4& view) {
    if (type == GraphType::WAVE) {
        waveformShader->bind();
        waveformShader->setUniformValue("projection", projection);
        waveformShader->setUniformValue("view", view);
        // waveformShader->setUniformValue("samples",
        //                                 uint32_t(wav_buffer.num_frames()));

        QOpenGLVertexArrayObject::Binder vaoBinder(&waveformVAO);

        // 每个声道独立drawcall(完整线段集)
        for (int ch = 0; ch < wav_buffer.afmt.channels; ++ch) {
            waveformVBO.bind();
            waveformVBO.allocate(wav_buffer.raw_ptrs()[ch],
                                 wav_buffer.num_frames() * sizeof(float));
            // 设置声道颜色
            waveformShader->setUniformValue("channel_color",
                                            channel_colors[ch]);
            GLCALL(glDrawArrays(GL_LINE_STRIP, 0, wav_buffer.num_frames()));
        }

        waveformShader->release();
    } else if (type == GraphType::SPECTRO) {
        // 频谱图渲染逻辑
    }
}
