#include <QDebug>
#include <chrono>
#include <glm/gtc/constants.hpp>
#include <layer/interact/InteractLayerGenerator.hpp>

#include "layer/interact/RealTimeInteractLayer.hpp"

// 析构InteractLayerGenerator
InteractLayerGenerator::~InteractLayerGenerator() {
    qDebug() << "交互图层生成线程释放";
}
// --- 1. 效果参数 ---
const int NUM_RINGS = 6;                                 // 组成光晕的同心圆数量
const float BASE_RADIUS = 3.0f;                          // 核心光点的半径
const float WAVE_AMPLITUDE = 15.0f;                      // 呼吸波动的最大半径
const float WAVE_SPEED = 1.5f;                           // 呼吸的速度
const glm::vec4 AURA_COLOR = {0.8f, 0.85f, 1.0f, 1.0f};  // 干净、柔和的浅蓝色

// 生成交互层的数据
void InteractLayerGenerator::generateLayer(ILayer::RenderDataBuffer& buffer) {
    // qDebug() << "生成交互图层";
    auto l = layer<RealTimeInteractLayer>();
    // --- 1. 获取并处理时间 ---
    static auto start = std::chrono::high_resolution_clock::now();
    // 获取当前时间点
    // 计算从 m_startTime 到现在的持续时间
    auto elapsed_duration = std::chrono::high_resolution_clock::now() - start;

    // 将其转换为一个浮点数表示的秒数。这个值会从 0.0 开始平滑增长。
    float elapsed_seconds =
        std::chrono::duration_cast<std::chrono::duration<float>>(
            elapsed_duration)
            .count();

    // --- 3. 循环创建每一个呼吸的圆环 ---
    for (int i = 0; i < NUM_RINGS; ++i) {
        RenderCommand cmd;

        // --- 每个圆环的时间都有一个微小的偏移，创造出波纹扩散的效果 ---
        float time_offset = (float(i) / NUM_RINGS) * glm::pi<float>();
        float current_time = elapsed_seconds * WAVE_SPEED + time_offset;

        // --- BaseInfo: 大小、位置、颜色 ---

        // 大小: 使用 sin 函数创造一个从基础大小向外平滑脉冲的效果
        // (1 + sin) / 2 的结果是 [0, 1]，非常适合用来做平滑的脉冲
        float pulse = (1.0f + std::sin(current_time)) / 2.0f;  // [0, 1]
        float current_radius = BASE_RADIUS + pulse * WAVE_AMPLITUDE;

        cmd.baseInfo.size = {current_radius * 2.0f, current_radius * 2.0f};

        // 位置: 始终居中于鼠标，并根据当前大小调整
        cmd.baseInfo.pos =
            glm::vec2{l->mouse.x(), l->mouse.y()} - glm::vec2(current_radius);

        // 颜色: 颜色固定，但透明度根据脉冲变化，向外扩散时变淡
        cmd.baseInfo.color = AURA_COLOR;
        cmd.baseInfo.color.a = (1.0f - pulse) * 0.7f;  // [0, 0.7]，越向外越透明

        // --- RadiusInfo: 纯粹的圆形，无任何特效 ---
        cmd.radiusInfo.radius = {1.f, 1.f};        // 保证是完美的圆形
        cmd.radiusInfo.radius_effect_param = 0.f;  // 禁用所有辉光特效

        // --- 将渲染指令推入缓冲区 ---
        buffer.push_back(cmd);
    }
}
