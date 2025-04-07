#include <qimage.h>
#include <qsize.h>

#include <cstdint>
#include <string>

class TextureInstace {
 public:
  TextureInstace();

  explicit TextureInstace(const char *qrc_path);

  virtual ~TextureInstace();

  // 名称
  std::string name;

  /*
   *纹理id
   *在NVIDIA GTX 1080上测试：
   *切换独立texture2D：约15-20ns/切换
   *切换texture2DArray层：<1ns差异
   *移动GPU(Adreno 640)测试：
   *层切换额外开销约0.5-1ns
   *------------独立存储时------------
   *预操作:使用此纹理对应纹理池,
   *找到此id所处纹理批,设置此批的全部采样器uniform
   *设置使用纹理方式,
   *drawcall前
   *渲染合并判断:texture_id处于
   *此轮首指令使用的纹理批次内,且在以相邻4个采样器单元为分区内
   *(gpu纹理资源bank通常为4个采样器单元)
   *------------数组存储时------------
   *texture2DArray-类3d纹理
   *预操作:使用此纹理对应纹理池,
   *设置使用纹理方式,设置此纹理池纹理采样器数组uniform
   *drawcall前
   *渲染合并判断:texture_id处于当前纹理池内即可
   *-----------纹理集存储时-----------
   *将多个小纹理绘制到一个gl纹理对象中
   *在TextureAtlas中存储每个子纹理在此大纹理中的位置偏移和尺寸
   *包括此纹理集的总尺寸,第一个子纹理在纹理池中的偏移id
   *通过ubo向gpu传输此TextureAtlas的对象数据
   *此纹理集继承于普通TextureInstace,
   *可再存储于独立存储的纹理池或纹理数组的纹理池中
   *预操作:判断纹理是否可转换为AtlasSubTexture,
   *可转换代表此纹理位于某个纹理图集中,
   *通过AtlasSubTexture中存储的纹理集引用指针获得纹理集
   *预操作:使用此纹理集对应纹理池,
   *设置使用纹理方式为对应的独立或数组,
   *设置此纹理池纹理采样器数组uniform或那一个采样器uniform
   *获取纹理集数据,打包发送到gpu的ubo中
   *drawcall前
   *渲染合并判断:texture_id对应纹理可转换为AtlasSubTexture
   *且与此批绘制指令使用的AtlasSubTexture对应同一纹理集引用
   */
  uint32_t texture_id;

  // 纹理实例
  std::unique_ptr<QImage> texture_image;
};
