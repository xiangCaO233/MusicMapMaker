#ifndef MMM_NOTELAYER_HPP
#define MMM_NOTELAYER_HPP

#include <layer/ILayer.hpp>

class NoteLayer : public ILayer {
   public:
    // 构造NoteLayer
    NoteLayer(Renderer2D* renderer, ECSCore* ecore);
    // 析构NoteLayer
    ~NoteLayer() override;
};

#endif  // MMM_NOTELAYER_HPP
