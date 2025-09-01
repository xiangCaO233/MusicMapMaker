#ifndef MMM_FRAMEBUFFER_HPP
#define MMM_FRAMEBUFFER_HPP

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

class GLCanvas;
class FrameBuffer {
   public:
    // 构造FrameBuffer
    FrameBuffer(GLCanvas* cvs);

    // 析构FrameBuffer
    virtual ~FrameBuffer();

    // 绑定和释放
    void bind();
    void release();

    // 更新视口
    void update_viewport(glm::vec2 size);

    // 添加纹理组件
    uint32_t add_color_attachment();

    // 获取纹理组件列表
    const std::vector<uint32_t>& textures() const;

   private:
    // 画布引用
    GLCanvas* cvs;
    // 帧缓冲本体
    uint32_t fbo;
    // 物理视口
    glm::vec2 physical_viewport;
    // 所有纹理组件
    std::vector<uint32_t> color_attachments;
};

#endif  // MMM_FRAMEBUFFER_HPP
