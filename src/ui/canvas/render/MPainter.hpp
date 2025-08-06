#ifndef MMM_MPAINTER_HPP
#define MMM_MPAINTER_HPP

#include <glm/fwd.hpp>
#include <string>
#include <string_view>
class Renderer2D;

// 绘制-渲染指令翻译器
class MPainter {
   public:
    // 构造MPainter
    explicit MPainter(Renderer2D* renderer2D);
    // 析构MPainter
    virtual ~MPainter();

    // 绘制线段
    void paintLine(glm::vec2 pos1, glm::vec2 pos2, glm::f32 lineWidth);

    // 在指定位置直接绘制图像(无缩放-直接绘制图像的原始尺寸)
    void paintImage(std::string_view resPath, glm::vec2 pos);

    // 在指定区域填充图像(有缩放无视原始比例)
    void fillImage(std::string_view resPath, glm::vec2 pos, glm::vec2 size);

    // 在指定区域平铺图像(有缩放保持原始比例)
    void tileImage(std::string_view resPath, glm::vec2 pos, glm::vec2 size);

   private:
    Renderer2D* renderer;
};

#endif  // MMM_MPAINTER_HPP
