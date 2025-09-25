#ifndef MMM_TIMEPIXELCONVERTER_HPP
#define MMM_TIMEPIXELCONVERTER_HPP

#include <cstdint>
class MapCanvasInfo;

class TimePixelConverter {
   public:
    virtual ~TimePixelConverter() = default;
    virtual float timeToPixel(int64_t timestamp, int64_t current_canvas_time,
                              const MapCanvasInfo* info) const = 0;
    virtual int64_t distanceToTime(float pixel_y,
                                   int64_t current_canvas_time) const = 0;
};

#endif  // MMM_TIMEPIXELCONVERTER_HPP
