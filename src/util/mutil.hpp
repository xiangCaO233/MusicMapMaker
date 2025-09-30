#ifndef MMM_MUTIL_HPP
#define MMM_MUTIL_HPP

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
#include <qsvgrenderer.h>
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
#include <QTableWidget>
#include <QTime>
#include <QWidget>
#include <QtSvgWidgets/QSvgWidget>
#include <TimingTableUsefulWidgets.hpp>
#include <glm/glm.hpp>
#include <mmm/DataStructures.hpp>
#include <mmm/timing/Beat.hpp>
#include <string>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

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

    return filename;
}

inline std::string sanitizeFilename_ascii(std::string filename) {
    for (char& c : filename) {
        // 检查字符是否为可打印的ASCII字符（0x20-0x7E），
        // 不包括控制字符（0x00-0x1F）和删除字符（0x7F）
        if (static_cast<unsigned char>(c) < 0x20 ||
            static_cast<unsigned char>(c) > 0x7E) {
            c = '_';  // 替换非ASCII字符为'_'
        }
    }
    return filename;
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

// 获取选择的文件夹
inline QString getDirectory(QWidget* parent, const QString& title,
                            const QString& defaultPath = QString()) {
    QFileDialog dialog(parent, title, defaultPath);

    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);

    if (QListView* listView = dialog.findChild<QListView*>("listView")) {
        listView->setSelectionMode(QAbstractItemView::SingleSelection);
    }
    if (QTreeView* treeView = dialog.findChild<QTreeView*>()) {
        treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    }

    if (dialog.exec() == QDialog::Accepted) {
        QStringList selected = dialog.selectedFiles();
        if (!selected.isEmpty()) {
            return selected.first();
        }
    }

    return QString();
}

// 获取保存的文件
inline QString getSaveDirectoryWithFilename(
    QWidget* parent, const QString& title, const QString& format_label,
    const QMap<QString, QString>& formatFilters,
    const QMap<QString, QString>& defaultFilenames,
    const QString& defaultFormat = QString()) {
    QFileDialog dialog(parent, title, QDir::homePath());

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

    // 禁用原生过滤器控件
    // 找到并隐藏原生过滤器下拉框
    if (QComboBox* nativeFilterCombo =
            dialog.findChild<QComboBox*>("fileTypeCombo")) {
        // 禁止单独选择
        nativeFilterCombo->setEnabled(false);
    }

    // 当格式改变时的处理
    QObject::connect(
        formatCombo, &QComboBox::currentIndexChanged, [&](int index) {
            if (index < 0 || index >= formatFilters.size()) return;
            qDebug() << "select:" << index;

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

    // 设置初始文件名
    if (lineEdit && !formatFilters.isEmpty()) {
        QString currentKey = formatFilters.keys().value(defaultIndex);
        QString initialFilename =
            defaultFilenames.value(currentKey, "untitled");
        QString initialExtension = formatFilters.value(currentKey);
        lineEdit->setText(initialFilename + initialExtension);
    }

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

/**
 * @brief 弹出一个文件保存对话框，用于保存单一类型的文件。
 *
 * @param parent            父窗口指针。
 * @param title             对话框的标题。
 * @param format_label      文件格式的描述性标签（例如 "PNG Image"）。
 * @param formatFilter      文件格式的过滤器（例如 "*.png"）。
 * @param defaultFilename   不带扩展名的默认文件名（例如 "untitled"）。
 * @param defaultPath       对话框打开时的默认路径。
 * @return QString 用户选择的完整文件路径；如果用户取消，则返回空字符串。
 */
/**
 * @brief 弹出一个文件保存对话框，用于保存单一类型的文件。（健壮版本）
 *
 * @param parent            父窗口指针。
 * @param title             对话框的标题。
 * @param format_label      文件格式的描述性标签（例如 "MMM Map File"）。
 * @param formatFilter      文件格式的过滤器（例如 "*.mmm" 或 ".mmm"
 * 都能接受）。
 * @param defaultFilename   不带扩展名的默认文件名（例如 "untitled"）。
 * @param defaultPath       对话框打开时的默认路径。
 * @return QString 用户选择的完整文件路径；如果用户取消，则返回空字符串。
 */
inline QString getSaveAsFile(QWidget* parent, const QString& title,
                             const QString& format_label,
                             const QString& formatFilter,
                             const QString& defaultFilename,
                             const QString& defaultPath = QDir::homePath()) {
    // --- 修正部分 开始 ---
    QString correctedFilter = formatFilter.trimmed();
    if (!correctedFilter.startsWith("*.")) {
        if (correctedFilter.startsWith(".")) {
            correctedFilter.prepend("*");  // .mmm -> *.mmm
        } else {
            correctedFilter.prepend("*.");  // mmm -> *.mmm
        }
    }
    // --- 修正部分 结束 ---

    // 1. 组合过滤器字符串 (使用修正后的过滤器)
    const QString filter =
        QString("%1 (%2)").arg(format_label, correctedFilter);

    // 2. 从过滤器中提取扩展名
    QString extension;
    const int dotIndex = correctedFilter.lastIndexOf('.');
    if (dotIndex != -1) {
        extension = correctedFilter.mid(dotIndex);  // 结果是 ".mmm"
    }

    // 3. 组合默认的文件路径
    const QString defaultFullPath =
        QDir(defaultPath).filePath(defaultFilename + extension);

    // 4. 调用静态函数显示对话框
    QString selectedFile =
        QFileDialog::getSaveFileName(parent, title, defaultFullPath, filter,
                                     nullptr, QFileDialog::DontUseNativeDialog);

    // 5. 后处理：确保文件有正确的扩展名
    if (!selectedFile.isEmpty() && !extension.isEmpty()) {
        QFileInfo fileInfo(selectedFile);
        if (fileInfo.suffix().isEmpty()) {
            selectedFile.append(extension);
        } else if (fileInfo.suffix().compare(extension.mid(1),
                                             Qt::CaseInsensitive) != 0) {
            // 如果用户输入了错误的后缀，也可以考虑强制修正
            // selectedFile = fileInfo.path() + "/" +
            // fileInfo.completeBaseName() + extension;
        }
    }

    return selectedFile;
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
 * @brief 弹出文件对话框以选择一个要打开的文件。
 * @param parent 父窗口指针。
 * @param title 对话框的标题。
 * @param formatFilters 文件格式过滤器 (例如: {"Images", ".png .jpg"}).
 *                      如果为空, 则显示 "All Files (*)"。
 * @param defaultPath 对话框打开时的默认路径。
 * @return 用户选择的文件的完整路径；如果用户取消，则返回空 QString。
 */
inline QString getOpenFile(QWidget* parent, const QString& title,
                           const QMap<QString, QString>& formatFilters,
                           const QString& defaultPath = QDir::homePath()) {
    QFileDialog dialog(parent, title, defaultPath);

    // 设置为选择单个已存在的文件
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    // 根据传入的 map 构建过滤器列表
    QStringList filterList;
    if (formatFilters.isEmpty()) {
        filterList.append("All Files (*)");
    } else {
        for (auto it = formatFilters.begin(); it != formatFilters.end(); ++it) {
            QString description = it.key();
            QString extensions = it.value();  // 例如 ".imd .mmm .osu"

            // --- 核心修正逻辑 ---
            // 1. 按空格分割扩展名
            QStringList extParts = extensions.split(' ', Qt::SkipEmptyParts);

            // 2. 为每个扩展名前面加上 "*"，确保它们都是有效的通配符模式
            for (QString& part : extParts) {
                if (!part.startsWith("*.")) {
                    part.prepend("*");
                }
            }

            // 3. 将处理后的通配符模式重新用空格连接起来
            QString wildcardPatterns =
                extParts.join(" ");  // 例如 "*.imd *.mmm *.osu"

            // 4. 构建最终的过滤器字符串
            filterList.append(
                QString("%1 (%2)").arg(description).arg(wildcardPatterns));
        }
    }
    dialog.setNameFilters(filterList);

    // 确保视图为单选模式，保持风格一致
    if (QListView* listView = dialog.findChild<QListView*>("listView")) {
        listView->setSelectionMode(QAbstractItemView::SingleSelection);
    }
    if (QTreeView* treeView = dialog.findChild<QTreeView*>()) {
        treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    }

    // 执行对话框并获取结果
    if (dialog.exec() == QDialog::Accepted) {
        QStringList selected = dialog.selectedFiles();
        if (!selected.isEmpty()) {
            return selected.first();
        }
    }

    return QString();  // 用户取消或未选择文件
}

/**
 * @brief 弹出文件对话框以选择一个或多个要打开的文件。
 * @param parent 父窗口指针。
 * @param title 对话框的标题。
 * @param formatFilters 文件格式过滤器 (例如: {"Images", ".png .jpg"}).
 *                      如果为空, 则显示 "All Files (*)"。
 * @param defaultPath 对话框打开时的默认路径。
 * @return 包含用户选择的所有文件完整路径的列表；如果用户取消，则返回空列表。
 */
inline QStringList getOpenFiles(QWidget* parent, const QString& title,
                                const QMap<QString, QString>& formatFilters,
                                const QString& defaultPath = QDir::homePath()) {
    QFileDialog dialog(parent, title, defaultPath);

    // 设置为选择多个已存在的文件
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    // 根据传入的 map 构建过滤器列表
    QStringList filterList;
    if (formatFilters.isEmpty()) {
        filterList.append("All Files (*)");
    } else {
        for (auto it = formatFilters.begin(); it != formatFilters.end(); ++it) {
            filterList.append(
                QString("%1 (*%2)").arg(it.key()).arg(it.value()));
        }
    }
    dialog.setNameFilters(filterList);

    // 注意：当使用 ExistingFiles 模式时, QFileDialog 会自动将其内部的视图
    // (QListView/QTreeView) 设置为支持多选 (ExtendedSelection)。
    // 因此, 不需要像单选函数中那样手动设置选择模式。

    // 执行对话框并获取结果
    if (dialog.exec() == QDialog::Accepted) {
        // selectedFiles() 会返回所有被选中的文件的路径列表
        return dialog.selectedFiles();
    }

    return QStringList();  // 用户取消或未选择文件，返回空列表
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

inline bool isApproxEqual(double a, double b, double tolerance) {
    return std::abs(a - b) <= tolerance;
}

// 计算分音策略(2~64)并在找到有效策略时设置divpos
inline int calculateDivisionStrategy(const NoteCollection& notes, Beat& beat,
                                     double tolerance) {
    double beat_length = beat.beat_length;
    const auto& hitobjects = notes.get_all_notes_unordered();
    if (beat_length <= 0) return -1;

    // 收集当前拍内的所有物件(包括重复时间戳的) [beat_start, beat_end)
    std::vector<NoteHandle> current_beat_object_handles =
        notes.query_range(beat.beat_start, beat.beat_start + beat_length);
    // 当前拍的时间戳表
    std::vector<uint32_t> current_beat_object_times;
    for (const auto& objhandle : current_beat_object_handles) {
        auto note = notes.get_note(objhandle);
        switch (note->notetype()) {
            case NoteType::NORMAL:
            case NoteType::SLIDE: {
                // 滑键或普通单键-直接添加时间戳
                auto time = note->timestamp();
                if (time >= beat.beat_start &&
                    time <= beat.beat_start + beat_length) {
                    current_beat_object_times.push_back(time);
                }
                break;
            }
            case NoteType::HOLD: {
                auto hold = static_cast<const Hold*>(note);
                // 面条要添加头和尾的时间戳
                auto time = note->timestamp();
                if (time >= beat.beat_start &&
                    time <= beat.beat_start + beat_length) {
                    current_beat_object_times.push_back(time);
                }

                auto end_time = hold->timestamp() + hold->duration();
                if (end_time >= beat.beat_start &&
                    end_time <= beat.beat_start + beat_length) {
                    current_beat_object_times.push_back(end_time);
                }
                break;
            }
            case NoteType::COMPOSITE: {
                auto composite = static_cast<const Composite*>(note);
                // 遍历添加所有子物件的头时间戳
                for (const auto& child : composite->children()) {
                    auto time = child->timestamp();
                    if (time >= beat.beat_start &&
                        time <= beat.beat_start + beat_length) {
                        current_beat_object_times.push_back(time);
                    }
                }
                // 若结尾是面条,则再添加一个面尾时间戳
                auto& last = composite->children().back();
                if (last->notetype() == NoteType::HOLD) {
                    auto hold = static_cast<const Hold*>(last.get());
                    auto end_time = hold->timestamp() + hold->duration();
                    if (end_time >= beat.beat_start &&
                        end_time <= beat.beat_start + beat_length) {
                        current_beat_object_times.push_back(end_time);
                    }
                }
                break;
            }
        }
    }

    // 拍内无物件-不算分析结果
    if (current_beat_object_times.empty()) return -1;

    // 从最小分音数开始检查（2到64）
    for (int n{2}; n <= 64; ++n) {
        bool valid = true;
        double sub_beat = beat_length / n;

        // 临时存储divpos值，验证通过后再设置
        std::unordered_set<int32_t> divpos_map;

        for (const auto& ts : current_beat_object_times) {
            // 计算最接近的分音点位置
            double pos = (ts - beat.beat_start) / sub_beat;
            int32_t rounded_pos = static_cast<int32_t>(std::round(pos));
            double closest_sub = beat.beat_start + rounded_pos * sub_beat;

            if (isApproxEqual(ts, closest_sub, tolerance)) {
                divpos_map.insert(rounded_pos);
            } else {
                valid = false;
                break;
            }
        }

        if (valid) {
            // 设置所有匹配物件的divpos
            // for (const auto& [obj, pos] : divpos_map) {
            //     obj->divpos = pos;
            // }
            return n;
        }
    }

    return -1;  // 无有效分音策略
}

inline void get_colored_icon_pixmap(QPixmap& pixmap, const char* svgPath,
                                    QColor& color, QSize& size) {
    // 将原始SVG渲染到一个临时的、透明的画布上，作为“形状模板”。
    auto file = QString(svgPath);
    QSvgRenderer renderer(file);
    if (!renderer.isValid() || size.isEmpty()) {
        pixmap = QPixmap();  // 返回空Pixmap
        return;
    }

    QPixmap shapePixmap(size);
    shapePixmap.fill(Qt::transparent);
    QPainter shapePainter(&shapePixmap);
    renderer.render(&shapePainter);

    // 步骤 2: 准备最终的输出画布。
    // 这是关键的修正：首先将目标pixmap填充为透明。
    // 这一步强制QPixmap分配一个Alpha通道，确保它能够处理透明度
    pixmap = QPixmap(size);
    pixmap.fill(Qt::transparent);

    // 步骤 3: 在这个透明的画布上执行颜色混合操作。
    QPainter painter(&pixmap);

    // a. 先铺上我们的目标颜色。
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);  // 不需要边框
    painter.drawRect(pixmap.rect());

    // b. 设置混合模式，使用 shapePixmap 的Alpha通道作为遮罩。
    painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);

    // c. 将形状绘制上去，应用遮罩。
    painter.drawPixmap(0, 0, shapePixmap);
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
                                QColor& color, int32_t w, int32_t h) {
    // 创建QPixmap
    QPixmap pixmap;
    QSize size(w, h);
    get_colored_icon_pixmap(pixmap, svgpath, color, size);

    // 设置图标
    action->setIcon(QIcon(pixmap));
}

/**
 * @brief 检查两个轴对齐的矩形是否重叠。
 *
 * @param rectA 第一个矩形，格式为 glm::vec4(x, y, width, height)。
 * @param rectB 第二个矩形，格式为 glm::vec4(x, y, width, height)。
 * @return 如果重叠则返回 true，否则返回 false。
 *
 * @note 此函数假设坐标系中 Y 轴向下增长（常见的屏幕/UI坐标系）。
 */
inline bool checkOverlap(const glm::vec4& rectA, const glm::vec4& rectB) {
    // 为了代码清晰，我们先计算出每个矩形的左右上下边界
    float a_left = rectA.x;
    float a_right = rectA.x + rectA.z;  // x + width
    float a_top = rectA.y;
    float a_bottom = rectA.y + rectA.w;  // y + height

    float b_left = rectB.x;
    float b_right = rectB.x + rectB.z;  // x + width
    float b_top = rectB.y;
    float b_bottom = rectB.y + rectB.w;  // y + height

    // 检查所有不重叠的情况
    // 1. A 在 B 的右侧
    if (a_left >= b_right) {
        return false;
    }
    // 2. A 在 B 的左侧
    if (a_right <= b_left) {
        return false;
    }
    // 3. A 在 B 的下方
    if (a_top >= b_bottom) {
        return false;
    }
    // 4. A 在 B 的上方
    if (a_bottom <= b_top) {
        return false;
    }

    // 如果以上所有“不重叠”的情况都不成立，那么它们必然重叠
    return true;
}

/**
 * @brief 检查一个点是否在一个轴对齐的矩形内部。
 *
 * @param point 要检查的点，格式为 glm::vec2(x, y)。
 * @param rect 矩形区域，格式为 glm::vec4(x, y, width, height)。
 * @return 如果点在矩形内则返回 true，否则返回 false。
 *
 * @note 此函数同样假设 Y 轴向下增长。
 *       边界条件为：包含左上边界，不包含右下边界。
 *       即 point.x >= rect.x 且 point.x < rect.x + rect.width。
 *       这对处理基于网格或像素的UI元素非常有用。
 */
inline bool checkPointInRect(const glm::vec2& point, const glm::vec4& rect) {
    // 检查点的 X 坐标是否在矩形的水平范围内
    bool inHorizontal = (point.x >= rect.x) && (point.x < rect.x + rect.z);

    // 检查点的 Y 坐标是否在矩形的垂直范围内
    bool inVertical = (point.y >= rect.y) && (point.y < rect.y + rect.w);

    // 只有当两个方向都在范围内时，点才在矩形内
    return inHorizontal && inVertical;
}

// 1. 实现一个自定义的哈希组合函数
//    这个函数用于将多个哈希值组合成一个
inline void hash_combine(std::size_t& seed, std::size_t hash) {
    seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// 2. 为 glm::vec4 提供一个哈希函数
struct Vec4Hash {
    std::size_t operator()(const glm::vec4& v) const {
        std::size_t seed = 0;

        // 对每个分量（x, y, z, w）分别计算哈希值，然后组合
        // 注意：这里我们使用 std::hash<float>，因为 glm::vec4 的分量是 float
        hash_combine(seed, std::hash<float>()(v.x));
        hash_combine(seed, std::hash<float>()(v.y));
        hash_combine(seed, std::hash<float>()(v.z));
        hash_combine(seed, std::hash<float>()(v.w));

        return seed;
    }
};

// 3. 为 glm::vec4 提供一个相等比较函数
//    unordered_set 不仅需要哈希，还需要一个相等比较来处理哈希冲突
struct Vec4Equal {
    bool operator()(const glm::vec4& a, const glm::vec4& b) const {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
    }
};

}  // namespace mutil

#endif  // MMM_MUTIL_HPP
