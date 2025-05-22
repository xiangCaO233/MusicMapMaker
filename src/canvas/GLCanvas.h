#ifndef GLCANVAS_H
#define GLCANVAS_H

#include <QtOpenGLWidgets/qopenglwidget.h>
#include <qopenglwindow.h>
#include <qpaintdevice.h>
#include <qpoint.h>
#include <qthread.h>
#include <qtmetamacros.h>

#include <filesystem>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

#include "RenderBuffer.hpp"

#ifdef __APPLE__
#include <QtOpenGL/qopenglfunctions_4_1_core.h>
#else
#include <QtOpenGL/qopenglfunctions_4_5_core.h>
#endif  //__APPLE__
#include <qwidget.h>

#include <QMatrix4x4>
#include <QTimer>

#include "FrameRateCounter.h"
#include "GlobalSettings.h"
#include "renderer/RendererManager.h"

enum class TexturePoolType;
class TextureAtlas;

class GLCanvas : public QOpenGLWindow,
#ifdef __APPLE__
                 // 苹果-opengl4.1
                 public QOpenGLFunctions_4_1_Core
#else
                 // 其他-opengl4.5
                 public QOpenGLFunctions_4_5_Core
#endif  //__APPLE__
{
    Q_OBJECT
    friend class AbstractRenderer;
    friend class StaticRenderer;
    friend class DynamicRenderer;

   public:
    // 构造GLCanvas
    explicit GLCanvas(QWidget *parent = nullptr);
    // 析构GLCanvas
    ~GLCanvas() override;
    // fps计数器
    FrameRateCounter *fpsCounter;

    // 刷新线程
    std::thread update_thread;
    // 计算线程
    std::thread calculate_thread;

    double refreshRate_ratio{2.0};

    // 目标刷新时间间隔
    int32_t des_update_time{8};

    // 停止刷新线程标识
    std::atomic<bool> stop_refresh{false};

    // 上一帧glcall
    long pre_glcalls{0};
    long pre_drawcall{0};

    // 上一次的帧生成时间
    long pre_frame_time{100};

    // 当前鼠标位置
    QPoint mouse_pos{0, 0};

    // 需要更新采样器uniform location
    static bool need_update_sampler_location;

    // 渲染管理器
    RendererManager *renderer_manager = nullptr;

    // 双缓冲管理器
    DoubleBufferManager frame_data_buffer_manager;

    // 全部纹理映射表(id-纹理对象)
    std::unordered_map<std::string, std::shared_ptr<TextureInstace>>
        texture_full_map;
    float gl_clear_color[4];
    // 当前主题
    GlobalTheme current_theme;

    // 使用主题
    void use_theme(GlobalTheme theme);

    // 从指定目录添加纹理
    void load_texture_from_path(const char *path);

    // 从指定目录添加纹理
    void load_texture_from_path(const std::filesystem::path &path);

    // 添加纹理
    void add_texture(const char *relative_path, const char *path);

    // 添加纹理
    void add_texture(std::filesystem::path &relative_path,
                     std::filesystem::path &path);

    // 设置垂直同步
    void set_Vsync(bool flag);

    // 渲染实际图形
    virtual void push_shape(BufferWrapper *current_back_buffer);

    void start_render();
    void start_pushshape();

    void rendergl();
    void process_render_params(const RenderParams &params);
    void use_render_settings(const RendererManagerSettings &settings);

   public slots:
    // 更新fps显示
    virtual void updateFpsDisplay(int fps);

   signals:
    void update_window_title_suffix(QString &title_suffix);

   protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    // qt事件
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};

#endif  // GLCANVAS_H
