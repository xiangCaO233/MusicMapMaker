#include "EffectFrameGenerator.h"

// 构造EffectGenerator
EffectGenerator::EffectGenerator(std::shared_ptr<MapEditor> editor)
    : editor_ref(editor) {}

// 析构EffectGenerator
EffectGenerator::~EffectGenerator() = default;

// 生成轨道渲染指令
void EffectGenerator::generate() {}
