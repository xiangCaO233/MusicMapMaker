#include <GLCanvas.hpp>
#include <render/pipeline/framebuffer/FrameBuffer.hpp>
#include <util/glcheck.hpp>

// 构造FrameBuffer
FrameBuffer::FrameBuffer(GLCanvas* canvas, glm::vec2 current_size)
    : cvs(canvas), physical_viewport(current_size) {
    GLCALL_V(cvs->glGenFramebuffers(1, &fbo), cvs);
}

// 析构FrameBuffer
FrameBuffer::~FrameBuffer() {
    // 删除全部纹理附件
    GLCALL_V(cvs->glDeleteTextures(color_attachments.size(),
                                   color_attachments.data()),
             cvs);
    // 删除fbo
    GLCALL_V(cvs->glDeleteFramebuffers(1, &fbo), cvs);
}
// 绑定和释放
void FrameBuffer::bind() {
    GLCALL_V(cvs->glBindFramebuffer(GL_FRAMEBUFFER, fbo), cvs);
    // 绑定FBO时同时设置对应的视口
    GLCALL_V(cvs->glViewport(0, 0, physical_viewport.x, physical_viewport.y),
             cvs);
}
void FrameBuffer::release() {
    GLCALL_V(
        cvs->glBindFramebuffer(GL_FRAMEBUFFER, cvs->defaultFramebufferObject()),
        cvs);
}

// 更新视口
void FrameBuffer::update_viewport(glm::vec2 size) {
    // 如果尺寸没有变化无需操作
    if (physical_viewport.x == size.x && physical_viewport.y == size.y) return;

    // 更新全部纹理附件的尺寸
    physical_viewport = size;
    // 重新分配纹理显存空间
    for (const auto& texture : color_attachments) {
        GLCALL_V(cvs->glBindTexture(GL_TEXTURE_2D, texture), cvs);
        GLCALL_V(cvs->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                                   physical_viewport.x, physical_viewport.y, 0,
                                   GL_RGBA, GL_UNSIGNED_BYTE, nullptr),
                 cvs);
    }
}

// 添加纹理组件
uint32_t FrameBuffer::add_color_attachment() {
    uint32_t texture{0};
    GLCALL_V(cvs->glGenTextures(1, &texture), cvs);
    GLCALL_V(cvs->glBindTexture(GL_TEXTURE_2D, texture), cvs);
    GLCALL_V(cvs->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, physical_viewport.x,
                               physical_viewport.y, 0, GL_RGBA,
                               GL_UNSIGNED_BYTE, nullptr),
             cvs);
    GLCALL_V(
        cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR),
        cvs);
    GLCALL_V(
        cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR),
        cvs);
    GLCALL_V(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                                  GL_CLAMP_TO_EDGE),
             cvs);
    GLCALL_V(cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                                  GL_CLAMP_TO_EDGE),
             cvs);

    // 将这个手动创建的纹理，附加到 main_fbo 的最后一个颜色槽位上
    bind();
    GLCALL_V(
        cvs->glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + color_attachments.size(),
            GL_TEXTURE_2D, texture, 0),
        cvs);
    // 检查FBO是否完整
    if (check_completeness()) {
        color_attachments.push_back(texture);
    } else {
        // 如果不完整清理掉刚创建的纹理
        GLCALL_V(cvs->glDeleteTextures(1, &texture), cvs);
        // 返回0表示失败
        texture = 0;
    }
    release();
    return texture;
}

// 获取纹理组件列表
const std::vector<uint32_t>& FrameBuffer::textures() const {
    return color_attachments;
}

bool FrameBuffer::check_completeness() {
    // 这个函数应该在绑定了FBO之后调用
    if (cvs->glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
        GL_FRAMEBUFFER_COMPLETE) {
        qDebug() << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!";
        return false;
    }
    return true;
}
