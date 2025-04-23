#ifndef M_RMHOLD_H
#define M_RMHOLD_H

#include <memory>

#include "../Hold.h"
#include "mmm/hitobject/Note/rm/ComplexNote.h"

// 节奏大师长条

class RMHold : public Hold {
 public:
  // 构造RMHold
  RMHold(uint32_t time, int32_t orbit_pos, uint32_t holdtime);
  // 析构RMHold
  ~RMHold() override;

  // 父组合键引用
  std::shared_ptr<ComplexNote> parent_reference{nullptr};
};

#endif  // M_RMHOLD_H
