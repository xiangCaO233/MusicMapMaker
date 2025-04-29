#ifndef TEXTURE_INSTANCE_H
#define TEXTURE_INSTANCE_H

#include <qimage.h>
#include <qsize.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

class BaseTexturePool;
class MTexturePool;

class TextureInstace {
 public:
  TextureInstace();

  TextureInstace(std::filesystem::path& relative_path,
                 std::filesystem::path& path,
                 std::shared_ptr<MTexturePool> preference = nullptr);

  TextureInstace(const char* relative_path, const char* path,
                 std::shared_ptr<MTexturePool> preference = nullptr);

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
   *通过其他方式向gpu传输此TextureAtlas的对象数据
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
  // 纹理集索引方式:
  // 0x20000001;
  // 当前纹理池第二个纹理集的第一个子纹理
  // 0xd0000101;
  // 当前纹理池第十五个纹理集的第一百零一个子纹理
  uint32_t texture_id;

  // 纹理在纹理池的层数
  uint32_t texture_layer;

  // 纹理尺寸
  int32_t width{-1};
  int32_t height{-1};

  // 纹理通道数
  int32_t channels;

  // 纹理数据
  unsigned char* data;

  // 纹理在图集中的位置
  int32_t woffset{0};
  int32_t hoffset{0};

  // 纹理池引用
  std::shared_ptr<MTexturePool> poolreference;

  // 从文件加载
  void load_from_file(std::filesystem::path& relative_path,
                      std::filesystem::path& file_path);
};

enum class TextureEffect : uint32_t {
  // 无特效
  NONE = 0x0000,
  // 模糊
  BLUR = 0x1000,
  // 灰度
  GRAYSCALE = 0x2000,
  // 光晕
  GLOWING = 0x3000,
  // 半透明
  HALF_TRANSPARENT = 0x4000,
};

// 纹理补充模式
enum class TextureComplementMode : uint32_t {
  FILL_COLOR = 0x0100,
  REPEAT_TEXTURE = 0x0200,
};

// 纹理对齐模式
enum class TextureAlignMode : uint32_t {
  // 对齐左下角
  ALIGN_TO_LEFT_BOTTOM = 0x0010,
  // 对齐右下角
  ALIGN_TO_RIGHT_BOTTOM = 0x0020,
  // 对齐左上角
  ALIGN_TO_LEFT_TOP = 0x0030,
  // 对齐右上角
  ALIGN_TO_RIGHT_TOP = 0x0040,
  // 对齐中心
  ALIGN_TO_CENTER = 0x0050,
};

// 纹理填充模式
enum class TextureFillMode : uint32_t {
  // 填充,直接塞入(比例不一致会变形)
  FILL = 0x0000,
  // 保持原尺寸
  // (纹理尺寸比图形大时无法完全显示,需指定对齐方式)
  // (纹理尺寸比图形小时无法填满图形,需指定对齐方式和空缺填充方式)
  KEEP = 0x0001,
  // 裁切--比例不一致会保证不变形的前提下裁剪一部分
  // 缩放并平铺(保证铺满图形且不变形---所以会裁剪一部分)
  // (选择会导致丢失像素最少的一边为基准裁剪)
  // 保证最大可视度
  SCALLING_AND_TILE = 0x0002,
  // 缩放并裁切
  // (强制指定以宽为基准)
  SCALLING_BASE_WIDTH_AND_CUT = 0x0003,
  // 缩放并裁切
  // (强制指定以高为基准)
  SCALLING_BASE_HEIGHT_AND_CUT = 0x0004,
};

#endif  // TEXTURE_INSTANCE_H
