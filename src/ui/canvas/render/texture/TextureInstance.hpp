#ifndef MMM_TEXTUREINSTANCE_HPP
#define MMM_TEXTUREINSTANCE_HPP
#include <cstdint>
class TextureInstance {
   public:
    TextureInstance();
    virtual ~TextureInstance();

   private:
    // 纹理池句柄
    uint16_t handle;
};
#endif  // MMM_TEXTUREINSTANCE_HPP
