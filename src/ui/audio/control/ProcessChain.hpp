#ifndef MMM_PROCESSCHAIN_HPP
#define MMM_PROCESSCHAIN_HPP
#include <memory>

#include "ice/core/IAudioNode.hpp"
#include "ice/core/SourceNode.hpp"
#include "ice/core/effect/Compresser.hpp"
#include "ice/core/effect/GraphicEqualizer.hpp"
#include "ice/core/effect/PitchAlter.hpp"
#include "ice/core/effect/TimeStretcher.hpp"

// 音效处理链
struct ProcessChain {
    std::shared_ptr<ice::SourceNode> source;
    std::shared_ptr<ice::TimeStretcher> stretcher;
    std::shared_ptr<ice::PitchAlter> pitchshifter;
    std::shared_ptr<ice::GraphicEqualizer> eq;
    std::shared_ptr<ice::Compressor> compressor;

    std::shared_ptr<ice::IAudioNode> output;

    explicit ProcessChain(const std::shared_ptr<ice::SourceNode>& src)
        : source(src) {
        stretcher = std::make_shared<ice::TimeStretcher>();
        stretcher->set_inputnode(source);

        pitchshifter = std::make_shared<ice::PitchAlter>();
        pitchshifter->set_inputnode(stretcher);

        std::vector<double> freqs = {31,   62,   125,  250,  500,
                                     1000, 2000, 4000, 8000, 16000};
        eq = std::make_shared<ice::GraphicEqualizer>(freqs);
        eq->set_inputnode(pitchshifter);

        compressor = std::make_shared<ice::Compressor>();
        compressor->set_inputnode(eq);

        output = compressor;
    };
};

#endif  // MMM_PROCESSCHAIN_HPP
