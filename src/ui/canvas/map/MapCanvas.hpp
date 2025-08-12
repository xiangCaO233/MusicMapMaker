#ifndef MMM_MAPCANVAS_HPP
#define MMM_MAPCANVAS_HPP

#include <qhash.h>
#include <qobject.h>

#include <canvas/GLCanvas.hpp>
#include <memory>
#include <tool/BaseTool.hpp>

class MMap;

class MapCanvas : public GLCanvas {
   public:
    // 构造MapCanvas
    MapCanvas();

    // 析构MapCanvas
    ~MapCanvas() override;

    // 切换到图
    void switch_map(MMap *smap);

   protected:
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

   private:
    // 绑定的谱面
    MMap *map{nullptr};

    // 工具集合
    QHash<QString, BaseTool *> tools;

    // 创建工具
    void creatTools();

    friend class MapEditor;
};
#endif  // MMM_MAPCANVAS_HPP
