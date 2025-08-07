#ifndef MMM_NOTE_HPP
#define MMM_NOTE_HPP

#include <cstdint>
#include <memory>
#include <mmm/MetaData.hpp>
#include <vector>

class MMap;
class Slide;

// 基本物件
class Note {
   public:
    // 构造Note
    explicit Note(const std::shared_ptr<MMap>& map) : map_ref(map) {};

    // 析构Note
    virtual ~Note() = default;

    // 打印用
    virtual std::string toString();

    // 访问时间戳
    inline uint32_t timestamp() const { return time; }

    // 访问轨道位置
    inline uint32_t trackpos() const { return track; }

    // 访问map引用
    inline std::shared_ptr<MMap> map() const { return map_ref.lock(); }

    // 设置时间戳
    virtual void set_timestamp(uint32_t t) { time = t; }

    // 设置轨道
    virtual void set_trackpos(uint32_t o) { track = o; }

    // 从滑键转化
    static std::vector<Note> from_slide(std::shared_ptr<Slide> slide);

   private:
    // 时间
    uint32_t time{0};

    // 轨道
    uint32_t track{0};

    // map弱引用
    std::weak_ptr<MMap> map_ref;

    // 元数据集
    std::unordered_map<NoteMetadataType, std::shared_ptr<NoteMetadata>>
        metadatas;
};

#endif  // MMM_NOTE_HPP
