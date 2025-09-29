#include <mapconfig.h>
#include <ui_mapconfig.h>

#include <mmm/map/MMap.hpp>

void MapConfig::on_confirm_button_clicked() {
    // 应用修改前检查合法性
    update_ifcompeleted();
    hide();
}

void MapConfig::on_cancel_button_clicked() {
    // 取消修改直接恢复数据为当前已有的元数据
    // 然后隐藏窗口
    hide();
}
