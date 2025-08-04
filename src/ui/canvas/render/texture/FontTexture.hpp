#ifndef MMM_FONTTEXTURE_HPP
#define MMM_FONTTEXTURE_HPP
#include "render/texture/TextureInstance.hpp"
class FontTexture : public TextureInstance {
   public:
    FontTexture();
    ~FontTexture() override;

   private:
    // 唯一句柄
    uint32_t handle;
    // 是否处于纹理集中
    bool in_atlas{false};
};
#endif  // MMM_FONTTEXTURE_HPP
