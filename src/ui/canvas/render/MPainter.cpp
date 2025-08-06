#include <glm/glm.hpp>
#include <render/MPainter.hpp>
#include <render/Renderer2D.hpp>
#include <string_view>

// 构造MPainter
MPainter::MPainter(Renderer2D* renderer2D) : renderer(renderer2D) {}
// 析构MPainter
MPainter::~MPainter() {
    // 析构时绘制
    renderer->finalize();
    renderer->render();
};

// 绘制线段
void MPainter::paintLine(glm::vec2 pos1, glm::vec2 pos2, glm::f32 lineWidth) {}

// 在指定位置直接绘制图像(无缩放-直接绘制图像的原始尺寸)
void MPainter::paintImage(std::string_view resPath, glm::vec2 pos) {
    if (auto texture = renderer->texture_pool()->get(
            "../resources/textures/default/物件/arrowright_selected.png");
        texture.has_value()) {
        const auto tex = texture.value();
        renderer->commit({pos,
                          {texture->origin_size.x, texture->origin_size.y},
                          0.f,
                          {1.f, 1.f, 1.f, 1.f},
                          tex,
                          0,
                          TexAlignMode::CENTER,
                          TexScaleMode::SCALE_TO_TILING});
    }
}

// 在指定区域填充图像(有缩放无视原始比例)
void MPainter::fillImage(std::string_view resPath, glm::vec2 pos,
                         glm::vec2 size) {}

// 在指定区域平铺图像(有缩放保持原始比例)
void MPainter::tileImage(std::string_view resPath, glm::vec2 pos,
                         glm::vec2 size) {}
