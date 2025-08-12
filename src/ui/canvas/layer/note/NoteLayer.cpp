#include <layer/note/NoteLayer.hpp>

// 构造NoteLayer
NoteLayer::NoteLayer() { setType(LayerType::NOTE); }

// 析构NoteLayer
NoteLayer::~NoteLayer() {}

// 更新信息
void NoteLayer::updateInfo(SharedCanvasInfo* info) {}
