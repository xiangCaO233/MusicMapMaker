#include <GLCanvas.hpp>
#include <render/pipeline/framebuffer/FrameBuffer.hpp>
#include <util/glcheck.hpp>

// 构造FrameBuffer
FrameBuffer::FrameBuffer(GLCanvas* canvas, glm::vec2 current_size)
    : cvs(canvas), physical_viewport(current_size) {
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
    // 重新创建纹理
    GLCALL(cvs->glDeleteTextures(color_attachments.size(),
                                 color_attachments.data()),
           cvs);
    GLCALL(
        cvs->glGenTextures(color_attachments.size(), color_attachments.data()),
        cvs);

    bind();
    for (int i{0}; i < color_attachments.size(); ++i) {
        const auto& texture = color_attachments[i];
        GLCALL(cvs->glBindTexture(GL_TEXTURE_2D, texture), cvs);
        GLCALL(cvs->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                                 physical_viewport.x, physical_viewport.y, 0,
                                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr),
               cvs);
        GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                    GL_LINEAR),
               cvs);
        GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                    GL_LINEAR),
               cvs);
        GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                                    GL_CLAMP_TO_EDGE),
               cvs);
        GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                                    GL_CLAMP_TO_EDGE),
               cvs);
        GLCALL(
            cvs->glFramebufferTexture2D(
                GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + color_attachments.size(),
                GL_TEXTURE_2D, texture, 0),
            cvs);
    }
    release();
}

// 添加纹理组件
uint32_t FrameBuffer::add_color_attachment() {
    uint32_t texture{0};
    GLCALL(cvs->glGenTextures(1, &texture), cvs);
    GLCALL(cvs->glBindTexture(GL_TEXTURE_2D, texture), cvs);
    GLCALL(cvs->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, physical_viewport.x,
                             physical_viewport.y, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                             nullptr),
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
