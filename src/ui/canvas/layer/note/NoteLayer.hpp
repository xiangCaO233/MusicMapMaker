#ifndef MMM_NOTELAYER_HPP
#define MMM_NOTELAYER_HPP

#include <layer/ILayer.hpp>

class NoteLayer : public ILayer {
   public:
    // 构造NoteLayer
    explicit NoteLayer(Renderer2D* renderer);
    // 析构NoteLayer
    ~NoteLayer() override;

   protected:
    // 更新信息
    void updateInfo(SharedCanvasInfo* info) override;
};

#endif  // MMM_NOTELAYER_HPP
