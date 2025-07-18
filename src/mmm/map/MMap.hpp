#ifndef MMM_MMAP_HPP
#define MMM_MMAP_HPP

#include <mmm/NoteCollection.hpp>

// map

class MMap {
   public:
    MMap();
    virtual ~MMap();

    NoteCollection notes;
};

#endif  // MMM_MMAP_HPP
