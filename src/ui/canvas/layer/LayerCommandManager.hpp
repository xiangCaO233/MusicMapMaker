#ifndef MMM_LAYERCOMMANDMANAGER_HPP
#define MMM_LAYERCOMMANDMANAGER_HPP

#include <vector>

#include "render/RenderCommand.hpp"

class LayerCommandManager {
   public:
    // 构造LayerCommandManager
    LayerCommandManager();
    // 析构LayerCommandManager
    virtual ~LayerCommandManager();

   private:
    using RenderData = std::vector<RenderCommand>;
};

#endif  // MMM_LAYERCOMMANDMANAGER_HPP
