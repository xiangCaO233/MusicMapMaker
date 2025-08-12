#include <layer/note/NoteLayer.hpp>

// 构造NoteLayer
NoteLayer::NoteLayer(Renderer2D* renderer) : ILayer(renderer) {
    setType(LayerType::NOTE);
}

// 析构NoteLayer
NoteLayer::~NoteLayer() {}

// 更新信息
void NoteLayer::updateInfo(SharedCanvasInfo* info) {}
