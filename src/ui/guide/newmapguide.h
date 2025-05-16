#ifndef NEWMAPGUIDE_H
#define NEWMAPGUIDE_H

#include <QDialog>
#include <QRegularExpressionValidator>

namespace Ui {
class NewMapGuide;
}

class NewMapGuide : public QDialog {
    Q_OBJECT

   public:
    explicit NewMapGuide(QWidget *parent = nullptr);
    ~NewMapGuide();

    // 输入验证器
    QRegularExpressionValidator *asciiValidator;
    QDoubleValidator *doubleValidator;
    QIntValidator *intValidator;

    // 输入获取结果
    std::string music_path;
    std::string bg_path;
    std::string title;
    std::string title_unicode;
    std::string artist;
    std::string version{"[mmm]"};
    std::string artist_unicode;
    std::string author{"mmm"};

    int32_t orbits{4};
    int32_t map_length{60000};
    double pbpm{60};

    // 音频是否加载成功
    bool is_audio_loaded{false};

   private slots:
    // 使用音频元数据按钮事件
    void on_use_music_titleunicode_button_clicked();

    void on_use_music_artistunicode_button_clicked();

    // 编辑事件
    // 音频路径变化
    void on_music_path_edit_textChanged(const QString &arg1);

    void on_title_edit_textChanged(const QString &arg1);

    void on_title_unicode_edit_textChanged(const QString &arg1);

    void on_artist_edit_textChanged(const QString &arg1);

    void on_artist_unicode_edit_textChanged(const QString &arg1);

    void on_bg_path_edit_textChanged(const QString &arg1);

    void on_pbpm_edit_textChanged(const QString &arg1);

    void on_author_edit_textChanged(const QString &arg1);

    void on_maplength_edit_textChanged(const QString &arg1);

    void on_mapversion_edit_textChanged(const QString &arg1);

    void on_keys_edit_textChanged(const QString &arg1);

    void on_read_musiclength_button_clicked();

    void on_music_path_browser_button_clicked();

    void on_bg_path_browser_button_clicked();

   private:
    Ui::NewMapGuide *ui;
};

#endif  // NEWMAPGUIDE_H
