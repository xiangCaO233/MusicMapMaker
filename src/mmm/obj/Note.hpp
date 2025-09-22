#ifndef MMM_NOTE_HPP
#define MMM_NOTE_HPP

#include <cstdint>
#include <list>
#include <memory>
#include <mmm/MetaData.hpp>

enum class NoteType : uint8_t {
    // 普通物件
    NORMAL = 0,
    // 滑键
    SLIDE = 1,
    // 长条
    HOLD = 2,
    // 组合键
    COMPOSITE = 3,
};

inline std::string to_string(NoteType type) {
    switch (type) {
        case NoteType::NORMAL:
            return "NORMAL";
        case NoteType::HOLD:
            return "HOLD";
        case NoteType::SLIDE:
            return "SLIDE";
        case NoteType::COMPOSITE:
            return "COMPOSITE";
        default:
            return "Unknown";
    }
}

class MMap;
class Slide;

// 基本物件
class Note {
   public:
    // 构造Note
    explicit Note(const MMap* map) : map_ref(map) {};

    // 析构Note
    virtual ~Note() = default;

    // 打印用
    virtual std::string toString() const;

    // 访问类型
    inline NoteType notetype() const { return type; }

    // 访问时间戳
    inline uint32_t timestamp() const { return time; }

    // 访问轨道位置
    inline uint32_t trackpos() const { return track; }

    // 访问map引用
    inline const MMap* map() const { return map_ref; }

    // 访问元数据
    inline std::unordered_map<NoteMetadataType, std::shared_ptr<NoteMetadata>>&
    metadata() {
        return metadatas;
    }

    // 从滑键转换
    static std::list<std::unique_ptr<Note>> from_slide(const Slide* slide);

    // 克隆物件
    virtual std::unique_ptr<Note> clone(const MMap* ref) const;

   protected:
    // 设置类型
    void set_notetype(NoteType t) { type = t; }

    // 设置时间戳
    virtual void set_timestamp(uint32_t t) { time = t; }

    // 设置轨道
    virtual void set_trackpos(uint32_t o) { track = o; }

   private:
    // 物件类型
    NoteType type{NoteType::NORMAL};

    // 时间
    uint32_t time{0};

    // 轨道
    uint32_t track{0};

    // map引用
    const MMap* map_ref;

    // 元数据集
    std::unordered_map<NoteMetadataType, std::shared_ptr<NoteMetadata>>
        metadatas;

    friend class Composite;
    friend class OsuNote;
    friend class MMap;
    friend class Hold;
    friend class MMapEditor;
};

#endif  // MMM_NOTE_HPP
