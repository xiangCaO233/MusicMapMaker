#ifndef MMM_TIMINGCOMPONENTS_HPP
#define MMM_TIMINGCOMPONENTS_HPP

// Timing组件,标记这是一个Timing实体
struct TimingComponent {
    double bpm;
    double beat_length;
    bool is_base_timing;
};

#endif  // MMM_TIMINGCOMPONENTS_HPP
