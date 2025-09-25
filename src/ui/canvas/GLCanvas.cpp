#include <QGuiApplication>
#include <QOpenGLFunctions>
#include <QScreen>
#include <canvas/GLCanvas.hpp>
#include <canvas/render/Renderer2D.hpp>
#include <chrono>
#include <layer/LayerManager.hpp>
#include <mmm/timing/Timing.hpp>
#include <render/command/GLDirectPainter.hpp>
#include <render/command/MCommandCollector.hpp>
#include <render/synchronize/tick/RenderDataLoop.hpp>
#include <render/texture/TexMode.hpp>
#include <render/texture/TexturePool.hpp>
#include <util/glcheck.hpp>

// 构造GLCanvas
GLCanvas::GLCanvas() {
    // 初始化帧率计数器
    fpsCounter = new FrameRateCounter();

    // 更新fps显示内容
    connect(fpsCounter, &FrameRateCounter::fpsUpdated, this,
            &GLCanvas::updateFpsDisplay);

    desiredFps = QGuiApplication::primaryScreen()->refreshRate();
    qDebug() << "显示器刷新率 : " << desiredFps;

    // 帧间隔
    // auto des_update_time = 1000.0 / desiredFps;
    // qDebug() << "目标帧间隔 : " << std::to_string(des_update_time);
}

// 析构GLCanvas
GLCanvas::~GLCanvas() { delete fpsCounter; }

// 释放活动线程
void GLCanvas::release_threads() {
    render_dataloop.reset();
    emit dataloop_stopped();
}

// 释放渲染器
void GLCanvas::release_render() { render.reset(); }

void GLCanvas::onAudioLoadcbkInitialized(AudioLoadCallback* cbk) {
    audioLoadcbk = cbk;
}

void GLCanvas::updateFpsDisplay(int fps) {
    size_t avgglcalls{0};
    size_t avgdrawcalls{0};
    if (fps != 0) {
        avgglcalls = mstat::gl_calls / fps;
        avgdrawcalls = mstat::draw_calls / fps;
    }
    QString title_suffix =
        QString(
            "%1 FPS(frametime: "
            "%2 us | updatetime(qt): %3 us) GLCALLS:%4 | DRAWCALLS:%5")
            .arg(fps)
            .arg(std::chrono::duration_cast<std::chrono::microseconds>(
                     pre_frame_time)
                     .count())
            .arg(last_update_time_us)
            .arg(avgglcalls)
            .arg(avgdrawcalls);
    mstat::gl_calls = 0;
    mstat::draw_calls = 0;
    emit update_window_suffix(title_suffix);
}

void GLCanvas::onUpdateTexinfo() {}

void GLCanvas::gotoTiming(Timing* timing) {
    canvas_info->realTimeInfo.current_time_info.logic_canvas_time =
        timing->timestamp -
        canvas_info->realTimeInfo.offset_info.global_offset_ms;
}

// 更新共享信息
void GLCanvas::update_sharedInfo() const {
    // 更新信息
    render_dataloop->update_info(canvas_info.get());
}

void GLCanvas::initializeGL() {
    initializeOpenGLFunctions();
    // 查询opengl版本
    auto version = GLCALL_R(glGetString(GL_VERSION), this);
    qDebug() << "OpenGL 版本: "
             << std::string(reinterpret_cast<const char*>(version));

    GLint maxVertices, maxComponents;
    GLCALL_V(glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &maxVertices),
             this);
    qDebug() << "几何着色器最大输出顶点数: " << std::to_string(maxVertices);
    GLCALL_V(
        glGetIntegerv(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, &maxComponents),
        this);
    qDebug() << "几何着色器最大输出顶点分量数: "
             << std::to_string(maxComponents);

    // 查询最大支持抗锯齿MSAA倍率
    GLint maxSamples;
    GLCALL_V(glGetIntegerv(GL_MAX_SAMPLES, &maxSamples), this);

    // 初始化驱动信息
    qDebug() << "启用最大抗锯齿倍率: " << std::to_string(maxSamples);
    // 启用 最大 MSAA
    context()->format().setSamples(maxSamples);

    // 检查最大ubo size
    int maxUBOSize;
    GLCALL_V(glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize), this);
    qDebug() << "最大UBO块容量: " << std::to_string(maxUBOSize);

    // 标准混合模式
    GLCALL_V(glEnable(GL_BLEND), this);
    GLCALL_V(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA), this);

    // 关闭深度测试，否则全屏矩形可能因为深度测试失败而被丢弃
    // GLCALL(glDisable(GL_DEPTH_TEST), this);
    // 在模糊pass中通常也不需要混合
    // GLCALL(glDisable(GL_BLEND), this);

    // 初始化渲染器
    render = std::make_unique<Renderer2D>(this);

    // 连接信号
    connect(render.get(), &Renderer2D::needUpdateTexinfo, this,
            &GLCanvas::onUpdateTexinfo);

// 加载字体
#ifdef __APPLE__
    render->add_font_from_path(
        "../../../../resources/font/ComicShannsMonoNerdFont_Bold.otf");
    render->add_font_from_path(
        "../../../../resources/font/NotoSansCJK-Bold.ttc");
#else
    render->add_font_from_path(
        "../resources/font/ComicShannsMonoNerdFont_Bold.otf");
    render->add_font_from_path("../resources/font/NotoSansCJK-Bold.ttc");
#endif  //__APPLE__

    // 加载蒙版

    // 暗化蒙版
    // render->newMask({0, 0, 1000, 1000}, {.2f, .2f, .2f, 0.2f},
    //                 MaskEffect::DARKEN);
    // 滤镜蒙版
    // render->newMask({0, 0, 1000, 1000}, {1.f, .5f, .2f, .75f},
    //                 MaskEffect::FILTER);
    // 透明蒙版
    // render->newMask({0, 0, 2000, 2000}, {.3f, .5f, .2f, .75f},
    //                 MaskEffect::ALPHA_SHIFT);
}

void GLCanvas::resizeGL(int w, int h) {
    GLCALL_V(glViewport(0, 0, w, h), this);
    glm::vec2 viewport = {w, h};
    render->update_viewport(viewport, viewport * float(devicePixelRatio()));
}

void GLCanvas::paintGL() {
    auto before = std::chrono::high_resolution_clock::now().time_since_epoch();

    // QuadCommand qcmd;
    // qcmd.cmdType = CommandType::QUAD;
    // qcmd.baseInfo.pos = {300, 300};
    // qcmd.baseInfo.size = {100, 100};
    // qcmd.baseInfo.color = {0, 1, 1, 1};

    // PrimitiveCommand cmd;
    // cmd.cmdType = CommandType::PRIMITIVE;
    // cmd.baseInfo.pos = {100, 100};
    // cmd.baseInfo.size = {100, 100};
    // cmd.baseInfo.color = {1, 1, 0, 1};
    // cmd.primitive = PrimitiveType::QUAD;

    // render->commit(qcmd);
    // render->commit(cmd);
    // render->finalize();
    // render->render();

    {
        MCommandCollector pc(render.get(), render_dataloop->layermanager());

        // 直接绘制
        // GLDirectPainter painter(render.get());

        // painter.paintImage(
        //     "../resources/textures/default/物件/arrowright_selected.png",
        //     {50, 50}, {{1.f, 1.f}, 16.f});

        // painter.fillImage(
        //     "../resources/textures/default/物件/arrowright_selected.png",
        //     {{350, 550}, {100, 100}});

        // painter.paintImage(
        //     "../resources/textures/default/打击特效/划键打击特效/1.png",
        //     {100, 100}, {{1.f, 1.f}, 0.f});

        // painter.paintLine({100, 50}, {200, 100}, {0.f, 0.f,0.f, 1.f}, 4.f);

        //    painter.paintString("ComicShannsMono Nerd Font", 16, U"nmsl",
        //                        {100, 100});
    }

    fpsCounter->frameRendered();
    pre_frame_time =
        std::chrono::high_resolution_clock::now().time_since_epoch() - before;
}
