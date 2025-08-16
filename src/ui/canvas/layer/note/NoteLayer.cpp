#include <layer/note/NoteLayer.hpp>

// 构造NoteLayer
NoteLayer::NoteLayer(Renderer2D* renderer, ECSCore* ecore)
    : ILayer(renderer, ecore) {
    setType(LayerType::NOTE);
}

// 析构NoteLayer
NoteLayer::~NoteLayer() {}
