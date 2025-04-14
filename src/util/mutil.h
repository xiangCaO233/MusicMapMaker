#ifndef M_UTIL_H
#define M_UTIL_H

#include <unicode/ucnv.h>
#include <unicode/ustring.h>

#include <string>

namespace mutil {
inline void string2u32sring(const std::string& src, std::u32string& des) {
  UErrorCode status = U_ZERO_ERROR;
  UConverter* conv = ucnv_open("UTF-8", &status);

  // 计算所需缓冲区大小
  int32_t destLength = 0;
  u_strFromUTF8(nullptr, 0, &destLength, src.c_str(), src.length(), &status);
  // 重置状态
  status = U_ZERO_ERROR;

  // 分配缓冲区并转换
  u_strFromUTF8(reinterpret_cast<UChar*>(&des[0]), destLength, nullptr,
                src.c_str(), src.length(), &status);
  ucnv_close(conv);
}
};  // namespace mutil

#endif  // M_UTIL_H
