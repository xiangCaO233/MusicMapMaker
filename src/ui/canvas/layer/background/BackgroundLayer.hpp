#ifndef MMM_BACKGROUNDLAYER_HPP
#define MMM_BACKGROUNDLAYER_HPP

#include <glm/fwd.hpp>
#include <layer/ILayer.hpp>
#include <string>

class BackgroundLayer : public ILayer {
   public:
    // 构造BackgroundLayer
    explicit BackgroundLayer(Renderer2D* renderer);
    // 析构BackgroundLayer
    ~BackgroundLayer() override;

    // 背景图片路径
    std::string background_image_path;

    // 背景图片信息
    TextureInfo texinfo;

    // 画布大小
    glm::vec2 canvas_size;

   protected:
    // 更新信息
    void updateInfo(SharedCanvasInfo* info) override;
    friend class BackgroundLayerGenerator;
};
#endif  // MMM_BACKGROUNDLAYER_HPP
