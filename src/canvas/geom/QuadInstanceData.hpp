#ifndef MMM_QUADINSTANCEDATA_HPP
#define MMM_QUADINSTANCEDATA_HPP

#include <qvectornd.h>

#include <QMatrix4x4>

// 矩形实例数据

struct QuadInstanceData {
    QMatrix4x4 model_matrix;
    QVector4D color;
};
#endif  // MMM_QUADINSTANCEDATA_HPP
