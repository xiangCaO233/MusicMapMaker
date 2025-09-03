#include <GLCanvas.hpp>
#include <render/pipeline/framebuffer/FrameBuffer.hpp>
#include <type_traits>
#include <util/statistic.hpp>
#include <utility>

template <typename Func>
auto glCallImpl(Func func, const char* funcStr,
                QOpenGLFunctions_4_1_Core* glf) {
    // 1. 先清除所有历史错误，确保我们只捕获当前调用的错误
    while (glf->glGetError() != GL_NO_ERROR);

    // 2. 对 lambda 本身的返回类型进行判断
    if constexpr (std::is_void_v<decltype(func())>) {
        func();  // 调用 lambda
    } else {
        auto&& result = func();  // 调用并捕获结果
        // 检查错误在调用之后
        if (GLenum error = glf->glGetError(); error != GL_NO_ERROR) {
            qDebug() << "OpenGL Error in [" << funcStr << "]: " << error
                     << "(Hex: 0x" << Qt::hex << error << Qt::dec << ")";
        }
        return std::forward<decltype(result)>(result);
    }

    // 针对void返回类型的lambda，在调用后检查错误
    if (GLenum error = glf->glGetError(); error != GL_NO_ERROR) {
        qDebug() << "OpenGL Error in [" << funcStr << "]: " << error
                 << "(Hex: 0x" << Qt::hex << error << Qt::dec << ")";
    }
}

// 用于包装 OpenGL 调用并检查错误
#define GLCALL(func, f)       \
    glCallImpl(               \
        [&]() {               \
            stat::gl_calls++; \
            return func;      \
        },                    \
        #func, f)
#define DRAWCALL(func, f)       \
    glCallImpl(                 \
        [&]() {                 \
            stat::draw_calls++; \
            stat::gl_calls++;   \
            return func;        \
        },                      \
        #func, f)
// 构造FrameBuffer
FrameBuffer::FrameBuffer(GLCanvas* canvas) : cvs(canvas) {
    GLCALL(cvs->glGenFramebuffers(1, &fbo), cvs);
}

// 析构FrameBuffer
FrameBuffer::~FrameBuffer() {
    // 删除全部纹理附件
    GLCALL(cvs->glDeleteTextures(color_attachments.size(),
                                 color_attachments.data()),
           cvs);
    // 删除fbo
    GLCALL(cvs->glDeleteFramebuffers(1, &fbo), cvs);
}
// 绑定和释放
void FrameBuffer::bind() {
    GLCALL(cvs->glBindFramebuffer(GL_FRAMEBUFFER, fbo), cvs);
}
void FrameBuffer::release() {
    GLCALL(
        cvs->glBindFramebuffer(GL_FRAMEBUFFER, cvs->defaultFramebufferObject()),
        cvs);
}

// 更新视口
void FrameBuffer::update_viewport(glm::vec2 size) {
    // 更新全部纹理附件的尺寸
    physical_viewport = size;
}

// 添加纹理组件
uint32_t FrameBuffer::add_color_attachment() {
    uint32_t texture{0};
    GLCALL(cvs->glGenTextures(1, &texture), cvs);
    GLCALL(cvs->glBindTexture(GL_TEXTURE_2D, texture), cvs);
    GLCALL(cvs->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, physical_viewport.x,
                             physical_viewport.y, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                             NULL),
           cvs);
    GLCALL(
        cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR),
        cvs);
    GLCALL(
        cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR),
        cvs);
    GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                                GL_CLAMP_TO_EDGE),
           cvs);
    GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                                GL_CLAMP_TO_EDGE),
           cvs);

    // 将这个手动创建的纹理，附加到 main_fbo 的最后一个颜色槽位上
    bind();
    GLCALL(cvs->glFramebufferTexture2D(
               GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + color_attachments.size(),
               GL_TEXTURE_2D, texture, 0),
           cvs);
    release();
    color_attachments.emplace_back(texture);
    return texture;
}

// 获取纹理组件列表
const std::vector<uint32_t>& FrameBuffer::textures() const {
    return color_attachments;
}
