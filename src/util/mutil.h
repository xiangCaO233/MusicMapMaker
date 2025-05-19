#ifndef M_UTIL_H
#define M_UTIL_H

#include <qcolor.h>
#include <qcombobox.h>
#include <qcontainerfwd.h>
#include <qfiledialog.h>
#include <qgridlayout.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qmenu.h>
#include <qpushbutton.h>
#include <qsize.h>
#include <qtoolbutton.h>
#include <qtreeview.h>
#include <qvariant.h>
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

#include "colorful-log.h"

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
    icu::UnicodeString utf16 = icu::UnicodeString::fromUTF8(utf8);
    std::u32string utf32;
    utf32.reserve(utf16.length());

    for (int32_t i = 0; i < utf16.length();) {
        UChar32 c = utf16.char32At(i);
        utf32.push_back(c);
        i += U16_LENGTH(c);
    }
    return utf32;
}

inline std::string sanitizeFilename(std::string filename) {
    // 定义非法字符（Windows + Unix 常见限制）
    const std::string illegalChars = "\\/:*?\"<>|";

    // 移除非法字符
    filename.erase(
        std::remove_if(filename.begin(), filename.end(),
                       [&illegalChars](char c) {
                           // 检查是否是控制字符或非法字符
                           return (c < 32) ||
                                  (illegalChars.find(c) != std::string::npos);
                       }),
        filename.end());

    // 替换空格为下划线（可选）
    std::replace(filename.begin(), filename.end(), ' ', '_');

    return filename;
}

// 判断路径是否以指定字符串结尾
inline bool endsWithExtension(const std::filesystem::path& filepath,
                              const std::string& suffix) {
    // 获取文件名（带扩展名）
    std::string filename = filepath.filename().string();

    // 检查是否以指定后缀结尾
    if (filename.length() >= suffix.length() &&
        filename.compare(filename.length() - suffix.length(), suffix.length(),
                         suffix) == 0) {
        return true;
    }
    return false;
}

inline QString getSaveDirectoryWithFilename(
    QWidget* parent, const QString& title, const QString& format_label,
    const QMap<QString, QString>& formatFilters,
    const QMap<QString, QString>& defaultFilenames,
    const QString& defaultFormat = QString()) {
    QFileDialog dialog(parent, title, XLogger::last_select_directory);

    // 基本设置
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    // 准备过滤器字符串
    QStringList filterList;
    for (auto it = formatFilters.begin(); it != formatFilters.end(); ++it) {
        filterList.append(QString("%1 (*%2)").arg(it.key()).arg(it.value()));
    }
    dialog.setNameFilters(filterList);

    // 创建格式选择组合框
    QComboBox* formatCombo = new QComboBox(&dialog);
    int defaultIndex = 0;
    int currentIndex = 0;

    for (auto it = formatFilters.begin(); it != formatFilters.end(); ++it) {
        formatCombo->addItem(
            it.key(), QVariant::fromValue(QPair<QString, QString>(
                          it.value(), defaultFilenames.value(it.key()))));

        // 检查是否是默认格式
        if (it.key() == defaultFormat) {
            defaultIndex = currentIndex;
        }
        currentIndex++;
    }

    // 将组合框添加到对话框布局
    QGridLayout* layout = static_cast<QGridLayout*>(dialog.layout());
    if (layout) {
        int rowCount = layout->rowCount();
        layout->addWidget(new QLabel(format_label, &dialog), rowCount, 0);
        layout->addWidget(formatCombo, rowCount, 1);
    }

    // 设置默认选择
    if (!formatFilters.isEmpty()) {
        formatCombo->setCurrentIndex(defaultIndex);
        dialog.selectNameFilter(filterList.value(defaultIndex));
    }

    // 获取文件名编辑框
    QLineEdit* lineEdit = dialog.findChild<QLineEdit*>();

    // 设置初始文件名
    if (lineEdit && !formatFilters.isEmpty()) {
        QString currentKey = formatFilters.keys().value(defaultIndex);
        QString initialFilename =
            defaultFilenames.value(currentKey, "untitled");
        QString initialExtension = formatFilters.value(currentKey);
        lineEdit->setText(initialFilename + initialExtension);
    }

    // 当格式改变时的处理
    QObject::connect(
        formatCombo, &QComboBox::currentIndexChanged, [&](int index) {
            if (index < 0 || index >= formatFilters.size()) return;

            // 获取当前选择的数据
            QPair<QString, QString> data =
                formatCombo->currentData().value<QPair<QString, QString>>();
            QString selectedExtension = data.first;
            QString defaultFilename = data.second;

            // 更新文件名
            if (lineEdit) {
                QString newFilename =
                    defaultFilename.isEmpty()
                        ? QFileInfo(lineEdit->text()).completeBaseName()
                        : defaultFilename;
                newFilename += selectedExtension;
                lineEdit->setText(newFilename);
            }

            // 更新文件过滤器
            dialog.selectNameFilter(filterList.at(index));
        });

    // 设置单选模式
    if (QListView* listView = dialog.findChild<QListView*>("listView")) {
        listView->setSelectionMode(QAbstractItemView::SingleSelection);
    }
    if (QTreeView* treeView = dialog.findChild<QTreeView*>()) {
        treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    }

    // 执行对话框
    if (dialog.exec() == QDialog::Accepted) {
        QStringList selected = dialog.selectedFiles();
        if (!selected.isEmpty()) {
            // 确保文件名有正确的扩展名
            QString selectedFile = selected.first();
            QPair<QString, QString> data =
                formatCombo->currentData().value<QPair<QString, QString>>();
            QString expectedExtension = data.first;

            if (!selectedFile.endsWith(expectedExtension,
                                       Qt::CaseInsensitive)) {
                // 如果用户输入的文件名没有正确的扩展名，自动添加
                if (QFileInfo(selectedFile).suffix().isEmpty()) {
                    selectedFile += expectedExtension;
                }
            }

            return selectedFile;
        }
    }

    return QString();  // 用户取消选择
}

// 判断字符串是否完全由数字组成
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
/**
 * @brief 将文件拷贝到指定目录并返回目标文件路径
 * @param f 源文件绝对路径
 * @param p 目标目录路径
 * @param[out] result_path 拷贝结果文件的绝对路径
 * @return 拷贝是否成功
 */
inline bool copyFileToPath(const std::filesystem::path& f,
                           const std::filesystem::path& p,
                           std::filesystem::path& result_path) {
    try {
        // 检查源文件是否存在且是常规文件
        if (!std::filesystem::exists(f) ||
            !std::filesystem::is_regular_file(f)) {
            std::cerr << "Source file does not exist or is not a regular file"
                      << std::endl;
            return false;
        }

        // 检查目标目录是否存在
        if (!std::filesystem::exists(p)) {
            std::cerr << "Target directory does not exist" << std::endl;
            return false;
        }

        // 获取文件名部分
        std::filesystem::path filename = f.filename();

        // 构建目标路径
        result_path = p / filename;

        // 拷贝文件 (使用覆盖选项)
        std::filesystem::copy_file(
            f, result_path, std::filesystem::copy_options::overwrite_existing);

        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return false;
    }
}
// 检查路径 p 中是否存在文件 f (f 是绝对路径)
inline bool fileExistsInPath(const std::filesystem::path& f,
                             const std::filesystem::path& p) {
    try {
        // 获取文件名部分
        std::filesystem::path filename = f.filename();

        // 构建目标路径
        std::filesystem::path target_path = p / filename;

        // 检查文件是否存在且是常规文件
        return std::filesystem::exists(target_path) &&
               std::filesystem::is_regular_file(target_path);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return false;
    }
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

// --- 辅助函数：完成片段并将其添加到结果集 (处理独立物件的 parent 和 compinfo)
inline void finalize_and_add_segment(  // 重命名以示区别
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>& res,
    std::shared_ptr<ComplexNote>& segment,  // 当前构建的片段，包含克隆的 Note
    const std::shared_ptr<ComplexNote>& original_comp_base) {
    if (!segment || segment->child_notes.empty()) {
        segment = nullptr;
        return;
    }

    if (segment->child_notes.size() == 1) {
        // 片段只有一个物件，它将成为独立的 Note
        auto single_hit_object_ptr = *segment->child_notes.begin();
        auto single_note_ptr =
            std::dynamic_pointer_cast<Note>(single_hit_object_ptr);

        if (single_note_ptr) {
            // *** 关键步骤：处理独立 Note ***
            // 设置父引用为 null
            single_note_ptr->parent_reference = nullptr;
            single_note_ptr->compinfo =
                ComplexInfo::NONE;  // 设置 compinfo 为 NONE
        }
        // (如果 single_hit_object_ptr 不是 Note，则不进行这些操作)

        res.insert(
            single_hit_object_ptr);  // 将这个（可能已修改的）独立物件加入结果集
    } else {
        // 片段包含多个物件，形成一个新的 ComplexNote
        // 子 Note 的 parent_ref 和 compinfo 已在加入 segment 时正确设置
        if (original_comp_base && !segment->child_notes.empty()) {
            segment->timestamp = (*segment->child_notes.begin())->timestamp;
            segment->orbit = original_comp_base->orbit;
            // 新 ComplexNote 自身的 compinfo
            segment->compinfo = ComplexInfo::NONE;
        }
        // 将新的 ComplexNote 加入结果集
        res.insert(segment);
    }

    // 重置 segment 指针
    segment = nullptr;
}

// --- 核心拆分逻辑 (使用 Note::clone() 来创建副本，不修改原始 comp) ---
/**
 * @brief 根据“被移除”的子对象拆分一个 Complex 对象。原始 comp
 * 及其子对象保持不变。
 * @param res [输出] 用于存放结果 Complex 对象或单个 (克隆的) Note 对象的
 * multiset。
 * @param comp [输入] 要拆分的原始 Complex 对象 (const，表示不修改其直接成员)。
 * @param removed_objs [输入] 包含 comp 子对象中“应被移除”的物件的
 * multiset。这些物件将作为断点。
 */
// --- 核心拆分逻辑 (使用 Note::clone() 并设置父引用) ---
inline void destruct_complex_note(
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>& res,
    const std::shared_ptr<ComplexNote>& comp,
    const std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>&
        removed_objs) {
    if (!comp || comp->child_notes.empty()) {
        return;
    }

    if (comp->child_notes.size() == 1) {
        const auto& single_original_child_ho = *comp->child_notes.begin();
        if (removed_objs.find(single_original_child_ho) == removed_objs.end()) {
            auto original_note_ptr =
                std::dynamic_pointer_cast<Note>(single_original_child_ho);
            if (original_note_ptr &&
                !std::dynamic_pointer_cast<ComplexNote>(original_note_ptr)) {
                // 克隆出来的 Note 的
                // parent_ref_raw 是 nullptr
                Note* cloned_raw_ptr = original_note_ptr->clone();
                // 单个 Note 不属于新的 ComplexNote 片段，所以其 parent_ref_raw
                // 保持 nullptr
                res.insert(std::shared_ptr<Note>(cloned_raw_ptr));
            }
        }
        return;
    }

    std::shared_ptr<ComplexNote> current_segment = nullptr;

    for (const auto& child_ho_original_ptr : comp->child_notes) {
        bool is_removed = (removed_objs.count(child_ho_original_ptr) > 0);

        if (is_removed) {
            // ... (同之前的 finalize_and_add_segment_WITH_CLONE 调用) ...
            if (current_segment) {
                if (!current_segment->child_notes.empty()) {
                    auto last_note_in_seg_clone =
                        std::dynamic_pointer_cast<Note>(
                            *current_segment->child_notes.rbegin());
                    if (last_note_in_seg_clone)
                        last_note_in_seg_clone->compinfo = ComplexInfo::END;
                }
                finalize_and_add_segment(res, current_segment,
                                         comp);  // 使用之前的 finalize 函数
            }
        } else {
            auto original_note_ptr =
                std::dynamic_pointer_cast<Note>(child_ho_original_ptr);
            // 克隆后的 Note (shared_ptr)
            std::shared_ptr<Note> note_for_segment = nullptr;

            if (original_note_ptr &&
                !std::dynamic_pointer_cast<ComplexNote>(original_note_ptr)) {
                Note* cloned_raw_ptr = original_note_ptr->clone();
                note_for_segment = std::shared_ptr<Note>(cloned_raw_ptr);
                // 保留原始 compinfo
                note_for_segment->compinfo = original_note_ptr->compinfo;
            } else {
                // 如果是 ComplexNote 或非 Note
                // HitObject，则中断当前片段，并跳过
                if (current_segment) {
                    if (!current_segment->child_notes.empty()) {
                        auto last_note_in_seg_clone =
                            std::dynamic_pointer_cast<Note>(
                                *current_segment->child_notes.rbegin());
                        if (last_note_in_seg_clone)
                            last_note_in_seg_clone->compinfo = ComplexInfo::END;
                    }
                    finalize_and_add_segment(res, current_segment, comp);
                }
                // 根据需要，决定是否将 child_ho_original_ptr (原始 ComplexNote
                // 或非 Note 对象) 加入 res if
                // (removed_objs.find(child_ho_original_ptr) ==
                // removed_objs.end()) {
                //     res.insert(child_ho_original_ptr);
                // }
                continue;
            }

            if (note_for_segment) {
                // 如果成功克隆了一个 Note
                if (!current_segment) {
                    current_segment = std::make_shared<ComplexNote>(
                        comp->timestamp, comp->orbit);
                    note_for_segment->compinfo = ComplexInfo::HEAD;
                } else {
                    note_for_segment->compinfo = ComplexInfo::BODY;
                }
                // 设置父引用
                // current_segment.get() 获取 ComplexNote* 指针
                note_for_segment->parent_reference = current_segment.get();
                current_segment->child_notes.insert(note_for_segment);
            }
        }
    }

    if (current_segment) {
        if (!current_segment->child_notes.empty()) {
            auto last_note_in_segment_clone = std::dynamic_pointer_cast<Note>(
                *current_segment->child_notes.rbegin());
            if (last_note_in_segment_clone) {
                last_note_in_segment_clone->compinfo = ComplexInfo::END;
                // 父引用已在加入时设置，这里不需要再次设置
            }
        }
        finalize_and_add_segment(res, current_segment, comp);
    }
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
