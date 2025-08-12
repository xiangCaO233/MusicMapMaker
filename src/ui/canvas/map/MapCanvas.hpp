#ifndef MMM_MAPCANVAS_HPP
#define MMM_MAPCANVAS_HPP

#include <qhash.h>
#include <qobject.h>

#include <canvas/GLCanvas.hpp>
#include <memory>
#include <tool/BaseTool.hpp>
#include <unordered_map>

#include "map/skin/MSkin.hpp"
#include "util/StringHash.hpp"

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
    void initializeGL() override;
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

   private:
    // 绑定的谱面
    MMap *map{nullptr};

    // 持有全部的编辑器皮肤
    std::unordered_map<std::string, std::unique_ptr<MSkin>, StringHash,
                       std::equal_to<>>
        editor_skins;

    // 当前使用的皮肤
    MSkin *skin;

    // 工具集合
    QHash<QString, BaseTool *> tools;

    // 创建工具
    void creatTools();

    friend class MapEditor;
};
#endif  // MMM_MAPCANVAS_HPP
