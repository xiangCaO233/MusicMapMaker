#ifndef MMM_SPECTROGRAMRENDERER_HPP
#define MMM_SPECTROGRAMRENDERER_HPP

#include <fftw3.h>
#include <qpaintdevice.h>

#include <QImage>
#include <audio/graphic/render/IRenderer.hpp>

class SpectrogramRenderer : public IRenderer {
   public:
    SpectrogramRenderer() {
        // 为 FFTW3 分配对齐的内存
        fftIn = static_cast<double*>(fftw_malloc(sizeof(float) * FFT_SIZE));
        fftOut = static_cast<fftw_complex*>(
            fftw_malloc(sizeof(fftwf_complex) * (FFT_SIZE / 2 + 1)));

        // 创建 FFTW3 执行计划 (实数到复数)
        fftPlan = fftw_plan_dft_r2c_1d(FFT_SIZE, fftIn, fftOut, FFTW_ESTIMATE);

        // 生成汉宁窗
        hannWindow.resize(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; ++i) {
            hannWindow[i] = 0.5 * (1.0 - cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
        }
    }
    ~SpectrogramRenderer() {
        // 清理 FFTW3 资源
        fftw_destroy_plan(fftPlan);
        fftw_free(fftIn);
        fftw_free(fftOut);
    }

    void process(const ProcessChain* chain,
                 const std::shared_ptr<ice::AudioTrack>& trackInfo,
                 size_t startFrame, size_t numFrames) override {
        if (!chain || numFrames == 0) return;

        // 拉取需要的音频数据
        ice::AudioBuffer buffer;
        buffer.resize(trackInfo->get_media_info().format, numFrames);

        chain->source->set_playpos(startFrame);
        chain->output->process(buffer);

        // 将多声道混合为单声道用于分析
        std::vector<double> monoData(numFrames);
        const float* const* planarData = buffer.raw_ptrs();
        for (size_t i = 0; i < numFrames; ++i) {
            float sample = 0.0f;
            for (int ch = 0; ch < buffer.afmt.channels; ++ch)
                sample += static_cast<double>(planarData[ch][i]);
            monoData[i] = sample / buffer.afmt.channels;
        }

        // 计算将生成多少个垂直的频谱线
        const int numSlices = (numFrames > FFT_SIZE)
                                  ? ((numFrames - FFT_SIZE) / HOP_SIZE + 1)
                                  : 1;

        // 创建或调整图像大小
        if (image.width() != numSlices || image.height() != (FFT_SIZE / 2)) {
            image = QImage(numSlices, FFT_SIZE / 2, QImage::Format_RGB32);
            image.fill(Qt::black);
        }

        // 循环处理每一块数据
        for (int i = 0; i < numSlices; ++i) {
            size_t frameOffset = i * HOP_SIZE;

            // 将数据块拷贝到 FFT 输入缓冲区并应用窗口函数
            for (int j = 0; j < FFT_SIZE; ++j) {
                if (frameOffset + j < numFrames) {
                    fftIn[j] = monoData[frameOffset + j] * hannWindow[j];
                } else {
                    fftIn[j] = 0.0f;
                }
            }

            // 执行 FFT
            fftw_execute(fftPlan);

            // 计算每个频率仓的幅度并映射为颜色
            for (int j = 0; j < FFT_SIZE / 2; ++j) {
                double real = fftOut[j][0];
                double imag = fftOut[j][1];
                // 幅度 = sqrt(real^2 + imag^2)
                double magnitude = sqrt(real * real + imag * imag);
                // 转换为分贝 (dB), 加上一个极小值避免 log(0)
                double magnitudeDb = 20.0 * log10(magnitude + 1e-9);

                // 将颜色绘制到图像的对应像素 (y轴是反的)
                image.setPixelColor(i, (FFT_SIZE / 2 - 1) - j,
                                    mapAmplitudeToColor(magnitudeDb));
            }
        }
    }

    void paint(QPainter* painter, QRect rect) override {
        if (!image.isNull()) {
            painter->drawImage(rect, image);
        }
    }

    // 颜色映射
    QColor mapAmplitudeToColor(double amplitudeDb) const {
        // dB 范围是 -60dB (安静) 到 0dB (最大)
        const double minDb = -60.0;
        const double maxDb = 0.0;

        // 将 dB 值归一化到 0.0 - 1.0
        double normalized = (amplitudeDb - minDb) / (maxDb - minDb);
        // 限制在 0-1 范围
        normalized = std::max(0.0, std::min(1.0, normalized));

        // 使用 HSV 颜色空间进行映射，从蓝色(低)到红色(高)
        return QColor::fromHsvF(normalized * 240.0 / 360.0, 1.0, normalized);
    }

   private:
    // FFT 相关参数
    // FFT 窗口大小
    static const int FFT_SIZE = 1024;
    // 帧之间的跳跃大小
    static const int HOP_SIZE = 512;

    // FFTW3 需要的缓冲区和计划
    // 输入缓冲区 (时域)
    double* fftIn;
    // 输出缓冲区 (频域)
    fftw_complex* fftOut;
    // FFTW3 的执行计划
    fftw_plan fftPlan;

    // 汉宁窗
    std::vector<double> hannWindow;

    // 生成频谱图
    QImage image;
};

#endif  // MMM_SPECTROGRAMRENDERER_HPP
