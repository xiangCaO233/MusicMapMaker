#ifndef MMM_TEXTURELOADCALLBACK_HPP
#define MMM_TEXTURELOADCALLBACK_HPP

#include <render/texture/TextureInfo.hpp>
#include <string_view>

// 纹理加载回调
class TextureLoadCallback {
   public:
    virtual ~TextureLoadCallback() = default;
    virtual void need_loadtexture_dir(std::string_view texdir) = 0;
    virtual void need_unloadtexture_dir(std::string_view texdir) = 0;
    virtual TextureInfo getInfo(std::string_view texname) = 0;
};
#endif  // MMM_TEXTURELOADCALLBACK_HPP
