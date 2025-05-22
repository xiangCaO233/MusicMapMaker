#include "GLCanvas.h"

#include <qcolor.h>
#include <qdir.h>
#include <qdiriterator.h>
#include <qlogging.h>
#include <qobject.h>
#include <qopenglversionfunctions.h>
#include <qpaintdevice.h>
#include <qpainter.h>
#include <qpoint.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include <QFile>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <unordered_set>

#include "colorful-log.h"
#include "renderer/font/FontRenderer.h"
#include "texture/pool/MTexturePool.h"

// 用于包装 OpenGL 调用并检查错误
#define GLCALL(func)                                             \
    func;                                                        \
    {                                                            \
        XLogger::glcalls++;                                      \
        GLenum error = glGetError();                             \
        if (error != GL_NO_ERROR) {                              \
            XERROR("在[" + std::string(#func) +                  \
                   "]发生OpenGL错误: " + std::to_string(error)); \
        }                                                        \
    }

bool GLCanvas::need_update_sampler_location = false;

GLCanvas::GLCanvas(QWidget *parent) {
    // 初始化帧率计数器
    fpsCounter = new FrameRateCounter();

    // 更新fps显示内容
    connect(fpsCounter, &FrameRateCounter::fpsUpdated, this,
            &GLCanvas::updateFpsDisplay);

    // 获取主屏幕的刷新率
    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    float refreshRate = primaryScreen->refreshRate();

    XINFO("显示器刷新率:" + std::to_string(refreshRate) + "Hz");

    // 垂直同步帧间隔
    des_update_time = 1000.0 / refreshRate / refreshRate_ratio;
}

GLCanvas::~GLCanvas() {
    // 释放渲染管理器
    if (renderer_manager) delete renderer_manager;
    // 确保刷新线程退出
    stop_refresh = true;
    frame_data_buffer_manager.notify_all_for_exit();
    // update_thread.join();
};

// 使用主题
void GLCanvas::use_theme(GlobalTheme theme) {
    switch (theme) {
        case GlobalTheme::DARK: {
            gl_clear_color[0] = .23f;
            gl_clear_color[1] = .23f;
            gl_clear_color[2] = .23f;
            gl_clear_color[3] = 1.0f;
            break;
        }
        case GlobalTheme::LIGHT: {
            gl_clear_color[0] = .86f;
            gl_clear_color[1] = .86f;
            gl_clear_color[2] = .86f;
            gl_clear_color[3] = 1.0f;
            break;
        }
    }
}

// 更新fps显示
void GLCanvas::updateFpsDisplay(int fps) {}

// qt事件
// 鼠标按下事件
void GLCanvas::mousePressEvent(QMouseEvent *event) {
    // 传递事件
    QOpenGLWindow::mousePressEvent(event);
}

// 鼠标释放事件
void GLCanvas::mouseReleaseEvent(QMouseEvent *event) {
    // 传递事件
    QOpenGLWindow::mouseReleaseEvent(event);
}

// 鼠标双击事件
void GLCanvas::mouseDoubleClickEvent(QMouseEvent *event) {
    // 传递事件
    QOpenGLWindow::mouseDoubleClickEvent(event);
}

// 鼠标移动事件
void GLCanvas::mouseMoveEvent(QMouseEvent *event) {
    // 传递事件
    QOpenGLWindow::mouseMoveEvent(event);
    mouse_pos = event->pos();
    // repaint();
}

// 鼠标滚动事件
void GLCanvas::wheelEvent(QWheelEvent *event) {
    // 传递事件
    QOpenGLWindow::wheelEvent(event);
}

// 键盘按下事件
void GLCanvas::keyPressEvent(QKeyEvent *event) {
    // 传递事件
    QOpenGLWindow::keyPressEvent(event);
}

// 键盘释放事件
void GLCanvas::keyReleaseEvent(QKeyEvent *event) {
    // 传递事件
    QOpenGLWindow::keyReleaseEvent(event);
}

// 取得焦点事件
void GLCanvas::focusInEvent(QFocusEvent *event) {
    // 传递事件
    QOpenGLWindow::focusInEvent(event);
}

// 失去焦点事件
void GLCanvas::focusOutEvent(QFocusEvent *event) {
    // 传递事件
    QOpenGLWindow::focusOutEvent(event);
}

// 调整尺寸事件
void GLCanvas::resizeEvent(QResizeEvent *event) {
    // 传递事件
    QOpenGLWindow::resizeEvent(event);
}

void GLCanvas::initializeGL() {
    XINFO("初始化OpenGL函数");
    initializeOpenGLFunctions();
    // 查询opengl版本
    auto version = GLCALL(glGetString(GL_VERSION));
    XINFO("OpenGL 版本: " +
          std::string(reinterpret_cast<const char *>(version)));

    // 查询最大支持多层纹理的最大层数
    GLint maxLayers;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxLayers);
    XINFO("多层纹理最大层数: " + std::to_string(maxLayers));

    // 查询纹理采样器最大连续数量
    GLint max_fragment_samplers;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_fragment_samplers);
    XINFO("纹理采样器最大连续数量: " +
          std::to_string(max_fragment_samplers / 2));
    if (max_fragment_samplers > 16) {
        max_fragment_samplers = 16;
    }

    // 查询纹理采样器最大数量
    GLint max_combined_samplers;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_combined_samplers);
    XINFO("纹理采样器最大数量: " + std::to_string(max_combined_samplers));

    // 查询最大支持抗锯齿MSAA倍率
    GLint maxSamples;
    GLCALL(glGetIntegerv(GL_MAX_SAMPLES, &maxSamples));

    // 初始化纹理池驱动信息
    MTexturePool::init_driver_info(maxLayers, max_combined_samplers,
                                   max_fragment_samplers);

    XINFO("启用最大抗锯齿倍率: " + std::to_string(maxSamples));
    // 启用 最大 MSAA
    context()->format().setSamples(maxSamples);

    // XINFO("启用垂直同步");

    // 检查最大ubo size
    int maxUBOSize;
    GLCALL(glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize));
    XINFO("最大UBO块容量: " + std::to_string(maxUBOSize));

    // 标准混合模式
    GLCALL(glEnable(GL_BLEND));
    GLCALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    // 初始化渲染管理器
    renderer_manager = new RendererManager(this, 64, 131072);

    // 启动gl渲染器
    start_render();

    // 启动pushshape线程
    start_pushshape();
}

void GLCanvas::start_pushshape() {
    auto calculate_work = [&]() {
        while (!stop_refresh.load(std::memory_order_acquire)) {
            // 1. 获取后端缓冲区进行写入
            BufferWrapper *current_back_buffer =
                frame_data_buffer_manager.acquire_back_buffer_for_writing(
                    stop_refresh);
            // 执行渲染
            // 计算图形
            push_shape(current_back_buffer);

            // 通知主线程可以绘制
            frame_data_buffer_manager.submit_back_buffer_and_notify_render();
        }
    };
    calculate_thread = std::thread(calculate_work);
    calculate_thread.detach();
}

void GLCanvas::start_render() {
    // 初始化定时器
    auto render_work = [&]() {
        QElapsedTimer timer;
        timer.start();

        while (!stop_refresh) {
            qint64 start = timer.elapsed();
            // 触发主线程更新
            QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);

            qint64 elapsed = timer.elapsed() - start;
            if (elapsed < des_update_time) {
                auto start = std::chrono::high_resolution_clock::now();
                auto end = start + std::chrono::microseconds(
                                       int((des_update_time - elapsed) * 1000));
                std::this_thread::sleep_for(
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        end - start));
            }
        }
    };
    update_thread = std::thread(render_work);
    update_thread.detach();
}

void GLCanvas::resizeGL(int w, int h) {
    GLCALL(glViewport(0, 0, w, h));
    // 投影矩阵
    QMatrix4x4 proj;
    // 计算正交投影矩阵
    proj.ortho(-(float)w / 2.0f, (float)w / 2.0f, -(float)h / 2.0f,
               (float)h / 2.0f, -1.0f, 1.0f);
    // 反转y轴
    proj.scale(1.0f, -1.0f, 1.0f);

    // 更新uniform
    renderer_manager->update_all_projection_mat("projection_mat", proj);

    // 标记需要更新纹理采样器位置
    renderer_manager->font_renderer->need_update_sampler_location = true;
    for (const auto &pool : renderer_manager->texturepools) {
        pool->need_update_sampler_location = true;
    }
}

void GLCanvas::rendergl() {
    XLogger::glcalls = 0;
    XLogger::drawcalls = 0;
    auto before = std::chrono::high_resolution_clock::now().time_since_epoch();
    // 背景色
    GLCALL(glClearColor(gl_clear_color[0], gl_clear_color[1], gl_clear_color[2],
                        gl_clear_color[3]));
    GLCALL(glClear(GL_COLOR_BUFFER_BIT));

    // 取出上一帧数据渲染
    // 1. 获取前端缓冲区进行读取 (内部会进行交换)
    BufferWrapper *current_front_buffer =
        frame_data_buffer_manager.acquire_front_buffer_for_reading(
            stop_refresh);
    // acquire 返回 nullptr 表示应该退出
    if (!current_front_buffer) return;

    // 处理绘制参数
#define PROCESS_QUEUE_ENCAPSULATED(queue_member)          \
    while (!current_front_buffer->queue_member.empty()) { \
        std::vector<RenderParams> &batch =                \
            current_front_buffer->queue_member.front();   \
        for (const auto &params : batch) {                \
            process_render_params(params);                \
        }                                                 \
        current_front_buffer->queue_member.pop();         \
    }
    PROCESS_QUEUE_ENCAPSULATED(bg_datas);
    PROCESS_QUEUE_ENCAPSULATED(orbits_datas);
    PROCESS_QUEUE_ENCAPSULATED(beats_datas);
    PROCESS_QUEUE_ENCAPSULATED(effects_datas);
    PROCESS_QUEUE_ENCAPSULATED(hitobjects_datas);
    PROCESS_QUEUE_ENCAPSULATED(preview_datas);
    PROCESS_QUEUE_ENCAPSULATED(selection_datas);
    PROCESS_QUEUE_ENCAPSULATED(info_datas);
    PROCESS_QUEUE_ENCAPSULATED(topbar_datas);
#undef PROCESS_QUEUE_ENCAPSULATED

    renderer_manager->renderAll();
    // 3. 释放前端缓冲区
    frame_data_buffer_manager.release_front_buffer_and_notify_calc();

    auto after = std::chrono::high_resolution_clock::now().time_since_epoch();
    pre_frame_time =
        std::chrono::duration_cast<std::chrono::microseconds>(after - before)
            .count();

    pre_glcalls = XLogger::glcalls;
    pre_drawcall = XLogger::drawcalls;
    fpsCounter->frameRendered();
}

// 绘制画布
void GLCanvas::paintGL() { rendergl(); }

void GLCanvas::use_render_settings(const RendererManagerSettings &settings) {
    renderer_manager->texture_effect = settings.texture_effect;
    renderer_manager->texture_fillmode = settings.texture_fillmode;
    renderer_manager->texture_alignmode = settings.texture_alignmode;
    renderer_manager->texture_complementmode = settings.texture_complementmode;
}

void GLCanvas::process_render_params(const RenderParams &params) {
    use_render_settings(params.render_settings);
    switch (params.func_type) {
        case FunctionType::MRECT: {
            renderer_manager->addRect(
                QRectF(params.xpos, params.ypos, params.width, params.height),
                params.texture, QColor(params.r, params.g, params.b, params.a),
                params.rotation, params.is_volatile);
            break;
        }
        case FunctionType::MROUNDRECT: {
            renderer_manager->addRoundRect(
                QRectF(params.xpos, params.ypos, params.width, params.height),
                params.texture, QColor(params.r, params.g, params.b, params.a),
                params.rotation, params.radius, params.is_volatile);
            break;
        }
        case FunctionType::MLINE: {
            renderer_manager->addLine(
                QPointF(params.x1, params.y1), QPointF(params.x2, params.y2),
                params.line_width, params.texture,
                QColor(params.r, params.g, params.b, params.a),
                params.is_volatile);
            break;
        }
        case FunctionType::MELLIPSE: {
            renderer_manager->addEllipse(
                QRectF(params.xpos, params.ypos, params.width, params.height),
                params.texture, QColor(params.r, params.g, params.b, params.a),
                params.rotation, params.is_volatile);
            break;
        }
        case FunctionType::MTEXT: {
            renderer_manager->addText(
                QPointF(params.xpos, params.ypos), params.str,
                params.line_width, params.font_family,
                QColor(params.r, params.g, params.b, params.a),
                params.rotation);
            break;
        }
    }
}

// 渲染实际图形
void GLCanvas::push_shape(BufferWrapper *current_back_buffer) {}

// 从指定目录添加纹理
void GLCanvas::load_texture_from_path(const char *p) {
    auto path = std::filesystem::path(p);
    load_texture_from_path(p);
}

// 从指定目录添加纹理
void GLCanvas::load_texture_from_path(const std::filesystem::path &path) {
    QString qps = QString::fromStdString(path.string());
    QString apath = QDir::currentPath() + "/" + qps + "/";
    QDir dir(apath);
    if (!dir.exists()) {
        qWarning() << "Directory does not exist:" << path;
        return;
    }

    // 筛选图片文件
    static const std::unordered_set<std::string> image_extention = {
        "png", "jpg", "jpeg"};
    // 递归遍历所有文件和子目录
    QDirIterator it(path, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString filePath = it.next();
        auto finfo = it.fileInfo();
        auto extention = finfo.suffix().toStdString();
        if (finfo.isFile()) {
            if (image_extention.find(extention) != image_extention.end()) {
                std::string filestr = filePath.toStdString();
                auto file = filestr.c_str();
                add_texture(apath.toStdString().c_str(), file);
            }
        }
    }
}

// 设置垂直同步
void GLCanvas::set_Vsync(bool flag) {
    // 切换V-Sync
    context()->format().setSwapInterval(flag);
}

// 添加纹理
void GLCanvas::add_texture(const char *relative_path, const char *qrc_path) {
    std::filesystem::path p(qrc_path);
    std::filesystem::path rp(relative_path);
    add_texture(rp, p);
}
void GLCanvas::add_texture(std::filesystem::path &relative_path,
                           std::filesystem::path &path) {
    // 初始化纹理
    auto texture = std::make_shared<TextureInstace>(relative_path, path);

    // 检查纹理是否已载入过
    auto texit = texture_full_map.find(texture->name);
    if (texit != texture_full_map.end()) {
        // 载入过了
        return;
    }

    // 是否需要新建纹理池
    bool need_new_pool{true};
    std::shared_ptr<MTexturePool> pool;
    for (const auto &p : renderer_manager->texturepools) {
        if (!p->isfull()) {
            // 有未满的纹理池,就用这个
            need_new_pool = false;
            pool = p;
        }
    }

    // 需要新建时
    if (need_new_pool) {
        pool = std::make_shared<MTexturePool>(this);
        // 加入纹理池表
        renderer_manager->texturepools.emplace_back(pool);
    }

    // 设置引用
    texture->poolreference = pool;

    // 添加到纹理池
    auto res = pool->load_texture(texture);

    if (res) {
        // 成功添加映射
        texture_full_map.try_emplace(texture->name, texture);
    }
}
