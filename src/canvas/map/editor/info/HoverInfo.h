#ifndef M_HOVERINFO_H
#define M_HOVERINFO_H

#include <memory>

class HitObject;
class Beat;

enum class HoverPart {
    // 悬浮于物件头
    HEAD,
    // 悬浮于长条身
    HOLD_BODY,
    // 悬浮于长条尾
    HOLD_END,
    // 悬浮于滑键尾
    SLIDE_END,
    // 悬浮于组合键节点
    COMPLEX_NODE,
};

struct HoverObjectInfo {
    std::shared_ptr<HitObject> hoverobj;
    Beat* hoverbeat;
    HoverPart part;

    HoverObjectInfo* clone() {
        auto info = new HoverObjectInfo;
        info->hoverobj = hoverobj;
        info->hoverbeat = hoverbeat;
        info->part = part;
        return info;
    }
};

#endif  // M_HOVERINFO_H
