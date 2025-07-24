#include <AutoResizeTableWidget.h>
#include <qheaderview.h>
#include <qlogging.h>
#include <qtablewidget.h>

#include <QEvent>
#include <QMouseEvent>
#include <cmath>

AutoResizeTableWidget::AutoResizeTableWidget(QWidget* parent)
    : QTableWidget(parent) {
    verticalHeader()->hide();

    // 监听列宽度调整
    connect(horizontalHeader(), &QHeaderView::sectionResized, this,
            &AutoResizeTableWidget::columsection_resized);
    horizontalHeader()->installEventFilter(this);
}
void AutoResizeTableWidget::columsection_resized(int logicalIndex, int oldSize,
                                                 int newSize) {
    // 不是用户操作，直接返回，避免无限递归
    if (!user_interective) {
        return;
    }

    // 计算宽度变化量
    int delta = newSize - oldSize;
    if (delta == 0) return;

    // 计算除了当前调整列之外，其他所有列的总拉伸比例
    int totalStretchExceptCurrent = 0;
    for (int i = 0; i < columnCount(); ++i) {
        if (i != logicalIndex) {
            totalStretchExceptCurrent += colum_stretchs[i];
        }
    }

    // 避免除零
    if (totalStretchExceptCurrent <= 0) return;

    // 按比例将 delta 分配到其他列
    // 避免信号的递归触发，需要临时阻塞信号
    horizontalHeader()->blockSignals(true);

    for (int i = 0; i < columnCount(); ++i) {
        if (i != logicalIndex) {
            // 计算当前列应该承担的变化量
            double proportion =
                (double)colum_stretchs[i] / totalStretchExceptCurrent;
            int change = std::round(delta * proportion);

            // 新的宽度 = 旧宽度 - 承担的变化量
            setColumnWidth(i, columnWidth(i) - change);
        }
    }

    // 重新启用信号
    horizontalHeader()->blockSignals(false);

    // 用户手动调整后，原来的比例已经失效，需要根据新的宽度重新计算比例
    updateStretchRatios();
}

// 设置拉伸比例
void AutoResizeTableWidget::setColumStretchs(QList<int>&& stretchs) {
    if (stretchs.size() != columnCount()) {
        qDebug() << "columnCount dosent match";
        return;
    }
    colum_stretchs = std::move(stretchs);
}

void AutoResizeTableWidget::updateStretchRatios() {
    int totalWidth = 0;
    for (int i = 0; i < columnCount(); ++i) {
        totalWidth += columnWidth(i);
    }

    if (totalWidth == 0) return;

    colum_stretchs.clear();
    for (int i = 0; i < columnCount(); ++i) {
        // 为了保持整数比例，可以乘以一个基数，比如1000
        colum_stretchs.append((columnWidth(i) * 1000) / totalWidth);
    }
    // qDebug() << "Ratios updated to:" << colum_stretchs;
}

bool AutoResizeTableWidget::eventFilter(QObject* watched, QEvent* event) {
    // 只处理来自水平表头的事件
    if (watched == this->horizontalHeader()) {
        // 检查鼠标事件类型
        if (event->type() == QEvent::HoverMove) {
            // 在其中移动
            user_interective = true;
        } else if (event->type() == QEvent::Leave) {
            // 离开了
            user_interective = false;
        }
    }

    // 将事件传递给原始的处理器
    return QTableWidget::eventFilter(watched, event);
}

void AutoResizeTableWidget::resizeEvent(QResizeEvent* e) {
    if (user_interective || colum_stretchs.isEmpty() || columnCount() == 0) {
        QTableWidget::resizeEvent(e);
        return;
    }
    horizontalHeader()->blockSignals(true);
    // 读取列宽度比例-自动调整占满组件
    int stretchcount{0};
    for (const auto& v : colum_stretchs) {
        stretchcount += v;
    }
    auto unit = width() / (double)stretchcount;

    double size_left{double(width()) * 0.99};

    for (int i = 0; i < colum_stretchs.size() - 1; ++i) {
        auto res = int(std::round(colum_stretchs[i] * unit));
        setColumnWidth(i, res);
        size_left -= res;
    }
    setColumnWidth(columnCount() - 1, int(size_left));
    horizontalHeader()->blockSignals(false);

    QTableWidget::resizeEvent(e);
}
