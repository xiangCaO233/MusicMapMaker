#include "ComplexNote.h"

#include "src/hitobject/Note/Note.h"

ComplexNote::ComplexNote(uint32_t time) : Note(time) {
  type = HitObjectType::COMPLEX;
}

ComplexNote::~ComplexNote() = default;
