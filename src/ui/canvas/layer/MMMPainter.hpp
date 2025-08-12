#ifndef MMM_MMMPAINTER_HPP
#define MMM_MMMPAINTER_HPP

#include <layer/ILayer.hpp>

class MMMPainter {
   public:
    // 构造MMMPainter
    explicit MMMPainter(ILayer::RenderDataBuffer& buffer);

    // 析构MMMPainter
    virtual ~MMMPainter();
};

#endif  // MMM_MMMPAINTER_HPP
