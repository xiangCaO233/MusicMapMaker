#include <QDebug>
#include <chrono>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <layer/interact/InteractLayerGenerator.hpp>

#include "layer/interact/RealTimeInteractLayer.hpp"

// 析构InteractLayerGenerator
InteractLayerGenerator::~InteractLayerGenerator() {
    qDebug() << "交互图层生成线程释放";
}
// --- 1. 动画参数和常量 (方便您随时调整) ---
const int NUM_PARTICLES = 20;            // 组成星环的粒子数量
const float ORBIT_RADIUS = 150.0f;       // 星环环绕鼠标的半径
const float BASE_PARTICLE_SIZE = 30.0f;  // 粒子的基础大小
const float ROTATION_SPEED = -0.4f;      // 粒子自身的旋转速度

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
    // --- 3. 循环创建每一个动态粒子 ---
    for (int i = 0; i < NUM_PARTICLES; ++i) {
        RenderCommand cmd;

        // --- 计算每个粒子独有的时间偏移，让它们在环上错开 ---
        // 这使得粒子们形成一条连续流动的“蛇”
        float particle_offset =
            (float(i) / NUM_PARTICLES) * 2.0f * glm::pi<float>();
        float particle_time = elapsed_seconds * 0.7f + particle_offset;

        // --- BaseInfo: 位置、大小、旋转、颜色 ---

        // 位置: 围绕鼠标做圆周运动
        cmd.baseInfo.pos.x =
            l->mouse.x() + ORBIT_RADIUS * std::cos(particle_time);
        cmd.baseInfo.pos.y =
            l->mouse.y() + ORBIT_RADIUS * std::sin(particle_time);

        // 大小: 使用 sin 函数创造平滑的脉冲缩放效果
        float size_pulse =
            0.5f * (1.0f + std::sin(particle_time * 2.0f));  // [0, 1]
        cmd.baseInfo.size = {
            BASE_PARTICLE_SIZE * (0.8f + size_pulse * 0.4f),
            BASE_PARTICLE_SIZE *
                (0.8f + size_pulse * 0.4f)};  // 在80%-120%大小间变化

        // 旋转: 持续、平滑地旋转
        cmd.baseInfo.rotation = elapsed_seconds * ROTATION_SPEED;

        // 颜色: 创造一种在“青色-品红-黄色”之间变化的赛博朋克风格颜色
        cmd.baseInfo.color.r =
            (1.0f + std::sin(particle_time * 1.1f)) / 2.0f;  // [0, 1]
        cmd.baseInfo.color.g =
            (1.0f + std::cos(particle_time * 0.8f)) / 2.0f;  // [0, 1]
        cmd.baseInfo.color.b =
            (1.0f + std::sin(particle_time * 0.5f)) / 2.0f;  // [0, 1]
        cmd.baseInfo.color.a = 0.8f;  // 稍微透明一点更有层次感

        // --- RadiusInfo: 圆角和辉光效果 ---

        // 圆角半径: 在方形和圆形之间平滑过渡
        float shape_morph = (1.0f + std::cos(particle_time)) / 2.0f;  // [0, 1]
        cmd.radiusInfo.radius = {shape_morph, shape_morph};  // 0=尖角, 1=圆形

        // 辉光/淡出效果: 让粒子产生呼吸灯一样的柔和光晕
        float glow_pulse =
            (1.0f + std::sin(particle_time * 1.5f)) / 2.0f;  // [0, 1]
        cmd.radiusInfo.radius_effect_param =
            30.0f + glow_pulse * 50.0f;  // 光晕半径在30-80像素间变化
        cmd.radiusInfo.radius_effect =
            RadiusEffect::FADE_IN_AND_OUT;  // 使用您指定的淡出特效

        // --- 重要！位置居中校正 ---
        // 由于 radius_effect_param
        // 会扩大最终绘制区域，我们需要重新校正位置使其居中
        float total_width =
            cmd.baseInfo.size.x + 2.0f * cmd.radiusInfo.radius_effect_param;
        float total_height =
            cmd.baseInfo.size.y + 2.0f * cmd.radiusInfo.radius_effect_param;
        cmd.baseInfo.pos.x -= total_width / 2.0f;
        cmd.baseInfo.pos.y -= total_height / 2.0f;

        // --- TexturesInfo: 保持默认纯色绘制 ---
        // (无需设置)

        // --- 将最终完成的渲染指令推入缓冲区 ---
        buffer.push_back(cmd);
    }
}
