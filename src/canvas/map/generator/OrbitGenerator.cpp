#include "OrbitGenerator.h"

// 构造OrbitGenerate
OrbitGenerator::OrbitGenerator(std::shared_ptr<MapEditor> &editor)
    : editor_ref(editor) {}

// 析构OrbitGenerate
OrbitGenerator::~OrbitGenerator() = default;

// 生成轨道渲染指令
void OrbitGenerator::generate() {
    //
}
