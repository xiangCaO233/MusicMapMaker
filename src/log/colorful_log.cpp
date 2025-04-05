#include <memory>

#include "colorful-log.h"

void ColorfulFormatter::format(const spdlog::details::log_msg &msg,
                               spdlog::memory_buf_t &dest) {
  // 时间处理
  // 使用chrono直接格式化时间点
  // 分离时间的秒和毫秒部分
  const auto time_since_epoch = msg.time.time_since_epoch();
  const auto sec =
      std::chrono::duration_cast<std::chrono::seconds>(time_since_epoch);
  const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
      time_since_epoch - sec);

  // 转换为本地时间
  const std::time_t t_c = std::chrono::system_clock::to_time_t(msg.time);
  std::tm tm_buf;

#ifdef _WIN32
  localtime_s(&tm_buf, &t_c);
#else
  localtime_r(&t_c, &tm_buf);
#endif

  // 格式化主要部分
  spdlog::fmt_lib::format_to(
      std::back_inserter(dest),
      "\033[40m[\033[36;1m{:%Y-%m-%d %H:%M:%S}.{:03d}\033[22m\033[37m] "
      "[\033[{};1m{}\033[37;22m/\033[32;1m{}\033[37;22m]",
      tm_buf, millis.count(), get_color(msg.level),
      spdlog::level::to_string_view(msg.level),
      msg.source.funcname ? msg.source.funcname : "unknown");

  // 添加日志内容
  spdlog::fmt_lib::format_to(std::back_inserter(dest), ": \033[{}m{}",
                             get_color(msg.level), msg.payload);
  // 添加文件信息,仅debug/error及以上
  if (msg.level == spdlog::level::debug || msg.level >= spdlog::level::err) {
    spdlog::fmt_lib::format_to(
        std::back_inserter(dest),
        " \033[37m(\033[35m{}\033[37m:\033[35m{}\033[37m)\033[0m\n",
        msg.source.filename, msg.source.line);
  } else {
    spdlog::fmt_lib::format_to(std::back_inserter(dest), "\033[0m\n");
  }
}

std::unique_ptr<spdlog::formatter> ColorfulFormatter::clone() const {
  return std::make_unique<ColorfulFormatter>();
}
const char *ColorfulFormatter::get_color(
    spdlog::level::level_enum level) const {
  switch (level) {
    case spdlog::level::trace:
      return "37";  // 白色
    case spdlog::level::debug:
      return "36";  // 青色
    case spdlog::level::info:
      return "32";  // 绿色
    case spdlog::level::warn:
      return "33";  // 黄色
    case spdlog::level::err:
      return "31";  // 红色
    case spdlog::level::critical:
      return "31;1";  // 亮红色
    default:
      return "0";
  }
}

uint32_t XLogger::glcalls = 0;
uint32_t XLogger::drawcalls = 0;
std::shared_ptr<spdlog::logger> XLogger::logger;

void XLogger::init() {
  // 创建三个sink（终端、全量文件、错误文件）
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  auto file_all_sink =
      std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/latest.log");
  auto file_error_sink =
      std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/error.log");

  // 设置统一的自定义格式
  auto formatter = std::make_unique<ColorfulFormatter>();
  console_sink->set_formatter(formatter->clone());
  file_all_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
  file_error_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");

  // 设置sink级别过滤
  console_sink->set_level(spdlog::level::trace);
  file_all_sink->set_level(spdlog::level::debug);
  file_error_sink->set_level(spdlog::level::err);

  // 创建组合logger
  logger = std::make_shared<spdlog::logger>(
      "MMM",
      spdlog::sinks_init_list{console_sink, file_all_sink, file_error_sink});

  // 设置全局日志级别
  logger->set_level(spdlog::level::trace);

  // 设置实时刷新
  logger->flush_on(spdlog::level::trace);

  spdlog::register_logger(logger);
  spdlog::set_default_logger(logger);

  XINFO("日志初始化完成");
}
void XLogger::shutdown() {
  // 销毁 logger
  spdlog::drop("MMM");
  // 直接销毁 logger 对象
  logger.reset();
  // 销毁所有 logger
  spdlog::shutdown();
}
void XLogger::enable() { logger->set_level(spdlog::level::trace); }
void XLogger::disable() { logger->set_level(spdlog::level::off); }

void XLogger::setlevel(spdlog::level::level_enum level) {
  // 设置全局日志级别
  logger->set_level(level);
}
