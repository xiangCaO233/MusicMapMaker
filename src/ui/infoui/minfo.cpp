#include "minfo.h"

#include <qdatetime.h>

#include "ui_minfo.h"

MInfo::MInfo(QWidget *parent) : QWidget(parent), ui(new Ui::MInfo) {
  ui->setupUi(this);
  mmetas = ui->mapmeta_edit;
}

MInfo::~MInfo() { delete ui; }

// 使用主题
void MInfo::use_theme(GlobalTheme theme) { ui->mapmeta_edit->use_theme(theme); }

const char *time_expr =
    "[<span style='color: #00FFFF; font-weight: normal;'>%1</span>] ";

// 3号根据日志等级设置粗细
const char *level_expr =
    "<span style='color: %1; font-weight: %2;'>[%3]: </span>";

const char *message_expr =
    "<span style='color: %1; font-weight: %2;'>%3</span>";

// 输出日志
void MInfo::log(MLogLevel level, const QString &message) {
  QString timestamp = QTime::currentTime().toString("hh:mm:ss");
  QString levelname;
  QString level_color;
  QString level_weight = "normal";
  switch (level) {
    case MLogLevel::INFO: {
      levelname = "info";
      level_color = "#00AA00";
      break;
    }
    case MLogLevel::WARN: {
      levelname = "warn";
      level_color = "#FFA500";
      break;
    }
    case MLogLevel::ERROR: {
      levelname = "error";
      level_color = "#D50000";
      level_weight = "bold";
      break;
    }
  }
  QString logline =
      QString(time_expr).arg(timestamp) +
      QString(level_expr).arg(level_color).arg(level_weight).arg(levelname) +
      QString(message_expr).arg(level_color).arg(level_weight).arg(message);

  ui->logTextArea->append(logline);
  if (level == MLogLevel::ERROR) ui->errorTextArea->append(logline);
}
