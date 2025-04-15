#ifndef M_HITOBJECT_H
#define M_HITOBJECT_H

#include <cstdint>
#include <string>

// 基本打击物件---之后可以实现更多音游的物件
enum class NoteType : uint8_t {
  // osu!mania的单键和长条
  NOTE = 0b00000001,
  HOLD = 0b01000000,
  // 另外的滑键和折线
  SLIDE = 0b00000010,
  COMPLEX = 0b00000100,
};

// 打击物件
class HitObject {
 public:
  // 构造HitObject
  explicit HitObject(uint32_t time);
  // 析构HitObject
  virtual ~HitObject();

  // 是否是面尾物件
  bool is_hold_end{false};

  // 物件时间戳
  uint32_t timestamp;

  // 打印用
  virtual std::string toString() = 0;

  bool operator>(const HitObject& other) const {
    return timestamp > other.timestamp;
  };

  bool operator>=(const HitObject& other) const {
    return timestamp >= other.timestamp;
  };

  bool operator<(const HitObject& other) const {
    return timestamp < other.timestamp;
  };

  bool operator<=(const HitObject& other) const {
    return timestamp <= other.timestamp;
  };
};

#endif  // M_HITOBJECT_H
