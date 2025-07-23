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
            // 格式: "描述 (*.ext1 *.ext2)"
            filterList.append(
                QString("%1 (*%2)").arg(it.key()).arg(it.value()));
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
}  // namespace mutil

#endif  // MMM_MUTIL_HPP
