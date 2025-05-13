#include "FrameRateCounter.h"

FrameRateCounter::FrameRateCounter(QWidget *parent) : QObject(parent) {
    connect(&timer, &QTimer::timeout, this, &FrameRateCounter::updateFPS);
    timer.start(1000);  // 每秒更新一次
}

void FrameRateCounter::frameRendered() { frameCount++; }

void FrameRateCounter::updateFPS() {
    emit fpsUpdated(frameCount);
    frameCount = 0;
}
