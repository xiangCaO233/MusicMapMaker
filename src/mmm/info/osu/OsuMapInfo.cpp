#include <mmm/info/osu/OsuMapInfo.hpp>

OsuMapMetadata::OsuMapMetadata() {
    type = MapMetadataType::OSU;
    // 添加章节
    chapters.try_emplace("General", std::make_unique<OsuMapChapterGeneral>());
    chapters.try_emplace("Editor", std::make_unique<OsuMapChapterEditor>());
    chapters.try_emplace("Metadata", std::make_unique<OsuMapChapterMetadata>());
    chapters.try_emplace("Difficulty",
                         std::make_unique<OsuMapChapterDifficulty>());
    chapters.try_emplace("Events", std::make_unique<OsuMapChapterEvents>());
    chapters.try_emplace("Colour", std::make_unique<OsuMapChapterColour>());
}

OsuMapMetadata::OsuMapMetadata(BaseMapMeta& basemetadata,
                               const std::filesystem::path& project_path) {
    type = MapMetadataType::OSU;
    // 添加章节
    auto general_chapter = static_cast<OsuMapChapterGeneral*>(
        chapters
            .try_emplace("General", std::make_unique<OsuMapChapterGeneral>())
            .first->second.get());
    general_chapter->AudioFilename =
        std::filesystem::relative(basemetadata.main_audio_path, project_path)
            .generic_string();
    general_chapter->PreviewTime = basemetadata.map_length / 2.f;
    general_chapter->Mode = 3;

    auto editor_chapter = static_cast<OsuMapChapterEditor*>(
        chapters.try_emplace("Editor", std::make_unique<OsuMapChapterEditor>())
            .first->second.get());

    auto metadata_chapter = static_cast<OsuMapChapterMetadata*>(
        chapters
            .try_emplace("Metadata", std::make_unique<OsuMapChapterMetadata>())
            .first->second.get());
    metadata_chapter->Title = basemetadata.title;
    metadata_chapter->TitleUnicode = basemetadata.title_unicode;
    metadata_chapter->Artist = basemetadata.artist;
    metadata_chapter->ArtistUnicode = basemetadata.artist_unicode;
    metadata_chapter->Creator = basemetadata.author;
    metadata_chapter->Version = basemetadata.version;

    auto difficulty_chapter = static_cast<OsuMapChapterDifficulty*>(
        chapters
            .try_emplace("Difficulty",
                         std::make_unique<OsuMapChapterDifficulty>())
            .first->second.get());

    auto event_chapter = static_cast<OsuMapChapterEvents*>(
        chapters.try_emplace("Events", std::make_unique<OsuMapChapterEvents>())
            .first->second.get());
    event_chapter->bg_file_name =
        std::filesystem::relative(basemetadata.main_cover_path, project_path)
            .generic_string();

    auto colour_chapter =
        chapters.try_emplace("Colour", std::make_unique<OsuMapChapterColour>())
            .first->second.get();
}

OsuMapMetadata::~OsuMapMetadata() = default;

// 更新统一属性表
void OsuMapMetadata::update_universe_properties() {
    // 更新通用属性仓库
    for (const auto& [chapter_name, chapter] : chapters) {
        chapter->update_propertie_map(map_properties);
    }
}

// osu格式默认的元数据
std::shared_ptr<MapMetadata> OsuMapMetadata::default_metadata() {
    // 构造一个OsuMapMetadata
    auto metadata = std::make_shared<OsuMapMetadata>();

    // 并填充更新通用属性仓库
    metadata->update_universe_properties();

    return metadata;
}

using enum MapMetadataType;

OsuMapChapterGeneral::OsuMapChapterGeneral() = default;

OsuMapChapterGeneral::~OsuMapChapterGeneral() = default;

void OsuMapChapterGeneral::update_propertie_map(
    std::unordered_map<MapMetadataType,
                       std::unordered_map<std::string, std::string, StringHash,
                                          std::equal_to<>>>& properties) {
    properties[OSU]["AudioFilename"] = AudioFilename;
    properties[OSU]["AudioLeadIn"] = std::to_string(AudioLeadIn);
    properties[OSU]["AudioHash"] = AudioHash;
    properties[OSU]["PreviewTime"] = std::to_string(PreviewTime);
    properties[OSU]["Countdown"] = std::to_string(Countdown);
    std::string sample_set_name;
    switch (sample_set) {
        using enum SampleSet;
        case NONE:
        case NORMAL: {
            sample_set_name = "Normal";
            break;
        }
        case SOFT: {
            sample_set_name = "Soft";
            break;
        }
        case DRUM: {
            sample_set_name = "Drum";
            break;
        }
    }
    properties[OSU]["SampleSet"] = sample_set_name;
    properties[OSU]["StackLeniency"] = std::to_string(StackLeniency);
    properties[OSU]["Mode"] = std::to_string(Mode);
    properties[OSU]["LetterboxInBreaks"] =
        std::to_string(LetterboxInBreaks ? 1 : 0);
    properties[OSU]["StoryFireInFront"] =
        std::to_string(StoryFireInFront ? 1 : 0);
    properties[OSU]["UseSkinSprites"] = std::to_string(UseSkinSprites ? 1 : 0);
    properties[OSU]["AlwaysShowPlayfield"] =
        std::to_string(AlwaysShowPlayfield ? 1 : 0);
    properties[OSU]["OverlayPosition"] = OverlayPosition;
    properties[OSU]["SkinPreference"] = SkinPreference;
    properties[OSU]["EpilepsyWarning"] =
        std::to_string(EpilepsyWarning ? 1 : 0);
    properties[OSU]["CountdownOffset"] = std::to_string(CountdownOffset);
    properties[OSU]["SpecialStyle"] = std::to_string(SpecialStyle ? 1 : 0);
    properties[OSU]["WidescreenStoryboard"] =
        std::to_string(WidescreenStoryboard ? 1 : 0);
    properties[OSU]["SamplesMatchPlaybackRate"] =
        std::to_string(SamplesMatchPlaybackRate ? 1 : 0);
}

OsuMapChapterEditor::OsuMapChapterEditor() = default;

OsuMapChapterEditor::~OsuMapChapterEditor() = default;

void OsuMapChapterEditor::update_propertie_map(
    std::unordered_map<MapMetadataType,
                       std::unordered_map<std::string, std::string, StringHash,
                                          std::equal_to<>>>& properties) {
    std::string bookmarks_str{""};
    if (!Bookmarks.empty()) {
        std::stringstream ss;
        // 将第一个元素写入流
        ss << Bookmarks.front();
        // 从第二个元素开始，先写逗号再写元素
        for (size_t i = 1; i < Bookmarks.size(); ++i) {
            ss << "," << Bookmarks[i];
        }
        bookmarks_str = ss.str();
    }
    properties[OSU]["Bookmarks"] = bookmarks_str;
    properties[OSU]["DistanceSpacing"] = std::to_string(DistanceSpacing);
    properties[OSU]["BeatDivisor"] = std::to_string(BeatDivisor);
    properties[OSU]["GridSize"] = std::to_string(GridSize);
    properties[OSU]["TimelineZoom"] = std::to_string(TimelineZoom);
}

OsuMapChapterMetadata::OsuMapChapterMetadata() = default;

OsuMapChapterMetadata::~OsuMapChapterMetadata() = default;

void OsuMapChapterMetadata::update_propertie_map(
    std::unordered_map<MapMetadataType,
                       std::unordered_map<std::string, std::string, StringHash,
                                          std::equal_to<>>>& properties) {
    properties[OSU]["Title"] = Title;
    properties[OSU]["TitleUnicode"] = TitleUnicode;
    properties[OSU]["Artist"] = Artist;
    properties[OSU]["ArtistUnicode"] = ArtistUnicode;
    properties[OSU]["Creator"] = Creator;
    properties[OSU]["Version"] = Version;
    properties[OSU]["Source"] = Source;

    std::string tags_str{""};
    if (!Tags.empty()) {
        std::stringstream ss;
        ss << Tags.front();
        for (size_t i = 1; i < Tags.size(); ++i) {
            ss << "," << Tags[i];
        }
        tags_str = ss.str();
    }
    properties[OSU]["Tags"] = tags_str;
    properties[OSU]["BeatmapID"] = std::to_string(BeatmapID);
    properties[OSU]["BeatmapSetID"] = std::to_string(BeatmapSetID);
}

OsuMapChapterDifficulty::OsuMapChapterDifficulty() = default;

OsuMapChapterDifficulty::~OsuMapChapterDifficulty() = default;

void OsuMapChapterDifficulty::update_propertie_map(
    std::unordered_map<MapMetadataType,
                       std::unordered_map<std::string, std::string, StringHash,
                                          std::equal_to<>>>& properties) {
    properties[OSU]["HPDrainRate"] = std::to_string(HPDrainRate);
    properties[OSU]["CircleSize"] = std::to_string(CircleSize);
    properties[OSU]["OverallDifficulty"] = std::to_string(OverallDifficulty);
    properties[OSU]["ApproachRate"] = std::to_string(ApproachRate);
    properties[OSU]["SliderMultiplier"] = std::to_string(SliderMultiplier);
    properties[OSU]["SliderTickRate"] = std::to_string(SliderTickRate);
}

OsuMapChapterEvents::OsuMapChapterEvents() = default;

OsuMapChapterEvents::~OsuMapChapterEvents() = default;

void OsuMapChapterEvents::update_propertie_map(
    std::unordered_map<MapMetadataType,
                       std::unordered_map<std::string, std::string, StringHash,
                                          std::equal_to<>>>& properties) {
    properties[OSU]["background_type"] = std::to_string(background_type);
    properties[OSU]["video_starttime"] = std::to_string(video_starttime);
    properties[OSU]["bg_file_name"] = bg_file_name;
    properties[OSU]["bgxoffset"] = std::to_string(bgxoffset);
    properties[OSU]["bgyoffset"] = std::to_string(bgyoffset);

    std::string breaks_str{""};
    if (!breaks.empty()) {
        std::stringstream ss;
        ss << "Break," << breaks.front().start << "," << breaks.front().end;
        for (int i = 1; i < breaks.size(); ++i) {
            ss << "\nBreak," << breaks[i].start << "," << breaks[i].end;
        }
        breaks_str = ss.str();
    }
    properties[OSU]["Breaks"] = breaks_str;
}

OsuMapChapterColour::OsuMapChapterColour() = default;

OsuMapChapterColour::~OsuMapChapterColour() = default;

void OsuMapChapterColour::update_propertie_map(
    std::unordered_map<MapMetadataType,
                       std::unordered_map<std::string, std::string, StringHash,
                                          std::equal_to<>>>& properties) {
    std::string combocolor_str{""};
    if (!ComboColor.empty()) {
        std::stringstream ss;
        ss << "Combo" << ComboColor.front();
        for (int i = 1; i < ComboColor.size(); ++i) {
            ss << "," << ComboColor[i];
        }
        combocolor_str = ss.str();
    }
    properties[OSU]["ComboColor"] = combocolor_str;

    std::string sliderTrackOverride_str{""};
    if (!ComboColor.empty()) {
        std::stringstream ss;
        ss << SliderTrackOverride.front();
        for (int i = 1; i < SliderTrackOverride.size(); ++i) {
            ss << "," << SliderTrackOverride[i];
        }
        sliderTrackOverride_str = ss.str();
    }
    properties[OSU]["SliderTrackOverride"] = sliderTrackOverride_str;

    std::string sliderBorder_str{""};
    if (!SliderBorder.empty()) {
        std::stringstream ss;
        ss << SliderBorder.front();
        for (int i = 1; i < SliderBorder.size(); ++i) {
            ss << "," << SliderBorder[i];
        }
        sliderBorder_str = ss.str();
    }
    properties[OSU]["SliderBorder"] = sliderBorder_str;
}
