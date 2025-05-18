#include "mpage.h"

#include <qapplication.h>
#include <qtmetamacros.h>

#include <memory>

#include "../../log/colorful-log.h"
#include "../../mmm/MapWorkProject.h"
#include "../../mmm/map/MMap.h"
#include "../../util/mutil.h"
#include "ui_mpage.h"

MPage::MPage(QWidget* parent) : QWidget(parent), ui(new Ui::MPage) {
    ui->setupUi(this);
    edit_area_widget = ui->edit_area_widget;

    // 连接选择map信号
    connect(this, &MPage::select_map, ui->edit_area_widget,
            &MEditorArea::on_selectnewmap);
}

MPage::~MPage() { delete ui; }

// 使用主题
void MPage::use_theme(GlobalTheme theme) {
    QColor file_button_color;
    switch (theme) {
        case GlobalTheme::DARK: {
            file_button_color = QColor(255, 255, 255);
            break;
        }
        case GlobalTheme::LIGHT: {
            file_button_color = QColor(0, 0, 0);
            break;
        }
    }
    mutil::set_button_svgcolor(ui->close_page_button, ":/icons/close.svg",
                               file_button_color, 16, 16);

    // 设置编辑区主题
    ui->edit_area_widget->use_theme(theme);
}

// 项目控制器选择了map事件
void MPage::project_controller_select_map(std::shared_ptr<MMap>& map) {
    // 检查是否存在page映射
    auto page_text = QString::fromStdString(map->map_name);
    auto mapit = pagetext_maps_map.find(page_text);

    if (mapit == pagetext_maps_map.end()) {
        // 不存在,添加page映射
        pagetext_maps_map.try_emplace(page_text).first->second = map;

        // 添加selector item
        ui->page_selector->addItem(page_text);
    }

    // 选择此页
    ui->page_selector->setCurrentText(page_text);
    if (map) {
        // 更新编辑区信息
        edit_area_widget->div_res_label->setText(QString("1/%1").arg(
            map->project_reference->config.default_divisors));
        if (map->project_reference->config.default_divisors % 3 == 0) {
            edit_area_widget->ratio_button->setText(QString::number(3));
            edit_area_widget->divisorslider->setValue(
                map->project_reference->config.default_divisors / 3);
        } else {
            edit_area_widget->ratio_button->setText(QString::number(2));
            edit_area_widget->divisorslider->setValue(
                map->project_reference->config.default_divisors / 2);
        }
    }
}

// 选择页事件
void MPage::on_page_selector_currentTextChanged(const QString& text) {
    auto mapit = pagetext_maps_map.find(text);
    if (mapit == pagetext_maps_map.end()) {
        XWARN("无[" + text.toStdString() + "]页");
        return;
    }

    emit select_map(mapit->second);
}

// 关闭页面事件
void MPage::on_close_page_button_clicked() {
    auto current_text = ui->page_selector->currentText();
    auto mapit = pagetext_maps_map.find(current_text);
    if (mapit == pagetext_maps_map.end()) {
        XWARN("未发现[" + current_text.toStdString() + "]页");
        return;
    }
    // 移除当前选中的item
    ui->page_selector->removeItem(ui->page_selector->currentIndex());
    // 移除索引
    pagetext_maps_map.erase(mapit);

    // 发送选择map信号更新画布
    std::shared_ptr<MMap> map;
    if (pagetext_maps_map.empty()) {
        // 设置null
        map = nullptr;
    } else {
        // 随便选一个
        map = pagetext_maps_map.begin()->second;
    }
    emit select_map(map);
}
