#include "FontRenderer.h"

#include <freetype/freetype.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "../../../log/colorful-log.h"
#include "../../GLCanvas.h"

// 用于包装 OpenGL 调用并检查错误
#define GLCALL(func)                                       \
  func;                                                    \
  {                                                        \
    XLogger::glcalls++;                                    \
    GLenum error = cvs->glGetError();                      \
    if (error != GL_NO_ERROR) {                            \
      XERROR("在[" + std::string(#func) +                  \
             "]发生OpenGL错误: " + std::to_string(error)); \
    }                                                      \
  }

// 当前字形id
uint32_t FontRenderer::current_glyph_id = 0;
// ftlibrary库
FT_Library FontRenderer::ft;
// 每层最大尺寸
const uint32_t FontRenderer::layer_size;
// 最大层数
const uint32_t FontRenderer::layer_count;

// ftlibrary库加载标识
bool FontRenderer::is_ft_library_loaded = false;

// 字体池数量
int FontRenderer::frenderer_count = 0;

FontRenderer::FontRenderer(GLCanvas* canvas,
                           std::shared_ptr<Shader> font_shader)
    : AbstractRenderer(canvas, font_shader, -1, 4096) {
  font_shader->use();
  frenderer_count++;
  // 检查字体库是否已初始化
  if (!is_ft_library_loaded) {
    // @return:
    //   FreeType error code.  0~means success.
    // FreeType函数在出现错误时将返回一个非零的整数值
    if (FT_Init_FreeType(&ft)) {
      XCRITICAL("FreeType初始化失败");
      return;
    } else {
      is_ft_library_loaded = true;
      XINFO("FreeType初始化成功");
    }
  }
  // 生成字体渲染器的实例缓冲对象
  GLCALL(cvs->glGenBuffers(1, &fInstanceVBO));
  GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, fInstanceVBO));
  /*
   *layout(location = 2) in vec2 character_pos;
   *layout(location = 3) in vec2 character_size;
   *layout(location = 4) in float character_rotation;
   *layout(location = 5) in float character_texture_layer;
   *layout(location = 6) in vec4 character_color;
   *
   ****character_uv[7] 会占用连续的 locations 7~10
   *layout(location = 7) in vec2 character_uvs[4];
   */

  auto stride = (2 + 2 + 1 + 1 + 1 + 4 + 2 * 4) * sizeof(float);
  /*
   *layout(location = 2) in vec2 character_pos;
   *layout(location = 3) in vec2 character_size;
   *layout(location = 4) in float character_rotation;
   *layout(location = 5) in float character_layer_index;
   *layout(location = 6) in float character_bearingy;
   *layout(location = 7) in vec4 character_color;
   *** character_uv[8] 会占用连续的 locations 8~11
   *layout(location = 8) in vec2 character_uvs[4];
   */
  // 位置信息
  // 描述location2 缓冲位置0~1float为float类型数据--位置信息(用vec2接收)
  GLCALL(cvs->glEnableVertexAttribArray(2));
  GLCALL(cvs->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, nullptr));
  // 每个实例变化一次
  GLCALL(cvs->glVertexAttribDivisor(2, 1));

  // 尺寸信息
  // 描述location3 缓冲位置2~3float为float类型数据--尺寸信息(用vec2接收)
  GLCALL(cvs->glEnableVertexAttribArray(3));
  GLCALL(cvs->glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride,
                                    (void*)(2 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(3, 1));

  // 旋转角度
  // 描述location4 缓冲位置4~4float为float类型数据--旋转角度(用float接收)
  GLCALL(cvs->glEnableVertexAttribArray(4));
  GLCALL(cvs->glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride,
                                    (void*)(4 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(4, 1));

  // 纹理集层数索引
  // 描述location5 缓冲位置5~5float为float类型数据--纹理集层数索引(用float接收)
  GLCALL(cvs->glEnableVertexAttribArray(5));
  GLCALL(cvs->glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, stride,
                                    (void*)(5 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(5, 1));

  // y偏移数据
  // 描述location6 缓冲位置6~6float为float类型数据--旋转角度(用float接收)
  GLCALL(cvs->glEnableVertexAttribArray(6));
  GLCALL(cvs->glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, stride,
                                    (void*)(6 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(6, 1));

  // 字符填充色
  // 描述location7 缓冲位置7~10float为vec4类型数据--文本填充色(用vec4接收)
  GLCALL(cvs->glEnableVertexAttribArray(7));
  GLCALL(cvs->glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, stride,
                                    (void*)(7 * sizeof(float))));
  GLCALL(cvs->glVertexAttribDivisor(7, 1));

  // 字符纹理集uv
  for (int i = 0; i < 4; i++) {
    // 描述location8+i
    // 缓冲位置11+2*i ~ 11+2*i+1float为vec2类型数据-- uv(用vec2接收)
    GLCALL(cvs->glEnableVertexAttribArray(8 + i));
    GLCALL(cvs->glVertexAttribPointer(8 + i, 2, GL_FLOAT, GL_FALSE, stride,
                                      (void*)((11 + 2 * i) * sizeof(float))));
    GLCALL(cvs->glVertexAttribDivisor(8 + i, 1));
  }

  GLCALL(cvs->glBufferData(GL_ARRAY_BUFFER, (max_shape_count * stride), nullptr,
                           GL_DYNAMIC_DRAW));

  // 创建纹理数组
  GLCALL(cvs->glGenTextures(1, &glyphs_texture_array));
  // 激活纹理单元
  GLCALL(cvs->glActiveTexture(GL_TEXTURE0 + 13));
  GLCALL(cvs->glBindTexture(GL_TEXTURE_2D_ARRAY, glyphs_texture_array));
#ifdef __APPLE__
#else
  // 分配存储空间--8位灰度图
  GLCALL(cvs->glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R8, layer_size,
                             layer_size, layer_count));
#endif  //__APPLE__
  GLCALL(cvs->glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

  // 设置纹理参数
  GLCALL(
      cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GLCALL(
      cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
  GLCALL(
      cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  GLCALL(
      cvs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  free_layers.try_emplace(current_max_layer_index, true);
  // 首次设置
  font_shader->unuse();
}

FontRenderer::~FontRenderer() {
  frenderer_count--;
  if (frenderer_count == 0 && is_ft_library_loaded) {
    // 纹理池已全部释放
    FT_Done_FreeType(ft);
    // 恢复标识
    is_ft_library_loaded = false;
  }
}

// 加载字体
int FontRenderer::load_font(const char* font_path) {
  // 载入字体
  FT_Face face;
  FT_Error ret = FT_New_Face(ft, font_path, 0, &face);
  auto it = ft_faces.try_emplace(face->family_name, std::move(face)).first;

  if (ret != 0) {
    XERROR("加载字体" + std::string(font_path) + "失败");
    return ret;
  } else {
    XINFO("加载字体" + std::string(font_path) + "成功");
    XINFO("font_family:" + std::string(it->second->family_name));
    return ret;
  }
}

// 初始化字符位图缓存
void FontRenderer::generate_ascii_buffer(
    const std::string_view& font_family_name) {
  auto face_it = ft_faces.find(std::string(font_family_name));
  if (face_it == ft_faces.end()) {
    XWARN("字体[" + std::string(font_family_name) + "]未加载过");
    return;
  }
  // 创建ascii字符串
  std::u32string asciistr(U"");
  for (char32_t c = 33; c < 127; c++) asciistr.append(1, c);

  // 检查载入
  check_u32string(asciistr, 12, face_it->second);
  check_u32string(asciistr, 14, face_it->second);
  check_u32string(asciistr, 16, face_it->second);
  check_u32string(asciistr, 18, face_it->second);
  check_u32string(asciistr, 20, face_it->second);
  check_u32string(asciistr, 22, face_it->second);
  check_u32string(asciistr, 24, face_it->second);
  check_u32string(asciistr, 28, face_it->second);
  check_u32string(asciistr, 36, face_it->second);
  check_u32string(asciistr, 48, face_it->second);
}
void FontRenderer::generate_cjk_buffer(
    const std::string_view& font_family_name) {
  auto face_it = ft_faces.find(std::string(font_family_name));
  if (face_it == ft_faces.end()) {
    XWARN("字体[" + std::string(font_family_name) + "]未加载过");
    return;
  }
  // 创建常用中文字符串
  std::u32string cjkstr(
      U" \\!\"#$%&'()*+,-./"
      U"0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
      U"abcdefghijklmnopqrstuvwxyz{|}~"
      U"一乙二十丁厂七卜人入八九几儿了力乃刀又三于干亏士工土才寸下大丈与万上小"
      U"口巾山千乞川亿个勺久凡及夕丸么广亡门义之尸弓己已子卫也女飞刃习叉马乡丰"
      U"王井开夫天无元专云扎艺木五支厅不太犬区历尤友匹车巨牙屯比互切瓦止少日中"
      U"冈贝内水见午牛手毛气升长仁什片仆化仇币仍仅斤爪反介父从今凶分乏公仓月氏"
      U"勿欠风丹匀乌凤勾文六方火为斗忆订计户认心尺引丑巴孔队办以允予劝双书幻玉"
      U"刊示末未击打巧正扑扒功扔去甘世古节本术可丙左厉右石布龙平灭轧东卡北占业"
      U"旧帅归且旦目叶甲申叮电号田由史只央兄叼叫另叨叹四生失禾丘付仗代仙们仪白"
      U"仔他斥瓜乎丛令用甩印乐句匆册犯外处冬鸟务包饥主市立闪兰半汁汇头汉宁穴它"
      U"讨写让礼训必议讯记永司尼民出辽奶奴加召皮边发孕圣对台矛纠母幼丝式刑动扛"
      U"寺吉扣考托老执巩圾扩扫地扬场耳共芒亚芝朽朴机权过臣再协西压厌在有百存而"
      U"页匠夸夺灰达列死成夹轨邪划迈毕至此贞师尘尖劣光当早吐吓虫曲团同吊吃因吸"
      U"吗屿帆岁回岂刚则肉网年朱先丢舌竹迁乔伟传乒乓休伍伏优伐延件任伤价份华仰"
      U"仿伙伪自血向似后行舟全会杀合兆企众爷伞创肌朵杂危旬旨负各名多争色壮冲冰"
      U"庄庆亦刘齐交次衣产决充妄闭问闯羊并关米灯州汗污江池汤忙兴宇守宅字安讲军"
      U"许论农讽设访寻那迅尽导异孙阵阳收阶阴防奸如妇好她妈戏羽观欢买红纤级约纪"
      U"驰巡寿弄麦形进戒吞远违运扶抚坛技坏扰拒找批扯址走抄坝贡攻赤折抓扮抢孝均"
      U"抛投坟抗坑坊抖护壳志扭块声把报却劫芽花芹芬苍芳严芦劳克苏杆杠杜材村杏极"
      U"李杨求更束豆两丽医辰励否还歼来连步坚旱盯呈时吴助县里呆园旷围呀吨足邮男"
      U"困吵串员听吩吹呜吧吼别岗帐财针钉告我乱利秃秀私每兵估体何但伸作伯伶佣低"
      U"你住位伴身皂佛近彻役返余希坐谷妥含邻岔肝肚肠龟免狂犹角删条卵岛迎饭饮系"
      U"言冻状亩况床库疗应冷这序辛弃冶忘闲间闷判灶灿弟汪沙汽沃泛沟没沈沉怀忧快"
      U"完宋宏牢究穷灾良证启评补初社识诉诊词译君灵即层尿尾迟局改张忌际陆阿陈阻"
      U"附妙妖妨努忍劲鸡驱纯纱纳纲驳纵纷纸纹纺驴纽奉玩环武青责现表规抹拢拔拣担"
      U"坦押抽拐拖拍者顶拆拥抵拘势抱垃拉拦拌幸招坡披拨择抬其取苦若茂苹苗英范直"
      U"茄茎茅林枝杯柜析板松枪构杰述枕丧或画卧事刺枣雨卖矿码厕奔奇奋态欧垄妻轰"
      U"顷转斩轮软到非叔肯齿些虎虏肾贤尚旺具果味昆国昌畅明易昂典固忠咐呼鸣咏呢"
      U"岸岩帖罗帜岭凯败贩购图钓制知垂牧物乖刮秆和季委佳侍供使例版侄侦侧凭侨佩"
      U"货依的迫质欣征往爬彼径所舍金命斧爸采受乳贪念贫肤肺肢肿胀朋股肥服胁周昏"
      U"鱼兔狐忽狗备饰饱饲变京享店夜庙府底剂郊废净盲放刻育闸闹郑券卷单炒炊炕炎"
      U"炉沫浅法泄河沾泪油泊沿泡注泻泳泥沸波泼泽治怖性怕怜怪学宝宗定宜审宙官空"
      U"帘实试郎诗肩房诚衬衫视话诞询该详建肃录隶居届刷屈弦承孟孤陕降限妹姑姐姓"
      U"始驾参艰线练组细驶织终驻驼绍经贯奏春帮珍玻毒型挂封持项垮挎城挠政赴赵挡"
      U"挺括拴拾挑指垫挣挤拼挖按挥挪某甚革荐巷带草茧茶荒茫荡荣故胡南药标枯柄栋"
      U"相查柏柳柱柿栏树要咸威歪研砖厘厚砌砍面耐耍牵残殃轻鸦皆背战点临览竖省削"
      U"尝是盼眨哄显哑冒映星昨畏趴胃贵界虹虾蚁思蚂虽品咽骂哗咱响哈咬咳哪炭峡罚"
      U"贱贴骨钞钟钢钥钩卸缸拜看矩怎牲选适秒香种秋科重复竿段便俩贷顺修保促侮俭"
      U"俗俘信皇泉鬼侵追俊盾待律很须叙剑逃食盆胆胜胞胖脉勉狭狮独狡狱狠贸怨急饶"
      U"蚀饺饼弯将奖哀亭亮度迹庭疮疯疫疤姿亲音帝施闻阀阁差养美姜叛送类迷前首逆"
      U"总炼炸炮烂剃洁洪洒浇浊洞测洗活派洽染济洋洲浑浓津恒恢恰恼恨举觉宣室宫宪"
      U"突穿窃客冠语扁袄祖神祝误诱说诵垦退既屋昼费陡眉孩除险院娃姥姨姻娇怒架贺"
      U"盈勇怠柔垒绑绒结绕骄绘给络骆绝绞统耕耗艳泰珠班素蚕顽盏匪捞栽捕振载赶起"
      U"盐捎捏埋捉捆捐损都哲逝捡换挽热恐壶挨耻耽恭莲莫荷获晋恶真框桂档桐株桥桃"
      U"格校核样根索哥速逗栗配翅辱唇夏础破原套逐烈殊顾轿较顿毙致柴桌虑监紧党晒"
      U"眠晓鸭晃晌晕蚊哨哭恩唤啊唉罢峰圆贼贿钱钳钻铁铃铅缺氧特牺造乘敌秤租积秧"
      U"秩称秘透笔笑笋债借值倚倾倒倘俱倡候俯倍倦健臭射躬息徒徐舰舱般航途拿爹爱"
      U"颂翁脆脂胸胳脏胶脑狸狼逢留皱饿恋桨浆衰高席准座脊症病疾疼疲效离唐资凉站"
      U"剖竞部旁旅畜阅羞瓶拳粉料益兼烤烘烦烧烛烟递涛浙涝酒涉消浩海涂浴浮流润浪"
      U"浸涨烫涌悟悄悔悦害宽家宵宴宾窄容宰案请朗诸读扇袜袖袍被祥课谁调冤谅谈谊"
      U"剥恳展剧屑弱陵陶陷陪娱娘通能难预桑绢绣验继球理捧堵描域掩捷排掉堆推掀授"
      U"教掏掠培接控探据掘职基著勒黄萌萝菌菜萄菊萍菠营械梦梢梅检梳梯桶救副票戚"
      U"爽聋袭盛雪辅辆虚雀堂常匙晨睁眯眼悬野啦晚啄距跃略蛇累唱患唯崖崭崇圈铜铲"
      U"银甜梨犁移笨笼笛符第敏做袋悠偿偶偷您售停偏假得衔盘船斜盒鸽悉欲彩领脚脖"
      U"脸脱象够猜猪猎猫猛馅馆凑减毫麻痒痕廊康庸鹿盗章竟商族旋望率着盖粘粗粒断"
      U"剪兽清添淋淹渠渐混渔淘液淡深婆梁渗情惜惭悼惧惕惊惨惯寇寄宿窑密谋谎祸谜"
      U"逮敢屠弹随蛋隆隐婚婶颈绩绪续骑绳维绵绸绿琴斑替款堪搭塔越趁趋超提堤博揭"
      U"喜插揪搜煮援裁搁搂搅握揉斯期欺联散惹葬葛董葡敬葱落朝辜葵棒棋植森椅椒棵"
      U"棍棉棚棕惠惑逼厨厦硬确雁殖裂雄暂雅辈悲紫辉敞赏掌晴暑最量喷晶喇遇喊景践"
      U"跌跑遗蛙蛛蜓喝喂喘喉幅帽赌赔黑铸铺链销锁锄锅锈锋锐短智毯鹅剩稍程稀税筐"
      U"等筑策筛筒答筋筝傲傅牌堡集焦傍储奥街惩御循艇舒番释禽腊脾腔鲁猾猴然馋装"
      U"蛮就痛童阔善羡普粪尊道曾焰港湖渣湿温渴滑湾渡游滋溉愤慌惰愧愉慨割寒富窜"
      U"窝窗遍裕裤裙谢谣谦属屡强粥疏隔隙絮嫂登缎缓编骗缘瑞魂肆摄摸填搏塌鼓摆携"
      U"搬摇搞塘摊蒜勤鹊蓝墓幕蓬蓄蒙蒸献禁楚想槐榆楼概赖酬感碍碑碎碰碗碌雷零雾"
      U"雹输督龄鉴睛睡睬鄙愚暖盟歇暗照跨跳跪路跟遣蛾蜂嗓置罪罩错锡锣锤锦键锯矮"
      U"辞稠愁筹签简毁舅鼠催傻像躲微愈遥腰腥腹腾腿触解酱痰廉新韵意粮数煎塑慈煤"
      U"煌满漠源滤滥滔溪溜滚滨粱滩慎誉塞谨福群殿辟障嫌嫁叠缝缠静碧璃墙撇嘉摧截"
      U"誓境摘摔聚蔽慕暮蔑模榴榜榨歌遭酷酿酸磁愿需弊裳颗嗽蜻蜡蝇蜘赚锹锻舞稳算"
      U"箩管僚鼻魄貌膜膊膀鲜疑馒裹敲豪膏遮腐瘦辣竭端旗精歉熄熔漆漂漫滴演漏慢寨"
      U"赛察蜜谱嫩翠熊凳骡缩慧撕撒趣趟撑播撞撤增聪鞋蕉蔬横槽樱橡飘醋醉震霉瞒题"
      U"暴瞎影踢踏踩踪蝶蝴嘱墨镇靠稻黎稿稼箱箭篇僵躺僻德艘膝膛熟摩颜毅糊遵潜潮"
      U"懂额慰劈操燕薯薪薄颠橘整融醒餐嘴蹄器赠默镜赞篮邀衡膨雕磨凝辨辩糖糕燃澡"
      U"激懒壁避缴戴擦鞠藏霜霞瞧蹈螺穗繁辫赢糟糠燥臂翼骤鞭覆蹦镰翻鹰警攀蹲颤瓣"
      U"爆疆壤耀躁嚼嚷籍魔灌蠢霸露囊罐匕刁丐歹戈夭仑讥冗邓艾夯凸卢叭叽皿凹囚矢"
      U"乍尔冯玄邦迂邢芋芍吏夷吁吕吆屹廷迄臼仲伦伊肋旭匈凫妆亥汛讳讶讹讼诀弛阱"
      U"驮驯纫玖玛韧抠扼汞扳抡坎坞抑拟抒芙芜苇芥芯芭杖杉巫杈甫匣轩卤肖吱吠呕呐"
      U"吟呛吻吭邑囤吮岖牡佑佃伺囱肛肘甸狈鸠彤灸刨庇吝庐闰兑灼沐沛汰沥沦汹沧沪"
      U"忱诅诈罕屁坠妓姊妒纬玫卦坷坯拓坪坤拄拧拂拙拇拗茉昔苛苫苟苞茁苔枉枢枚枫"
      U"杭郁矾奈奄殴歧卓昙哎咕呵咙呻咒咆咖帕账贬贮氛秉岳侠侥侣侈卑刽刹肴觅忿瓮"
      U"肮肪狞庞疟疙疚卒氓炬沽沮泣泞泌沼怔怯宠宛衩祈诡帚屉弧弥陋陌函姆虱叁绅驹"
      U"绊绎契贰玷玲珊拭拷拱挟垢垛拯荆茸茬荚茵茴荞荠荤荧荔栈柑栅柠枷勃柬砂泵砚"
      U"鸥轴韭虐昧盹咧昵昭盅勋哆咪哟幽钙钝钠钦钧钮毡氢秕俏俄俐侯徊衍胚胧胎狰饵"
      U"峦奕咨飒闺闽籽娄烁炫洼柒涎洛恃恍恬恤宦诫诬祠诲屏屎逊陨姚娜蚤骇耘耙秦匿"
      U"埂捂捍袁捌挫挚捣捅埃耿聂荸莽莱莉莹莺梆栖桦栓桅桩贾酌砸砰砾殉逞哮唠哺剔"
      U"蚌蚜畔蚣蚪蚓哩圃鸯唁哼唆峭唧峻赂赃钾铆氨秫笆俺赁倔殷耸舀豺豹颁胯胰脐脓"
      U"逛卿鸵鸳馁凌凄衷郭斋疹紊瓷羔烙浦涡涣涤涧涕涩悍悯窍诺诽袒谆祟恕娩骏琐麸"
      U"琉琅措捺捶赦埠捻掐掂掖掷掸掺勘聊娶菱菲萎菩萤乾萧萨菇彬梗梧梭曹酝酗厢硅"
      U"硕奢盔匾颅彪眶晤曼晦冕啡畦趾啃蛆蚯蛉蛀唬唾啤啥啸崎逻崔崩婴赊铐铛铝铡铣"
      U"铭矫秸秽笙笤偎傀躯兜衅徘徙舶舷舵敛翎脯逸凰猖祭烹庶庵痊阎阐眷焊焕鸿涯淑"
      U"淌淮淆渊淫淳淤淀涮涵惦悴惋寂窒谍谐裆袱祷谒谓谚尉堕隅婉颇绰绷综绽缀巢琳"
      U"琢琼揍堰揩揽揖彭揣搀搓壹搔葫募蒋蒂韩棱椰焚椎棺榔椭粟棘酣酥硝硫颊雳翘凿"
      U"棠晰鼎喳遏晾畴跋跛蛔蜒蛤鹃喻啼喧嵌赋赎赐锉锌甥掰氮氯黍筏牍粤逾腌腋腕猩"
      U"猬惫敦痘痢痪竣翔奠遂焙滞湘渤渺溃溅湃愕惶寓窖窘雇谤犀隘媒媚婿缅缆缔缕骚"
      U"瑟鹉瑰搪聘斟靴靶蓖蒿蒲蓉楔椿楷榄楞楣酪碘硼碉辐辑频睹睦瞄嗜嗦暇畸跷跺蜈"
      U"蜗蜕蛹嗅嗡嗤署蜀幌锚锥锨锭锰稚颓筷魁衙腻腮腺鹏肄猿颖煞雏馍馏禀痹廓痴靖"
      U"誊漓溢溯溶滓溺寞窥窟寝褂裸谬媳嫉缚缤剿赘熬赫蔫摹蔓蔗蔼熙蔚兢榛榕酵碟碴"
      U"碱碳辕辖雌墅嘁踊蝉嘀幔镀舔熏箍箕箫舆僧孵瘩瘟彰粹漱漩漾慷寡寥谭褐褪隧嫡"
      U"缨撵撩撮撬擒墩撰鞍蕊蕴樊樟橄敷豌醇磕磅碾憋嘶嘲嘹蝠蝎蝌蝗蝙嘿幢镊镐稽篓"
      U"膘鲤鲫褒瘪瘤瘫凛澎潭潦澳潘澈澜澄憔懊憎翩褥谴鹤憨履嬉豫缭撼擂擅蕾薛薇擎"
      U"翰噩橱橙瓢蟥霍霎辙冀踱蹂蟆螃螟噪鹦黔穆篡篷篙篱儒膳鲸瘾瘸糙燎濒憾懈窿缰"
      U"壕藐檬檐檩檀礁磷了瞬瞳瞪曙蹋蟋蟀嚎赡镣魏簇儡徽爵朦臊鳄糜癌懦豁臀藕藤瞻"
      U"嚣鳍癞瀑襟璧戳攒孽蘑藻鳖蹭蹬簸簿蟹靡癣羹鬓攘蠕巍鳞糯譬霹躏髓蘸镶瓤矗");

  // 检查载入
  // check_u32string(cjkstr, 12, face_it->second);
  // check_u32string(cjkstr, 14, face_it->second);
  check_u32string(cjkstr, 16, face_it->second);
  // check_u32string(cjkstr, 18, face_it->second);
  // check_u32string(cjkstr, 20, face_it->second);
  // check_u32string(cjkstr, 22, face_it->second);
  check_u32string(cjkstr, 24, face_it->second);
  // check_u32string(cjkstr, 28, face_it->second);
  check_u32string(cjkstr, 36, face_it->second);
  check_u32string(cjkstr, 48, face_it->second);
}

// 获取空闲层
uint32_t FontRenderer::get_free_layer() {
  for (const auto& [layer, flag] : free_layers) {
    if (flag) {
      return layer;
    }
  }
  // 无空闲，新建层
  current_max_layer_index++;
  free_layers.try_emplace(current_max_layer_index, true);
  return current_max_layer_index;
}

// 检查载入字符串
void FontRenderer::check_u32string(const std::u32string& str,
                                   uint32_t font_size, FT_Face& face) {
  // 获取字符包
  // 尝试创建字体包映射
  auto& pack = font_packs_mapping.try_emplace(face->family_name)
                   .first->second.try_emplace(font_size)
                   .first->second;
  pack.font_size = font_size;

  // 获取空闲层
  int32_t free_layer_index = get_free_layer();

  bind();
  FT_Set_Pixel_Sizes(face, 0, font_size);
  for (const auto& c : str) {
    // 检查字符是否已缓存过
    auto cit = pack.character_set.find(c);
    if (cit == pack.character_set.end()) {
      // 未加载过,加载字符的字形
      if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
        XERROR("加载字符" + std::to_string(c) + "位图失败");
        continue;
      } else {
        CharacterGlyph character;
        FT_Bitmap& bitmap = face->glyph->bitmap;

        // 检查是否需要换行
        if (currentX + bitmap.width > layer_size) {
          currentX = 0;
          if (currentY + maxRowHeight < layer_size - 50) {
            currentY += maxRowHeight;
          } else {
            // 当前层满，获取其他空闲层
            free_layers[free_layer_index] = false;
            free_layer_index = get_free_layer();
            currentY = 0;
          }
          maxRowHeight = 0;
        }
        // 更新行高（考虑下伸部分）
        int glyphHeight = bitmap.rows;
        int glyphDescend =
            (face->glyph->metrics.height - face->glyph->metrics.horiBearingY) /
            64;
        maxRowHeight = std::max(maxRowHeight, glyphHeight + glyphDescend);

        // 上传字符位图到纹理

        GLCALL(cvs->glActiveTexture(GL_TEXTURE0 + 13));
        GLCALL(cvs->glBindTexture(GL_TEXTURE_2D_ARRAY, glyphs_texture_array));
        GLCALL(cvs->glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, currentX, currentY,
                                    free_layer_index, bitmap.width, bitmap.rows,
                                    1, GL_RED, GL_UNSIGNED_BYTE,
                                    bitmap.buffer));

        character.pos_in_atlas.setX(currentX);
        character.pos_in_atlas.setY(currentY);

        character.size.setWidth(bitmap.width);
        character.size.setHeight(bitmap.rows);

        character.glyph_id = free_layer_index;
        character.xadvance = face->glyph->advance.x;

        character.bearing.setX(face->glyph->metrics.horiBearingX / 64);
        character.bearing.setY(face->glyph->metrics.horiBearingY / 64);

        // 放入字符集
        pack.character_set.try_emplace(c, character);
        // 移动写入位置
        currentX += (bitmap.width + 2);
      }
    }
  }

  unbind();
}
// 获取字形
void FontRenderer::get_character_glyph(const std::string& font_family,
                                       uint32_t font_size,
                                       const char32_t character,
                                       CharacterGlyph& glyph) {
  bool query_failed{false};
  auto packs_it = font_packs_mapping.find(font_family);
  if (packs_it == font_packs_mapping.end()) {
    XCRITICAL("不存在字体[" + std::string(font_family) + "]");
    query_failed = true;
  }
  auto pack_it = packs_it->second.find(font_size);
  if (pack_it == packs_it->second.end()) {
    XCRITICAL("不存在[" + std::to_string(font_size) + "]号字体包");
    query_failed = true;
  }

  auto character_it = pack_it->second.character_set.find(character);
  if (character_it == pack_it->second.character_set.end()) {
    XCRITICAL("不存在[" +
              std::string(reinterpret_cast<const char*>(&character)) +
              "]字符数据");
    query_failed = true;
  }

  if (query_failed) {
    glyph.pos_in_atlas = {0, 0};
    glyph.size = {0, 0};
    glyph.glyph_id = 0;
  } else {
    glyph = character_it->second;
  }
}

// 同步数据
void FontRenderer::synchronize_data(InstanceDataType data_type,
                                    size_t instance_index, void* data) {
  switch (data_type) {
    case InstanceDataType::POSITION: {
      auto pos = static_cast<QVector2D*>(data);
      if (position_data.empty() || position_data.size() <= instance_index) {
        // XWARN("添加位置数据");
        position_data.push_back(*pos);
        synchronize_update_mark(instance_index);
      } else {
        if (*pos != position_data.at(instance_index)) {
          // 位置数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此实例位置数据
          position_data[instance_index] = *pos;
        }
      }
      break;
    }
    case InstanceDataType::ROTATION: {
      auto rotation = static_cast<float*>(data);
      if (rotation_data.empty() || rotation_data.size() <= instance_index) {
        // XWARN("添加角度数据");
        rotation_data.push_back(*rotation);
        synchronize_update_mark(instance_index);
      } else {
        if (*rotation != rotation_data.at(instance_index)) {
          // 角度数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此实例角度数据
          rotation_data[instance_index] = *rotation;
        }
      }
      break;
    }
    case InstanceDataType::FILL_COLOR: {
      auto fill_color = static_cast<QVector4D*>(data);
      if (fill_color_data.empty() || fill_color_data.size() <= instance_index) {
        // XWARN("添加填充颜色数据");
        fill_color_data.push_back(*fill_color);
        synchronize_update_mark(instance_index);
      } else {
        if (*fill_color != fill_color_data.at(instance_index)) {
          // 填充颜色数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此实例填充颜色数据
          fill_color_data[instance_index] = *fill_color;
        }
      }
      break;
    }
    case InstanceDataType::TEXT: {
      auto text = static_cast<Text*>(data);

      CharacterGlyph glyph;
      get_character_glyph(text->font_family, text->font_size, text->character,
                          glyph);

      // layer_index
      auto layer_index = glyph.glyph_id;
      if (layer_index_data.empty() ||
          layer_index_data.size() <= instance_index) {
        // XWARN("添加层数索引数据");
        layer_index_data.push_back(layer_index);
        synchronize_update_mark(instance_index);
      } else {
        if (layer_index != layer_index_data.at(instance_index)) {
          // 字符层数索引数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此实例层数索引数据
          layer_index_data[instance_index] = layer_index;
        }
      }

      // bearingY
      auto bearingY = glyph.bearing.y();
      if (bearing_y_data.empty() || bearing_y_data.size() <= instance_index) {
        // XWARN("添加y保留高度数据");
        bearing_y_data.push_back(bearingY);
        synchronize_update_mark(instance_index);
      } else {
        if (bearingY != bearing_y_data.at(instance_index)) {
          // 字符y保留高度数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此字符y保留高度数据
          bearing_y_data[instance_index] = bearingY;
        }
      }

      // size
      auto size = QVector2D(glyph.size.width(), glyph.size.height());
      if (size_data.empty() || size_data.size() <= instance_index) {
        // XWARN("添加尺寸数据");
        size_data.push_back(size);
        synchronize_update_mark(instance_index);
      } else {
        if (size != size_data.at(instance_index)) {
          // 尺寸数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此实例尺寸数据
          size_data[instance_index] = size;
        }
      }

      // uv
      CharacterUVSet uvset;
      uvset.p1.setX(float(glyph.pos_in_atlas.x()));
      uvset.p1.setY(float(glyph.pos_in_atlas.y()));
      uvset.p2.setX(float(glyph.pos_in_atlas.x() + glyph.size.width()));
      uvset.p2.setY(float(glyph.pos_in_atlas.y()));
      uvset.p3.setX(float(glyph.pos_in_atlas.x() + glyph.size.width()));
      uvset.p3.setY(float(glyph.pos_in_atlas.y() + glyph.size.height()));
      uvset.p4.setX(float(glyph.pos_in_atlas.x()));
      uvset.p4.setY(float(glyph.pos_in_atlas.y() + glyph.size.height()));

      if (uvset_data.empty() || uvset_data.size() <= instance_index) {
        // XWARN("同步字符uv集数据");
        uvset_data.push_back(uvset);
        synchronize_update_mark(instance_index);
      } else {
        if (uvset != uvset_data.at(instance_index)) {
          // 字符uv集数据发生变化
          // 同步更新位置标记
          synchronize_update_mark(instance_index);
          // 更新此实例字符uv集数据
          uvset_data[instance_index] = uvset;
        }
      }
      break;
    }
    default:
      break;
  }
}

// 同步更新位置标记
void FontRenderer::synchronize_update_mark(size_t instance_index) {
  // 同步更新标记
  if (update_list.empty()) {
    // 空,创建新的更新标记
    update_list.emplace_back(instance_index, 1);
  } else {
    // 非空
    // 确保不是同一对象的不同属性变化
    if (update_list.back().first != instance_index) {
      // 检查是否连续上一更新标记
      if (update_list.back().first + update_list.back().second ==
          instance_index) {
        // 连续--增加更新数量
        update_list.back().second++;
      } else {
        if (update_list.back().first + update_list.back().second - 1 !=
            instance_index) {
          // 确保未记录过
          // 不连续,创建新的更新标记
          update_list.emplace_back(instance_index, 1);
        }
      }
    }
  }
}

// 重置更新内容
void FontRenderer::reset_update() { update_list.clear(); }
// 绑定渲染器
void FontRenderer::bind() {
  AbstractRenderer::bind();
  if (need_update_sampler_location) {
    // 需要更新
    shader->set_sampler("glyph_atlas_array", 13);
  }
}

// 更新gpu数据
void FontRenderer::update_gpu_memory() {
  // 绑定实例缓冲区
  GLCALL(cvs->glBindBuffer(GL_ARRAY_BUFFER, fInstanceVBO));

  for (const auto& [instance_start_index, instance_count] : update_list) {
    uint32_t float_stride = (2 + 2 + 1 + 1 + 1 + 4 + 2 * 4);
    // 构建内存块
    std::vector<float> memory_block(instance_count * float_stride);
    for (int i = instance_start_index;
         i < instance_start_index + instance_count; i++) {
      //// 图形位置数据
      memory_block[(i - instance_start_index) * float_stride] =
          position_data[i].x();
      memory_block[(i - instance_start_index) * float_stride + 1] =
          position_data[i].y();
      //// 图形尺寸
      memory_block[(i - instance_start_index) * float_stride + 2] =
          size_data[i].x();
      memory_block[(i - instance_start_index) * float_stride + 3] =
          size_data[i].y();
      //// 旋转角度
      memory_block[(i - instance_start_index) * float_stride + 4] =
          rotation_data[i];
      //// 纹理集层数索引
      memory_block[(i - instance_start_index) * float_stride + 5] =
          layer_index_data[i];
      //// y保留高度
      memory_block[(i - instance_start_index) * float_stride + 6] =
          bearing_y_data[i];
      //// 填充颜色
      memory_block[(i - instance_start_index) * float_stride + 7] =
          fill_color_data[i].x();
      memory_block[(i - instance_start_index) * float_stride + 8] =
          fill_color_data[i].y();
      memory_block[(i - instance_start_index) * float_stride + 9] =
          fill_color_data[i].z();
      memory_block[(i - instance_start_index) * float_stride + 10] =
          fill_color_data[i].w();
      // 纹理uv集
      const auto& uvset = uvset_data[i];
      memory_block[(i - instance_start_index) * float_stride + 11] =
          uvset.p1.x();
      memory_block[(i - instance_start_index) * float_stride + 12] =
          uvset.p1.y();

      memory_block[(i - instance_start_index) * float_stride + 13] =
          uvset.p2.x();
      memory_block[(i - instance_start_index) * float_stride + 14] =
          uvset.p2.y();

      memory_block[(i - instance_start_index) * float_stride + 15] =
          uvset.p3.x();
      memory_block[(i - instance_start_index) * float_stride + 16] =
          uvset.p3.y();

      memory_block[(i - instance_start_index) * float_stride + 17] =
          uvset.p4.x();
      memory_block[(i - instance_start_index) * float_stride + 18] =
          uvset.p4.y();
    }
    // 上传内存块到显存
    GLCALL(cvs->glBufferSubData(
        GL_ARRAY_BUFFER, (instance_start_index * float_stride * sizeof(float)),
        memory_block.size() * sizeof(float), memory_block.data()));
  }
}
