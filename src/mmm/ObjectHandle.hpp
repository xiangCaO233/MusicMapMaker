#ifndef MMM_NOTEHANDLE_HPP
#define MMM_NOTEHANDLE_HPP

#include <math.h>

#include <cstddef>
#include <cstdint>
#include <functional>

// --- 稳定的句柄 (Stable Handle) ---

// Timing点的唯一标识句柄
struct TimingHandle {
    int32_t timestamp;
    double beat_length;  // 使用 beat_length 来区分红线和绿线

    // 重载 operator== 以便在 map 和 set 中比较
    bool operator==(const TimingHandle& other) const {
        return timestamp == other.timestamp &&
               std::abs(beat_length - other.beat_length) <
                   1e-9;  // 比较double要用精度
    }

    // 为句柄提供一个哈希函数，以便用作 std::unordered_map 的 Key
    struct Hash {
        std::size_t operator()(const TimingHandle& h) const {
            // 一个简单的组合哈希实现
            auto hash1 = std::hash<int32_t>{}(h.timestamp);
            auto hash2 = std::hash<double>{}(h.beat_length);
            return hash1 ^ (hash2 << 1);  // 或者使用更复杂的组合哈希
        }
    };
};

/**
 * @struct NoteHandle
 * @brief 一个稳定、唯一的音符标识符。
 * @details 结合了在存储中的索引和“代(generation)”计数，以防止ABA问题。
 *          即使一个槽位被回收并重新分配给新的音符，旧的句柄也会因为generation不匹配而失效。
 */
struct NoteHandle {
    uint32_t index;       ///< 在 SlottedArray 中的索引。
    uint32_t generation;  ///< 槽位的“代”，每次回收时递增。

    bool operator==(const NoteHandle& other) const = default;
    bool operator!=(const NoteHandle& other) const { return !(*this == other); }

    // 提供一个简单的工厂函数来获取无效句柄，比手动创建更清晰。
    static NoteHandle invalid() { return {0, 0}; }

    // 提供一个检查句柄有效性的方法。通常 generation 为 0 表示无效。
    bool isValid() const { return generation != 0; }

    // 为 std::unordered_map 提供哈希函数。
    struct Hash {
        size_t operator()(const NoteHandle& h) const {
            // 将两个32位整数合并成一个64位整数进行哈希，以获得更好的分布。
            return std::hash<uint64_t>{}(
                (static_cast<uint64_t>(h.generation) << 32) | h.index);
        }
    };
};

using NoteUUID = uint64_t;
// 定义一个常量来表示无效的稳定ID。
constexpr NoteUUID InvalidNoteUUID = 0;

#endif  // MMM_NOTEHANDLE_HPP
