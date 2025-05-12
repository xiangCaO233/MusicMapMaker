#ifndef M_UTIL_H
#define M_UTIL_H

#include <qcolor.h>
#include <qcontainerfwd.h>
#include <qmenu.h>
#include <qpushbutton.h>
#include <qsize.h>
#include <qtoolbutton.h>
#include <unicode/ucnv.h>
#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include <unicode/ustring.h>

#include <QFile>
#include <QPainter>
#include <QSvgRenderer>
#include <QTime>
#include <QtSvgWidgets/QSvgWidget>
#include <memory>
#include <string>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#include "../mmm/Beat.h"
#include "../mmm/hitobject/HitObject.h"
#include "../mmm/hitobject/Note/rm/ComplexNote.h"

namespace mutil {
#ifdef _WIN32
// UTF-8 → UTF-16
inline std::wstring utf8_to_utf16(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                          (int)utf8.size(), nullptr, 0);
    std::wstring utf16(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), utf16.data(),
                        size_needed);
    return utf16;
}

// UTF-16 → UTF-32
inline std::u32string utf16_to_utf32(const std::wstring& utf16) {
    std::u32string utf32;
    for (wchar_t c : utf16) {
        utf32.push_back(
            static_cast<char32_t>(c));  // 简单转换（假设没有代理对）
    }
    return utf32;
}

// 组合函数：UTF-8 → UTF-32
inline std::u32string utf8_to_utf32(const std::string& utf8) {
    std::wstring utf16 = utf8_to_utf16(utf8);
    return utf16_to_utf32(utf16);
}
#endif

// 替换弃用的 wstring_convert
inline std::u32string cu32(const std::string& utf8) {
    icu_76::UnicodeString utf16 = icu_76::UnicodeString::fromUTF8(utf8);
    std::u32string utf32;
    utf32.reserve(utf16.length());

    for (int32_t i = 0; i < utf16.length();) {
        UChar32 c = utf16.char32At(i);
        utf32.push_back(c);
        i += U16_LENGTH(c);
    }
    return utf32;
}

inline bool isStringAllDigits_Iteration(const QString& str) {
    // 1. 处理空字符串的情况 (根据需求，空字符串可能算 true 或 false)
    // 通常认为空字符串不全是数字，所以返回 false
    if (str.isEmpty()) {
        return false;
    }

    // 2. 遍历字符串中的每个字符
    for (const QChar& ch : str) {
        // 3. 如果遇到任何一个非数字字符，立即返回 false
        if (!ch.isDigit()) {
            return false;
        }
    }

    // 4. 如果循环结束都没有返回 false，说明所有字符都是数字
    return true;
}

// 判断是否全选组合键内的子键
inline bool full_selected_complex(
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>& srcobjs,
    std::shared_ptr<ComplexNote> comp) {
    if (srcobjs.size() != comp->child_notes.size()) return false;
    auto srcit = srcobjs.begin();
    auto compit = comp->child_notes.begin();
    while (srcit != srcobjs.end() && compit != comp->child_notes.end()) {
        if (*srcit != *compit) {
            return false;
        }
        ++srcit;
        ++compit;
    }
    return true;
}
// --- 辅助函数：完成片段并添加到结果集 ---
// 参数 segment 通过引用传递，以便在函数内将其重置为 nullptr
inline void finalize_and_add_segment(
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>& res,
    std::shared_ptr<ComplexNote>& segment)  // 引用传递，用于重置
{
    // 如果片段为空或无效，则直接返回并重置
    if (!segment || segment->child_notes.empty()) {
        segment = nullptr;  // 确保重置
        return;
    }

    // 可选但推荐：检查片段是否真的有效（以 HEAD 开始，以 END 结束）
    bool is_valid_structure = false;
    if (!segment->child_notes.empty()) {
        auto first_child =
            std::dynamic_pointer_cast<Note>(*segment->child_notes.begin());
        auto last_child =
            std::dynamic_pointer_cast<Note>(*segment->child_notes.rbegin());
        if (first_child && last_child &&
            first_child->compinfo == ComplexInfo::HEAD &&
            last_child->compinfo == ComplexInfo::END) {
            is_valid_structure = true;
        }
    }

    if (is_valid_structure) {
        // 根据规则：如果拆分出的 comp（即 segment）只有一个物件，则加 Note
        if (segment->child_notes.size() == 1) {
            // 一个有效的 HEAD...END 序列至少包含两个元素（HEAD 和 END）
            // 所以 size == 1 且 is_valid_structure 为 true 的情况理论上不应发生
            // 除非 HEAD 和 END 是同一个对象？
            // 或者这里的规则指的是：如果最终构建的有效片段只包含一个 Note
            // 实例？ 我们遵循字面意思：如果这个完成的、有效的 segment
            // 里只有一个元素 （这暗示原始的 HEAD 和 END
            // 可能是同一个对象，且该对象存在）， 则将该 Note 加入 res。
            res.insert(*segment->child_notes.begin());
        } else {  // segment->child_objects.size() > 1
            // 如果包含多个物件，则将 Complex 对象加入 res
            // 可以考虑将原始 comp 的一些非子对象属性复制到 segment
            // segment->some_metadata = original_comp->some_metadata;
            res.insert(segment);
        }
    } else {
        // 如果片段结构无效（例如，没有以 END 结尾就被中断了），则丢弃它
        // std::cout << "Debug: Discarding invalid segment." << std::endl;
    }

    // 重置 segment 指针，为下一个可能的片段做准备
    segment = nullptr;
}

// 拆分组合键
/**
 * @brief 根据可用的子对象拆分一个 Complex 对象。
 * @param res [输出] 用于存放结果 Complex 对象或单个 Note 对象的 multiset。
 * @param comp [输入] 要拆分的原始 Complex 对象。
 * @param remove_objs [输入] 包含 comp 子对象交集的 multiset (只读)。
 */
inline void destruct_complex_note(
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>& res,
    std::shared_ptr<ComplexNote> comp,
    const std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>&
        srcobjs) {
    // 基本检查
    if (!comp || comp->child_notes.empty() || srcobjs.empty()) {
        return;  // 无事可做
    }

    // --- 特殊情况处理：原始 comp 只有一个子对象 ---
    if (comp->child_notes.size() == 1) {
        // 获取这唯一的子对象
        const auto& single_original_child = *comp->child_notes.begin();
        // 检查这个子对象是否存在于 remove_objs 中
        auto it_found = srcobjs.find(single_original_child);
        if (it_found != srcobjs.end()) {
            // 如果存在，根据规则，直接将这个 Note 加入结果集
            res.insert(
                *it_found);  // 注意：这里插入的是 remove_objs 中的指针实例
        }
        return;  // 处理完毕
    }

    // --- 正常处理流程 ---
    auto orig_it = comp->child_notes.begin();  // 迭代器指向原始子对象序列
    auto orig_end = comp->child_notes.end();
    auto present_it = srcobjs.begin();  // 迭代器指向可用子对象序列
    auto present_end = srcobjs.end();

    std::shared_ptr<ComplexNote> current_segment =
        nullptr;                                          // 当前正在构建的片段
    HitObjectComparator comparator = srcobjs.key_comp();  // 获取比较器实例

    while (orig_it != orig_end && present_it != present_end) {
        // 1. 将 present_it 向前移动，跳过那些在 orig_it 之前的可用对象
        //    (因为两个序列都已排序)
        while (present_it != present_end && comparator(*present_it, *orig_it)) {
            ++present_it;
        }

        // 如果 present_it 到达末尾，说明后面不可能有匹配 orig_it
        // 或更后元素的可用对象了
        if (present_it == present_end) {
            break;
        }

        // 2. 检查当前 orig_it 指向的对象是否存在于 present_it 的位置
        //    使用比较器判断等价性：!comp(a, b) && !comp(b, a)
        //    由于 present_it 已经 >= orig_it，我们只需要检查 !comp(orig_it,
        //    present_it)
        bool match_found = !comparator(*orig_it, *present_it);

        if (match_found) {
            // --- 找到匹配 ---
            // 获取当前匹配到的 Note 对象及其类型信息
            // 注意：*present_it 是 HitObject 指针，需要转为 Note 指针
            auto current_note = std::dynamic_pointer_cast<Note>(*present_it);
            if (!current_note) {
                // 理论上不应发生，因为 remove_objs 是与 comp->child_objects
                // (Note集合) 的交集 但为健壮性考虑，可以跳过或报错
                std::cerr << "Warning: Found object in intersection that is "
                             "not a Note?"
                          << std::endl;
                ++orig_it;
                ++present_it;  // 消耗掉这个匹配但不合法的对象
                continue;
            }

            ComplexInfo info = current_note->compinfo;

            // --- 根据 Note 类型处理 ---
            if (info == ComplexInfo::HEAD) {
                // 如果当前正在构建片段，说明之前的片段未正常结束（缺少
                // END），需要先 finalize（通常会丢弃）
                finalize_and_add_segment(res, current_segment);

                // 开始一个新的片段
                current_segment = std::make_shared<ComplexNote>(
                    current_note->timestamp, current_note->orbit);
                // 可选：从原始 comp 复制一些元数据到 new segment
                current_segment->child_notes.insert(current_note);  // 加入 HEAD

            } else if (info == ComplexInfo::BODY) {
                if (current_segment) {
                    // 如果已开始片段，将 BODY 加入
                    current_segment->child_notes.insert(current_note);
                } else {
                    // 孤立的 BODY，忽略 (因为没有 HEAD 开始)
                }
            } else if (info == ComplexInfo::END) {
                if (current_segment) {
                    // 如果已开始片段，加入 END 并完成这个片段
                    current_segment->child_notes.insert(current_note);
                    finalize_and_add_segment(
                        res, current_segment);  // 完成并加入结果集 (如果有效)
                    // finalize_and_add_segment 会将 current_segment 设为
                    // nullptr
                } else {
                    // 孤立的 END，忽略 (因为没有 HEAD/BODY 开始)
                }
            } else {  // ComplexInfo::NONE 或其他未知类型
                if (current_segment) {
                    // 在片段中遇到非组合键部分，视为序列中断
                    finalize_and_add_segment(
                        res, current_segment);  // finalize (可能丢弃)
                }
                // 忽略这个 NONE 类型的音符
            }

            // 成功匹配并处理后，两个迭代器都向前移动
            ++orig_it;
            ++present_it;

        } else {
            // --- 未找到匹配 ---
            // 当前 orig_it 指向的原始子对象在 remove_objs 中不存在
            // 这意味着任何正在构建的片段都被中断了
            if (current_segment) {
                finalize_and_add_segment(
                    res, current_segment);  // finalize (可能丢弃)
                // current_segment 被设为 nullptr
            }
            // 只移动 orig_it，继续检查原始序列的下一个期望对象
            // present_it 不动，因为它指向的对象可能匹配原始序列中更后面的对象
            ++orig_it;
        }
    }
    // 循环结束后，如果 current_segment 仍然存在，说明最后一个片段没有以 END
    // 正常结束 需要 finalize 它 (通常会被丢弃，因为它不完整)
    finalize_and_add_segment(res, current_segment);
}

// 将毫秒值转换为 "hh:mm:ss.zzz" 格式 QString
inline QString millisecondsToQString(long long totalMilliseconds) {
    if (totalMilliseconds < 0) {
        totalMilliseconds = 0;  // QTime 不直接处理负的总毫秒数，这里将其视为0
    }
    // QTime::fromMSecsSinceStartOfDay 处理的是一天内的毫秒数
    // 如果毫秒数可能超过一天 (24 * 3600 * 1000)，需要手动计算小时
    const qlonglong msPerDay = 24LL * 60 * 60 * 1000;
    qlonglong days = totalMilliseconds / msPerDay;
    int msecs = totalMilliseconds % msPerDay;

    QTime time = QTime::fromMSecsSinceStartOfDay(msecs);

    // 手动加上超过24小时的部分
    qlonglong totalHours = days * 24 + time.hour();

    // 格式化输出，注意小时数可能超过两位
    return QString("%1:%2:%3.%4")
        .arg(totalHours, 2, 10, QChar('0'))  // 至少2位，用0填充
        .arg(time.minute(), 2, 10, QChar('0'))
        .arg(time.second(), 2, 10, QChar('0'))
        .arg(time.msec(), 3, 10, QChar('0'));  // 毫秒总是3位

    // --- 或者，如果保证毫秒数不超过一天，可以简化 ---
    // QTime time = QTime(0,0,0,0).addMSecs(static_cast<int>(totalMilliseconds %
    // msPerDay)); // 注意 addMSecs 参数是 int return
    // time.toString("hh:mm:ss.zzz"); 但这种方式对于超过 int
    // 范围或超过一天的毫秒数会出问题或不准确
    // 因此，上面的手动计算方式更可靠处理任意大的 long long 毫秒值
}

// 将 "hh:mm:ss.zzz" 格式 QString 转换为毫秒值
inline long long qstringToMilliseconds(const QString& timeString) {
    // QTime::fromString 不能直接处理超过 23:59:59.999 的时间
    // 我们需要手动解析
    QStringList parts = timeString.split(':');
    if (parts.size() != 3) {
        qWarning()
            << "Invalid time string format (parts based on ':'). Expected "
               "hh:mm:ss.zzz, got:"
            << timeString;
        return -1;  // 或者抛出异常
    }

    QStringList secMsPart = parts[2].split('.');
    if (secMsPart.size() != 2) {
        qWarning()
            << "Invalid time string format (parts based on '.'). Expected "
               "hh:mm:ss.zzz, got:"
            << timeString;
        return -1;  // 或者抛出异常
    }

    bool okH, okM, okS, okMs;
    qlonglong hh = parts[0].toLongLong(&okH);
    int mm = parts[1].toInt(&okM);
    int ss = secMsPart[0].toInt(&okS);
    int ms = secMsPart[1].toInt(&okMs);

    // 检查转换是否成功和基本范围
    if (!okH || !okM || !okS || !okMs || hh < 0 || mm < 0 || mm >= 60 ||
        ss < 0 || ss >= 60 || ms < 0 || ms >= 1000) {
        qWarning()
            << "Invalid time component values or conversion failed in string:"
            << timeString;
        return -1;  // 或者抛出异常
    }

    long long totalMilliseconds = 0;
    totalMilliseconds += hh * 60 * 60 * 1000;
    totalMilliseconds += static_cast<long long>(mm) * 60 * 1000;
    totalMilliseconds += static_cast<long long>(ss) * 1000;
    totalMilliseconds += ms;

    return totalMilliseconds;
}

inline bool isApproxEqual(double a, double b, double tolerance) {
    return std::abs(a - b) <= tolerance;
}
// 计算分音策略(2~64)并在找到有效策略时设置divpos
inline int calculateDivisionStrategy(
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>& hitobjects,
    const std::shared_ptr<Beat>& beat, double tolerance) {
    auto beat_length = beat->end_timestamp - beat->start_timestamp;
    if (hitobjects.empty() || beat_length <= 0) return 1;

    // 收集当前拍内的所有物件(包括重复时间戳的) [beat_start, beat_end)
    std::vector<std::shared_ptr<HitObject>> current_beat_objects;
    for (const auto& obj : hitobjects) {
        if (obj->timestamp >= beat->start_timestamp &&
            obj->timestamp < beat->end_timestamp) {
            current_beat_objects.push_back(obj);
        }
    }
    if (current_beat_objects.empty()) return 1;

    // 从最小分音数开始检查（2到64）
    for (int n = 2; n <= 64; ++n) {
        bool valid = true;
        double sub_beat = beat_length / n;

        // 临时存储divpos值，验证通过后再设置
        std::unordered_map<std::shared_ptr<HitObject>, int32_t> divpos_map;

        for (const auto& obj : current_beat_objects) {
            double ts = obj->timestamp;
            // 计算最接近的分音点位置
            double pos = (ts - beat->start_timestamp) / sub_beat;
            int32_t rounded_pos = static_cast<int32_t>(std::round(pos));
            double closest_sub = beat->start_timestamp + rounded_pos * sub_beat;

            if (isApproxEqual(ts, closest_sub, tolerance)) {
                divpos_map[obj] = rounded_pos;
            } else {
                valid = false;
                break;
            }
        }

        if (valid) {
            // 设置所有匹配物件的divpos
            for (const auto& [obj, pos] : divpos_map) {
                obj->divpos = pos;
            }
            return n;
        }
    }

    return 1;  // 无有效分音策略
}

inline void format_music_time2u32(std::u32string& res, double srcmilliseconds) {
    // 确保毫秒数为非负数
    if (srcmilliseconds < 0) {
        srcmilliseconds = 0;
    }

    // 计算分钟、秒和毫秒
    uint32_t total_ms = static_cast<uint32_t>(std::round(srcmilliseconds));
    uint32_t minutes = total_ms / 60000;
    uint32_t remaining_ms = total_ms % 60000;
    uint32_t seconds = remaining_ms / 1000;
    uint32_t milliseconds = remaining_ms % 1000;
    // 格式化为 "mm:ss:mmm"
    std::string temp;

    // 分钟部分（两位数）
    if (minutes < 10) {
        temp += '0';
    }
    temp += std::to_string(minutes);
    temp += ':';

    // 秒部分（两位数）
    if (seconds < 10) {
        temp += '0';
    }
    temp += std::to_string(seconds);
    temp += ':';

    // 毫秒部分（三位数）
    if (milliseconds < 100) {
        temp += '0';
        if (milliseconds < 10) {
            temp += '0';
        }
    }
    temp += std::to_string(milliseconds);

    res.clear();
#ifdef _WIN32
    res = utf8_to_utf32(temp);
#else
    res = cu32(temp);
#endif  //_WIN32
}

inline void format_music_time2milliseconds(const std::string& src,
                                           double& desmilliseconds) {}

inline void get_colored_icon_pixmap(QPixmap& pixmap, const char* svgpath,
                                    QColor& color, QSize& size) {
    // 加载SVG文件
    QSvgWidget svgWidget(svgpath);
    QByteArray svgData;
    QFile file(svgpath);
    if (file.open(QIODevice::ReadOnly)) {
        svgData = file.readAll();
        file.close();
    }

    // 添加或替换fill属性
    QString svgString = QString::fromUtf8(svgData);
    if (svgString.contains("fill=\"\"")) {
        // 替换为指定颜色
        svgString.replace("fill=\"\"", "fill=\"" + color.name() + "\"");
    } else {
        // 如果没有fill属性，直接插入
        svgString.replace("<path ", "<path fill=\"" + color.name() + "\" ");
    }

    // 创建QPixmap
    QSvgRenderer renderer(svgString.toUtf8());
    pixmap = QPixmap(size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
}

inline void set_button_svgcolor(QPushButton* button, const char* svgpath,
                                QColor& color, int32_t w, int32_t h) {
    // 创建QPixmap
    QPixmap pixmap;
    QSize size(w, h);
    get_colored_icon_pixmap(pixmap, svgpath, color, size);

    // 设置图标
    button->setIcon(QIcon(pixmap));
}
inline void set_toolbutton_svgcolor(QToolButton* button, const char* svgpath,
                                    QColor& color, int32_t w, int32_t h) {
    // 创建QPixmap
    QPixmap pixmap;
    QSize size(w, h);
    get_colored_icon_pixmap(pixmap, svgpath, color, size);

    // 设置图标
    button->setIcon(QIcon(pixmap));
}
inline void set_action_svgcolor(QAction* action, const char* svgpath,
                                QColor& color) {
    // 创建QPixmap
    QPixmap pixmap;
    QSize size(28, 28);
    get_colored_icon_pixmap(pixmap, svgpath, color, size);

    // 设置图标
    action->setIcon(QIcon(pixmap));
}

inline void set_menu_svgcolor(QMenu* menu, const char* svgpath, QColor& color) {
    // 创建QPixmap
    QPixmap pixmap;
    QSize size(28, 28);
    get_colored_icon_pixmap(pixmap, svgpath, color, size);

    // 设置图标
    menu->setIcon(QIcon(pixmap));
}
};  // namespace mutil

#endif  // M_UTIL_H
