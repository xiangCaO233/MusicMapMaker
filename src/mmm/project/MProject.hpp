#ifndef MMM_MPROJECT_HPP
#define MMM_MPROJECT_HPP

#include <ice/manage/AudioTrack.hpp>
#include <map>
#include <memory>
#include <mmm/map/MMap.hpp>
#include <set>
#include <string>

#include "mmm/project/AudioLoadCallback.hpp"

class TextureLoadCallback;
class MProject {
   public:
    // 构造MProject
    explicit MProject(TextureLoadCallback* texloadcbk,
                      AudioLoadCallback* audioLoadcbk);

    // 析构MProject
    virtual ~MProject();

    // 打开项目
    void open(std::string_view project_path);

    // 关闭项目
    void close();

   private:
    // 项目路径
    std::filesystem::path project_path;

    // 项目谱面表(谱面资源持有)
    std::map<std::string, std::unique_ptr<MMap>, std::less<>>
        project_maps_table;

    // 项目音频列表(非音频资源持有)
    std::map<std::string, std::weak_ptr<ice::AudioTrack>, std::less<>>
        project_audios_table;

    // 项目图片列表
    std::set<std::string, std::less<>> project_image_table;

    // 项目视频列表
    std::set<std::string, std::less<>> project_video_table;

    // 回调指针
    TextureLoadCallback* texcallback;
    AudioLoadCallback* audiocallback;

    // 项目管理器可直接访问私有成员
    friend class ProjectManager;
};

#endif  // MMM_MPROJECT_HPP
