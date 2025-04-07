#include <qimage.h>
#include <qsize.h>

#include <cstdint>
#include <string>

class TextureInstace {
 public:
  // 名称
  std::string name;
  // 纹理id
  uint32_t texture_id;
  // 纹理实例
  std::unique_ptr<QImage> texture_image;

  // 纹理在图集中的位置
  uint32_t woffset{0};
  uint32_t hoffset{0};

  TextureInstace();
  explicit TextureInstace(const char *qrc_path);
  virtual ~TextureInstace();
};
