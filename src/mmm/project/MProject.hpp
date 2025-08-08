#ifndef MMM_MPROJECT_HPP
#define MMM_MPROJECT_HPP

#include <map>
#include <memory>
#include <mmm/map/MMap.hpp>
#include <string>

class MProject {
   public:
    // 构造MProject
    MProject();
    // 析构MProject
    virtual ~MProject();

   private:
    // 项目谱面表(name-map)
    std::map<std::string, std::shared_ptr<MMap>, std::less<>>
        project_maps_table;
};

#endif  // MMM_MPROJECT_HPP
