#ifndef M_OSUNOTE_H
#define M_OSUNOTE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../Note.h"
#include "OsuInfo.h"

class Slide;

class OsuNote : public Note {
   public:
    // 构造OsuNote
    OsuNote(uint32_t time, int32_t orbit_pos);
    // 从父类转化-填充属性
    explicit OsuNote(std::shared_ptr<Note> src);
    OsuNote();
    // 析构OsuNote
    ~OsuNote() override;

    // note采样
    NoteSample sample;

    // 物件音效组
    NoteSampleGroup sample_group;

    // 打印用
    std::string toString() override;

    // osu物件默认的元数据
    static std::shared_ptr<NoteMetadata> default_metadata();

    // 从滑键转化
    static std::vector<OsuNote> from_slide(std::shared_ptr<Slide> slide);

    // 从osu描述加载
    void from_osu_description(std::vector<std::string> &description,
                              int32_t orbit_count);
    // 转化为osu描述
    std::string to_osu_description(int32_t orbit_count);
};

#endif  // M_OSUNOTE_H
