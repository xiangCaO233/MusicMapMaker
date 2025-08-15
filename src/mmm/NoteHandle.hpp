#ifndef MMM_NOTEHANDLE_HPP
#define MMM_NOTEHANDLE_HPP

#include <cstddef>
#include <cstdint>
#include <functional>

// --- 稳定的句柄 (Stable Handle) ---

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

    // 为 std::unordered_map 提供哈希函数。
    struct Hash {
        size_t operator()(const NoteHandle& h) const {
            // 将两个32位整数合并成一个64位整数进行哈希，以获得更好的分布。
            return std::hash<uint64_t>{}(
                (static_cast<uint64_t>(h.generation) << 32) | h.index);
        }
    };
};

#endif  // MMM_NOTEHANDLE_HPP
