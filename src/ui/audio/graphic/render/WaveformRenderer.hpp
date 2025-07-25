#ifndef MMM_WAVEFORMRENDERER_HPP
#define MMM_WAVEFORMRENDERER_HPP
#include <qpaintdevice.h>

#include <QLineF>
#include <QVector>
#include <audio/graphic/render/IRenderer.hpp>

#include "ice/manage/AudioBuffer.hpp"

class WaveformRenderer : public IRenderer {
   public:
    void process(const ProcessChain* chain,
                 const std::shared_ptr<ice::AudioTrack>& trackInfo,
                 size_t startFrame, size_t numFrames) override {
        lines.clear();
        if (!chain || numFrames == 0) return;

        ice::AudioBuffer buffer;
        buffer.resize(trackInfo->get_media_info().format, numFrames);
        chain->source->play();
        chain->source->set_playpos(startFrame);
        chain->output->process(buffer);
        chain->source->pause();

        int numChannels = buffer.afmt.channels;
        lines.resize(numChannels);

        for (int ch = 0; ch < numChannels; ++ch) {
            // 为每个采样点都画一条线，没有做min/max优化
            lines[ch].reserve(numFrames);
            for (size_t i = 0; i < numFrames; ++i) {
                float sample = buffer.raw_ptrs()[ch][i];
                lines[ch].append(
                    QLineF(i, 0.5 - sample * 0.5, i, 0.5 + sample * 0.5));
            }
        }
    };

    void paint(QPainter* painter, QRect rect) override {
        if (lines.isEmpty()) return;

        painter->save();
        const static QVector<QColor> channelColors = {Qt::cyan, Qt::green};

        // 设置变换，将我们的逻辑坐标(0-numFrames, 0-1)映射到控件矩形
        painter->setWindow(0, 0, lines[0].size(), 1);
        painter->setViewport(rect);

        for (int ch = 0; ch < lines.size(); ++ch) {
            QColor color = channelColors[ch % channelColors.size()];
            color.setAlpha(180);
            // 0宽度画笔表示最细的线
            painter->setPen(QPen(color, 0));
            painter->drawLines(lines[ch]);
        }
        painter->restore();
    }

   private:
    // 缓存绘制数据
    QVector<QVector<QLineF>> lines;
};
#endif  // MMM_WAVEFORMRENDERER_HPP
