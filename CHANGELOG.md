## EXE v3.6.76 / DLL v3.5.50 - 2026年06月21日 04:10
### Changes
- `qml__injector_exe/main.qml`: 删除注入按钮上方的皮肤入口按钮、皮肤页和只服务该入口的全屏爆闪特效，保留主 DLL 注入按钮、注入进度条和模型页 3D 渲染。
- `src__injector_exe/backend.h`, `src__injector_exe/backend.cpp`: 删除皮肤预览/普通皮肤待命对 QML 暴露的属性和方法，断开登录验证后自动启动 `NoteBotClassicSkinPatcher.exe` 的 helper 链。
- `resources__injector_exe/app.qrc`: 移除皮肤预览 JSON/PNG 和 `NoteBotClassicSkinPatcher.exe` 的资源打包入口。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.76 / 30676`，准备重新构建发布 EXE。

### Reason
- 本轮目标是彻底删除皮肤注入/皮肤待命功能，同时严格保留主 DLL 注入链、进程扫描、注入进度条和模型页 3D 渲染，避免误伤真正的业务 DLL 注入。

## EXE v3.6.75 / DLL v3.5.50 - 2026年06月21日 03:31
### Changes
- `qml__injector_exe/main.qml`: 修复 `PROCESSES` 进程列表里 EXE 名称和 PID 之间的乱码分隔符，改为稳定的 `/ PID` 显示。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.75 / 30675`，准备重新构建发布 EXE。

### Reason
- 进程列表属于启动后第一屏高频区域，乱码会直接影响目标进程选择；本轮只修这一处可见文本，不扩大到其它面板文案。

## EXE v3.6.74 / DLL v3.5.50 - 2026年06月21日 03:11
### Changes
- `qml__injector_exe/main.qml`: 修复左侧 `skinPreviewBtn` 按钮被乱码污染的中文文案，恢复为可读的两行皮肤入口按钮。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.74 / 30674`，准备重新构建发布 EXE。

### Reason
- 该按钮是当前 UI 中最明显的全乱码按钮，文字已经无法正常阅读，但按钮结构和点击逻辑仍然有效；本轮只做最小文案修复，不扩大到其它皮肤页历史乱码。

## EXE v3.6.73 / DLL v3.5.50 - 2026年06月16日 08:56
### Changes
- `qml__injector_exe/main.qml`: 批量修复被前面脚本写坏的字符串字面量，恢复 QML 可解析状态。
- `qml__injector_exe/main.qml`: 删除后半段已经坏掉的旧隐藏界面残骸，只保留当前主界面所需结构，避免继续触发语法错误。
- `qml__injector_exe/main.qml`: 把 `SKIN` 面板临时收敛成最小稳定布局，优先保证程序能正常启动。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.73 / 30673`，并重新完成 EXE 正式构建，`dist__release_artifacts\\NoteBotInjector.exe` 已更新。

### Reason
- 当前最高优先级不是继续做皮肤页细节，而是先把已经被多轮手改和乱码写坏的 `main.qml` 拉回“可启动、可进界面”的状态。
- 这次先止血，把最容易反复炸启动的坏字符串和废旧残骸一起清掉，后续再单独回头修 `SKIN` 具体显示内容。
## EXE v3.6.67 / DLL v3.5.50 - 2026年06月16日 07:40
### Changes
- `qml__injector_exe/main.qml`: 把 `skinPanelRestored` 从 `modelPanelRestored` 里拆出来，恢复成同级面板，避免切到 `skin` 时被外层 `models` 可见性一起隐藏。
- `qml__injector_exe/main.qml`: 顺手修正 `SmallButton` 与 `splash` 交界处被前面手改带坏的两处多余 / 缺失闭合，恢复整份 QML 的稳定结构。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.67 / 30667`，并重新完成 EXE 正式构建，`dist__release_artifacts\\NoteBotInjector.exe` 已更新到新版本。

### Reason
- 这次空白不是皮肤 PNG、骨架 JSON 或 3D 预览本身没加载，而是 `SKIN` 面板被错误塞进了 `MODELS` 面板内部。切换到 `skin` 时，父层先被设成不可见，子层再怎么 `visible: true` 也画不出来，所以最终只剩一块空框。
- 先把面板层级拨正，`SKIN` 内容区才能重新参与渲染，后面再谈具体预览细节。
## EXE v3.6.65 / DLL v3.5.50 - 2026年06月16日 07:27
### Changes
- `qml__injector_exe/main.qml`: 修复启动页日志区域 `Connections` 少一个闭合括号的问题。
- `qml__injector_exe/main.qml`: 修复模型空状态文案区域 `Text` 少一个闭合括号的问题，并让整份 QML 大括号重新配平，恢复根界面可加载状态。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.65 / 30665`，并重新完成 EXE 正式构建，`dist__release_artifacts\NoteBotInjector.exe` 已更新到新版本。

### Reason
- 这次不是皮肤预览渲染算法先坏，而是 `main.qml` 在前面多轮手改后丢了两处结构闭合，导致 QML 引擎连根窗口都建不起来，所以一启动就直接弹“QML 界面加载失败”。
- 先把语法骨架救活，才能继续处理后面的 SKIN 布局和预览显示问题。
# NoteBot Injector 开发日记`r`n## EXE v3.6.47 / DLL v3.5.50 - 2026年06月15日 16:04
### Changes
- `src__injector_exe/modelpreviewitem.cpp`, `src__injector_exe/modelpreviewitem.h`: 让模型预览在加载几何时读取 `description.texture_width / texture_height`，并把 box UV 的步长按模型纹理尺寸自动缩放。这样 128 经典皮肤不再继续沿用 64 的固定 UV 尺寸，预览错乱问题得到修正。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.47 / 30647`，并重新完成 EXE 正式构建，`dist__release_artifacts\\NoteBotInjector.exe` 已更新到新版本。

### Reason
- 之前的问题不在“有没有切到 128 模板”，而在于模板切换后箱体 UV 仍旧按 64 的固定步长去铺，导致 128 图里本该成倍放大的面片直接错位。现在把 UV 步长绑到模型自身的 `texture_width / texture_height`，预览就能按模板尺寸自己对齐。# NoteBot Injector 开发日记
## EXE v3.6.45 / DLL v3.5.50 - 2026年06月15日 15:54
### Changes
- `qml__injector_exe/main.qml`: 把左侧此前被整块隐藏的皮肤入口 `skinPreviewBtn` 重新开放，恢复可见、可点击和布局高度。现在用户可以再次从主界面直接切进 `skin` 面板，不用再靠临时状态或隐藏入口。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.45 / 30645`，并重新完成 EXE 正式构建，`dist__release_artifacts\\NoteBotInjector.exe` 已更新到新版本。

### Reason
- 当前皮肤页后端链路、预览链路和待命链路都还活着，真正被关掉的是“通往皮肤页的门”。这次先做最小恢复，只把入口重新打开，让现有功能重新能被用户点到，不在这一轮顺手改动皮肤页内部逻辑。
- 本轮第一次尝试曾把版本推到 `3.6.44`，但构建时 `NoteBotInjector.exe` 输出文件被占用，导致新版本没有真正落盘。按版本铁律，第二次正式重构建前继续前推到 `3.6.45`，最终以 `3.6.45` 作为实际可用产物落盘。

## EXE v3.6.41 / DLL v3.5.50 - 2026年06月11日 22:31
### Changes
- `src__injector_exe/backend.cpp`, `src__injector_exe/modelpreviewitem.cpp`: 把普通皮肤的本地文件路径统一改成 `file:///...` URL 供 QML `Image` 使用，同时让 3D 预览控件把本地文件 URL 还原回真实磁盘路径读取。这样自选 PNG 不会出现“atlas 不显示 / 3D 预览吃不到本地路径”的分裂问题。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.41 / 30641`，并重新完成 EXE 构建。

### Reason
- 上一版虽然把普通皮肤待命链接上了，但本地图片路径同时要喂给 QML `Image` 和原生 `ModelPreview`。这两边对路径格式的口味不一样：一个喜欢 URL，一个喜欢真实本地路径。这里不收口的话，界面上就会出现一边能看见、一边读不到的半残状态，所以这版专门把路径语义统一掉。

## EXE v3.6.40 / DLL v3.5.50 - 2026年06月11日 22:25
### Changes
- `src__injector_exe/backend.h`, `src__injector_exe/backend.cpp`: 给皮肤页接上真正的“普通皮肤待命链”。现在会保存所选 PNG、校验 `64x64 / 128x128`、在授权通过后自动拉起内置 `NoteBotClassicSkinPatcher.exe`，并把 helper 的启动、待命、退出和错误日志回流到主日志。
- `qml__injector_exe/main.qml`: 皮肤页不再只是样例壳子。左边 3D 预览和中间 atlas 都会跟着当前选中的普通皮肤 PNG 更新，右侧状态 badge 会显示 `已关闭 / 待验证 / 待启动 / 待命中`，并新增了真实的“选择皮肤”入口。
- `resources__injector_exe/classic_preview_64.json`, `resources__injector_exe/classic_preview_128.json`, `resources__injector_exe/app.qrc`: 新增默认经典方块人骨架预览资源，按 64 和 128 两种普通皮肤尺寸切换，不再拿自定义样例模型硬充默认几何。
- `resources__injector_exe/embedded/NoteBotClassicSkinPatcher.exe`: 把现成 `classic_login_skin_patcher.py` 打成 helper exe 并嵌进主程序资源，主 EXE 会按需释放到本地后启动。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.40 / 30640`，并重新完成 EXE 构建。

### Reason
- 这轮先不碰自定义几何、不碰网页模型返回，只解决“普通皮肤贴图要能急用”这件事。核心做法是把早期皮肤 patch 保持成独立 helper，但把入口、状态、文件选择和日志全部收回到现有 EXE 里，这样用户只在一个程序里操作，逻辑也不会和现在的 DLL 注入链搅成一锅。

## EXE v3.6.39 / DLL v3.5.50 - 2026年06月10日 19:22
### Changes
- `src__injector_exe/modelcatalogmodel.h`, `src__injector_exe/modelcatalogmodel.cpp`, `src__injector_exe/backend.cpp`: 把模型卡右下角状态从单一 `state` 文本角色拆成 `stateCode + stateLabel`。后端继续负责给出 `active / owned / unowned` 三态和对应文案，激活模型时也同步按这套新字段刷新。
- `qml__injector_exe/main.qml`: 模型卡 badge 不再读取会和 QML 内建属性撞名的 `state`，改成读取 `stateCode` 和 `stateLabel`，颜色判断和文案显示都走专用字段。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.39 / 30639`，并重新完成 EXE 构建。

### Reason
- 这次空占位不是服务端没给状态，也不是模型列表没拉到，而是前端把列表角色命名成了 `state`，正好撞上 QML 控件自己的 `state` 属性。结果右下角 badge 读到的是控件默认空串，所以看起来像“有框没字”。把逻辑状态和显示文案拆开之后，这块就彻底不再吃这种命名冲突了。

## EXE v3.6.38 / DLL v3.5.50 - 2026年06月10日 19:07
### Changes
- `qml__injector_exe/main.qml`: 把满屏愤怒特效继续往上顶了一档。现在这层不只是红闪和飞字，还会跟着动画做整层抖动、脉冲白闪、外扩怒气环，以及更多喷射文字和第二层中心爆字。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.38 / 30638`，并重新完成 EXE 构建。

### Reason
- 用户反馈“不够劲爆”，所以这轮不是修 bug，而是单纯把反馈强度继续往上堆，做成更吵、更抖、更像整个界面一起发疯的效果。

## EXE v3.6.37 / DLL v3.5.50 - 2026年06月10日 19:04
### Changes
- `qml__injector_exe/main.qml`: 修正“点按钮却看不到愤怒特效”的挂载位置错误。上一轮那层全屏特效其实写进了旧的 `visible: false` 隐藏 UI 分支里，所以点击时根本不会显示；这一版把整套特效层挪回当前主界面的有效层级。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.37 / 30637`，并重新完成 EXE 构建。

### Reason
- 这次不是重新设计特效，而是把之前那口锅认出来并端掉。问题不在“效果太弱”，而在于特效层被塞进了隐藏分支，等于写了也永远看不见。

## EXE v3.6.36 / DLL v3.5.50 - 2026年06月10日 18:59
### Changes
- `qml__injector_exe/main.qml`: 给左侧皮肤按钮接上全屏愤怒特效。现在点它不只是切到 `skin` 页，还会同时触发一层覆盖全窗的红闪、斜向怒气条、满屏飞字和中心爆字。
- `qml__injector_exe/main.qml`: 在主界面状态层新增 `angerBurstLevel / angerBurstSeed / angerBurstActive` 和一段独立的特效动画控制，整套爆发效果会自动衰减收尾，不会一直糊在屏幕上。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.36 / 30636`，并重新完成 EXE 构建。

### Reason
- 你要的是“点一下就炸出满屏愤怒特效”，不是小打小闹的按钮高亮。这轮就是把那个吐槽按钮彻底做成会发癫的入口，让点击反馈足够解气。

## EXE v3.6.35 / DLL v3.5.50 - 2026年06月10日 18:56
### Changes
- `qml__injector_exe/main.qml`: 左侧皮肤按钮改成真正的多行吐槽块，不再只塞一行短文案。按钮高度从 `34` 提到 `122`，整段话会在按钮内完整换行显示。
- `qml__injector_exe/main.qml`: 去掉这块文案的单行省略语义，改成 `Wrap + 居中 + 固定行高`，保证文本不会横向溢出，也不会被强行裁成一条。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.35 / 30635`，并重新完成 EXE 构建。

### Reason
- 这一轮很纯粹，就是把这个按钮的情绪值拉满，而且确保整段文案老老实实塞在按钮里，不再出现放不下、挤成一行或者超出显示范围的问题。

## EXE v3.6.34 / DLL v3.5.50 - 2026年06月10日 18:51
### Changes
- `qml__injector_exe/main.qml`: 把 `skin` 面板重排成三栏。左侧只保留压缩后的 `3D LIVE` 实时立体检视，中间独立显示 `SKIN ATLAS` 贴图预览，右侧新增 `PREVIEW FILE` 文件接口区，不再让 3D 和贴图挤在同一块区域里。
- `qml__injector_exe/main.qml`: 右侧文件接口区补上当前预览文件信息、默认骨架文件、经典皮肤格式说明和 `64x64 / 128x128` 兼容尺寸提示，并预留后续云端/本地皮肤文件接入口。
- `src__injector_exe/backend.h`, `src__injector_exe/backend.cpp`, `qml__injector_exe/main.qml`: 新增真正可切换的 `skinPreviewEnabled` 状态，并把它接到右侧 `启用皮肤 / 关闭皮肤` 控件。关闭后会立即压暗左侧 3D 和中间 atlas，日志里也会记一条皮肤开关状态。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.34 / 30634`，并重新完成 EXE 构建。

### Reason
- 这轮不是再加内容，而是把皮肤页结构收顺。你要的是左边小一点的实时看模、中间专门看贴图、右边专门放接口和启停开关，这样后面无论接真实皮肤文件、激活状态还是云端资源，落点都很清楚。

## EXE v3.6.33 / DLL v3.5.50 - 2026年06月10日 18:41
### Changes
- `qml__injector_exe/main.qml`: 在左侧注入按钮上方新增一个全宽长按钮，文案固定为 `鸡巴的Naive.天天催更要皮肤`，交互风格沿用现有 `日志 / 模型` 切换按钮的高亮与边框语义。
- `qml__injector_exe/main.qml`: 右侧详情区新增 `skin` 面板。现在点这个按钮后，会切到独立的皮肤预览页，标题显示为 `SKIN`，不再混进日志或模型列表里。
- `qml__injector_exe/main.qml`: 皮肤预览页先接了一版本地方块人预览接口，直接复用现有 `ModelPreview` GPU 渲染链，加载内置 `ysm_sample_main.json + ysm_sample_skin.png`，能看到一只持续旋转的默认骨架预览；右侧同时显示皮肤 atlas 平铺图，作为后续云端皮肤入口的占位面。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.33 / 30633`，并重新完成 EXE 构建。

### Reason
- 这一轮先把“皮肤预览”这条入口单独立起来，位置放在你点注入前最顺手的区域，后面要接真实皮肤资源、绑定皮肤或云端下载时，就直接往这个面板里接，不用再去挤模型页。

## EXE v3.6.32 / DLL v3.5.50 - 2026年06月10日 06:57
### Changes
- `src__injector_exe/backend.cpp`, `src__injector_exe/backend.h`, `src__injector_exe/modelcatalogmodel.*`: 客户端模型列表改成三态驱动。`model_entitlements_v1` 现在可以区分 `owned/unowned`，本地会把模型卡渲染成 `未拥有 / 已拥有 / 已激活`，并把“已激活模型 ID”按密钥写进 `QSettings`。
- `qml__injector_exe/main.qml`: 模型卡支持两种预览路径。已拥有模型继续走本地 `geometry + texture` 的 3D 预览；未拥有模型只显示封面图。点击已拥有模型卡会点亮整卡并切成 `已激活` 状态。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.32 / 30632`，并重新完成 EXE 构建。

### Reason
- 这一轮目标不是继续修渲染，而是把模型授权形态本身搭完整：所有有密钥的用户都能看到“全员可展示模型”，但未拥有时客户端拿不到完整模型资源，只能看封面；已拥有模型才会拉完整 geometry/texture；已激活则只是本地占位状态，先把 UI 和数据流跑通。

## EXE v3.6.31 / DLL v3.5.50 - 2026年06月10日 06:11
### Changes
- `qml__injector_exe/main.qml`: 删掉左侧侧栏里的 `TARGET` 标题和 `Minecraft.Windows.exe` 手动输入框，不再允许在 UI 里手动填写扫描目标名。
- `qml__injector_exe/main.qml`: 回退上一轮误隐藏的 `PROCESSES` 进程选择列表和其布局占位，保留原来的进程选择功能。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.31 / 30631`，并重新完成 EXE 构建。

### Reason
- 用户要删的是 `TARGET` 输入框本身，不是进程选择列表。此版把误删的内容完整恢复，只移除手动填写目标名那块 UI，同时让密钥区自然上移。

## EXE v3.6.30 / DLL v3.5.50 - 2026年06月10日 05:42
### Changes
- `qml__injector_exe/main.qml`: 默认主界面右侧内容区不再显示 `PROCESSES` 列表块，相关容器直接隐藏并把高度压到 0。
- `qml__injector_exe/main.qml`: 同步移除进程列表下方那块纯撑开用的空白占位，让下面的授权/日志主面板直接上提。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.30 / 30630`，并重新完成 EXE 构建。

### Reason
- 用户不再需要 UI 里那块可见的进程名/进程列表区域，只希望保留更简洁的界面，把密钥区域整体往上提。此版只做界面收口，不改注入逻辑和模型渲染链。

## EXE v3.6.29 / DLL v3.5.50 - 2026年06月10日 00:40
### Changes
- `resources__injector_exe/shaders/modelpreview.vert`: 顶点 shader 新增 `depthbias` 顶点属性，并在 clip-space `z` 上减去这份极小偏移。
- `src__injector_exe/modelpreviewitem.cpp`: GPU 顶点结构扩成 `position + uv + depthBias`，并在构建三角形时按“离模型包围盒中心越远，偏移越大”的规则生成很小的壳层深度优先级。
- `src__injector_exe/modelpreviewitem.cpp`: RHI 顶点布局同步扩到 6 个 `float`，把新的 `depthbias` 喂进顶点 shader。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.29 / 30629`，重新编译顶点 shader 并完成 EXE 构建。

### Reason
- 用户描述的是非常具体的“外壳包住内层，但内层还是顶出来”的模型结构。这已经不是整体渲染或 alpha 混合问题，更像近距离重叠层之间缺少稳定的壳层优先级。此版不再改全局矩阵，而是只给更外层的三角形一个极小的深度前推，让包裹层比内层更稳定地赢下深度测试。

## EXE v3.6.28 / DLL v3.5.50 - 2026年06月10日 00:20
### Changes
- `src__injector_exe/updater.cpp`: 更新清单 TCP 请求新增失败后强制直连重试。首次请求失败或被系统代理卡住时，会重新创建 `QTcpSocket` 并设置 `QNetworkProxy::NoProxy` 再请求一次。
- `src__injector_exe/updater.cpp`: 修正更新清单连接失败 / 等待响应超时日志为可读中文，并新增“切换强制直连模式重试”“强制直连模式已获取更新清单”两条诊断日志。
- `CMakeLists.txt`, `src__injector_exe/backend.cpp`, `resources__injector_exe/app_icon.rc`: 主程序版本前推到 `3.6.28 / 30628`，并重新完成 EXE 构建。

### Reason
- 干净环境只拿到单 EXE 时，需要先通过 `update_manifest_v3` 拉取 `%LOCALAPPDATA%\NoteBotInjector\NoteBotAuth.dll`。用户实测系统代理开启后，首次私有 TCP 更新清单请求会长时间等待并超时，随后因为本地没有 Auth DLL 而弹出“DLL 缺失”。此版本不改服务端、不加 HTTPS 备用通道，只在客户端内对清单请求做一次强制直连重试，绕开常见系统代理对私有 TCP 端口的干扰。
## EXE v3.6.27 / DLL v3.5.50 - 2026年06月09日 23:50
### Changes
- `src__injector_exe/modelpreviewitem.cpp`: GPU 投影矩阵的 z 缩放改为负向，让旋转后的近处表面在当前 `Less` 深度测试下得到更小深度值，优先压住后方内层。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.27 / 30627`，并重新完成 EXE 构建。
### Reason
- 用户反馈 cutout / 关闭 blending 后，外层包住内层的问题仍然存在。这说明内层不是靠 alpha 混合透出来，而是 GPU 深度方向仍然可能让远处内层赢过近处外层。此版只翻转深度映射方向，不改 UV / alpha / 模型几何。

## EXE v3.6.26 / DLL v3.5.50 - 2026年06月09日 23:26
### Changes
- `resources__injector_exe/shaders/modelpreview.frag`: 模型预览 fragment shader 改成 cutout 语义。透明度低于阈值的像素直接丢弃，通过阈值的像素强制按不透明输出，避免半透明颜色把内层模型混出来。
- `src__injector_exe/modelpreviewitem.cpp`: 关闭模型管线自身颜色 blending，保持 alpha test 后的不透明像素正常写深度，用外层模型硬遮住内层模型。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.26 / 30626`，重新编译 shader 并完成 EXE 构建。
### Reason
- 用户反馈当前渲染已经丝滑、不再抽搐，但本应被外层包住的重叠模型仍会显示内层。根因是原 shader 只在 `alpha < 0.01` 时丢弃，其余 alpha 原样输出，同时 pipeline 开着 blending；这会让半透明外层和边缘像素把内层模型混出来。Minecraft 皮肤预览这里更适合 cutout：透明就丢，不透明就盖住。

## EXE v3.6.25 / DLL v3.5.50 - 2026年06月09日 23:20
### Changes
- `src__injector_exe/modelpreviewitem.cpp`: 撤回 `v3.6.24` 中对普通模型启用 `Back` 背面剔除的改动，恢复 `CullMode::None`。保留上一版的深度平移和去掉 `depthBias` 混入坐标的修正。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.25 / 30625`，并重新完成 EXE 构建。
### Reason
- 用户反馈 `v3.6.24` 虽然不再闪烁，但所有角度都出现大量面丢失。根因是当前模型三角形 winding 和 QRhi/D3D 后端的正反面约定并不稳定，直接启用 `Back` culling 会把大量正常可见面误判为背面剔掉。此版只撤回错误剔除，不改 UV / 贴图解析 / GPU 矩阵主链。

## EXE v3.6.24 / DLL v3.5.50 - 2026年06月09日 23:09
### Changes
- `src__injector_exe/modelpreviewitem.cpp`: 修正 GPU 矩阵版模型预览的深度空间。模型顶点不再把旧的排序 `depthBias` 混进本地坐标，避免旋转后把内部面错误顶到外面。
- `src__injector_exe/modelpreviewitem.cpp`: GPU 投影矩阵把模型深度明确平移到可见范围中间，并收窄 z 缩放，避免近裁剪/深度边界造成“模型像被横截面切开”的观感。
- `src__injector_exe/modelpreviewitem.cpp`: 普通立方体渲染恢复背面剔除语义，薄片模型仍通过原来的双面三角形逻辑显示，减少内侧面参与深度竞争。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.24 / 30624`，并重新完成 EXE 构建。
### Reason
- `v3.6.23` 把旋转从 CPU 几何重建挪到 GPU 矩阵后，性能方向是对的，但深度语义没有完整复刻旧 CPU 投影。原本用于稳定排序的微小 `depthBias` 被带进了 GPU 顶点坐标，同时深度值围绕 0 摆动，叠加关闭背面剔除后，旋转时容易看到模型内部面，视觉上就像横截面被打开。此版只修遮挡和深度，不改 UV / 贴图解析。

## EXE v3.6.23 / DLL v3.5.50 - 2026年06月09日 09:59
### Changes
- `src__injector_exe/modelpreviewitem.cpp`, `src__injector_exe/modelpreviewitem.h`: 模型预览旋转正式改成 GPU 矩阵路径。模型 JSON / UV / 贴图解析后只上传一次本地空间三角形，`yaw / pitch / zoom` 不再触发三角形重建，只更新 uniform matrix。
- `resources__injector_exe/shaders/modelpreview.vert`, `resources__injector_exe/shaders/modelpreview.frag`: 重新编译模型预览 shader，顶点 shader 接收 `mvp` 矩阵，fragment shader 保留透明裁切和原有纹理采样语义。
- `qml__injector_exe/main.qml`: 恢复模型页所有卡片默认持续自动旋转，滚轮滑动页面时不再把模型动画停掉。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.23 / 30623`，并重新完成 EXE 构建，产物已同步到 `build__injector_exe_cache\NoteBotInjector.exe` 与 `dist__release_artifacts\NoteBotInjector.exe`。
### Reason
- 用户反馈模型一多就卡成 PPT，并且滚轮滑动时也要求所有模型继续流畅旋转。继续限制“只有悬停卡旋转”不符合需求。根因是旧 GPU 版只是把最终绘制交给显卡，但每次 `setYaw / setPitch / setZoom` 仍把几何缓存打脏，导致每张模型卡每 13ms 都重跑骨骼矩阵、三角形生成、排序和 GPU 顶点数组转换。此版把模型姿态变化收口成 GPU 侧 64 字节矩阵更新，CPU 只在模型/贴图资源变化时重建几何。

## EXE v3.6.22 / DLL v3.5.50 - 2026年06月09日 09:58
### Changes
- `qml__injector_exe/main.qml`: 模型列表改为只有鼠标悬停的模型卡才实时自转/跟随，非悬停模型卡保持静态预览角度，不再所有卡同时 13ms 刷新。
- `src__injector_exe/modelpreviewitem.cpp`: `autoRotate=false` 时同时清理 hover tracking、停止自转 timer 和恢复 timer，确保非实时卡不继续在后台跑几何重算。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.22 / 30622`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。
### Reason
- 用户反馈拖动时仍能明显感到卡顿。继续定位后，主要开销不是单个模型“画不动”，而是列表里每张模型卡都有独立 13ms timer，每次 yaw 改变都会重建三角形顶点。模型越多，CPU 同时重算越多。此版把实时渲染限制到悬停卡，列表浏览和窗口拖动时负载会低很多。

## EXE v3.6.21 / DLL v3.5.50 - 2026年06月09日 09:48
### Changes
- `qml__injector_exe/main.qml`: 移除模型页滚动期间暂停模型自转的条件。现在只要处于模型页，模型预览就保持自动旋转，鼠标滚轮滚动列表不会再让模型停住。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.21 / 30621`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。
### Reason
- 用户反馈鼠标滚轮滑动模型页时模型会暂停旋转，不希望这个行为存在。根因是 `previewPausedForScroll` 把 `GridView.moving / dragging / flicking` 都当作暂停条件，滚轮滚动也会命中。此版移除该暂停链。

## EXE v3.6.20 / DLL v3.5.50 - 2026年06月09日 09:42
### Changes
- `src__injector_exe/modelpreviewitem.cpp`, `src__injector_exe/modelpreviewitem.h`: 模型预览深度遮挡改成更稳定的判定。GPU pipeline 的深度比较从 `LessOrEqual` 改为严格 `Less`，避免近乎同深度的外层/内层面互相通过。
- `src__injector_exe/modelpreviewitem.cpp`: 三角形排序改为 `stable_sort`，并为每个三角形加入极小、稳定的 `depthBias`。这样当外层衣服/装甲与内层身体非常接近时，不会因为深度值抖动或同深度排序不稳定导致贴图闪烁。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.20 / 30620`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。
### Reason
- 用户反馈某些模型里外层材质本应 100% 包住内层，但预览里会和内层颜色叠在一起闪。根因方向是深度冲突：外层/内层面距离过近，GPU 深度缓冲在旋转时会出现抢深度。此版先做低风险稳定化，不改模型解析和 UV 映射。

## EXE v3.6.19 / DLL v3.5.50 - 2026年06月09日 09:32
### Changes
- `qml__injector_exe/main.qml`: 模型卡资源摘要行从名称/描述 `Column` 中拆出，改为固定锚在信息区底部。这样模型没有描述时，`bones / cubes` 不会贴紧模型名称，而是继续保持原来的低位。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.19 / 30619`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。
### Reason
- 用户反馈没有描述的模型卡会让资源摘要贴到名称下面，视觉上像信息挤在一起。这版把资源摘要固定在底部，描述有无都不改变它的位置。

## EXE v3.6.18 / DLL v3.5.50 - 2026年06月09日 09:25
### Changes
- `qml__injector_exe/main.qml`: 模型卡底部信息重新排版。左侧改成名称、描述、资源摘要三层结构，服务端返回的 `subtitle` 现在明确显示在名称下面，作为低调的小字描述。
- `qml__injector_exe/main.qml`: 保留右侧状态胶囊，并把它固定在信息区右上角。当前仍显示现有 `state` 字段，后续可继续由服务端填入“未激活 / 未解锁 / 限定”等状态文案。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.18 / 30618`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。
### Reason
- 用户希望后台上传模型时填写的描述能显示在客户端模型名称下面，并保留右侧空状态位作为后续服务端控制的标签。当前数据链已经有 `subtitle/state`，这版主要把 UI 信息层级摆正。

## EXE v3.6.17 / DLL v3.5.50 - 2026年06月09日 09:16
### Changes
- `qml__injector_exe/main.qml`: 回退 `v3.6.16` 新增的模型页 `WheelHandler + SmoothedAnimation` 平滑滚轮层，恢复 Qt `GridView` 默认滚轮处理。
- `qml__injector_exe/main.qml`: 同步撤掉这次实验里的 `flickDeceleration / maximumFlickVelocity` 调整，避免滚轮输入被额外动画层拦截后出现明显拖滞。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.17 / 30617`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。
### Reason
- 用户反馈 `v3.6.16` 后列表滑动明显变卡。根因是平滑滚轮层在连续滚轮输入时反复停止并重启动画，实际形成拖滞，不适合当前模型页。此版只撤回这条实验链，保留 `75Hz` tick、GPU 渲染、三角缓存等前面更有效的改动。

## EXE v3.6.16 / DLL v3.5.50 - 2026年06月09日 09:10
### Changes
- `qml__injector_exe/main.qml`: 模型页 `GridView` 新增 `WheelHandler + SmoothedAnimation` 平滑滚轮层。滚轮不再直接一格一格跳 `contentY`，而是先计算目标位置，再用较短平滑动画过渡过去。
- `qml__injector_exe/main.qml`: 调整模型页 `flickDeceleration` 与 `maximumFlickVelocity`，让拖动/惯性滚动更轻一点，减少列表上下滑动时的阶梯感。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.16 / 30616`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。
### Reason
- 用户反馈 75Hz 版本整体好像更丝滑，但模型页鼠标滚轮上下滑动列表仍有轻微卡顿感。这里的表现更像默认滚轮步进太硬，不是模型渲染本身掉帧；因此这版先给模型列表单独补一层短距离平滑滚轮过渡。

## EXE v3.6.15 / DLL v3.5.50 - 2026年06月09日 09:02
### Changes
- `src__injector_exe/modelpreviewitem.cpp`: 模型预览自转刷新间隔从 `16ms` 调到 `13ms`，按约 75fps 的节奏尝试推进模型旋转与悬停跟随。
- `src__injector_exe/main.cpp`: 启动早期设置 `QSG_FIXED_ANIMATION_STEP=13`，让 Qt Quick 动画驱动按约 75Hz 的固定步进尝试运行，配合模型预览的 13ms tick 观察整体页面滑动和过渡是否更顺。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.15 / 30615`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。
### Reason
- 用户希望把整个页面按 75Hz 试一版。Qt 不能强制物理显示器刷新率，但可以把应用内部动画步进和模型预览 tick 调到约 13ms，先做一个小范围实验版，验证“卡顿感”是否来自刷新节奏。

## EXE v3.6.14 / DLL v3.5.50 - 2026年06月09日 08:55
### Changes
- `src__injector_exe/modelpreviewitem.cpp`, `src__injector_exe/modelpreviewitem.h`: 给模型预览加入三角形缓存。现在 `buildTriangles(...)` 不再在每次 RHI `synchronize()` 都无条件重跑，只在 yaw / pitch / zoom / 尺寸 / 资源变化时重建，避免多模型列表每帧重复做骨骼矩阵、投影、排序和 UV 转换。
- `src__injector_exe/modelpreviewitem.cpp`: renderer 同步路径不再每帧全量 `std::equal` 比较所有 GPU 顶点，改为通过 `consumeGeometryDirty()` / `consumeTextureDirty()` 消费脏标记后再上传，减少模型卡空转时的 CPU 扫描。
- `src__injector_exe/modelpreviewitem.cpp`: 模型卡离屏 MSAA 从 `4x` 降到 `1x`。多卡同时显示时优先保证滚动和旋转流畅度，抗锯齿后续再用更轻的 shader/FXAA 路线补，不继续用每卡 4x 离屏放大成本。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.14 / 30614`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。
### Reason
- 用户反馈上一轮后页面和模型滑动仍然“一卡一卡”。继续检查后发现，虽然像素栅格已经交给 GPU，但 CPU 仍在每张模型卡每帧重建三角形顶点，并且还会全量比较顶点数组。也就是说瓶颈从“逐像素画图”变成了“逐帧重算几何”。这版把几何重算收成脏标记缓存，并降低多卡离屏 MSAA 成本。

## EXE v3.6.13 / DLL v3.5.50 - 2026年06月09日 08:47
### Changes
- `src__injector_exe/modelpreviewitem.cpp`: 删除模型卡的鼠标滚轮缩放行为。现在滚轮事件会透传给外层 `GridView`，不会再被单张模型预览吃掉，模型页可以正常用滚轮上下滚动。
- `src__injector_exe/modelpreviewitem.cpp`: 模型自转 timer 从 `33ms` 提到 `16ms`，目标从约 30fps 回到 60fps 档位，减少自动旋转时“一卡一卡”的观感。
- `qml__injector_exe/main.qml`: 模型列表滚动、拖动、惯性滑动期间自动暂停所有模型卡自转，滚动停下后恢复。这样页面滚轮滑动时不会和每张卡的模型刷新抢同一帧。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.13 / 30613`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。
### Reason
- 用户反馈模型页仍然帧数不够高，滚轮滑动整个页面也不丝滑，并且不需要模型自身滚轮缩放。根因是模型控件仍拦截滚轮做缩放，导致页面滚动链路被打断；同时所有模型卡在滚动时继续刷新自转，也会抢 Qt Quick 的滚动帧。这版把滚轮还给页面，并在滚动期间暂停预览动画。

## EXE v3.6.12 / DLL v3.5.50 - 2026年06月09日 08:40
### Changes
- `src__injector_exe/modelpreviewitem.cpp`, `src__injector_exe/modelpreviewitem.h`: 模型预览正式迁到 `QQuickRhiItem + QRhi` 渲染路径。保留前面已经验证正确的 JSON 解析、骨骼矩阵、几何、UV、薄片双面和鼠标交互逻辑，只把最后的绘制阶段从 `QImage + depthBuffer` CPU 逐像素栅格改成 GPU vertex buffer + texture pipeline。
- `resources__injector_exe/shaders/modelpreview.vert`, `resources__injector_exe/shaders/modelpreview.frag`, `resources__injector_exe/app.qrc`: 新增模型预览专用 shader，并把编译后的 `modelpreview.vert.qsb / modelpreview.frag.qsb` 打进 EXE 资源。fragment shader 保留透明裁切语义，贴图采样使用 nearest + clamp，避免重新引入之前的贴图边缘串色。
- `CMakeLists.txt`: 为 Qt 静态包显式加入 RHI 头文件目录，保证当前 Qt 6.11 静态构建能编译 QRhi 路线。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.12 / 30612`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。
### Reason
- 上一版虽然让 Qt Quick 窗口走了 D3D11，但模型卡本身仍然是 CPU 每帧把所有三角形逐像素画进一张 `QImage`，再把整张图上传成纹理。模型一多时，瓶颈仍在 CPU 栅格和反复纹理重建，所以会出现“显卡提示弹出来了，但还是 PPT”的现象。这一版把真正重的栅格化交给 GPU，CPU 只负责沿用已验证正确的几何/UV 结果生成三角顶点。

## EXE v3.6.11 / DLL v3.5.50 - 2026年06月09日 03:00
### Changes
- `src__injector_exe/modelpreviewitem.cpp`, `src__injector_exe/modelpreviewitem.h`: 给模型预览加了“空闲卡降频 + 活跃卡提清晰度”的分层策略。默认自动旋转心跳从 `16ms` 收到 `33ms`，空闲卡的 CPU 栅格分辨率降到约 `0.72x`；鼠标悬停或拖拽时再切到约 `1.2x` 渲染分辨率。这样多卡同时展示时，不再让每张卡都以满分辨率、接近 60fps 的频率重栅格整张 framebuffer。
- `src__injector_exe/modelpreviewitem.cpp`: 活跃卡的最终纹理过滤切到 `Linear`，空闲卡继续用 `Nearest`。这不是完美 MSAA，但能在不把 CPU 软栅格成本继续拉爆的前提下，让当前交互中的模型边缘观感更顺一点。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.11 / 30611`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。

### Reason
- 当前本地渲染链虽然已经正确，但“每张卡都 16ms 做一次整张 CPU 栅格并重建纹理”在模型一多时会直接卡成 PPT。与此同时，用户还明确反馈边缘没有抗锯齿。这个阶段最合理的策略不是继续全量提画质，而是先把资源花在“当前你正在看的那张卡”上：空闲卡先活下来，交互卡再尽量更顺、更软。

## EXE v3.6.10 / DLL v3.5.50 - 2026年06月09日 02:43
### Changes
- `src__injector_exe/modelpreviewitem.cpp`: 修正模型在鼠标离开后上下视角不会回正的问题。悬停结束后，旋转心跳现在会在继续自动转圈的同时，把 `pitch` 按平滑插值拉回默认值；这样鼠标挪走后，模型不会一直挂着刚才抬头/低头的角度。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.10 / 30610`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。

### Reason
- 用户反馈“鼠标挪走之后上下的视角不会回正”。根因很直接：离开悬停后只恢复了 `yaw` 自动旋转，但没有任何逻辑把 `pitch` 拉回默认值。这一轮就是把这个回正动作正式补上。

## EXE v3.6.9 / DLL v3.5.50 - 2026年06月09日 02:37
### Changes
- `src__injector_exe/modelpreviewitem.cpp`: 再次明显放大模型悬停跟随幅度，并继续提升跟手速度。`yaw` 跟随范围从 `24` 提到 `36`，`pitch` 跟随范围从 `18` 提到 `28`；悬停状态下的平滑系数从 `0.30/0.30` 提到 `0.34/0.34`。现在鼠标悬停到模型上时，左右和上下的追踪动作都会更夸张、更容易看出来。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.9 / 30609`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。

### Reason
- 用户继续反馈“上下幅度仍然不够，左右幅度仍然不够”。这说明上一轮虽然已经比最早明显，但还是偏收着。这一轮就直接把横向、纵向和跟手速度再一起拔高一整档，优先满足“悬停上去就很明显在追鼠标”这个体感目标。

## EXE v3.6.8 / DLL v3.5.50 - 2026年06月09日 02:29
### Changes
- `src__injector_exe/modelpreviewitem.cpp`: 继续放大模型悬停跟随幅度，并把跟随插值速度一起提起来。`yaw` 跟随范围从 `15` 提到 `24`，`pitch` 跟随范围从 `10` 提到 `18`；悬停状态下的平滑系数从 `0.22/0.22` 提到 `0.30/0.30`。现在鼠标放到模型上时，左右和上下的“盯着鼠标看”动作都会明显得多，而且不会再有“鼠标动了但模型反应太保守”的感觉。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.8 / 30608`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。

### Reason
- 用户继续反馈“还是左右的，幅度也不够大，上下的幅度也不大”。这说明上一轮只把范围从保守调到中等，仍然不够明显。这一轮就不再温吞了，直接把横向、纵向和跟手速度一起放开，让悬停表现更有存在感。

## EXE v3.6.7 / DLL v3.5.50 - 2026年06月09日 02:23
### Changes
- `src__injector_exe/modelpreviewitem.cpp`: 把模型悬停跟随幅度继续放大，`yaw` 跟随范围从 `10` 提到 `15`，`pitch` 跟随范围从 `7` 提到 `10`。现在鼠标放到模型上时，模型“盯着鼠标看”的动作会更明显，但仍保留现有的平滑插值，不会突然抽动。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.7 / 30607`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。

### Reason
- 用户希望“鼠标放到模型身上的时候，旋转幅度大一点”。当前逻辑手感已经是对的，只是范围偏保守，所以这一轮只放大跟随幅度，不改平滑速度和回正逻辑，避免又把交互做得太躁。

## EXE v3.6.6 / DLL v3.5.50 - 2026年06月09日 02:16
### Changes
- `src__injector_exe\backend.cpp`, `src__injector_exe\modelpreviewitem.h`: 把模型卡的初始 `yaw` 从 `0` 统一改到 `180`，并同步改掉预览控件自身的默认朝向、默认重置朝向以及悬停跟随基准朝向。现在模型页一打开就会从正面开始自动旋转，不再先露背面再慢慢转回来。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.6 / 30606`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。

### Reason
- 当前预览器的坐标语义里，`yaw = 0` 实际对应的是模型背向观察者，所以虽然渲染链已经稳定了，但用户一进模型页还是会先看到背后开始旋转，观感很别扭。这一轮不碰渲染本体，只把默认起手朝向纠正到正面。

## EXE v3.6.5 / DLL v3.5.50 - 2026年06月09日 02:03
### Changes
- `src__injector_exe/modelpreviewitem.cpp`: 把软件栅格阶段的 UV 采样从“纹理边界”收进“像素中心”。每个面的四个 UV 角现在会先向内缩半个 texel，再参与插值和采样；同时采样坐标改成 `floor(u * width)` / `floor(v * height)` 语义，不再继续用 `round(... * (size - 1))`。这轮主要针对“边缘裂缝”“蹭到隔壁贴图材质”“本来同一片边缘却偶尔取到旁边 atlas 区块”的问题。
- `src__injector_exe/modelpreviewitem.cpp`: 三角覆盖判定加入轻微容差，减少由于浮点边界判断过严，导致同一块面两半三角之间出现细裂缝的情况。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.5 / 30605`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。

### Reason
- `3.6.4` 之后，丢面和大块误填都已经收住了，但用户继续反馈“会有裂缝”“会用到其他的贴图材质”。这类现象更像采样问题，不像遮挡问题：说明 z-buffer 方向已经对了，但纹理采样仍然在贴图边界附近蹭到了隔壁像素。这一轮就是把采样点明确收进像素中心，同时放宽一点边界覆盖，专门压 seam 和 bleed。

### 渲染定稿
- **最后证明确认可行的路线，不是 `Scene Graph 直接画投影后三角`，而是 `CPU 先栅格完整 framebuffer，再把结果整张交给 Qt 显示`。** 前者看起来更像“真 GPU”，但在这个模型体系下会把遮挡、薄片、三角前后顺序、边界采样这些问题缠在一起，很容易出现“某些角度丢面”“薄片忽隐忽现”“怎么补都不彻底”。后者虽然朴素，但每个像素到底该显示谁，由我们自己决定，结果稳定得多。
- **正确的分层应该是 5 步：**
  1. **解析几何 JSON**：把骨骼、cube、pivot、rotation、box UV / per-face UV 先读成稳定的本地结构，不要在渲染阶段临时猜字段。
  2. **统一坐标语义**：`pivot / rotation / origin` 先转换成和网页预览一致的本地坐标，再由 `buildBoneMatrices()` 把骨骼世界矩阵算好。
  3. **生成三角真相**：每个可见面固定拆成两个三角，UV 角通过 `mapUvCorners(...)` 统一处理，薄片（`size <= 0.02`）单独走平面逻辑，不要硬当厚盒子画 6 个面。
  4. **本地逐像素栅格**：在 `renderPreviewImage(...)` 里用 barycentric 插值逐像素扫描三角，先做 `depth buffer`，再做 alpha 裁切，最后才落颜色。真正解决“谁盖住谁”的关键就在这一步。
  5. **显示层只负责贴最终结果**：`PreviewNode` 只承载一张最终纹理四边形，不再承载模型三角本身。也就是说，Qt 只负责“显示结果”，不负责“决定结果”。
- **UV 处理的关键教训也定死：**
  - 采样点必须收进像素中心，不能踩在贴图边界上，不然 atlas 很容易串到隔壁。
  - 采样坐标用 `floor(u * width)` / `floor(v * height)` 更稳，别继续用容易左右抖动的 round 语义。
  - 对零厚度 / 超薄片，不能把占位背面的 1x1 UV 也喂进去，否则本来该透明的地方会被错误填色。
  - 同一面拆成两个三角后，覆盖判定要给一点轻微容差，否则边界会出现肉眼可见的裂缝。
- **这次最终收敛后的结论可以一句话概括：**
  “几何和 UV 真相来自网页语义；遮挡真相来自本地 z-buffer；显示层只贴最终 framebuffer。”
- **以后如果再扩模型系统，渲染层别再回到‘直接让 Qt Scene Graph 画所有投影三角’这条路。** 它适合显示结果，不适合在这里充当最终的遮挡裁判。

## EXE v3.6.4 / DLL v3.5.50 - 2026年06月09日 01:56
### Changes
- `src__injector_exe/modelpreviewitem.cpp`: 修正超薄片平面逻辑，不再把“占位背面”的 1x1 UV 也拿来参与正式渲染。现在薄片如果只有一面是真贴图，就只画那一面并双面显示；只有当正反两面都各自带真实 UV 时，才分别按两面渲染。这样可以避免 `3.6.3` 里那种本来应该是空白/透明的位置，被错误拿去贴上一整块别的颜色或皮肤区域的问题。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.4 / 30604`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。

### Reason
- `3.6.3` 之后，贴图丢失确实消失了，但用户马上反馈出现“大块乱贴”和“本来应该空白的位置被填上东西”。这说明 z-buffer 路线是对的，但薄片平面的取面规则还错着：之前为了防止背面消失，把占位背面也一起喂进渲染，结果等于把本来不该出现的 UV 区块强行画到了屏幕上。这一轮就是把“只画真正有内容的那一面”这条规则补实。

## EXE v3.6.3 / DLL v3.5.50 - 2026年06月09日 01:50
### Changes
- `src__injector_exe/modelpreviewitem.h`, `src__injector_exe/modelpreviewitem.cpp`: 放弃“把投影后三角直接交给 Qt Scene Graph 逐片贴图”的默认渲染链，改回本地逐像素栅格。现在模型预览会先在 CPU 侧把三角形做 barycentric 栅格化、`depth buffer` 遮挡判断和 alpha 裁切，得到一张完整 framebuffer，再整张上传给 Scene Graph 当作最终预览图显示。
- `src__injector_exe/modelpreviewitem.cpp`: `PreviewNode` 改成只承载一张最终纹理四边形，不再直接承载整批模型三角。也就是说，遮挡正确性不再依赖 Qt Scene Graph 对那堆 2D 投影三角的提交顺序，而是由本地 z-buffer 先决定谁该盖住谁。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.3 / 30603`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。

### Reason
- 用户明确确认“这几次更改让问题完全没有得到改善”，这说明之前围绕薄片、双面和局部排序的补丁都没有真正击中根因。根因其实更底层：上一条 Scene Graph 路线本质上还是“先投影成 2D，再拿平均深度/提交顺序凑遮挡”，没有真正的 per-pixel 深度判定，所以某些角度下贴图丢失是结构性问题，不是参数问题。这一轮直接把遮挡决定权收回到本地 z-buffer，先把正确性拿稳。

## EXE v3.6.2 / DLL v3.5.50 - 2026年06月09日 01:39
### Changes
- `src__injector_exe/modelpreviewitem.cpp`: 继续针对云端模型里的超薄片几何补强。现在这类“按平面处理”的面，不只是少画无效侧面，还会把每个三角以双面方式提交一遍；也就是同一块贴图会同时带正反两种绕序，尽量压住 Qt Scene Graph / 底层图形后端在某些视角下把薄片背面优化掉后，发片、饰片、光环边条突然整块消失的问题。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.2 / 30602`，并重新完成 EXE 构建，新的构建产物已同步落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。

### Reason
- `3.6.1` 之后用户继续反馈“特定角度下仍有贴图不渲染”，说明问题不只是把薄片从六面盒子收成平面这么简单，还可能叠着底层对三角绕序/背面的处理差异。既然这些云端模型里本来就有大量 0 厚度或近似 0 厚度的装饰片，这一轮就直接把它们按双面三角去喂，先把“看一眼角度就蒸发”的路径堵死。

## EXE v3.6.1 / DLL v3.5.50 - 2026年06月09日 01:31
### Changes
- `src__injector_exe/modelpreviewitem.h`, `src__injector_exe/modelpreviewitem.cpp`: 给模型 cube 正式补上原始 `size` 记录，并在预览构面时把 `size <= 0.02` 的超薄片按“平面”处理，不再强行当成极薄盒子去绘制六个面。对于这类平面，如果正反两面里只有一面是真正的贴图、另一面只是 1x1 占位 UV，会自动把真实贴图复制给另一面，减少模型转到斜角时装饰片、发片、光环边条突然隐身或乱闪。
- `src__injector_exe/modelpreviewitem.cpp`: 悬停跟随逻辑改成“先回到默认正面，再平滑盯着鼠标看”。进入卡片时，目标朝向不再拿当前旋转角度当基准，而是先朝默认视角收拢；后续由 16ms 定时器持续插值到当前鼠标对应的小范围 yaw/pitch，避免一挪上去就按旧角度硬拧。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.1 / 30601`，并重新完成 EXE 构建，新的构建产物已落到 `build__injector_exe_cache\\NoteBotInjector.exe` 与 `dist__release_artifacts\\NoteBotInjector.exe`。

### Reason
- 这次继续追下来后，问题已经不是“普通立方体贴图排序有一点误差”那么简单，而是当前云端模型里本来就包含大量 `0 / 0.00001 / 0.01` 级别的薄片几何。旧逻辑为了避免零厚度，把它们硬撑成极薄盒子再画六个面，结果在斜角下非常容易出现前后关系打架、贴图忽隐忽现。同时，悬停跟随的基准角取自进入瞬间，也就注定不会出现用户想要的“先回正，再盯鼠标”的观感。这一轮就是针对这两个根因下刀。

## EXE v3.6.0 / DLL v3.5.50 - 2026年06月09日 01:14
### Changes
- `src__injector_exe/modelpreviewitem.cpp`: 把模型预览的贴图三角面从“整面排序”改成“逐三角排序”，每个面拆出的两个三角形会分别按深度参与绘制顺序，减少斜角观察时前后层级互相穿插导致的“某块贴图忽隐忽现”。
- `src__injector_exe/modelpreviewitem.cpp`: 在三角形落到屏幕后补了一次顶点朝向归正，遇到屏幕空间绕序翻掉的三角形会自动交换顶点顺序，避免某些角度下三角朝向反复翻面，贴图看起来像一会儿消失一会儿回来。
- `src__injector_exe/modelpreviewitem.cpp`: 悬停跟随从“大幅度按鼠标位置转头”改成更轻的“盯着鼠标看”模式，缩小 yaw/pitch 跟随范围，并加了平滑过渡；鼠标挪到卡片上时模型会像眼神跟着鼠标，而不是整颗头猛地拧过去。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.0 / 30600`，并完成重新构建；中途先处理了占用 `build__injector_exe_cache\\NoteBotInjector.exe` 的旧测试进程，随后成功重链，最终产物时间戳为 `2026/06/09 01:13:43`。

### Reason
- 这次问题本质上不是“模型位置错了”，而是三角贴图在某些视角下的前后关系不稳定，所以你会看到局部贴图像闪烁一样时有时无。同时，原来的悬停逻辑更像在强行转整个模型，不像“模型盯着鼠标看”。这一轮就是把这两个观感问题一起收口：先稳住三角绘制顺序，再把鼠标跟随改得更自然。

## EXE v3.5.99 / DLL v3.5.50 - 2026年06月09日 01:02
### Changes
- `src__injector_exe/modelcatalogmodel.h`, `src__injector_exe/modelcatalogmodel.cpp`: 给 `ModelCatalogModel` 正式补上 `count` 属性和 `countChanged` 通知，让 QML 里的 `modelCatalogModel.count > 0` / `=== 0` 可见性判断不再吃到假空值。模型数据即使只有 1 条，也会正确从“空状态”切到卡片列表。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本面前推到 `3.5.99 / 30599`，并保持运行时自报版本与资源版本一致。
- `build_all.bat`, `build__injector_exe_cache/NoteBotInjector.exe`, `dist__release_artifacts/NoteBotInjector.exe`: 重新完整增量构建 EXE。中途先结束了占用 `C:\NB\build__injector_exe_cache\NoteBotInjector.exe` 的旧测试进程，随后重链成功，新的构建产物时间戳为 `2026/06/09 01:01:43`。

### Reason
- 现场日志已经证明模型授权链拿到了 `1 个` 模型，但界面仍显示空白，说明问题不在服务端也不在下载链，而在 QML 的“列表/空状态切换”判断。现有 `GridView` 和空面板都依赖 `modelCatalogModel.count`，但这个 C++ 模型之前并没有把 `count` 作为正式可通知属性暴露给 QML，导致“数据进了模型、界面仍判空”的情况出现。这一轮就是把这个桥补实。

## EXE v3.5.98 / DLL v3.5.50 - 2026年06月09日 00:36
### Changes
- `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`: 把 Auth DLL 版本面统一前推到 `3.5.50 / 30550`，修掉上一轮“源码动作链已加上，但对外版本资源仍停在旧版”的分裂状态。
- `src__injector_exe/backend.cpp`: 把主程序源码里记录的当前 Auth DLL 版本同步抬到 `3.5.50 / 30550`，避免后续整包构建时又把旧版本常量带回去。
- `build_dll.bat`, `build__auth_dll_cache/NoteBotAuth.dll`: 重新增量构建 Auth DLL，产出新的发布 DLL（SHA256 `2d6a3d066dd85ca9a4a4ef7cacf96073d02c72773f1d8954e32fd563795dfa3d`，大小 `12709376` 字节），用于覆盖当前线上那份仍会把 `model_entitlements_v1` 回成 `unknown_action` 的旧 DLL。

### Reason
- 当前用户截图里的 `[MODEL] 模型授权获取失败：unknown_action` 不是服务端拒绝，而是本地运行时加载的 `NoteBotAuth.dll` 根本不认识 `model_entitlements_v1`。源码里动作分发早就接好了，问题在于客户端本地拿到的还是旧发布 DLL。这一轮就是把发布面抬到一版新的 Auth DLL，并准备推上服务端让客户端强制更新。

## EXE v3.5.98 / DLL v3.5.49 - 2026年06月08日 13:18
### Changes
- `src__injector_exe/modelcatalogmodel.*`, `src__injector_exe/backend.*`, `src__injector_exe/main.cpp`, `qml__injector_exe/main.qml`: 模型页不再吃本地写死样例，改成由授权成功后的 `model_entitlements_v1` 结果驱动。客户端会把云端下发的可用模型列表落到本地 `models_v1/` 缓存，命中本地 `sha256 + size` 时直接复用；模型页空状态、卡片文案和底部摘要也一起改成真实数据链。
- `src__injector_exe/modelpreviewitem.*`: 模型预览正式从 CPU `QImage` 逐像素软渲染改成 Qt Quick Scene Graph 三角形贴图渲染，保留网页那套几何/UV 语义，但不再每帧重栅格整张离屏图。默认三卡都正面朝前持续旋转；悬停时跟随鼠标角度，离开后继续自动旋转。
- `src__auth_dll/src/v3/v3_rpc_client.*`, `src__auth_dll/src/v3/v3_state.*`, `src__auth_dll/src/v3/v3_actions.cpp`: 新增 `model_entitlements_v1` RPC / 动作链，让 EXE 可以通过 Auth DLL 正式获取模型授权列表，不再在主程序里塞假数据。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`, `src__updater_exe/version_info.rc`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`: 版本面统一前推到 `EXE 3.5.98 / DLL 3.5.49 / updater 3.5.71`，并重新走完整构建。

### Reason
- 之前模型页虽然“看起来有东西”，但本质还是两层假的：数据是假样例，渲染是假正式 GPU。用户这次要的是把“模型授权”和“模型预览”都收成真正可上线的链路，所以这一轮直接把本地占位列表拆掉，把授权拉取、缓存、QML 模型页和 GPU 预览器一起换成正式形态。

## EXE v3.5.97 / DLL v3.5.48 - 2026年06月08日 11:50
### Changes
- `qml__injector_exe/main.qml`: 模型页卡片高度改成跟当前面板高度自适应，缩小了单卡总高度与模型预览区域高度，让默认窗口下刚好能完整放下一排模型卡，不再因为卡片写死过高而挤出垂直滚动。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.97 / 30597`。

### Reason
- 用户要的是“模型页默认高度刚刚好放进去”。问题根因不在下方面板，而在模型卡本身的固定高度比当前半屏面板还高，所以这一轮直接把卡片和预览区做成随容器高度收缩。

## EXE v3.5.96 / DLL v3.5.48 - 2026年06月08日 11:44
### Changes
- `qml__injector_exe/main.qml`: 在进程列表区和下方详情面板之间补了一个伸缩占位层，把日志/模型面板真正顶到底边；继续保留“下方面板约等于窗口一半高度”的比例规则。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.96 / 30596`。

### Reason
- 上一版高度公式已经改成半屏了，但面板仍然没有贴底，原因是中间缺少把它往下顶的伸缩层。本轮只修这个布局受力点，不改别的界面逻辑。

## EXE v3.5.95 / DLL v3.5.48 - 2026年06月08日 11:26
### Changes
- `qml__injector_exe/main.qml`: 把下方公共详情面板重新收回到“约等于窗口一半高度”的固定比例，不再继续把剩余空间全部吃满。日志页和模型页共用这块面板，因此两种视图都会一起回到更规整的半屏高度。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.95 / 30595`。

### Reason
- 用户明确要求把日记高度和模型窗口高度一起改回整个窗口的一半。本轮只调这一处公共布局比例，不碰渲染、交互和其他界面元素。

## EXE v3.5.94 / DLL v3.5.48 - 2026年06月08日 11:22
### Changes
- `qml__injector_exe/main.qml`: 撤掉上一版模型卡里额外加的舞台背景和底部投影装饰，恢复成更干净的原始展示面，只保留“当前焦点卡实时旋转、其余两张保持样片”的性能策略。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.94 / 30594`。

### Reason
- 用户明确不要这层额外的视觉装饰。本轮只回退这部分观感层改动，不动刚刚已经生效的性能收口，避免为了撤装饰把卡顿优化也一起带回去。

## EXE v3.5.93 / DLL v3.5.48 - 2026年06月08日 11:11
### Changes
- `qml__injector_exe/main.qml`: 模型页三卡从“三张一起实时旋转”改成“只让当前焦点卡实时转，另外两张保持静态样片角度”；悬停到哪张卡就切哪张进实时预览。顺手给模型卡补了轻量的舞台背景和底部投影，让画面不再只是生硬地贴在纯黑底上。
- `src__injector_exe/modelpreviewitem.cpp`: 自动旋转节拍改成 16ms 精确定时，步进改小，让单卡实时预览时旋转更细一些、更顺一点。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.93 / 30593`。

### Reason
- 上一版的瓶颈已经很清楚了：这套本地 CPU 渲染器一旦让三张卡同时自动转，就会把性能浪费在“三份重复的实时重绘”上，结果就是又卡又朴素。既然三张卡本来就是同一个样例模型的不同视角，那就收成“默认展示三张样片，当前焦点卡再进实时模式”，把算力集中到用户正在看的那一张上。

## EXE v3.5.92 / DLL v3.5.48 - 2026年06月08日 10:39
### Changes
- `src__injector_exe/modelpreviewitem.cpp`, `src__injector_exe/modelpreviewitem.h`: 给本地样例渲染器补上可复用的 `QImage` / depth buffer 缓存，不再每帧重新分配整块离屏图和深度缓冲；同时去掉 z-buffer 已经足够覆盖时的额外三角深度排序。
- `src__injector_exe/modelpreviewitem.cpp`: 预览离屏渲染分辨率固定收口到 `1x` 逻辑尺寸，不再跟着窗口 DPR 膨胀；显示节点也显式锁成最近邻采样，减少放大后的发糊感。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.92 / 30592`。

### Reason
- 当前模型页已经能正确显示，但新的瓶颈已经很明显：三张卡同时自动旋转时，CPU 每帧都在重新栅格化、重新分配缓冲、再把整图上传成纹理，既抬高内存，也把帧率压成 PPT。先把最重的重复分配和高 DPR 离屏成本收掉，再顺手修一下显示层的“发糊”。

## EXE v3.5.90 / DLL v3.5.48 - 2026年06月08日 10:26
### Changes
- `src__injector_exe/modelpreviewitem.cpp`: 修正本地样例渲染器的贴图 `v` 坐标语义。当前预览器已经是 CPU 直接从 `QImage` 采样，不再继续沿用网页/WebGL 那套 `1 - v` 翻转，避免几何位置正确但整片 atlas 贴图上下/落点错乱。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.90 / 30590`。

### Reason
- 当前问题已经收窄得很明确：几何基本是对的，但贴图映射整片错乱。根因不是 `json/png` 被改坏，而是本地渲染器从 GPU 纹理坐标思路切到 CPU `QImage` 采样后，仍保留了 `1 - v` 翻转，等于把贴图坐标又倒了一次。

## EXE v3.5.89 / DLL v3.5.48 - 2026年06月08日 10:17
### Changes
- `src__injector_exe/modelpreviewitem.cpp`, `src__injector_exe/modelpreviewitem.h`: 模型预览从“按整面平均深度排序的伪 3D”改成“逐三角形软件栅格 + z-buffer + alpha 裁切”流程，继续严格沿用网页 `ysm_preview.js` 的面顺序、三角拆分、UV 旋转与 `1 - v` 采样语义。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.89 / 30589`。

### Reason
- 这次已经确认贴图错乱的核心原因不是 `json/png` 资源本身，也不只是单个 UV 公式，而是 EXE 本地预览器之前没有真正的深度裁决，只靠整张面排序，遇到 packed atlas、衣服外层、耳朵/发饰这种遮挡关系时会在转动角度后开始串片。先把网页里真正依赖的遮挡语义补回来。

## EXE v3.5.88 / DLL v3.5.48 - 2026年06月08日 10:00
### Changes
- `qml__injector_exe/main.qml`: 三张模型卡改为直接读取 `C:\Users\William\Desktop\YSM\凋零娘\main.128.packed.json` 与 `wither.128.packed.png`，不改原文件内容，只把它们作为本地样例输入源。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.88 / 30588`。

### Reason
- 需要验证“当前渲染器面对另一套 packed 资源时到底会画成什么样”，并且按要求不动原始 `json/png` 内容，只把它们原样喂给 EXE 预览控件。

## EXE v3.5.87 / DLL v3.5.48 - 2026年06月08日 09:50
### Changes
- `src__injector_exe/modelpreviewitem.cpp`: UV 写入顺序改成与网页 `ysm_preview.js` 一致，先做 `1 - v` 翻转，再应用 `uv_rotation`，最后直接把标准化坐标写进 Scene Graph 顶点。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.87 / 30587`。

### Reason
- `v3.5.86` 已经把后端切到 `Direct3D11`，但贴图仍然严重错乱。当前高概率根因是 EXE 本地渲染器的 UV 变换顺序和网页不同：之前是“先旋转后翻 Y”，而网页是“先翻 Y 后旋转”。

## EXE v3.5.86 / DLL v3.5.48 - 2026年06月08日 09:47
### Changes
- `src__injector_exe/modelpreviewitem.cpp`: 样例渲染器写入纹理坐标时补上 `1 - v` 翻转，对齐网页 `ysm_preview.js` 的 UV Y 轴处理。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.86 / 30586`。

### Reason
- 网页端在生成 WebGL UV 时明确做了 `1 - uvY / textureHeight`，而 EXE 本地渲染器之前直接使用了未翻转的 Y 坐标，极容易采样到贴图反面区域，表现出来就是模型卡片一片黑。

## EXE v3.5.85 / DLL v3.5.48 - 2026年06月08日 09:43
### Changes
- `src__injector_exe/main.cpp`: Qt Quick 渲染后端从尝试的 `OpenGL` 改为 `Direct3D11`。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.85 / 30585`。

### Reason
- `v3.5.84` 日志已确认当前这套静态 Qt 构建没有 OpenGL 支持，所以模型不可见的根因不是样例数据，而是后端选错了。改为 Windows 下更自然的 `Direct3D11`，继续给 Scene Graph 三角形渲染器提供硬件后端。

## EXE v3.5.84 / DLL v3.5.48 - 2026年06月08日 09:39
### Changes
- `src__injector_exe/main.cpp`: Qt Quick 渲染后端从强制 `Software` 改为 `OpenGL`，让新的 Scene Graph 样例模型控件真正走硬件纹理三角形渲染链。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.84 / 30584`，其余链路不动。

### Reason
- `v3.5.83` 的模型控件已经编进 EXE，但界面里完全看不见内容。当前最可疑的根因不是模型数据本身，而是程序启动时仍强制 `Qt Quick` 走软件后端，而新的样例渲染控件依赖 Scene Graph 纹理三角形。先切回硬件渲染链验证这一层。

## EXE v3.5.83 / DLL v3.5.48 - 2026年06月08日 09:32
### Changes
- `src__injector_exe/modelpreviewitem.h`, `src__injector_exe/modelpreviewitem.cpp`: 新增 `ModelPreview` 样例渲染控件，改用 Qt Quick Scene Graph 纹理三角形绘制，不再走 `QPainter` 伪 3D 路线。
- `qml__injector_exe/main.qml`: 模型页三张卡恢复为同一样例模型的三视角预览，支持自动旋转、鼠标拖拽旋转、滚轮缩放、双击或小按钮复位。
- `resources__injector_exe/app.qrc`, `resources__injector_exe/ysm_sample_main.json`, `resources__injector_exe/ysm_sample_skin.png`: 重新接入网页验证过的样例模型资源，改用 `main_128_packed.json + skin_128_packed.png` 作为单文件样例集。
- `src__injector_exe/main.cpp`, `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 注册 `NoteBot 1.0 / ModelPreview`，并把主程序版本递增到 `3.5.83 / 30583`。

### Reason
- 当前需求是把网页端正确的样例渲染重新装回 EXE，但不嵌网页引擎，也不再使用上一轮已经证明不稳定的 `QPainter` 贴图方案。这版先把“样例模型 + 三卡交互预览”重新落地，后续再决定是否继续扩到云端模型列表。

## EXE v3.5.82 / DLL v3.5.48 - 2026年06月08日 09:16
### Changes
- `qml__injector_exe/main.qml`: 模型页彻底改成占位卡片，不再引用任何模型文件、贴图文件或预览图资源。
- `resources__injector_exe/app.qrc`, `resources__injector_exe/ysm_little_main.json`, `resources__injector_exe/ysm_little_skin.png`, `resources__injector_exe/ysm_preview_little.png`: 从 EXE 打包链和资源目录移除模型 JSON、贴图 PNG、预览 PNG。
- `src__injector_exe/modelpreviewitem.cpp`, `src__injector_exe/modelpreviewitem.h`: 删除 EXE 内原有模型解析、几何构建、贴图映射和软件渲染逻辑。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.82 / 30582`，Auth DLL 保持 `3.5.48`，Updater 保持 `3.5.70`。

### Reason
- 当前目标已经不是继续救那套渲染器，而是先把 EXE 里所有相关解析和渲染逻辑拆干净，留一个稳定占位壳，避免后续设计被旧上下文和残留资源干扰。

## EXE v3.5.81 / DLL v3.5.48 - 2026年06月08日 09:09
### Changes
- `qml__injector_exe/main.qml`: 删除已经失效的 `import NoteBot 1.0`，模型页继续使用网页端导出的静态预览图，不再依赖已移除的 `ModelPreview` 类型。
- `src__injector_exe/backend.cpp`: 把主程序版本和 `versionCode` 同步修正到 `3.5.81 / 30581`，避免版本字符串和更新判断分裂。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`: 主程序版本资源递增到 `3.5.81`，Auth DLL 保持 `3.5.48`，Updater 保持 `3.5.70`。

### Reason
- `v3.5.80` 虽然已经把坏掉的 3D 渲染控件移掉了，但 QML 顶部还残留 `import NoteBot 1.0`，导致程序启动时直接报 “module NoteBot is not installed”。顺手把版本码同步补齐，避免后续更新链再出阴间问题。

## EXE v3.5.80 / DLL v3.5.48 - 2026年06月08日 09:00
### Changes
- `qml__injector_exe/main.qml`: 模型卡片停止使用坏掉的 `ModelPreview` C++ 软件渲染控件，改回直接显示网页端导出的真实预览图 `qrc:/preview/ysm_little.png`。
- `src__injector_exe/main.cpp`, `CMakeLists.txt`: 从主程序启动链移除 `ModelPreviewItem` 注册与编译，避免模型页继续出现黑屏、贴图错乱或材质丢失。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.80`，Auth DLL 保持 `3.5.48`，Updater 保持 `3.5.70`。

### Reason
- `v3.5.79` 继续证明手写 Qt 软件渲染器不可靠，已经从局部贴图丢失恶化到黑屏/不渲染。当前先止血，客户端只使用网页端已经正确渲染出的预览图；后续若要动态旋转，应该直接接网页端 Three.js 或离屏生成帧，不再继续维护这套手搓渲染器。

## EXE v3.5.79 / DLL v3.5.48 - 2026年06月08日 08:47
### Changes
- `src__injector_exe/modelpreviewitem.h`, `src__injector_exe/modelpreviewitem.cpp`: 撤销 `v3.5.78` 的标准 patch 烘焙与背面剔除方案，避免模型贴图整体错乱。
- `src__injector_exe/modelpreviewitem.cpp`: 模型面绘制改为按网页端索引顺序拆成两个三角形分别做纹理映射，不再依赖整块四边形一次性变换。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.79`，Auth DLL 保持 `3.5.48`，Updater 保持 `3.5.70`。

### Reason
- `v3.5.78` 证明标准 patch 烘焙方向不对，会让模型贴图比上一版更乱。根据测试现象，“正面基本正常、只要转角就开始丢材质”更像是面顶点、UV 角顺序和四边形变换稳定性问题。这版改成两三角形绘制，贴近网页端几何索引，先把角度变化导致的丢面问题压下去。

## EXE v3.5.78 / DLL v3.5.48 - 2026年06月08日 08:31
### Changes
- `src__injector_exe/modelpreviewitem.h`, `src__injector_exe/modelpreviewitem.cpp`: 模型预览贴图改成先裁剪标准 patch，再按 `flipX / flipY / uvRotation` 进行本地变换，最后把标准矩形映射到目标四边形。
- `src__injector_exe/modelpreviewitem.cpp`: 增加近似网页端 `FrontSide` 的背面剔除，避免不可见背面参与排序后盖住正面材质。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.78`，Auth DLL 保持 `3.5.48`，Updater 保持 `3.5.70`。

### Reason
- `v3.5.77` 仍会随模型旋转角度出现局部材质丢失，说明不是资源加载问题，而是某些 UV 翻转/投影四边形组合让绘制变换失败或被背面覆盖。这版把源贴图片段标准化，避免翻转 UV 直接参与 `quadToQuad`，并减少背面干扰。

## EXE v3.5.77 / DLL v3.5.48 - 2026年06月08日 08:02
### Changes
- `src__injector_exe/modelpreviewitem.h`, `src__injector_exe/modelpreviewitem.cpp`: 对照 `YSM/12_little/preview/ysm_preview.js` 修正模型预览贴图链路，保留 UV 正反方向、`mirror` 与 `uv_rotation` 信息。
- `src__injector_exe/modelpreviewitem.cpp`: 绘制模型面时改为四边形纹理变换，不再把每个贴图片段硬拉伸到投影面的外接矩形，减少旋转时出现的贴图碎裂和错位。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.77`，Auth DLL 保持 `3.5.48`，Updater 保持 `3.5.70`。

### Reason
- `v3.5.76` 的模型位置基本对了，但贴图渲染明显错误。根因是 C++ 预览器把 UV 坐标提前归一化，丢掉了翻转/倒贴语义，并把斜四边形面当成矩形图片绘制；这和网页端 Three.js 的真实四边形纹理映射不一致。

## EXE v3.5.76 / DLL v3.5.48 - 2026年06月08日 06:11
### Changes
- `src__injector_exe/modelpreviewitem.h`, `src__injector_exe/modelpreviewitem.cpp`: `ModelPreview` 增加 `autoRotate` 与 `yawDegrees` 属性，内置 33ms 定时器持续旋转模型，方便直接观察几何面、UV 和深度排序问题。
- `qml__injector_exe/main.qml`: 模型卡片启用自动旋转，三个样例卡片使用不同初始角度，避免全部卡片同一瞬间看起来完全一样。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.76`，Auth DLL 保持 `3.5.48`，Updater 保持 `3.5.70`。

### Reason
- 当前模型预览渲染仍需要继续校准。静态视角只能看到一个面，问题不容易描述；持续旋转能把正面、侧面、背面、贴图错位和排序问题直接暴露出来，方便下一轮按具体画面修几何/UV。

## EXE v3.5.75 / DLL v3.5.48 - 2026年06月08日 06:05
### Changes
- `src__injector_exe/modelpreviewitem.cpp`: 修正 `qrc:/...` 到 Qt 资源路径的转换，模型 JSON 和贴图现在会正确从内置资源读取，不再在模型卡片里显示“加载失败”。
- `qml__injector_exe/main.qml`: 日志页和模型页统一使用同一套高展示框；进程列表在两种视图下都保持较低高度，下面的大区域用于日志或模型展示，避免两个页面高度风格不一致。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.75`，Auth DLL 保持 `3.5.48`，Updater 保持 `3.5.70`。

### Reason
- 模型卡片显示“加载失败”的根因是资源路径从 `qrc:/models/...` 转成本地资源名时少截了一位，实际读成了错误路径。顺手把日志框和模型框高度统一，保持页面切换时布局稳定。

## EXE v3.5.74 / DLL v3.5.48 - 2026年06月08日 05:59
### Changes
- `qml__injector_exe/main.qml`: 模型卡片高度从小卡片调整为更接近展示位的高卡片，模型渲染区域同步加高，避免模型页虽然外框变高、但内容仍然挤在一条矮行里。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.74`，Auth DLL 保持 `3.5.48`，Updater 保持 `3.5.70`。

### Reason
- `v3.5.73` 已经接入真实几何预览控件，但卡片尺寸仍然沿用图片壳时期的矮布局，视觉上不像“模型展示区”。这版把模型卡片高度和模型视窗一起拉开，让模型页的空间分配更符合后续模型选择场景。

## EXE v3.5.73 / DLL v3.5.48 - 2026年06月08日 05:55
### Changes
- `src__injector_exe/modelpreviewitem.h`, `src__injector_exe/modelpreviewitem.cpp`: 新增 `ModelPreview` QML 控件，用 C++ 软件渲染读取 Bedrock/YSM geometry JSON 与 PNG 贴图。实现了骨骼层级、cube 解析、Blockbench box UV、`ZYX` 欧拉旋转、正交投影、面深度排序和贴图面绘制，作为模型选择页的本地 3D 预览核心。
- `src__injector_exe/main.cpp`, `CMakeLists.txt`: 注册 `NoteBot 1.0 / ModelPreview`，并把新渲染控件加入主程序构建。
- `resources__injector_exe/app.qrc`, `resources__injector_exe/ysm_little_main.json`, `resources__injector_exe/ysm_little_skin.png`: 内置 `YSM/12_little` 的模型 JSON 与贴图 PNG，模型页不再只依赖静态截图。
- `qml__injector_exe/main.qml`: 模型卡片改用真实 `ModelPreview` 控件渲染；模型视图下会压低进程列表高度，把下方模型展示框拉高，避免模型区像日志框一样矮。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.73`，Auth DLL 保持 `3.5.48`，Updater 保持 `3.5.70`。

### Reason
- 上一版只是模型选择页的图片壳，视觉能看但没有真正几何渲染。你明确要把 `YSM/12_little` 里的渲染原理接进来，所以这版先把本地模型解析和 3D 预览核心装进启动器，让模型页具备真实渲染链路；后续再把模型列表来源从内置样例换成按密钥下发的云端资源。

## EXE v3.5.72 / DLL v3.5.48 - 2026年06月08日 03:32
### Changes
- `qml__injector_exe/main.qml`: 左侧侧边栏新增“日志 / 模型”视图切换。默认仍显示日志，点击“模型”后，原日志区域切换为模型预览网格。
- `qml__injector_exe/main.qml`: 新增模型预览卡片空壳，默认按 3 列展示，窗口变宽后会按可用宽度自动增加列数。卡片包含模型图、名称、状态和基础元信息，为后续接入密钥绑定模型列表和云端模型数据预留界面位置。
- `resources__injector_exe/app.qrc`, `resources__injector_exe/ysm_preview_little.png`: 从 `YSM/12_little` 的参考预览结果中内置一张 Little Wine Fox 预览图，先让模型库页面有真实渲染视觉，不依赖本地 YSM 目录路径。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.72`，Auth DLL 保持 `3.5.48`，Updater 保持 `3.5.70`。

### Reason
- 这轮先做模型选择界面的“展示壳”，不接自动皮肤注入。目标是先把用户入口、切换位置、卡片布局和预览视觉跑通，后续再把服务端按密钥下发的模型列表、皮肤资源下载、游戏进程出现后的自动注入逻辑逐步接进来。

## EXE v3.5.71 / DLL v3.5.48 - 2026年06月08日 03:17
### Changes
- `src__injector_exe/backend.cpp`: 进一步压缩注入流程日志。正常路径只保留设备检查、业务 DLL 信息、本地缓存/下载、票据准备、载入 DLL、等待验证、最终验证结果这些关键节点，移除 `[1/7]` 这类编号式刷屏和重复确认文案。
- `src__injector_exe/backend.cpp`: 不再把底层注入器的正常系统调用过程输出到用户日志；只有底层注入错误才透传，避免正常点击注入时出现过多 `OpenProcess / VirtualAlloc / CreateRemoteThread` 类调试信息。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本递增到 `3.5.71`，Auth DLL 保持 `3.5.48`，Updater 保持 `3.5.70`。

### Reason
- 上一版已经完成脱敏，但注入时对普通用户来说仍然像开发调试流水账。这版继续把日志压成人能读的流程提示：知道现在走到哪、是否命中缓存、是否下载、是否验证成功即可，不再把每个内部小步骤都铺满窗口。

## EXE v3.5.70 / DLL v3.5.48 - 2026年06月08日 03:10
### Changes
- `src__injector_exe/backend.cpp`, `qml__injector_exe/main.qml`: 注入按钮可用条件补上“必须存在目标进程”。扫描列表为空时会立刻清空旧 `selectedPid`，选中的 PID 消失时自动换到当前有效进程；`doInject()` 开始前还会二次确认目标 PID 仍在当前游戏进程列表里，避免没有游戏进程时还能误点注入。
- `src__injector_exe/backend.cpp`, `src__injector_exe/updater.cpp`, `src__injector_exe/win32injector.cpp`: 日志统一脱敏和收简。下载/缓存/校验日志只显示文件名和阶段结果，不再把本地目录、缓存路径、结果文件路径、Gate 日志路径、下载 URL、远程内存地址、LoadLibrary 地址等细节直接展示给用户。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`: EXE / updater 版本递增到 `3.5.70`，版本资源同步更新。
- `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`: Auth DLL 版本递增到 `3.5.48`，并同步宿主版本快照到 `3.5.70`。

### Reason
- 这轮主要修两个用户能直接看到的问题：第一，没有任何游戏进程或旧进程已经退出时，注入按钮不应该继续可点；第二，更新和注入日志之前太像开发调试输出，路径、缓存、底层地址和文件位置暴露太多。现在 UI 只保留“是否命中缓存、是否下载、下载进度、校验结果、注入阶段、最终结果”这些对用户有用的信息。

## EXE v3.5.69 / DLL v3.5.47 - 2026年06月08日 01:12
### Changes
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__auth_dll/version_info.rc`, `src__injector_exe/backend.cpp`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`: 版本面正式前推到 `EXE/updater 3.5.69`、`Auth DLL 3.5.47`，并完成本轮重配后全量构建，产出新的 `NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotAuth.dll`。
- `src__injector_exe/backend.cpp`: 注入链从“签票后先准备/注入 `NoteBotOverlayV3.dll`”改成“签票后直接注入真正业务 DLL”。桥 DLL 的准备、路径解析、注入文案和相关更新状态都一起移除，日志里不再出现“验证桥 DLL 已就绪 / 正在注入验证桥 DLL”这类中间层措辞。
- `CMakeLists.txt`, `build_clean.bat`, `build_all.bat`: 正式移除 `NoteBotOverlayV3.dll` 的构建与分发位，不再把它当成宿主产物复制进 `dist__release_artifacts`。
- `src__auth_dll/src/v3/v3_state.cpp`: 宿主更新快照默认项收口成 `main_exe / auth_dll / updater_exe` 三件套，不再给 `overlay_dll` 预留 bootstrap 槽位。

### Reason
- 这一轮目标很单纯，就是把“第三个 DLL”从整条正式链里拿掉。业务 DLL 既然已经能自己在游戏进程内验票，那 EXE 这边就不该再额外准备一个 `NoteBotOverlayV3.dll` 当中间人；构建链、更新快照、分发目录也得一起收口，否则后面很容易又从别的角落把这层桥悄悄带回来。

## EXE v3.5.68 / DLL v3.5.46 - 2026年06月08日 00:13
### Changes
- `src__injector_exe/backend.cpp`: 移除了上一轮“外部路径启动时临时强制下载主程序更新”的测速逻辑，主程序更新判定恢复成正式行为：只有当前运行主程序和服务端清单不匹配时，才下载 `main_exe` 并交给 updater 替换。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`: 版本面继续前推到 `EXE/updater/overlay 3.5.68`、`Auth DLL 3.5.46`，并重新完成整轮增量构建，用来覆盖上一版测试包。

### Reason
- 你这次已经确认对象存储下载速度没问题，那段“强制主程序下载一次”的测试逻辑就该立刻撤掉。它继续留着只会让客户端在外部路径启动场景里反复自我替换、无限重启，已经从测试辅助变成了干扰项。

## EXE v3.5.67 / DLL v3.5.45 - 2026年06月08日 00:04
### Changes
- `src__injector_exe/backend.cpp`: 主程序更新判定补了一层“外部路径启动”识别。现在如果当前运行的 `NoteBotInjector.exe` 不是 `%LOCALAPPDATA%\\NoteBotInjector\\NoteBotInjector.exe` 这条正式安装路径，就算它和服务端清单哈希相同，也会临时强制走一次 `main_exe` 下载与 updater 替换链，不再被“当前运行 EXE 已匹配清单版本”直接短路。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`: 版本面继续前推到 `EXE/updater/overlay 3.5.67`、`Auth DLL 3.5.45`，并重新完成整轮增量构建，为这次强制主程序测速提供新的启动更新包。

### Reason
- 你这次一直测不到 `main_exe` 下载，不是服务端没下发，而是你实际启动的主程序本身已经是最新哈希，客户端在“当前运行 EXE”和服务端清单一比，直接判定匹配后跳过了主程序下载。补上“外部路径启动也要强制走一次完整主程序更新”之后，这种测试场景终于不会再被本地启动入口短路。

## EXE v3.5.66 / DLL v3.5.44 - 2026年06月07日 23:49
### Changes
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/version_info.rc`, `src__injector_exe/backend.cpp`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`: 把宿主版本面整体前推到 `EXE/updater/overlay 3.5.66`、`Auth DLL 3.5.44`，并重新完成整轮增量构建，产出一套新的启动更新包。
- `build__injector_exe_cache/NoteBotInjector.exe`, `build__injector_exe_cache/NoteBotUpdater.exe`, `build__injector_exe_cache/NoteBotOverlayV3.dll`, `build__auth_dll_cache/NoteBotAuth.dll`: 这轮产物哈希全部变化，后续同步到服务端后，客户端启动阶段会被强制判定为需要更新，不会再因为“本地已匹配清单版本”而直接跳过主程序测速。

### Reason
- 这轮目的不是修业务逻辑，而是强制制造一组真正更高的新包，让你这台客户端下一次启动时一定会去下主 EXE 和 updater。只有这样，测出来的才是主程序大文件那一档真实下载速度，而不是被本地版本命中短路后的假结果。

## EXE v3.5.65 / DLL v3.5.43 - 2026年06月07日 21:52
### Changes
- `src__injector_exe/updater.h`, `src__injector_exe/updater.cpp`, `src__injector_exe/backend.cpp`: 正式补上下载凭证续期链。现在主程序更新链在每次真正开始下载 `updater_exe / main_exe / auth_dll` 前，都会先看当前 manifest 里那批下载 URL 还剩多少有效时间；如果已经快过期，就会先重新请求 `update_manifest_v3` 换一批新的下载 URL 再继续下，不再拿着上一轮快失效的票硬冲。即使极限情况下仍撞到 `403`，下载器也会自动补申领一次并重试，而不是直接宣判失败。
- `src__injector_exe/backend.cpp`: 启动更新链里之前漂掉的一批宿主版本常量一起拉齐到了 `EXE/updater/overlay 3.5.65`、`Auth DLL 3.5.43`。这样宿主更新快照、自报版本、注入票据里的 `exe_version` 和这次真正构建出来的产物终于重新说同一种话，不会再出现构建升版了、请求里却还报旧号的裂缝。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`: 按构建铁律把版本面继续前推到 `EXE/updater/overlay 3.5.65`、`Auth DLL 3.5.43`，并完成本轮完整增量构建。

### Reason
- 这次 `403` 的根因不是“网络突然抽风”，而是“同一批短时下载票被串行慢下载吃掉了寿命”。第一个大文件下太久，后面几个文件还在拿旧票排队，自然就会被服务端拦下。现在把“下载前先看票还有没有寿命，不够就补票”这一步真正塞进流程里，这条链才算像个能跑慢网的正式更新器。

## EXE v3.5.64 / DLL v3.5.42 - 2026年06月07日 19:11
### Changes
- `qml__injector_exe/main.qml`: 注入按钮的成功态现在改成真正的三段式状态机。注入完成后会先短暂亮起成功态，然后进入一小段带呼吸高光的冷却 `WAIT...`，冷却结束后自动回到正常可点击的注入按钮，不会再一直卡在绿色 `SUCCESS / 成功` 上。
- `qml__injector_exe/main.qml`: 两套注入按钮样式分支都一起收口了。旧备用按钮和当前主界面里正在用的 `injectBtnRestored` 都同步接入成功闪烁、冷却禁点和恢复逻辑，避免后面切布局时一边好一边坏。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_state.cpp`: 宿主版本面前推到 `3.5.64`，把 EXE / updater / overlay 以及 Auth DLL 里引用到的宿主版本串统一抬齐，避免本轮构建后界面、资源版本和请求上报口径打架。

### Reason
- 你这个问题本质上就是“按钮只会报喜，不会收尾”。成功态如果一直钉在那里，视觉上像是程序停在某个一次性终点，下一次还能不能点也说不清。现在把它改成“亮一下告诉你成了，再短冷却一下，最后自己回到待命”，手感会像一个真正还能继续工作的注入按钮。

## EXE v3.5.63 / DLL v3.5.42 - 2026年06月07日 16:30
### Changes
- `src__injector_exe/backend.cpp`: 注入第 `[7/7]` 步的结果回写等待链做了收口。现在超时从 `15s` 缩到 `9s`，等待提示从每 `3s` 一刷改成每 `4s` 一刷，`Gate 日志有新写入: <完整路径>` 也改成更干净的 `Gate 日志已更新`，不再一直把同一条路径糊满日志框。
- `src__injector_exe/backend.cpp`: 等待轮询 sleep 从 `60ms` 放宽到 `80ms`，把这段“盯结果文件”的噪声再压一点，减少 UI 日志刷屏感。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__injector_exe/backend.cpp`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`: 因为本轮重新构建本地四类产物，版本面统一前推到 `EXE/updater/overlay 3.5.63`、`Auth DLL 3.5.42`，并同步抬齐宿主更新快照、注入请求里的 `exe_version`、Auth DLL 自报版本和文件资源版本。

### Reason
- 你这个感觉是对的：结果文件真没回来时，原来那套等待像站在门口每隔三秒大喊一次“还没来吗”，既等得偏久，又吵。现在先把这段收成更短、更安静的默认行为。后面如果还有“真的慢但会成功”的特殊情况，我们再按真实链路去抓，不让默认体验先恶心人。

## EXE v3.5.62 / DLL v3.5.41 - 2026年06月07日 15:42
### Changes
- `src__injector_exe/backend.cpp`: 注入阶段的业务 DLL 准备链现在会先检查 `%LOCALAPPDATA%\\NoteBotInjector\\dlls_v3\\<bound dll>`。只要本地文件的 `sha256/md5/size` 全都和服务端当前绑定策略一致，就直接记录“命中本地 dlls_v3，sha256/md5/size 校验通过，跳过网络下载”，不再为这次注入重复发起 HTTP 下载；只有本地缺失或校验不一致时，才回退到原本的下载缓存流程。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__injector_exe/backend.cpp`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`: 因为本轮重新构建本地四类产物，版本面统一前推到 `EXE/updater/overlay 3.5.62`、`Auth DLL 3.5.41`，并同步抬齐宿主更新快照、注入请求里的 `exe_version`、Auth DLL 自报版本和文件资源版本。

### Reason
- 之前这条链像“先去网上重新问一遍仓库里有没有同一把钥匙，再低头看自己口袋里明明已经有”。结果就是本地 `dlls_v3` 明明已经是正确的业务 DLL，每次注入前还是会多走一遍网络下载，只是最后不覆盖而已。这一版把顺序摆正：先验本地，命中就彻底跳过网络；只有本地不对，才真的去下。

## EXE v3.5.61 / DLL v3.5.40 - 2026年06月07日 09:16
### Changes
- `src__overlay_dll/overlay_main.cpp`: 验证桥现在会导出当前已验证的 `tier / feature_flags / verified` 上下文，并在成功放行业务 DLL 之前把这份上下文挂到进程里，供后续业务 DLL 读取。
- `src__injector_exe/backend.cpp`, `src__injector_exe/backend.h`, `qml__injector_exe/main.qml`: 注入前会强制确保桥接 DLL 真正同步到本地；若固定文件名被游戏占用，会自动改用 `NoteBotOverlayV3_<sha12>.dll` 这类版本化文件名继续注入，不再卡死在旧桥文件锁上。等待阶段日志也继续做了节流和自动追底修正。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.61`、`Auth DLL 3.5.40`，并重新构建。

### Reason
- 这轮已经证明“外层 V3 验证链成功、业务 DLL 也被 LoadLibrary 进游戏”，但业务 DLL 本体还活在旧授权世界里。桥接层如果不把已验证上下文显式交出去，业务 DLL 就只能继续拿旧 `auth.dat` 自证，最后把自己挡在门外。这一版先把桥真正搭起来，让业务 DLL 有正式的新入口可读。

## EXE v3.5.60 / DLL v3.5.39 - 2026年06月07日 08:49
### Changes
- `src__injector_exe/backend.cpp`, `src__injector_exe/backend.h`: 把验证桥更新链补成真正能落地的版本。现在启动检查会把 `overlay_dll` 真的同步到本地，不再只是看见有更新却不替换；注入前也会再次确认验证桥可用。若固定文件名的 `NoteBotOverlayV3.dll` 正被游戏占用，就会自动落到 `NoteBotOverlayV3_<sha12>.dll` 这样的新文件并直接注入那份，不再被旧桥文件锁卡住。
- `src__injector_exe/backend.cpp`: 等待结果阶段的日志节流了。现在等待提示从每秒刷一次改成每 3 秒一次，`Gate 日志已更新` 也只有在文件真实发生新写入时才会再报，不会再像刷弹幕一样把日志面板淹掉。
- `qml__injector_exe/main.qml`: 注入中和启动更新阶段恢复强制自动追底，日志会优先跟住最新状态；平时不在注入/初始化阶段时，仍保留手动查看旧日志的余地。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.60`、`Auth DLL 3.5.39`，并完成本轮重新构建。

### Reason
- 这次真正绊脚的不是服务端，也不是密钥绑定，而是“桥接 DLL 自己没更新进去”。前面几层都换新了，偏偏中间那块旧转接头还钉在原地，于是程序表面上一路走到注入，实际进游戏的还是老逻辑。现在把“桥接更新”“文件锁绕开”“等待刷屏”三件事一起收掉，后面再看业务 DLL 本体问题时，至少不会再被旧桥拖后腿。

## EXE v3.5.58 / DLL v3.5.38 - 2026年06月07日 08:40
### Changes
- `qml__injector_exe/main.qml`: 修掉主界面和启动界面的日志区滚动故障。现在日志 delegate 改成显式高度，不再因为长行换行把 `contentHeight` 算歪；自动追尾也改成“只有用户没手动翻日志时才跟到底”，不会再一边想看旧日志一边被程序强行拽回底部，更不会凭空滚出大片空白区。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面继续前推到 `EXE/updater/overlay 3.5.58`，并重新构建最新主程序产物，确保这次滚动修复和前一轮注入链修复一起进入可更新版本。

### Reason
- 这次不是注入逻辑又炸了，而是日志面板自己在跟人较劲。它一边自动滚到底，一边又在长文本换行时重新计算高度，最后就像把电梯楼层算错了一样，滚轮一动就把视图送进一块空白地带。把“日志项高度”和“是否自动追尾”拆开以后，这块终于像正常控件了。

## EXE v3.5.57 / DLL v3.5.38 - 2026年06月07日 08:29
### Changes
- `src__injector_exe/win32injector.cpp`, `src__injector_exe/backend.cpp`, `src__injector_exe/logmodel.cpp`: 把注入黑盒改成可追踪链路。现在远程 `LoadLibrary` 超时不会再被误判成“注入成功”，业务 DLL 下载、票据写入、桥接 DLL 路径、结果文件路径、Gate 日志路径都会逐步打印出来；同时 UI 日志会落盘到 `state_v3/injector_ui.log`，不再只能盯着窗口猜。
- `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/src/protected/protected_verify_ops.cpp`: 把桥接 Overlay 的业务 DLL 验票核补成正式形态。哈希校验已经明确改成比对“服务端绑定的业务 DLL”，不再冒出误导性的 `self_sha256_mismatch`；新增 `overlay_gate_v3.log` 分阶段 trace，并给业务 DLL `LoadLibrary` 加上 10 秒超时兜底，防止卡死后连结果文件都写不出来。
- `_remote_patch/smoke_overlay_gate_v3.ps1`: 把本地 smoke 脚本从旧架构修正到新架构，改成“验证桥 DLL + 真实业务 DLL”分离验证。当前已实测跑通 `success / replay_blocked / signature_invalid / result_hmac_failed / license_pubkey_metadata_mismatch`，其中 success 分支会带回真实的 `verified_dll_sha256=b746...`.
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面继续前推到 `EXE/updater/overlay 3.5.57`、`Auth DLL 3.5.38`，并完成本轮完整增量构建。

### Reason
- 这一轮真正要修的不是单个报错，而是“失败时没有证据，超时时还装成功”。之前整条注入链像快递到了中转站就失联，前台只能看见一个模糊的“成功/没反应”。现在把每一站都亮灯以后，后面就算业务 DLL 自己再出问题，我们也能明确知道是下载没到、票据没写、桥没进、验票没过，还是业务 DLL 自己加载卡住了。

## EXE v3.5.56 / DLL v3.5.37 - 2026年06月07日 07:49
### Changes
- `src__auth_dll/src/v3/v3_state.cpp`: 把 `download_overlay_dll_v3` 从开发态的“本地找构建产物复制”改成正式行为：先走 `dll_policy_v3` 拿到服务端下发的 `dll_name / sha256 / md5 / size / download_url`，再通过 HTTPS 下载、校验、落盘到 `dlls_v3`。这样注入前准备的业务 DLL 终于和服务端绑定策略说的是同一个东西，不会再出现服务端明明绑着 `overlay_v6.4.75...`，本地却只准备了 `NoteBotOverlayV3.dll` 壳文件的错位。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面继续前推到 `EXE/updater/overlay 3.5.56`、`Auth DLL 3.5.37`，为这一轮正式下载链重构构建与更新分发对齐。

### Reason
- 之前这条链最离谱的地方不在服务端，而在本地：上层拿的是服务端业务 DLL 策略，下层准备的却是本地验证壳 DLL。结果就像菜单点了牛排，后厨端上来的是盘子本身，最后当然会在“目标业务 DLL 不存在”那里翻车。这一轮把下载准备收回正式协议口型，才算真对齐到 V3。

## EXE v3.5.55 / DLL v3.5.36 - 2026年06月07日 07:39
### Changes
- `src__injector_exe/backend.cpp`: 启动进入主界面后，如果本地快照已经显示“已激活”且当前已记住密钥，宿主现在会自动补跑一次会话验证，把 `authSessionVerified` 续起来，不再出现“界面显示已激活，但注入按钮仍然灰着，必须手点一次验证”的断层。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面继续前推到 `EXE/updater/overlay 3.5.55`、`Auth DLL 3.5.36`，准备这一轮重构建与更新分发。

### Reason
- 之前的启动流程只负责把“已激活”这个显示状态读出来，却没有把“本次会话已验证”续上，于是按钮逻辑和状态展示像两个人在各说各话。这一轮把启动后的自动续验补上，让“已激活”和“可注入”回到同一条链上。

## EXE v3.5.54 / DLL v3.5.35 - 2026年06月07日 07:32
### Changes
- `src__injector_exe/backend.cpp`: 修正“检查密钥 / 激活”按钮在服务端已清空设备绑定后的错误分支。现在只要本地快照显示当前设备未激活，或者 `device_check_v3` 明确回了“当前设备尚未完成联网激活”，按钮都会自动改走 `activate_device_v3`，不再卡在一串无意义的 `1007` 设备检查里打转。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面继续前推到 `EXE/updater/overlay 3.5.54`、`Auth DLL 3.5.35`，为这一轮重新构建和更新分发对齐。

### Reason
- 这次问题不是密钥又坏了，而是按钮脑子短路了。服务端那边我刚把设备绑定清空，本地却还拿着旧 `license_v3.dat`，结果按钮只看“key_id 还是同一把 key”，就死命走 `device_check_v3`，完全没有意识到“这其实应该重新激活”。这轮把它改成按真实激活状态说话，而不是按旧缓存自作聪明。

## EXE v3.5.53 / DLL v3.5.34 - 2026年06月07日 07:03
### Changes
- `src__injector_exe/backend.cpp`: 修掉 Auth DLL 强制更新状态的残留问题。现在更新检查、主路径已匹配、下载后同步完成这三条路径都会重新按主路径文件哈希回算 `m_authUpdateRequired`，不会再出现 DLL 已经换新但当前会话还被卡在“仅允许进入最小壳”的假红灯。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 按规则把版本面继续前推到 `EXE/updater/overlay 3.5.53`、`Auth DLL 3.5.34`，为这一轮重新构建和更新分发做准备。

### Reason
- 之前那次问题不是更新链没跑通，而是“更新完成后的门禁状态没刷新”。程序像是已经把新锁芯装上了，但脑子里还记着旧门禁，所以界面放进来了，授权和注入却还被挡在外面。这一轮把状态判断收回到主路径文件本身，后续就按真正落地的 DLL 说话。

## EXE v3.5.52 / DLL v3.5.33 - 2026年06月07日 06:24
### Changes
- `src__overlay_dll/overlay_main.cpp`: 修正 Overlay 侧读取 `license_v3.dat` 的方式。现在不再把它误当成裸 JSON，而是按 Auth DLL 同一套 `NBV3 + schema + DPAPI` 二进制壳先解包，再读取 `server_pubkey_version / server_pubkey_fingerprint` 做一致性校验。
- `src__overlay_dll/overlay_main.cpp`: 补回本地 little-endian 读取 helper，避免 Overlay 构建面因为 `readLe32` 缺失而假通过、真没编进新 DLL。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__injector_exe/backend.cpp`: 版本面继续前推到 `EXE/updater/overlay 3.5.52`、`Auth DLL 3.5.33`，并完成这一轮完整增量构建，四个产物重新落地。
- `_remote_patch/smoke_overlay_gate_v3.ps1`: 新增专用 smoke 脚本，用同一套正式私钥现签 inner ticket，直接覆盖 `success / replay_blocked / signature_invalid / result_hmac_failed` 这几条本地验票链关键分支。

### Reason
- 之前那条“Overlay 直接按 JSON 读 `license_v3.dat`”的路子表面看着合理，实际上和 Auth DLL 真正写盘格式不一致，会让本地公钥元数据校验变成一颗歪螺丝。这一轮把文件格式口径拉齐以后，本地 EXE/Auth DLL/Overlay 三者才算真站到同一条线上。

## EXE v3.5.50 / DLL v3.5.31 - 2026年06月07日 06:11
### Changes
- `src__auth_dll/src\v3\v3_state.cpp`, `src__auth_dll/src\v3\v3_actions.cpp`, `src__injector_exe\backend.cpp`: 把 smoke 抓出来的版本面漏点彻底补齐。`status_snapshot`、`ping/get_update_snapshot`、宿主更新快照、版本码常量、`issue_inject_ticket_v3` 默认 `exe_version` 现在统一前推到 `EXE/updater/overlay 3.5.50`、`Auth DLL 3.5.31`，不再出现产物已更新但对外 JSON 还报旧版本的情况。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__overlay_dll/overlay_main.cpp`: 按规则再次前推版本面，并完成本轮第二次构建，四个产物重新落地到 `build__*` 与 `dist__release_artifacts\\`。

### Reason
- 验票核本身已经收得差不多了，但如果版本面还分叉，后面服务端联调、更新判断和排错都会被旧号干扰。这一轮专门把“代码是真的新、嘴上也必须说自己是新的”这件事做干净。

## EXE v3.5.49 / DLL v3.5.30 - 2026年06月07日 06:08
### Changes
- `src__auth_dll/src/protected/protected_ticket_ops.*`, `src__auth_dll/src/protected/protected_verify_ops.*`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/src/v3/v3_state.cpp`: 把验票核最后几颗螺丝拧紧了。结果文件 HMAC 计算正式下沉到 protected，Overlay 侧新增 `license_v3.dat` 里的 `server_pubkey_version / server_pubkey_fingerprint` 与内置正式公钥元数据的一致性校验，DLL 自身 `sha256` 比对也改为 protected 判定，最终 `NBVmp_Verify_FinalAllow(...)` 不再吃一串硬编码 `true`。
- `tools__project_helpers/vmp_plan/NoteBotAuth.protect.tsv`: 新增 `NBVmp_Ticket_ComputeResultHmacHex`、`NBVmp_Verify_ServerKeyMetadataMatches`、`NBVmp_Verify_DllShaMatches` 三个符号登记，保证这次补上的验票核收尾逻辑也能进保护表。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.49`、`Auth DLL 3.5.30`，并同步更新注入请求里的 `exe_version` 与宿主更新快照版本。
- `build_all.bat`: 本轮完整增量构建已通过，重新生成了 `build__auth_dll_cache\\NoteBotAuth.dll`、`build__injector_exe_cache\\NoteBotInjector.exe`、`build__injector_exe_cache\\NoteBotUpdater.exe`、`build__injector_exe_cache\\NoteBotOverlayV3.dll`，并同步复制到 `dist__release_artifacts\\`。

### Reason
- 上一轮已经把验票主链接上了，但还差“本地授权记录与正式公钥元数据对死”“结果 HMAC helper 正式下沉”“最终放行分支不留假活”这几处收尾。补完这些以后，本地 EXE/Auth DLL/Overlay 之间的验票边界就更接近文档里要的最终定稿态。

## EXE v3.5.48 / DLL v3.5.29 - 2026年06月07日 05:31
### Changes
- `src__auth_dll/src/protected/protected_ticket_ops.*`, `src__auth_dll/src/protected/protected_verify_ops.*`, `src__overlay_dll/overlay_main.cpp`: 把业务 DLL 的 V3 验票核真正接上了。现在会在本地 protected 路径里完成 wrapper AES-GCM 解密、inner ticket 解析、服务端签名验签、PID 校验、30 秒 TTL 校验、业务 DLL 自身 sha256 校验、replay 判定和最终放行，不再是失败关闭占位壳。
- `src__overlay_dll/overlay_main.cpp`: 结果文件正式扩成带验证摘要的 V3 结果，新增 `granted_tier`、`granted_feature_flags`、`verified_dll_sha256`、`issued_at_server`、`expires_at_server`、`server_pubkey_version`、`server_pubkey_fingerprint`，并新增 `overlay_gate_v3.log` 本地摘要日志。
- `src__auth_dll/src/v3/v3_state.*`, `src__auth_dll/src/v3/v3_rpc_client.*`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/api.cpp`: Auth DLL 现在会消费新的结果字段、把扩展 verified 信息带到 `report_inject_result_v3`，并在“远程注入失败 / 结果文件无效 / 等待超时”时主动作废待处理票据、写本地 replay、再尝试上报失败结果，不再让票据生命周期停在半空。
- `tools__project_helpers/vmp_plan/NoteBotAuth.protect.tsv`, `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`: 新增的 `NBVmp_*` 验票符号已全部登记进保护表，并把 `protected_verify_ops.cpp` 正式编进 `NoteBotOverlayV3` 目标，保证构建产物真的带上这套核。
- `AUTH_V3_REWRITE_SPEC.md`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/version_info.rc`, `src__injector_exe/backend.cpp`: 文档同步补齐了结果 HMAC 覆盖字段与 `report_inject_result_v3` 扩展字段；版本面前推到 `EXE/updater/overlay 3.5.48`、`Auth DLL 3.5.29`。
- `build_clean.bat`, `build_all.bat`: 本轮先完整 clean build，`NoteBotAuth.dll`、`NoteBotOverlayV3.dll`、`NoteBotUpdater.exe` 正常生成；中途 `NoteBotInjector.exe` 因构建目录里的运行中实例占用链接失败。定位并只结束 `C:\NB\build__injector_exe_cache\NoteBotInjector.exe` 后，补跑 `build_all.bat` 成功，四个本地产物最终全部重新落地。
- `_remote_patch/smoke_auth_v3.ps1`: 对新 `NoteBotAuth.dll` 跑了一次本地 smoke，确认 `nb_self_check=0`、`nb_init=0`、`protocol=3`、`abi=1`，并且 `ping / get_update_snapshot / get_status_snapshot / device_check_v3 / dll_policy_v3 / issue_inject_ticket_v3 / device_heartbeat_v3` 都还能返回可解析的 V3 JSON。

### Reason
- 之前本地最大的安全空洞不是“算法没有”，而是“业务 DLL 的真正放行权还没装进去”。这轮把文档里已经写死的本地票据链、结果链、replay 链和服务端上报扩展真正焊到代码里，后面服务端只需要按这套固定口型去接，不用再反推 EXE/DLL 想法。

## EXE v3.5.47 / DLL v3.5.28 - 2026年06月07日 02:03
### Changes
- `qml__injector_exe/main.qml`: 注入失败后不再让按钮一直保持红色 `FAILED`。失败时先短暂红色闪现 `FAILED`，随后进入灰紫色 `WAIT...` 冷却态，避免红色错误块长期挂在界面上。
- `qml__injector_exe/main.qml`: 可见恢复版注入按钮和隐藏旧布局注入按钮同步使用同一套失败短闪 / 冷却状态，防止未来切布局后旧问题复发。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.47`、`Auth DLL 3.5.28`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 注入失败需要被用户看到，但不应该让红色失败态一直占据主按钮；本轮改成短反馈加冷却态，更接近已有的不可点击状态。

## EXE v3.5.46 / DLL v3.5.27 - 2026年06月07日 01:35
### Changes
- `src__injector_exe/backend.cpp`: 修正“当前输入密钥与本地已绑定授权不一致”时的错误门禁。现在输入新密钥不会被 EXE 本地旧缓存直接拦截，而是按文档口径进入 `activate_device_v3` 激活链，由服务端判断新密钥是否有效、是否允许换绑或是否命中冷却。
- `src__injector_exe/backend.cpp`: 旧密钥仍继续走 `device_check_v3`，空密钥仍然本地拒绝；本地 `license_v3.dat` 只作为旧授权状态缓存，不再充当新输入密钥的裁判。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.46`、`Auth DLL 3.5.27`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 文档规定 `activate_device_v3` 使用当前输入密钥交给服务端判断；旧实现把本地旧 `license_v3.dat` 当成输入密钥门禁，导致客户换密钥时被本地误拦。

## EXE v3.5.45 / DLL v3.5.26 - 2026年06月07日 00:33
### Changes
- `qml__injector_exe/main.qml`: 主界面两处日志列表改成和启动更新界面一致的单行 `Text` delegate，使用 `时间 + 两个空格 + 内容` 的同款显示方式。
- `qml__injector_exe/main.qml`: 主界面日志字体同步为启动更新界面的 `Consolas 11px`、`lineHeight 1.15` 和同款标签配色；只改日志文字显示，不改日志框大小、主布局、按钮或授权逻辑。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.45`、`Auth DLL 3.5.26`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 启动更新界面的日志字形和排法更顺眼，主界面原本分列显示导致进入主页面后观感明显下滑；本轮把主界面日志文字统一到启动页同款。

## EXE v3.5.44 / DLL v3.5.25 - 2026年06月07日 00:29
### Changes
- `qml__injector_exe/main.qml`: 回退上一版日志字体改动，启动更新界面和主界面日志不再使用 `Microsoft YaHei UI` / `Cascadia Mono` 组合，恢复到上一版更稳定的 `Consolas` 日志显示。
- `qml__injector_exe/main.qml`: 删除上一版新增的 `logTextColor(...)` 统一色阶和额外 `NativeRendering` / 行高调整，恢复原来的日志颜色、时间列透明度和行距，避免启动更新页被新字体拉丑。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.44`、`Auth DLL 3.5.25`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 上一版为了让日志更柔和改用了中文 UI 字体，但启动更新界面和主界面日志整体变丑；本轮按反馈直接回退这次字体和日志色阶改动。

## EXE v3.5.43 / DLL v3.5.24 - 2026年06月07日 00:24
### Changes
- `qml__injector_exe/main.qml`: 重新调整启动页和主界面日志字体观感，中文正文从 `Consolas` 改为 `Microsoft YaHei UI`，时间列单独使用窄字重，避免放大后像硬邦邦的代码终端。
- `qml__injector_exe/main.qml`: 日志颜色改成更柔和的高可读色阶，并补充 `logTextColor(...)` 统一管理普通、成功、警告、错误、联网和强调日志的显示色；保留 12px 可读字号，但降低生硬感。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.43`、`Auth DLL 3.5.24`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 上一版只是把日志字号放大，中文正文继续使用代码字体，视觉上偏硬、偏生；本轮把日志改成更像产品界面里的运行记录，而不是开发控制台。

## EXE v3.5.42 / DLL v3.5.23 - 2026年06月07日 00:14
### Changes
- `src__auth_dll/src/v3/v3_state.cpp`: pending 本地设备身份不再对外展示“已生成本地设备身份，待联网激活”，统一降为 `未激活`；联网失败时显示 `联网失败，未完成激活`，服务端拒绝仍显示 `激活失败：密钥无效`。
- `qml__injector_exe/main.qml`: 授权状态卡增加旧 pending 文案兜底转换，即使本地旧状态里还带“待联网激活”，界面也显示 `未激活`。
- `qml__injector_exe/main.qml`: 主界面日志字体放大，恢复主界面后日志正文从 `10px` 提到 `12px`，时间列从 `10px` 提到 `11px`；另一套主界面日志也同步到 `12px`。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.42`、`Auth DLL 3.5.23`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 旧 pending 文案看起来像成功提示，实际只是未激活状态；同时主界面日志字体过小，不利于判断更新和授权流程。

## EXE v3.5.41 / DLL v3.5.22 - 2026年06月06日 21:58
### Changes
- `src__auth_dll/src/v3/v3_state.cpp`: 服务端拒绝激活时，状态快照和返回消息统一改为 `激活失败：密钥无效`，不再显示“已生成本地设备身份，待联网激活”这种容易误判为半成功的文案。
- `src__auth_dll/src/v3/v3_state.cpp`: 拒绝激活后把 `online_state_cache.last_server_message` 记录为 `activation_rejected`，下次启动重新读取本地 pending 状态时也能继续显示 `激活失败：密钥无效`。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.41`、`Auth DLL 3.5.22`。
- `build_clean.bat`: 第一次构建因旧 `build__injector_exe_cache\\NoteBotInjector.exe` 进程占用链接失败；结束占用进程后重新 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 无效密钥被服务端拒绝时，旧状态文案仍强调“本地设备身份已生成”，容易让人误以为激活部分成功；本轮把用户可见状态改成直接、准确的失败原因。

## EXE v3.5.40 / DLL v3.5.21 - 2026年06月06日 21:00
### Changes
- `src__injector_exe/backend.h`, `src__injector_exe/backend.cpp`: 新增 `authSessionVerified` 会话级验证门禁；本地 `license_v3.dat` 的 `active=true` 只保留为状态展示，不再直接解锁注入。
- `src__injector_exe/backend.cpp`: “检查密钥 / 激活”现在必须使用当前输入密钥；本地授权文件存在时，会先校验输入密钥派生出的 `key_id` 是否与本地绑定授权一致，不一致直接拒绝本次验证。
- `src__injector_exe/backend.cpp`: 注入前继续执行 `device_check_v3`，检查失败会清空本次会话验证状态；只有检查成功后才允许进入后续票据和注入链。
- `qml__injector_exe/main.qml`: 注入按钮启用条件改为 `backend.authSessionVerified`，不再使用本地缓存 `backend.isActivated` 作为注入放行条件；检查按钮也不再因为本地缓存已激活而在空密钥下可点。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.40`、`Auth DLL 3.5.21`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 修正授权边界错误：本地授权缓存只能证明“以前绑定过”，不能等同于“当前输入密钥已通过服务端验证”，也不能直接让注入按钮可用。

## EXE v3.5.39 / DLL v3.5.20 - 2026年06月06日 19:17
### Changes
- `qml__injector_exe/main.qml`: 修正“检查密钥 / 激活”按钮文字发黑问题，改为固定高亮文字，并显式使用 `authCheckBtnRestored.ready` 控制颜色与点击状态。
- `qml__injector_exe/main.qml`: 两个授权控件继续保持暗底细边框，状态条降低绿色饱和度，避免和左侧整体风格冲突。
- `qml__injector_exe/main.qml`: 保持主界面日志框高度为主区域 `50%`。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.39`、`Auth DLL 3.5.20`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 上一版按钮文字仍接近黑色，不满足基本可读性；本轮先修正可读性，再保持整体紫黑风格。

## EXE v3.5.38 / DLL v3.5.19 - 2026年06月06日 19:12
### Changes
- `qml__injector_exe/main.qml`: 修正左侧“检查密钥 / 激活”控件的禁用绘制问题，不再让整个 Rectangle 进入 Qt disabled 状态，避免文字被系统压黑；只在 TapHandler 层控制可点击。
- `qml__injector_exe/main.qml`: 两个授权控件统一成暗底、细边框、清晰文字的同一套视觉语言，去掉上一版过突兀的整块亮色。
- `qml__injector_exe/main.qml`: 保持主界面日志框高度为主区域 `50%`。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.38`、`Auth DLL 3.5.19`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 上一版控件在整体界面中仍然突兀，并且按钮文字被 Qt disabled 状态压暗；本轮修正绘制逻辑并把两块控件统一到现有紫黑风格中。

## EXE v3.5.37 / DLL v3.5.18 - 2026年06月06日 19:07
### Changes
- `qml__injector_exe/main.qml`: 收回上一版过亮的洋红按钮和绿色状态条，把“检查密钥 / 激活”改成更贴合整体紫黑界面的扁平深紫按钮，只保留清晰的紫色边线和浅紫文字。
- `qml__injector_exe/main.qml`: “已激活 / 开发版”改成暗底状态条，不再整块高饱和亮绿；状态信息通过边线和文字颜色表达，避免像贴片一样跳出界面。
- `qml__injector_exe/main.qml`: 保持主界面日志框高度为主区域 `50%`，不回退日志区域调整。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.37`、`Auth DLL 3.5.18`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户指出上一版高饱和按钮完全破坏整体风格，所以这轮把控件收回到主界面的紫黑视觉语言里，保持扁平但不突兀。

## EXE v3.5.36 / DLL v3.5.17 - 2026年06月06日 19:02
### Changes
- `qml__injector_exe/main.qml`: 把左侧“检查密钥 / 激活”按钮彻底换成更扁、更亮、更简洁的实色控件，移除上一版复杂渐变、高光线、左侧竖条和底部短线。
- `qml__injector_exe/main.qml`: 把“已激活 / 开发版”状态条改成更扁的亮色状态块，启用态使用更鲜明的绿色，非启用态按状态给出更直接的颜色。
- `qml__injector_exe/main.qml`: 主界面日志框高度从主区域 `36%` 拉到 `50%`，最低高度同步提高，让日志区接近窗口一半。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.36`、`Auth DLL 3.5.17`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确要求这两个控件不要保留任何原来的样子，改成最简约、最好看、最扁的状态，并把进入主界面后的日志框高度拉到窗口一半。

## EXE v3.5.35 / DLL v3.5.16 - 2026年06月06日 18:44
### Changes
- `qml__injector_exe/main.qml`: 回退上一轮误删操作，把左侧“检查密钥 / 激活”按钮、“已激活 / 开发版”状态条，以及它们之间的分隔线恢复到删除前的界面状态。
- `qml__injector_exe/main.qml`: 顺手修正回退过程中引入的 QML 收尾括号问题，恢复主界面正常加载。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.35`、`Auth DLL 3.5.16`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确要求“立刻马上回退”，因此这轮只恢复误删的两块 UI，不做新的设计尝试。

## EXE v3.5.33 / DLL v3.5.14 - 2026年06月06日 18:35
### Changes
- `qml__injector_exe/main.qml`: 彻底删除左侧“检查密钥 / 激活”按钮、“已激活 / 开发版”状态条，以及它们之间的分隔线，界面里不再保留这两块任何 UI 痕迹。
- `qml__injector_exe/main.qml`: 同步修正删块后留下的 QML 结构收尾，恢复主界面正常加载。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.33`、`Auth DLL 3.5.14`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确要求先把这两块 UI 全删掉，避免继续受上一轮视觉上下文干扰，后面再从空白状态重写。

## EXE v3.5.31 / DLL v3.5.12 - 2026年06月06日 18:26
### Changes
- `qml__injector_exe/main.qml`: 把上一版“检查并激活”按钮里那颗蹦出来的小操作岛整体删掉，重新收回成一个完整按钮，不再拆结构。
- `qml__injector_exe/main.qml`: 按钮颜色改成更克制一点的浅紫渐变，保留统一按钮本体，只加细竖向装饰、高光带和底部短线，让它有一点质感但不再花里胡哨。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.31`、`Auth DLL 3.5.12`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确要求按钮必须还是一个整体，不要再蹦出一个小按钮，所以这轮重点是把结构收回去，只留克制的装饰和轻微反馈。

## EXE v3.5.30 / DLL v3.5.11 - 2026年06月06日 18:21
### Changes
- `qml__injector_exe/main.qml`: 再次重做左侧“检查密钥 / 激活”按钮，这次不再只是调紫色，而是改成更有结构感的双层按钮：外层暗壳、内层紫色主芯、左侧分级文字、右侧独立小操作岛，整体更像成熟桌面软件里的真实主操作键。
- `qml__injector_exe/main.qml`: 加入更细的悬停/按下反馈，让按钮不再像一块死色贴片；禁用态也同步压暗，避免启用和禁用时观感混成一坨。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.30`、`Auth DLL 3.5.11`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确说“颜色还有风格还是很难看”，所以这轮不再围着单一色相打转，直接把按钮本身做成更有层次和记忆点的控件结构。

## EXE v3.5.29 / DLL v3.5.10 - 2026年06月06日 18:11
### Changes
- `qml__injector_exe/main.qml`: 重新收口左侧“检查密钥 / 激活”按钮，只改这一块的视觉语言，改成更克制的深紫桌面工具按钮：外层壳更稳、内层渐变更干净、悬停和按下只做细微层次变化，不再像之前那样发闷、发死。
- `qml__injector_exe/main.qml`: 同步微调按钮字号、圆角、禁用态和高光处理，让中文文字更清楚，整体更接近成品桌面应用里的主操作键。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.29`、`Auth DLL 3.5.10`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确说“按钮不好看”，而且不要花里胡哨，所以这轮只收左侧主按钮本身，让它更简洁、更像真实桌面工具里的操作按钮。

## EXE v3.5.28 / DLL v3.5.9 - 2026年06月06日 17:50
### Changes
- `qml__injector_exe/main.qml`: 重做左侧“检查密钥 / 激活”和“已激活 / 开发版”两块的视觉，统一成更接近浏览器工具按钮的质感：更规整的圆角、克制的顶边高光、内层细边框，以及更扎实的中文文字显示。
- `qml__injector_exe/main.qml`: 按钮和状态条继续保持区分，但不再粗糙直楞，整体观感更像一套真实桌面应用工具按钮，而不是两块临时色条。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.28`、`Auth DLL 3.5.9`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确说“已激活”和“密钥检测”太粗糙，希望更像浏览器按钮，所以这轮重点是把这两个控件收成更像成品控件的视觉语言。

## EXE v3.5.27 / DLL v3.5.8 - 2026年06月06日 08:29
### Changes
- `src__injector_exe\\backend.cpp`: 删除启动阶段多余的 `[SYS]` 播报、更新清单里“忽略旧的...”四连刷，以及本地状态读取/缓存状态这种重复播报，只保留真正有判断价值的日志。
- `src__injector_exe\\updater.cpp`: 去掉“请求更新清单”“已激活设备签名请求”“更新清单已解析”这类过程噪声，更新链只在有结果或失败时说话。
- `src__auth_dll\\src\\api.cpp`: 把 DLL 初始化阶段的两条重复骨架日志并成一条 `[AUTH] 授权核心已就绪`。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.27`、`Auth DLL 3.5.8`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确说问题不是日志框，而是日志内容噪声太多，所以这轮直接砍掉无效和重复播报，让日志回到“只说有用的话”。

## EXE v3.5.26 / DLL v3.5.7 - 2026年06月06日 08:20
### Changes
- `qml__injector_exe/main.qml`: 收紧主界面日志面板，把高度从原来偏大的占比压低，减少内边距、标题间距和日志行间距，让右侧日志区不再显得又大又臃肿。
- `qml__injector_exe/main.qml`: 同步降低日志时间戳和正文的字号与透明度，减轻整块日志卡片的视觉重量。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.26`、`Auth DLL 3.5.7`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确说日志区域“又大又臃肿”，所以这轮只收日志块的高度、留白和文字密度，不碰其他布局结构。

## EXE v3.5.25 / DLL v3.5.6 - 2026年06月06日 07:37
### Changes
- `qml__injector_exe/main.qml`: 按用户要求把“检查密钥 / 激活”按钮颜色从偏蓝的深色直接改回深紫色，只改这一处色相，不再碰结构、文字或状态条。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.25`、`Auth DLL 3.5.6`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确要求“改成深紫色”，所以这轮只修按钮色相本身，不再围着别的视觉问题继续打转。

## EXE v3.5.23 / DLL v3.5.4 - 2026年06月06日 07:28
### Changes
- `qml__injector_exe/main.qml`: 把“检查密钥 / 激活”按钮从之前过于接近注入按钮的紫色里拆出来，改成偏蓝灰的深色按钮，并补上更明显的 hover / press 反馈。
- `qml__injector_exe/main.qml`: 把“已激活 / 开发版”状态改成更亮、更鲜的深绿色条，同时把文字改成完全居中显示，字号和字重再抬一档。
- `qml__injector_exe/main.qml`: 两处文案继续保持中文展示，不再出现 `Dev` 这类英文等级字样。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.23`、`Auth DLL 3.5.4`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确指出按钮颜色和注入按钮撞色、文字没有真正居中、状态条又灰又不鲜，所以这轮只围绕这几个点做局部收口。

## EXE v3.5.21 / DLL v3.5.2 - 2026年06月06日 07:14
### Changes
- `qml__injector_exe/main.qml`: 把上方“检查密钥 / 激活”按钮的底色从灰闷紫改成更干净、更鲜艳的纯紫，同时略微提亮边框高光，避免继续显得又脏又灰。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.21`、`Auth DLL 3.5.2`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确指出按钮色“不鲜艳”，所以这轮只把紫色从灰紫拉回更纯的紫，不再改结构和文案。

## EXE v3.5.19 / DLL v3.5.0 - 2026年06月06日 07:07
### Changes
- `qml__injector_exe/main.qml`: 去掉左侧授权按钮和状态提示里的圆点装饰，让两块都直接用纯文字表达。
- `qml__injector_exe/main.qml`: 把状态文案统一收成中文展示，`Dev` 改为“开发版”，并把按钮文字和状态文字都加大加粗，避免细、小、发虚。
- `qml__injector_exe/main.qml`: 把上方按钮紫色继续降饱和，改成更深更灰的紫；下方已激活提示改成更沉的深绿，并提升边框与文字对比。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src\v3\v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.19`、`Auth DLL 3.5.0`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确要求去掉花哨的点缀、改成中文、字别再那么小细虚，同时让按钮颜色更深更不鲜艳，所以这轮只围绕这些点收口。

## EXE v3.5.18 / DLL v3.4.99 - 2026年06月06日 06:59
### Changes
- `qml__injector_exe/main.qml`: 只继续压深下面这条已激活提示。把绿色底色改成更墨更沉的深绿，边框高光加强一点，状态点和文字也提亮，避免继续发灰发浅。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.18`、`Auth DLL 3.4.99`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确指出当前绿色提示“颜色太浅了”，所以这轮只做减法范围内的颜色加深，不再碰排版和结构。

## EXE v3.5.17 / DLL v3.4.98 - 2026年06月06日 06:45
### Changes
- `qml__injector_exe/main.qml`: 只加强下面这条状态提示。把高度从 `32` 提到 `38`，把前导点和正文字号都抬高一档，同时把底色和边框整体压深，不再显得又小又细又浅。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.17`、`Auth DLL 3.4.98`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确说当前提示“又小又细又浅”，所以这轮不再碰按钮和布局，只把状态提示做厚、做深、做大。

## EXE v3.5.16 / DLL v3.4.97 - 2026年06月06日 06:37
### Changes
- `qml__injector_exe/main.qml`: 统一左侧授权按钮和状态条的排版骨架。两块现在都使用相同的左侧前导点、相同的字号、相同的字重、相同的左边距和相近的高度。
- `qml__injector_exe/main.qml`: 继续微调颜色与边框透明度，只为了让两块看上去属于同一套界面语言，不再像两个完全不同的控件。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.16`、`Auth DLL 3.4.97`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确指出核心是字体大小和排版系统完全不同，所以这一轮的目标不是再修色，而是把两块收成同一套 typography 与 layout。

## EXE v3.5.15 / DLL v3.4.96 - 2026年06月06日 06:22
### Changes
- `qml__injector_exe/main.qml`: 统一左侧授权按钮和状态条的排版系统。两者现在都使用同样的左对齐、同样的前导点、同样的字号、同样的字重和接近一致的左右留白。
- `qml__injector_exe/main.qml`: 继续微调两块的高度、颜色重量和边框透明度，让它们看起来像同一套控件，而不是两个不同来源的模块。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.15`、`Auth DLL 3.4.96`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确指出问题不只是颜色，而是字体大小、排版方式和整体系统完全不是一套，所以这轮核心是统一 typography 和 layout，而不是继续只改色块。

## EXE v3.5.14 / DLL v3.4.95 - 2026年06月06日 06:16
### Changes
- `qml__injector_exe/main.qml`: 继续统一左侧授权按钮和状态条的重量感。上面的紫色按钮降饱和、降存在感，下面的绿色状态条加深并抬高到接近相同高度，文字尺寸也同步拉齐。
- `qml__injector_exe/main.qml`: 保持简洁方向，不再新增任何结构，只收颜色、尺寸、边框透明度和文字密度，让两块看起来像同一套控件。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.14`、`Auth DLL 3.4.95`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确指出不是单个控件丑，而是上下两块颜色、大小、重量严重失衡，所以这轮只做统一化收口。

## EXE v3.5.13 / DLL v3.4.94 - 2026年06月06日 05:48
### Changes
- `qml__injector_exe/main.qml`: 把左侧授权按钮继续去装饰，去掉渐变层次、顶线和额外结构，只保留最基本的纯色按钮、细边框和简单文本。
- `qml__injector_exe/main.qml`: 把状态提示再压成更轻的一条，tier 不再单独做小块，直接并入一行文本，整体更素、更安静。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.13`、`Auth DLL 3.4.94`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确说还是太花，所以这轮目标只有一个：继续减法，直到这两个控件看起来不再像“做过设计”，而像正常工具按钮和状态条。

## EXE v3.5.12 / DLL v3.4.93 - 2026年06月06日 05:34
### Changes
- `qml__injector_exe/main.qml`: 把左侧授权按钮彻底收成简约实用风。去掉前几轮的结构花活，只保留干净的紫色主按钮、轻微高光顶线和稳定的点击反馈。
- `qml__injector_exe/main.qml`: 把授权状态提示压成更实用的紧凑状态条。状态点、主状态文本和 tier 小片都保留，但整体更安静、更像工具界面。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.12`、`Auth DLL 3.4.93`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 这轮不再追求“创新造型”，而是回到你明确说的方向：简约、好看、实用，让左侧这两个控件先像正常成品。

## EXE v3.5.11 / DLL v3.4.92 - 2026年06月06日 05:19
### Changes
- `qml__injector_exe/main.qml`: 不再沿用“普通按钮缩放/换皮”的套路，按全新构型重做左侧授权按钮。按钮改成细边框暗壳 + 内层命令芯的双层结构，左侧加入微型 `AUTH` 眉标和窄竖线，右侧改成独立箭头岛，形成更有识别度的命令条交互。
- `qml__injector_exe/main.qml`: 授权状态提示同步改成同一套语言的紧凑状态条。主状态只保留一行，tier 缩进为内嵌小片，整体从“提示框”收成更像设备面板里的状态读数。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.11`、`Auth DLL 3.4.92`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户要求“创新”，不是继续围着同一种按钮造型打转，所以这轮直接换掉原来的块状按钮思路，做成更像命令模块和设备状态读数的局部交互。

## EXE v3.5.10 / DLL v3.4.91 - 2026年06月06日 05:12
### Changes
- `qml__injector_exe/main.qml`: 把左侧“检查密钥/激活”按钮从厚重的大圆角块继续压瘦，改成更低的高度、更小的圆角、更轻的边框反馈，去掉外圈光晕和大体积感。
- `qml__injector_exe/main.qml`: 把授权提示条同步压成窄条，降低高度和圆角，只保留单行状态文字与状态点，不再占左侧太多视觉体积。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.10`、`Auth DLL 3.4.91`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 用户明确否定了上一版的“大圆角”和“占了一大坨”的感觉，所以这轮目标不是换风格，而是把按钮和提示压回更薄、更硬、更紧凑的工具条语言。

## EXE v3.5.9 / DLL v3.4.90 - 2026年06月06日 05:09
### Changes
- `qml__injector_exe/main.qml`: 按你点名的旧版 `NoteBot_injector` 风格，把左侧“激活按钮”和“提示状态”彻底重构回工具型表达。按钮回归单块渐变主按钮，只保留清楚的点击感和轻量 hover 光晕；状态提示回归单行状态条，直接显示 `授权状态 + tier`，不再堆叠副标题、尾部标签或装饰结构。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.9`、`Auth DLL 3.4.90`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 之前几轮把这两块做得太像单独的“设计组件”，不够像你真正想要的老派工具界面。这轮直接以旧版布局语言为准，把信息和动作都收回到最直给、最顺手的表达。

## EXE v3.5.8 / DLL v3.4.89 - 2026年06月06日 05:04
### Changes
- `qml__injector_exe/main.qml`: 收窄右侧状态标签，去掉溢出感和英文副标，把“已通过验证”状态收回卡片内部。
- 同步推进 `EXE/updater/overlay 3.5.8`、`Auth DLL 3.4.89` 并完成 clean build。

### Reason
- 这一步主要是把上一版里最刺眼的外翻小标签和英文提示先清掉，但还没有完全回到你真正喜欢的旧版语言。

## EXE v3.5.7 / DLL v3.4.88 - 2026年06月06日 05:01
### Changes
- `qml__injector_exe/main.qml`: 对左侧“检查/激活”和“验证通过”做过一次设计化重构尝试，加入更强的结构感与动效反馈。
- 同步推进 `EXE/updater/overlay 3.5.7`、`Auth DLL 3.4.88` 并完成 clean build。

### Reason
- 这一步是一次方向试探，后续已被你否掉，并在后续版本里继续回收。

## EXE v3.5.6 / DLL v3.4.87 - 2026年06月06日 03:49
### Changes
- `qml__injector_exe/main.qml`: 参考你指定的本地 `ui-ux-designer` 思路，把“检查/激活”从普通按钮收成更像设计系统控件的验证条：低圆角、左侧强调线、双层文案、右侧轻量状态片。
- `qml__injector_exe/main.qml`: 把“已通过验证”重做成更有层级的验证模块：低圆角深绿底、左侧校验标记、主副文案分层，以及右侧小型等级片。
- `qml__injector_exe/main.qml`: 保留已经确认的安全细节：中文“密钥”、圆形 `●` 密文、绝不明文显示。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.6`、`Auth DLL 3.4.87`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 这轮目标不是瞎炫技，而是把这两个最显眼的交互控件做成更像一套成品设计系统里的组件，同时不破坏你已经定下来的布局骨架。

## EXE v3.5.5 / DLL v3.4.86 - 2026年06月06日 03:29
### Changes
- `qml__injector_exe/main.qml`: 按用户要求回退“检查/激活”和“已通过验证”到上一版设计语言，移除上一版新加的左侧强调线、双层文案、右侧状态片、勾选块等重构元素。
- `qml__injector_exe/main.qml`: 保留回退前已经确认的安全与文字样式，包括中文“密钥”标签、圆形 `●` 密文、密钥绝不明文显示。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.5`、`Auth DLL 3.4.86`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 这轮不是继续尝试新设计，而是严格回退到用户刚刚认可过的上一版视觉，先把误改的重构风格撤掉。

## EXE v3.5.4 / DLL v3.4.85 - 2026年06月06日 03:19
### Changes
- `qml__injector_exe/main.qml`: “检查/激活”按钮重做为更偏网页工具面的验证条控件。去掉单块胶囊感，改为窄圆角深色条、左侧强调线、双层文案和右侧小状态片。
- `qml__injector_exe/main.qml`: “已通过验证”状态重做为更有完成度的验证模块。加入左侧高亮线、校验标记、小型 tier 片和更明确的文字层级，不再只是普通绿色框。
- `qml__injector_exe/main.qml`: 密钥区继续保持绝不明文显示，标签保留中文“密钥”，密文符使用圆形 `●`。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.4`、`Auth DLL 3.4.85`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 这轮重点不是再压扁控件，而是把“验证动作”和“验证结果”做得更像成品界面：有信息层级、有状态语义，但不浮夸、不像 AI 胶囊块。

## EXE v3.5.3 / DLL v3.4.84 - 2026年06月06日 03:07
### Changes
- `qml__injector_exe/main.qml`: “检查/激活”按钮重新提亮并加深层次，保留工具按钮比例，但恢复更明确的紫色存在感和更稳的边框反馈。
- `qml__injector_exe/main.qml`: 通过验证状态条重新设计成更明确的深绿验证条，标题改为“已通过验证”，细节行显示 `tier / 当前状态`，整体识别度更高。
- `qml__injector_exe/main.qml`: 密钥区继续保持不可见输入，但标签改成中文“密钥”，密码符统一改为圆形 `●`，显示观感更顺。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.3`、`Auth DLL 3.4.84`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 之前我们把安全和收敛做到了，但把视觉也一起压得太扁。这轮把“验证入口”和“已通过验证”重新做成更像成品的控件，同时不破坏你已经定下来的整体布局。

## EXE v3.5.3 / DLL v3.4.84 - 2026年06月06日 03:00
### Changes
- `qml__injector_exe/main.qml`: 密钥区域标签从英文 `LICENSE KEY` 改为中文“密钥”，避免左侧控制区出现不一致的语言风格。
- `qml__injector_exe/main.qml`: 密钥密码符从方正的 `*` 改为圆形 `●`，并使用中文 UI 字体与轻微字距，让密文显示更圆润但仍不可见真实内容。
- `qml__injector_exe/main.qml`: “检查/激活”按钮加深紫色层次，提高可识别度，同时保留克制的 hover/press 反馈。
- `qml__injector_exe/main.qml`: 验证状态条增强绿色识别度，文案改为“已通过验证”，细节行显示 `tier / 状态`，整体不再灰淡。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.3`、`Auth DLL 3.4.84`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 上一版把安全性收住了，但视觉太淡，密钥区域语言也不统一。这里把密钥、验证按钮、验证状态三处重新压成同一套深色工具面板风格。

## EXE v3.5.2 / DLL v3.4.83 - 2026年06月06日 02:48
### Changes
- `qml__injector_exe/main.qml`: 授权状态提示从大块绿色卡片压成更小的状态条，降低圆角、降低绿色填充和边框强度，避免像一个突兀的大提示框。
- `qml__injector_exe/main.qml`: “检查/激活”按钮进一步收敛为扁平工具按钮，降低圆角、降低高度、降低紫色饱和度，hover/press 只保留轻微反馈。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.2`、`Auth DLL 3.4.83`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 上一版密钥安全已经收住，但授权状态提示块仍然太大、圆角太明显，按钮也还像一个装饰块。本轮把这两个控件压回工具界面的尺度。

## EXE v3.5.1 / DLL v3.4.82 - 2026年06月06日 02:39
### Changes
- `qml__injector_exe/main.qml`: 密钥输入框彻底固定为 `TextInput.Password`，删除所有可见/隐藏切换入口；可见布局和隐藏旧布局里都不再保留“显/藏”文字或切换行为。
- `qml__injector_exe/main.qml`: “检查/激活”按钮收敛为朴素工具按钮，去掉左侧状态点、右侧箭头和过重的渐变反馈，不再添油加醋。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.1`、`Auth DLL 3.4.82`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 密钥属于敏感输入，界面不应该提供任何显示入口；授权按钮也应该像一个清楚的工具控件，而不是带一堆装饰效果的主视觉按钮。

## EXE v3.5.0 / DLL v3.4.81 - 2026年06月06日 02:21
### Changes
- `qml__injector_exe/main.qml`: 撤销误回退到 `3.4.84` 三栏卡片布局的改动，恢复到上一版截图对应的左侧窄栏主界面：左侧品牌/目标/密钥/状态/INJECT，右侧上方进程列表，下方日志。
- `qml__injector_exe/main.qml`: 恢复上一版启动页布局：品牌与进度条偏左，启动日志面板在右下角显示。
- `qml__injector_exe/main.qml`: 保留上一版的密钥隐藏显示、`检查密钥/激活` 单入口、注入按钮授权门禁和更新日志降噪。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.5.0`、`Auth DLL 3.4.81`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 上一条“回退所有操作”被误解成回到 `3.4.84` 三栏卡片版，实际用户要的是恢复到刚才的上一版左侧窄栏界面。本轮把错误回退覆盖掉，并继续保持安全与授权门禁修正。

## EXE v3.4.99 / DLL v3.4.80 - 2026年06月06日 02:21
### Changes
- `qml__injector_exe/main.qml`: 回退主界面视觉到 `3.4.84` 备份 EXE 对应的三栏卡片布局：左侧 License/Inject，中间 Process，右侧 Log；移除这轮造成观感失控的左窄栏主界面。
- `qml__injector_exe/main.qml`: 启动页回退为居中 Logo + 进度条，不再使用右侧启动日志面板和偏移布局。
- `qml__injector_exe/main.qml`: 保留密钥默认隐藏显示和注入前授权硬门禁，避免回退时把明文密钥与空密钥可注入这两个安全问题带回来。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.4.99`、`Auth DLL 3.4.80`，避免回退后的新产物被更新链重新覆盖。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 这次回退是为了撤掉上一轮 UI 调整带来的方向偏移：主界面回到旧版三栏卡片结构，先把可用、熟悉、稳定的布局拿回来；安全门禁继续保留，避免为了视觉回退把已经修掉的功能风险也一起退回去。

## EXE v3.4.98 / DLL v3.4.79 - 2026年06月06日 02:02
### Changes
- `qml__injector_exe/main.qml`: 密钥输入框改为默认密码模式，不再明文显示；新增小型“显/藏”切换按钮，默认隐藏，按需查看。
- `qml__injector_exe/main.qml`: “检查密钥/激活”按钮改成更完整的主控件风格：更明确的深紫渐变、左侧状态点、右侧箭头反馈，hover/press 更自然。
- `qml__injector_exe/main.qml`: 授权状态卡不再只写“已激活/未激活”占位字样，改成真实状态文案，例如“待联网激活”“离线缓存状态”“已被其他设备顶下线”“Premium 已联网验证”等，并补一个 tier/V3 状态小胶囊。
- `src__injector_exe\\backend.cpp`, `src__injector_exe\\updater.cpp`: 启动更新日志降噪，去掉 DLL 加载路径、本地缓存路径、清单版本号等过细信息，改成更泛化的更新阶段提示。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.4.98`、`Auth DLL 3.4.79`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 这轮主要是把“看起来像半成品”的几个点一起收掉：密钥不能再明文，授权按钮要有完成度，状态卡要反映真实状态，更新日志只保留用户需要理解的层次，不再把版本号和本地路径直接甩到界面上。

## EXE v3.4.97 / DLL v3.4.78 - 2026年06月06日 01:43
### Changes
- `qml__injector_exe/main.qml`: 收敛启动页 NoteBot 图标呼吸效果。缩放幅度从明显起伏改为极轻微慢速变化，避免过渡感太强。
- `qml__injector_exe/main.qml`: 同步放缓图标外圈光晕变化，降低透明度波动，让启动阶段的品牌动效更稳。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.4.97`、`Auth DLL 3.4.78`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 上一版图标呼吸幅度太明显，容易看出动画过渡生硬。这里把缩放和光晕都压成低频、低幅度，让它只提供轻微生命感，不抢注意力。

## EXE v3.4.96 / DLL v3.4.77 - 2026年06月06日 01:38
### Changes
- `qml__injector_exe/main.qml`: 启动页日志面板进一步靠近窗口边缘，右/下外边距从 52px 收到 32px。
- `qml__injector_exe/main.qml`: 启动页日志面板加宽并略微增高，宽度范围调整为 360-460px，高度范围调整为 190-260px。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.4.96`、`Auth DLL 3.4.77`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 上一版日志面板离边缘仍偏远，横向空间也不够。这里让它更贴近右下角，同时横向加宽、高度略增，读日志更舒服。

## EXE v3.4.95 / DLL v3.4.76 - 2026年06月06日 01:34
### Changes
- `qml__injector_exe/main.qml`: 启动页右侧日志面板改矮，当前高度约 170-230px，不再占右侧大高块。
- `qml__injector_exe/main.qml`: 启动页日志面板改为右下角布局，右边距和下边距统一为 52px，面板内部左/下留白统一为 14px。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.4.95`、`Auth DLL 3.4.76`。
- `build_clean.bat`: 第一次构建时构建目录里的 `NoteBotInjector.exe` 被本地进程占用；只结束该构建目录进程后重跑，第二次 clean build 通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 右侧日志面板上一版太高，启动页右侧视觉压力偏大。现在把它压扁并放到右下角，外边距和内部留白也统一，整体更规整。

## EXE v3.4.94 / DLL v3.4.75 - 2026年06月06日 01:27
### Changes
- `qml__injector_exe/main.qml`: 启动页布局改为左右分区。NoteBot 图标、标题、更新进度条和状态文本整体向左移动；启动日志面板移到窗口右侧，作为右侧整块日志区显示。
- `qml__injector_exe/main.qml`: 启动日志面板从底部短条改成右侧面板，宽度约 320-390px，高度随窗口高度变化，并恢复日志自动换行，避免底部区域挤压主体。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.4.94`、`Auth DLL 3.4.75`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 启动页底部日志条仍然和中心视觉抢空间。改成左侧品牌/进度、右侧日志后，启动状态更像一个明确的两栏界面：左边看当前更新进度，右边看详细日志。

## EXE v3.4.93 / DLL v3.4.74 - 2026年06月06日 01:22
### Changes
- `qml__injector_exe/main.qml`: 收敛“检查密钥/激活”按钮动效。去掉上一版的扫光效果，保留更朴素的 hover/press 亮度、边框和轻微缩放反馈，避免按钮看起来过头或怪。
- `qml__injector_exe/main.qml`: 去掉注入按钮未验证态的额外透明外边框和顶部叠加高光，只保留深紫渐变和低亮文字，视觉上更干净。
- `qml__injector_exe/main.qml`: 缩小启动阶段日志面板，从原来的大块日志改为较小的底部日志条，避免还没进入主界面时把启动主体顶没。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.4.93`、`Auth DLL 3.4.74`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 上一版交互试得太满：检查按钮扫光显得奇怪，注入按钮叠加边框和高光也破坏了干净感。这里把效果收回到更克制的按钮反馈，同时让启动日志只承担反馈作用，不占掉主视觉。

## EXE v3.4.92 / DLL v3.4.73 - 2026年06月06日 01:17
### Changes
- `qml__injector_exe/main.qml`: “检查密钥/激活”按钮重新做成低调但有反馈的辅助按钮。默认是轻紫暗底，鼠标悬停时边框和底色增强，并有一条轻微扫光从左到右划过；按下时有短促缩放反馈。
- `qml__injector_exe/main.qml`: 注入按钮未验证态改成更有活性的深紫状态。保留 `INJECT` 文案，改用 `#452A68 -> #2C1D49` 的深紫渐变、低亮文字、顶部弱高光和轻微呼吸边光，仍然不可点击但不再像死色块。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.4.92`、`Auth DLL 3.4.73`。
- `build_clean.bat`: 第一次构建时构建目录里的 `NoteBotInjector.exe` 又被正在运行的本地进程占用；只结束该构建目录进程后重跑，第二次 clean build 通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 授权入口不能抢主按钮风头，但也不能像没做完；它需要在 hover/press 时给明确反馈。注入未验证态需要继续表达不可点，但颜色和边光要有一点活性，避免看起来像程序死了。

## EXE v3.4.91 / DLL v3.4.72 - 2026年06月06日 01:10
### Changes
- `qml__injector_exe/main.qml`: 参考旧版按钮观感，降低“检查密钥/激活”按钮存在感。现在它使用低亮背景、轻紫边框和弱白文字，不再像主操作按钮一样抢眼。
- `qml__injector_exe/main.qml`: 注入按钮未验证态改回旧版思路：按钮文字保持 `INJECT`，通过深色低饱和渐变和低亮文字表现不可点击，不再显示突兀的“待验证”。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.4.91`、`Auth DLL 3.4.72`。
- `build_clean.bat`: 第一次构建时 `NoteBotInjector.exe` 被正在运行的本地进程占用，已只结束该构建目录进程并重跑；第二次 clean build 通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 授权检查入口只是辅助动作，不应该比注入按钮更显眼。注入按钮的未验证态也应该像旧版一样靠深色和不可交互表达状态，而不是换成难看的大块提示文案。

## EXE v3.4.90 / DLL v3.4.71 - 2026年06月06日 01:03
### Changes
- `qml__injector_exe/main.qml`: 注入按钮未验证态改为更深、更低饱和的灰紫色渐变：上半 `#3E285D`，下半 `#281B3F`。保持不透明，视觉上是深紫压暗，不是普通灰色。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.4.90`、`Auth DLL 3.4.71`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 未验证态需要表达“这是按钮，但现在不可点”。上一版灰紫还偏亮，这次改成更深的灰紫，降低可点击感，同时不再靠透明度把按钮洗成普通灰色。

## EXE v3.4.89 / DLL v3.4.70 - 2026年06月06日 00:58
### Changes
- `qml__injector_exe/main.qml`: 回退刚才放大的日志字体显示。启动页日志标题恢复为 10px、日志正文为 11px；主页面日志标题恢复为 10px、时间戳和日志正文为 11px。
- `qml__injector_exe/main.qml`: 保留启动页日志面板、主页面日志区加高、注入按钮灰紫色未验证态，不再扩大字体。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.4.89`、`Auth DLL 3.4.70`。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 上一版把日志字号也抬大了，实际观感过重。这里只保留“启动页能看到日志”和“主页面日志区域更高”这两个结构改动，把字体显示退回原来的小号日志风格。

## EXE v3.4.88 / DLL v3.4.69 - 2026年06月06日 00:53
### Changes
- `qml__injector_exe/main.qml`: 注入按钮未验证态从“透明变灰”改为不透明灰紫色渐变。密钥为空或授权未通过时仍显示为按钮，但文字固定为“待验证”，不可点击。
- `qml__injector_exe/main.qml`: 启动页新增实时日志面板。更新检查、下载、加载授权模块等初始化日志会在进入主页面前直接显示，字号提升到 12，避免启动阶段像卡住一样没有反馈。
- `qml__injector_exe/main.qml`: 主页面底部日志区改为按窗口高度动态占比，约占右侧内容区一半，并提升日志标题和正文字号。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.4.88`、`Auth DLL 3.4.69`，同步更新版本快照、清单请求与注入请求字段。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 禁用态应该是“灰紫色不可用”，不是整体降低透明度后发灰。启动阶段也需要把更新检查日志直接展示出来，让用户知道程序正在做什么；主页面日志区加高后，排查联网、授权、注入问题时不用挤在很小的日志框里看。

## EXE v3.4.87 / DLL v3.4.68 - 2026年06月06日 00:46
### Changes
- `qml__injector_exe/main.qml`: 授权区从“激活 / 检查 / 重置授权”三个按钮收成单一入口“检查密钥/激活”。该入口会同步当前输入框密钥，然后走现有 `verifyLicense()` 流程：有本地 V3 状态时检查，没有本地 V3 状态时激活。
- `qml__injector_exe/main.qml`: 注入按钮增加明确的未验证禁用态。密钥为空或授权未通过时显示灰紫色“待验证”，不会响应点击；选中进程、授权通过且不在冷却/注入中后才恢复可点击。
- `src__injector_exe/backend.cpp`: `injectBlocked()` 增加后端硬门禁。即使绕过 QML 直接调用 `doInject()`，空密钥或未通过验证也会被拒绝，避免只靠界面禁用造成误放行。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面前推到 `EXE/updater/overlay 3.4.87`、`Auth DLL 3.4.68`，并同步更新快照、清单请求和注入请求里的版本字段。
- `build_clean.bat`: 本轮 clean build 已通过，重新生成 `dist__release_artifacts\NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`，以及 `build__auth_dll_cache\NoteBotAuth.dll`。

### Reason
- 当前授权主链已经不是三个手动入口的产品形态。用户只需要一个“检查密钥/激活”入口，重置授权不应作为常规按钮暴露。注入按钮必须等密钥输入、联网检查/激活通过后再可用，否则空密钥或未完成授权时容易误触发注入链。

## EXE v3.4.86 / DLL v3.4.67 - 2026年06月06日 00:29
### Changes
- `qml__injector_exe/main.qml`: 回退掉临时三栏卡片 UI，按 `D:\Downloads\cheatengine-mcp-bridge-main\cheatengine-mcp-bridge-main\NoteBot\NoteBot_injector\qml\main.qml` 的远古版布局重建 EXE 主界面。现在恢复为左侧固定控制栏、右侧进程列表、底部日志区，并保留 V3 的激活、检查、重置、注入动作绑定。
- `src__injector_exe/main.cpp`: 启动时禁用 QML/Shader 磁盘缓存，避免 `qrc:/qml/main.qml` 复用旧编译缓存导致界面更新后仍显示旧 UI。
- `build_clean.bat`: 真正删除整个 `build__injector_exe_cache`，不再只删 `CMakeFiles/CMakeCache.txt`。这修掉了旧 `NoteBotInjector_autogen` 资源对象残留导致 QML 没进 EXE 的问题。
- `src__injector_exe/backend.cpp`: 更新请求里的 `version_code` 改为与真实版本同步，并增加降级防护。服务端返回低于本机 `version_code` 的 `main_exe/auth_dll/updater_exe/overlay_dll` 会被忽略；等版本仍允许按 hash 补齐缺失文件。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`: 版本面统一前推到 `EXE/updater/overlay 3.4.86`、`Auth DLL 3.4.67`。
- `dist__release_artifacts` 与远端 V3 清单已同步：`main_exe/updater_exe/overlay_dll=3.4.86`，`auth_dll=3.4.67`，hash 从远端实际文件重新计算。

### Reason
- 这次问题不是单纯“UI 不好看”，而是三层事故叠在一起：UI 被误改、所谓 clean build 没清掉旧 QML 资源、更新链又因为旧 `version_code` 把新 EXE 降级回旧包。现在三层都收掉，最终 smoke 截图已确认运行中 EXE 是 `3.4.86`，界面回到远古版左栏布局，不再被服务端旧清单替回三栏 UI。

## EXE v3.4.80 / DLL v3.4.61 - 2026年06月05日 22:46
### Changes
- `qml__injector_exe/main.qml`: 修正第一版主界面布局里右侧进程面板被挤出窗口的问题。左侧授权/注入面板现在固定宽度，右侧进程列表正常占用剩余空间，日志面板继续横跨底部。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__auth_dll/version_info.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__overlay_dll/overlay_main.cpp`, `src__injector_exe/backend.cpp`: 因为这次继续重新构建客户产物，版本面前推到 `EXE 3.4.80 / DLL 3.4.61`，并同步更新文件资源、内部版本串、更新快照和请求版本。
- `build_clean.bat`: 本轮完整 clean build 已通过，四个产物已重新生成。

### Reason
- 3.4.79 已经解决“只有启动页”和“重复下载 updater”的功能问题，但窗口截图暴露出布局仍不适合交付：右侧进程面板被挤到窗口外，用户还是没法自然操作。现在把主界面布局收成可用形态，再重新出一版干净产物。

## EXE v3.4.79 / DLL v3.4.60 - 2026年06月05日 22:35
### Changes
- `qml__injector_exe/main.qml`: 修复 EXE 初始化完成后仍停在启动页的问题。旧 QML 实际只有启动页，没有任何主界面内容；现在改成启动页 + 正式主界面双状态，`backend.initializing=false` 后会显示授权、进程、注入和日志操作面板。
- `src__injector_exe/backend.cpp`: 修复每次启动都重复下载 `updater.exe` 的问题。现在会先校验 `%LOCALAPPDATA%\\NoteBotInjector\\updater\\NoteBotUpdater.exe` 和本地缓存，hash/size 匹配清单时直接复用，不再无条件重新下载。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__auth_dll/version_info.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__overlay_dll/overlay_main.cpp`: 因为本轮重新构建出客户产物，版本面前推到 `EXE 3.4.79 / DLL 3.4.60`，并同步更新文件资源、内部版本串、更新快照和请求版本。
- `build_clean.bat`: 本轮完整 clean build 已通过，重新生成四个产物：`NoteBotInjector.exe`、`NoteBotUpdater.exe`、`NoteBotOverlayV3.dll`、`NoteBotAuth.dll`。

### Reason
- 这次是真正的启动体验 bug：后端已经完成初始化，但前端文件被收得太狠，只剩启动页，所以用户看到“就绪”后没有下一步。重复下载则是更新链的缓存判断漏在 `updater_exe` 分支，导致清单里只要出现 updater 就每次拉一遍。现在两处都已按真实客户启动路径修掉。

## EXE v3.4.78 / DLL v3.4.59 - 2026年06月05日 22:16
### Changes
- `src__auth_dll/src/v3/v3_state.cpp`: 修正心跳成功后 `applyLicenseRecordLocked()` 重建快照导致 `network_available` 被旧缓存覆盖的问题。现在 `device_heartbeat_v3` 成功后会稳定保持 `online=true / network_available=true`，避免 UI 看起来像“心跳成功但网络不可用”。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__auth_dll/version_info.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__overlay_dll/overlay_main.cpp`, `src__injector_exe/backend.cpp`: 因为本轮重新构建出客户产物，版本面前推到 `EXE 3.4.78 / DLL 3.4.59`，并同步更新文件资源、Auth DLL 自报版本、宿主更新快照、更新请求和注入请求里的版本字段。
- `build_clean.bat`: 本轮完整 clean build 已通过，重新生成 `dist__release_artifacts\\NoteBotInjector.exe`、`dist__release_artifacts\\NoteBotUpdater.exe`、`dist__release_artifacts\\NoteBotOverlayV3.dll` 和 `build__auth_dll_cache\\NoteBotAuth.dll`。

### Reason
- 远端 V3 链路打通后，本地 smoke 暴露出一个会误导用户判断的小尾巴：检查和票据都已经成功，但心跳后的快照仍显示网络不可用。这个问题不是联网失败，而是本地快照应用顺序把成功状态覆盖了。现在这块已收掉，并顺手把客户要发出去的一组产物版本统一抬齐。

## EXE v3.4.77 / DLL v3.4.58 - 2026年06月05日 21:41
### Changes
- `src__injector_exe/updater.cpp`: 修正更新下载链在当前公网环境下的 TLS 卡点。`update_manifest_v3` 仍然走 `notebot-api.fucku.top:30165` 私有 TCP 主协议；实际下载 URL 可以走同一台服务器的 `https://183.66.27.20:30165/...`，更新器只在 IP 形式的 HTTPS 下载上允许证书主机名不匹配，并强制校验 `notebot-api.fucku.top` 正式证书 SHA-256 指纹 `755a7e09f472a45ef9b6a50c87c845f7059fd1be825cfcdf61c7c503ba109126`。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__injector_exe/backend.cpp`: 因为本轮重新出 EXE / updater / overlay 产物，版本面继续前推到 `EXE 3.4.77`，宿主更新快照、更新请求和注入请求里的 `exe_version` 一并同步。
- `dist__release_artifacts\\NoteBotInjector.exe`, `dist__release_artifacts\\NoteBotUpdater.exe`, `dist__release_artifacts\\NoteBotOverlayV3.dll`, `build__auth_dll_cache\\NoteBotAuth.dll`: 本轮 `build_clean.bat` 已重新通过，四类本地产物已重新落地。

### Reason
- 这轮的重点是把客户 EXE 的启动更新下载从“域名 SNI 被当前网络环境掐掉”里解出来，同时不退回明文 HTTP、不启用第二域名、不放宽到任意证书。主业务协议仍然固定走 `notebot-api.fucku.top:30165`；下载也仍然走 `30165`，只是对下载数据面使用 IP 地址并用证书指纹钉死服务器身份，确保后续 EXE 发给客户时不会继续卡在 Auth DLL 下载落地这一步。

## EXE v3.4.76 / DLL v3.4.58 - 2026年06月05日 01:38
### Changes
- `src__auth_dll/src/v3/v3_rpc_client.cpp`, `src__injector_exe/updater.cpp`: 这轮把客户端默认联网端口从旧 `30065` 正式切到新 V3 正式口 `30165`。Auth DLL 的授权/票据主协议和 EXE 的 `update_manifest_v3` 更新请求现在都默认直连 `notebot-api.fucku.top:30165`，不再继续指向旧服务端口。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__auth_dll/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__updater_exe/version_info.rc`, `src__injector_exe/backend.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`: 因为这次又重新出本地产物，版本面继续前推到 `EXE 3.4.76 / DLL 3.4.58`，并同步对齐资源版本、宿主更新快照、自报版本和请求里的 `exe_version`。

### Reason
- 刚才复查时发现一个很要命但很隐蔽的老尾巴：服务端已经稳定在 `30165/30166/30167`，但本地客户端默认连接口还停在 `30065`。这会导致你以为“整个方案都已经定死了”，实际一跑却还在撞旧服务。现在这个默认口已经收正，你后面按新端口开服和联调，客户端才会真的连到正确的 V3 服务端。

## EXE v3.4.75 / DLL v3.4.57 - 2026年06月05日 01:34
### Changes
- `src__auth_dll/src/v3/v3_server_pubkey.h`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__auth_dll/src/api.cpp`: 这轮把本地内置正式服务端公钥根整体切到了当前服务器正在使用的签票钥匙，正式公钥版本同步前推到 `server_pubkey_version=4`，对应指纹改为 `sha256:caba170778c0380bbcc3535ce540a35d75bd25f4fb62bd29e56ac072ff6b59f8`，避免 `issue_inject_ticket_v3` 继续卡在 `ticket_signing_key_mismatch`。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__auth_dll/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__updater_exe/version_info.rc`, `src__injector_exe/backend.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`: 因为这次要重新出本地产物，版本面继续前推到 `EXE 3.4.75 / DLL 3.4.57`，并把文件资源、宿主更新快照、Auth DLL 自报版本、请求里的 `exe_version` 一并抬齐。
- `AUTH_V3_REWRITE_SPEC.md`, `AUTH_V3_SERVER_IMPLEMENTATION_GUIDE.md`: 文档也同步收口到当前正式根，不再继续引用旧的服务端公钥版本/指纹，避免后续服务端和本地各自认不同的“正式真相”。

### Reason
- 这轮的核心不是再做一层兼容，而是把“正式签票根到底是谁”彻底定死。前面的真实阻塞不是协议形状，而是服务端正在用的私钥和本地内置公钥链根本不是一对，导致后台自检、票据签发、Overlay 验签永远会在最后一步互相否认。现在这套正式根已经被统一到同一个版本和同一个指纹，后面继续联调时，不会再被这类结构性错配反复绊住。

## EXE v3.4.74 / DLL v3.4.56 - 2026年06月04日 21:31
### Changes
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app.qrc`, `resources__injector_exe/app_icon.rc`, `src__auth_dll/version_info.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__injector_exe/backend.cpp`, `src__injector_exe/backend.h`, `src__injector_exe/updater.cpp`, `src__injector_exe/updater.h`, `src__overlay_dll/overlay_main.cpp`: 这轮把本地正式版本面继续前推到 `EXE 3.4.74 / DLL 3.4.56`，并把四类产物的文件资源、宿主更新快照、注入请求里的 `exe_version`、Auth DLL 自报版本一起抬齐，避免源码、资源和产物再各说各话。
- `src__auth_dll/src/v3/v3_server_pubkey.h`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/src/v3/v3_local_stub_keys.h`: 本地 stub 正式路线这次彻底摘掉。Auth DLL 不再保留 `NB_ENABLE_LOCAL_STUB` 正式分支，也不再接受本地自签票据 fallback；overlay 只认内置正式服务端公钥，并且会对本地 `license_v3.dat` 中的 `server_pubkey_version / server_pubkey_fingerprint` 做一致性检查。旧 `v3_local_stub_keys.h` 已从正式代码面移除。
- `src__injector_exe/updater.cpp`, `src__injector_exe/backend.cpp`, `resources__injector_exe/app.qrc`, `build_clean.bat`: 本地更新链继续按正文收口。未激活状态现在走匿名最小 `update_manifest_v3` 请求；已激活状态会直接用本地 `license_v3.dat + DPAPI 私钥 blob` 在 EXE 侧生成正式设备签名。`NoteBotUpdater.exe` 也改成了“先独立构建，再嵌入 EXE 资源，运行时从 `:/embedded/NoteBotUpdater.exe` 释放到 `%LOCALAPPDATA%\\NoteBotInjector\\updater\\NoteBotUpdater.exe`”的正式路线，不再把 EXE 同目录文件复制当作正式行为。
- `src__injector_exe/backend.cpp`: 把 `required=true` 的本地门禁补上了。现在 `main_exe` 必需更新仍然拦主 UI；`auth_dll` 必需更新允许进最小壳，但会阻止授权检查与注入；`overlay_dll` 必需更新不拦 UI，但会阻止注入。这一层已经按文档的三档处理在宿主侧落了地。
- `build__auth_dll_cache\\NoteBotAuth.dll`, `dist__release_artifacts\\NoteBotInjector.exe`, `dist__release_artifacts\\NoteBotUpdater.exe`, `dist__release_artifacts\\NoteBotOverlayV3.dll`: 再次完整 `build_clean.bat` 通过；四个产物文件详情版本现已对齐为 `3.4.56.0 / 3.4.74.0 / 3.4.74.0 / 3.4.74.0`。同时重新跑了新进程 smoke，确认 `nb_self_check=0`、`nb_init=0`、`protocol=3`、`abi=1`，`ping / get_status_snapshot / get_update_snapshot` 都返回了与本轮版本一致的 V3 JSON。

### Reason
- 这轮的目标不是再补一块功能，而是把你前面反复盯住的本地尾巴真正封死：正式 stub 不能再从任何角落偷偷活着，overlay 不能再认开发公钥，updater 不能再靠“旁边正好放了一个文件”来假装内置释放，更新协议也不能再只会匿名空签名。现在这几条本地基线都已经实打实落到了源码、构建链和产物上，后面再去做服务端联调，至少不会再被这些本地职责边界反复拖住。

## EXE v3.4.72 / DLL v3.4.54 - 2026年06月04日 20:01
### Changes
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__auth_dll/version_info.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src\v3\v3_state.cpp`, `src__injector_exe/backend.cpp`: 这轮继续按规格书把本地收口，因此又做了一次真实构建版本递增。现在四类产物的文件资源、内部版本串、宿主更新快照和注入请求版本都统一到了 `EXE 3.4.72 / DLL 3.4.54`。
- `src__auth_dll/src/v3/v3_state.cpp`, `src__overlay_dll/overlay_main.cpp`: 本地状态机和结果口径继续往正文靠拢。默认 `get_update_snapshot` 不再留空版本槽位；缺失 `license_v3.dat` 时快照展示收成 `未激活`、消息仍保留 `未发现 license_v3.dat`；离线和 kicked 文案改得更贴近文档状态表；overlay 结果状态也继续往 `wrapper_invalid / signature_invalid / dll_hash_mismatch / replay_blocked / ticket_expired / inject_failed` 这套推荐口径收。
- `src__injector_exe/backend.cpp`: EXE 本地加载 `NoteBotAuth.dll` 时新增了 DLL 自检和协议门禁。现在会先跑 `nb_self_check()`，再检查 `nb_get_protocol_version()==3`、`nb_get_abi_version()==1`；不通过就直接拒绝加载，避免“协议不符还硬着头皮继续跑”。
- `build__auth_dll_cache\\NoteBotAuth.dll`, `dist__release_artifacts\\NoteBotInjector.exe`, `dist__release_artifacts\\NoteBotUpdater.exe`, `dist__release_artifacts\\NoteBotOverlayV3.dll`: 本轮再次完整 clean build 通过；重新核对后四个本地产物的文件详情版本分别是 `3.4.54.0 / 3.4.72.0 / 3.4.72.0 / 3.4.72.0`，Auth DLL 导出面也保持不变。

### Reason
- 这轮的重点不是“再堆一点功能”，而是把文档里已经写死的本地契约继续压到代码和产物上，尤其是三件事最容易在服务端联调前埋雷：一是 EXE 没有真正做 DLL 自检和协议门禁，二是状态快照文案和正文状态表还没完全贴齐，三是更新快照默认形状和结果状态口径看着像 V3，细节上却还留着半旧半新的痕迹。现在这些本地边角继续被收掉了，后面再进服务端阶段时，至少本地不会因为这些基础口型反复打架。

## EXE v3.4.71 / DLL v3.4.53 - 2026年06月04日 19:08
### Changes
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__auth_dll/version_info.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 这次把本地四类产物的版本面又统一收了一次，不光字符串版本升到了 `EXE 3.4.71 / DLL 3.4.53`，连文件详情里之前还停在 `3.4.69 / 3.4.51` 的数值段也一起抬齐了，避免文件资源和真实产物再打架。
- `src__auth_dll/app.qrc`, `src__auth_dll/embed_qml.py`, `src__auth_dll/qml/main.qml`, `src__auth_dll/src/qml_embedded.h`, `src__auth_dll/include/notebot_auth.h`, `src__auth_dll/src/api.cpp`, `src__auth_dll/CMakeLists.txt`, `src__injector_exe/backend.h`, `src__injector_exe/backend.cpp`, `src__injector_exe/main.cpp`, `build_clean.bat`: 本地 Auth DLL 的旧主界面链路这次正式退役到位。DLL 不再导出 `nb_get_qml`，EXE 也不再等 `dllUiReady` 或从 DLL 拉主界面文本；构建链里原来的嵌入式 QML 生成和资源依赖都已经摘掉，主界面本地只走 EXE 自带 QML。
- `build__auth_dll_cache\\NoteBotAuth.dll`, `dist__release_artifacts\\NoteBotInjector.exe`, `dist__release_artifacts\\NoteBotUpdater.exe`, `dist__release_artifacts\\NoteBotOverlayV3.dll`: 重新跑了一次完整 clean build，四个本地产物都已重新生成；随后直接对 `NoteBotAuth.dll` 做了导出和 smoke 核对，确认 `nb_init / nb_get_protocol_version / nb_get_abi_version / nb_call` 都正常，`ping / get_status_snapshot / get_protocol_info / get_update_snapshot / device_check_v3 / dll_policy_v3 / issue_inject_ticket_v3 / report_inject_result_v3 / reset_v3_local_state` 也都按当前本地状态返回了可解析的 V3 JSON。

### Reason
- 这一轮不是再堆新功能，而是把“本地到底有没有真的定稿”从感觉题变成证据题。前面最容易骗人的地方其实有两个：一是源码说版本对了，但文件详情数值还旧；二是逻辑上说 DLL 不管主界面了，磁盘和构建里却还留着整条旧 QML 影子链。现在这两块已经一并收口，本地后续再进服务端联调时，至少 EXE / Auth DLL / updater / overlay 这几层的职责边界不会再反复打架。

## EXE v3.4.70 / DLL v3.4.52 - 2026年06月04日 17:39
### Changes
- `src__auth_dll/src/v3/v3_rpc_client.h`, `src__auth_dll/src/v3/v3_rpc_client.cpp`, `src__auth_dll/src/v3/v3_state.h`, `src__auth_dll/src/v3/v3_state.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`: 把本地 V3 票据链和结果回报从“主要靠本地拼装/本地确认”继续往正式协议口型收。现在 `dll_policy_v3`、`issue_inject_ticket_v3`、`report_inject_result_v3` 都已经有真实的远端 RPC 请求模型和签名输入规则；默认正式路径优先走远端协议，本地 stub 只在显式 `NB_ENABLE_LOCAL_STUB=1` 时保留开发用途。
- `src__injector_exe/backend.h`, `src__injector_exe/backend.cpp`: 把主程序自更新从“识别到 `main_exe required` 就报尚未接线”补成可执行闭环。当前宿主会先准备 `%LOCALAPPDATA%\\NoteBotInjector\\updater\\NoteBotUpdater.exe`，再按 `--replace-main --pid --src --dst --restart --backup` 拉起替换器完成主 EXE 覆盖与重启；不再只是口头承认有更新。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__auth_dll/version_info.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__injector_exe/backend.cpp`, `src__auth_dll/qml/main.qml`, `src__auth_dll/src/qml_embedded.h`: 这次真实构建把版本面整体抬到了 `EXE 3.4.70 / DLL 3.4.52`，并同步对齐了文件详情、QML 页脚、宿主更新快照、注入请求里的 `exe_version`、Auth DLL 内部版本串和 release 产物。
- 本次 clean build 已重新通过，四个本地产物已经重新落地：
  - `dist__release_artifacts\\NoteBotInjector.exe`
  - `build__auth_dll_cache\\NoteBotAuth.dll`
  - `dist__release_artifacts\\NoteBotUpdater.exe`
  - `dist__release_artifacts\\NoteBotOverlayV3.dll`

### Reason
- 这轮的目标不是再加表面功能，而是把本地真正能独立定死的地方往“正式 V3 形状”收。尤其是三件事最容易在后续服务端联调时炸开：票据请求字段签名不一致、结果回报仍停在 `local_deferred`、主 EXE 更新只有协议没有替换器闭环。现在这三块本地已经明显更接近规格书，后续服务端施工不必再一边改客户端口型一边猜本地到底想怎么跑。

## EXE v3.4.69 / DLL v3.4.51 - 2026年06月04日 16:04
### Changes
- `src__injector_exe/updater.h`, `src__injector_exe/updater.cpp`, `src__injector_exe/backend.cpp`: 本地更新链正式从旧 `auth_dll_bootstrap` 口型收成 `update_manifest_v3` 宿主语义。`Updater` 不再把世界写死成“只看 Auth DLL 单产物”，而是先解析统一更新清单，再优先消费 `auth_dll / updater_exe / overlay_dll / main_exe` 四类产物信息；当前本地先把 `auth_dll` 真下载链切到新模型，并把 `main_exe` 强制更新识别成明确错误态，避免继续偷偷走旧协议。
- `src__auth_dll/src/v3/v3_state.cpp`, `src__auth_dll/src/api.cpp`: 本地 stub 票据链从“默认正式路径”降成“默认关闭的开发后门”。现在只有显式设置环境变量 `NB_ENABLE_LOCAL_STUB=1` 时，`use_local_stub` 才会生效；否则缺少真实服务端票据时固定失败，不再默认 fallback 本地自签。
- `resources__injector_exe/app_icon.rc`, `src__auth_dll/version_info.rc`, `src__updater_exe/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__overlay_dll/overlay_main.cpp`, `src__auth_dll/qml/main.qml`, `src__auth_dll/src/qml_embedded.h`: 把版本资源、overlay DLL 内部版本串、QML 页脚和嵌入式 QML 一起拉齐到这次真实构建版本，避免再出现“工程版本、文件详情、界面角落版本号各说各话”的老问题。
- 这次完整 clean build 已重新通过，四个本地产物保持可生成：
  - `build__auth_dll_cache\\NoteBotAuth.dll`
  - `build__injector_exe_cache\\NoteBotInjector.exe`
  - `build__injector_exe_cache\\NoteBotUpdater.exe`
  - `build__injector_exe_cache\\NoteBotOverlayV3.dll`

### Reason
- 这轮不是继续加新功能，而是把“本地已经准备进服务端联调，但仍有旧更新协议残留、stub 默认开启、版本面不完全一致”这三个最容易在后续联调里反复恶心人的问题一次性钉死。现在本地至少已经明确区分了“正式协议口型”和“开发兜底后门”，不会再一边说按规格书，一边默认走旧语义。

## EXE v3.4.68 / DLL v3.4.50 - 2026年06月04日 15:32
### Changes
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__auth_dll/version_info.rc`, `src__overlay_dll/version_info.rc`, `src__updater_exe/version_info.rc`: 这次真实构建把本地三层产物的版本面和文件详情信息一起补齐了。现在 `NoteBotInjector.exe`、`NoteBotAuth.dll`、`NoteBotOverlayV3.dll`、`NoteBotUpdater.exe` 都带自己的版本资源，不需要先 `LoadLibrary` 才知道版本。
- `src__updater_exe/updater_main.cpp`, `src__updater_exe/version_info.rc`: 正式新增独立 `updater.exe` 本地产物，按规格书落了 `--replace-main / --replace-auth-dll / --replace-file` 三种命令行模式，带等待旧进程退出、backup、替换、复制后 sha256 复核、可选重启和 `%LOCALAPPDATA%\\NoteBotInjector\\updater\\logs\\updater.log` 日志落地。
- `src__overlay_dll/overlay_main.cpp`, `src__overlay_dll/version_info.rc`: 本地业务 DLL 不再只是名字占位。它现在会真实读取新的 `inject_ticket_v3.dat` 头部、派生 wrapper key、解包 wrapper、验证本地 stub 服务端签名、核对目标 PID / ticket sha / 自身 sha256 / 30 秒窗口 / replay cache，并回写 `inject_result_v3.dat`。
- `src__auth_dll/src/v3/v3_state.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/protected/protected_ticket_ops.cpp`, `src__auth_dll/src/api.cpp`: Auth DLL 本地闭环继续收口。修正了 `download_overlay_dll_v3` 的覆盖更新语义，不再“只复制第一次”；票据包装和版本串统一前推到本轮版本；旧 `nb_inject` 兼容壳不再偷偷写本地假结果，而是改成真的远程注入 + 等待业务 DLL 写回结果。
- `build_clean.bat`, `build_all.bat`: 构建脚本现在会一起分发 `NoteBotUpdater.exe` 和 `NoteBotOverlayV3.dll` 到 `dist__release_artifacts`，并修掉了根工程 `cmake` 失败却继续往下跑的假成功问题。
- 这次完整 clean build 已真实通过：`build__auth_dll_cache\\NoteBotAuth.dll`、`build__injector_exe_cache\\NoteBotInjector.exe`、`build__injector_exe_cache\\NoteBotUpdater.exe`、`build__injector_exe_cache\\NoteBotOverlayV3.dll` 全部成功生成，`dist__release_artifacts` 里也已经有对应三类本地产物。

### Reason
- 前一轮最卡人的不是“设计还没定”，而是本地工程处在一种很危险的半拉状态：根 `CMake` 已经引用了 updater / overlay 目标，但源码没补完；票据文件布局已经改了，但本地业务 DLL 还没真正消费；构建脚本看上去会结束，实际上根工程 configure 失败后还可能拿旧缓存假装成功。这一轮就是把这些本地断点全部补上，让 `C:\\NB` 至少进入“本地三产物可构建、文件版本可见、Auth DLL/EXE/业务 DLL 真正开始按 V3 文件协议协作”的状态。

## EXE v3.4.66 / DLL v3.4.48 - 2026年06月04日 13:35
### Changes
- `src__auth_dll/src/v3/v3_state.h`, `src__auth_dll/src/v3/v3_state.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`: 把本地 V3 票据/结果链从“能生成和消费第一版文件”补成更完整的本地闭环。新增了正式的 `report_inject_result_v3` 本地动作壳、`get_update_snapshot / set_host_update_snapshot` 宿主边带接口、本地 stub 结果生成、待上报结果上下文，以及 `consumed_tickets_v3.json` 的真实反重放逻辑（查询、写入、24h 裁剪、256 条窗口裁剪）。
- `src__auth_dll/src/protected/protected_verify_ops.*`, `tools__project_helpers/vmp_plan/NoteBotAuth.protect.tsv`, `src__auth_dll/CMakeLists.txt`: 新增了 VMP 友好的校验保护模块，把 replay 匹配和 result HMAC 比对从普通状态代码里剥出来进入保护表，并把新的 protected 模块正式接入 Auth DLL 工程。
- `src__auth_dll/src/api.cpp`: 旧 `nb_inject` 不再只是报“尚未接入”。它现在会走本地 V3 注入编排壳：拉当前 DLL 策略、发本地 stub 票据、轮询消费结果、执行本地确认上报，并用统一日志/状态回传把注入链串起来。
- `src__injector_exe/backend.h`, `src__injector_exe/backend.cpp`, `src__injector_exe/win32injector.cpp`, `CMakeLists.txt`: EXE 宿主侧不再把注入动作直接丢给旧 DLL 异步壳。`doInject()` 现在显式走 `device_check_v3 -> dll_policy_v3 -> 本地 DLL hash 校验 -> issue_inject_ticket_v3 -> 注入 -> consume_inject_result_v3 -> report_inject_result_v3` 的 V3 顺序；同时补上宿主更新快照同步，把 `main_exe/auth_dll/updater_exe/overlay_dll` 四类槽位通过 `set_host_update_snapshot` 同步给 Auth DLL。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `src__injector_exe/backend.cpp`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`: 按本轮两次真实构建前推版本，最终收口到 `EXE v3.4.66 / DLL v3.4.48`。
- 重新构建通过了 `build__auth_dll_cache\\NoteBotAuth.dll` 和 `build__injector_exe_cache\\NoteBotInjector.exe`。中途暴露出两个真实工程缺口：一是 `protected_verify_ops` 命名空间未和现有 protected 模块对齐，二是 EXE 工程未把 `win32injector.cpp` 编进来；两处都已修平并验证构建闭环恢复正常。

### Reason
- 这一轮的目的不是继续堆骨架，而是把“本地授权三件套已经活了，但本地票据/结果/宿主注入/更新边带/保护面还断着”的状态真正收口。现在本地 V3 至少已经从“很多动作能返回 JSON”推进到“宿主、Auth DLL、票据文件、结果文件、反重放缓存、更新快照都开始按同一套契约协同工作”。

## EXE v3.4.64 / DLL v3.4.46 - 2026年06月04日 12:53
### Changes
- `src__auth_dll/src/protected/protected_ticket_ops.*`, `tools__project_helpers/vmp_plan/NoteBotAuth.protect.tsv`, `src__auth_dll/CMakeLists.txt`: added the first V3 local ticket protected helpers and registered them in the VMP protection table. The Auth DLL now has protected wrapper-key derivation and protected ticket-sha256 derivation instead of leaving those steps as pure planning text.
- `src__auth_dll/src/v3/v3_state.h`, `src__auth_dll/src/v3/v3_state.cpp`: added the first real local ticket/result state to `StateManager`. The DLL can now issue a local inject ticket file, derive a wrapper key, write `inject_ticket_v3.dat`, keep a pending ticket context in memory, read `inject_result_v3.dat`, verify `result_hmac`, delete consumed local result/ticket files, and clear/reset the pending ticket state.
- `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/api.cpp`: replaced more V3 placeholders with real local actions. `dll_policy_v3` now returns the current cached DLL policy from the verified local state, `issue_inject_ticket_v3` now writes a local wrapper file through `nb_call`, `consume_inject_result_v3` now consumes a local result file through `nb_call`, and `nb_diagnose()` now bridges to the local V3 DLL policy path instead of always returning a pure stub failure.
- `src__injector_exe/backend.cpp`: updated the EXE-side diagnose logging so the current launcher reads the V3-style `data.dll_name` / `data.dll_sha256` payload instead of the older placeholder shape.
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: bumped the EXE-facing source version to `3.4.64` and the Auth DLL version surfaces to `3.4.46`.
- Rebuilt both `NoteBotAuth.dll` and `NoteBotInjector.exe`. Then ran a local smoke path that temporarily materialized a verified `license_v3.dat`, confirmed `dll_policy_v3` returns cached V3 policy data, confirmed `issue_inject_ticket_v3` writes a local ticket through the `use_local_stub` path, and confirmed `consume_inject_result_v3` accepts a locally HMAC-signed `inject_result_v3.dat` and deletes the consumed files.

### Reason
- The previous state had a live authorization/check/heartbeat chain, but the entire local ticket/result side was still empty. This round starts turning the V3 inject path into a real client-side file protocol instead of a comment placeholder, while still staying honest about the missing server ticket issuance and game-DLL consumption halves.

## EXE v3.4.63 / DLL v3.4.43 - 2026年06月04日 12:01
### Changes
- `AUTH_V3_REWRITE_SPEC.md`: 修订了 V3 规格书里当前最容易打架的契约点，明确 `get_status_snapshot` 的正式返回固定为 `data.status_snapshot`，顶层 `status_snapshot` 只保留兼容镜像；同步修正状态快照样本字段、`license_v3.dat` 样本字段、生命周期状态定义，以及 `activate_device_v3 / device_check_v3 / device_heartbeat_v3` 的状态规则。
- `src__auth_dll/src/v3/v3_state.cpp`, `src__auth_dll/src/v3/v3_state.h`: 收紧 `status_snapshot` 的正式字段面，去掉旧兼容字段参与正式输出；修复 `checkWithServer()` 里递归拿同一把锁导致的自锁问题，把本地状态检查拆成可重入的 locked 版本；保持 `license_v3.dat`、设备身份、DPAPI 私钥、签名链继续走 V3 本地状态主线。
- `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/api.cpp`: 让旧导出兼容壳真正转接到 V3 联网动作，`nb_activate()` 走 `activateWithServer()`，`nb_verify()` 走 `checkWithServer()`；同时给 `activate_device_v3` 的 `data` 补齐 `tier_name / tier_value / status_snapshot`，让动作返回更贴近规格书第 36 节。
- `src__injector_exe/backend.cpp`: EXE 改成优先读取 `data.status_snapshot`，不再把顶层兼容镜像当正式契约；状态消费改读 `tier_name` 等 V3 字段；初始化阶段改成先同步本地 V3 状态快照，不再把旧 `verify` 语义当作“本地授权检查”；激活和检查日志也改成 V3 `rc/message` 映射。
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__injector_exe/backend.cpp`: 版本面统一前推到 `EXE v3.4.63 / DLL v3.4.43`。
- 关闭了占用中的 `build__injector_exe_cache\\NoteBotInjector.exe`，重新构建通过了 `NoteBotAuth.dll` 和 `NoteBotInjector.exe`；随后对 `NoteBotAuth.dll` 做了 smoke，确认 `nb_init`、`nb_get_protocol_version`、`nb_get_abi_version`、`ping`、`get_status_snapshot`、`activate_device_v3`、`device_check_v3` 都能返回 V3 形状的 JSON。当前本机 smoke 下，`activate_device_v3` 会收到服务端拒绝并返回 `rc=1007`，`device_check_v3` 在未完成联网激活时也会稳定返回 `rc=1007`，不再异常卡死。

### Reason
- 当前 V3 最大的问题不是“完全没骨架”，而是“骨架已经有了，但正文规范、兼容壳、EXE 消费面、实际回包口型彼此还在打架”。这一轮先把文档和当前客户端主链收拢到同一套语言上，再把联网三件套推进到真正可构建、可 smoke、不会自锁的状态，为后续票据链、更新链和服务端 V3 继续往下铲打地基。

## EXE v3.4.62 / DLL v3.4.41 - 2026年06月04日 09:31
### Changes
- `src__auth_dll/src/v3/v3_state.h`, `src__auth_dll/src/v3/v3_state.cpp`: turned the V3 local state from a read-only shell into a real local identity layer. The DLL now creates and persists `license_v3.dat`, derives `key_id` / `device_id`, stores a machine-DPAPI protected private-key blob, fixes the V3 local paths to `%LOCALAPPDATA%\\NoteBotInjector\\state_v3`, and recreates `consumed_tickets_v3.json` with the V3 retention window metadata.
- `src__auth_dll/src/v3/v3_actions.cpp`, `src__auth_dll/src/api.cpp`: aligned the V3 action envelope around `ok/action/rc/message/data`, expanded `get_status_snapshot` into the V3-style snapshot fields, restored `name_get_config` / `name_set_enabled` / `name_save_config` / `name_apply_now`, and changed local activation/check so `activate_device_v3` now generates a pending local device identity instead of being a pure placeholder.
- `src__injector_exe/backend.cpp`, `src__auth_dll/qml/main.qml`: corrected the current check button path so the visible button calls `backend.verifyLicense()` and the EXE first checks whether `license_v3.dat` already exists before deciding between local device check and first activation.
- `tools__project_helpers/vmp_plan/NoteBotAuth.protect.tsv`, `tools__project_helpers/vmp_plan/*.ps1`, `src__auth_dll/CMakeLists.txt`, `build_clean.bat`: added the checked-in Auth DLL VMP protection table, switched Auth VMP selection generation to explicit table-driven matching, taught the smoke/export checks about `nb_get_protocol_version` / `nb_get_abi_version` / `get_status_snapshot`, and added the CMake / batch toggles for `NB_AUTH_ENABLE_VMP_MARKERS` and `NB_VMP_SDK_DIR`.
- `CMakeLists.txt`, `src__injector_exe/backend.cpp`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/v3/v3_state.cpp`, `src__auth_dll/src/v3/v3_actions.cpp`: bumped the EXE-facing source version to `3.4.62` and the Auth DLL version surfaces to `3.4.41`.
- Rebuilt `NoteBotAuth.dll` successfully and smoke-tested `nb_init`, `nb_get_protocol_version`, `nb_get_abi_version`, `ping`, `get_status_snapshot`, and a local `activate_device_v3` -> `license_v3.dat` write path. A full EXE rebuild was attempted, but the linker could not replace `build__injector_exe_cache\\NoteBotInjector.exe` because a running `NoteBotInjector.exe` process was holding that file open.

### Reason
- The old V3 layer could only describe the intended local state; it could not actually materialize a device identity, persist a proper `license_v3.dat`, or expose the V3 snapshot contract cleanly. This round turns the DLL into a usable local V3 state core while also tightening the VMP planning chain around explicit protected-function ownership.

## EXE v3.4.61 / DLL v3.4.40 - 2026年06月04日 00:27
### Changes
- `src__injector_exe/backend.cpp`: changed the target-process missing red error from English to Chinese.
- `src__injector_exe/win32injector.cpp`: changed legacy direct-inject red error messages to Chinese while keeping WinAPI names intact.
- `src__auth_dll/src/api.cpp`: fixed the mojibake red inject errors to proper Chinese: not activated and invalid/missing DLL path.
- `CMakeLists.txt`, `src__injector_exe/backend.cpp`: bumped the EXE-facing version to `3.4.61`.
- `src__auth_dll/CMakeLists.txt`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/auth/cloud_client_tcp.cpp`, `src__auth_dll/qml/main.qml`, `src__auth_dll/src/qml_embedded.h`: bumped Auth DLL version surfaces to `3.4.40`.
- Rebuilt both `NoteBotInjector.exe` and `NoteBotAuth.dll`.

### Reason
- The current red error list still had two mojibake strings and several EXE-side English messages. This build fixes those while deliberately leaving the DLL injection-button flow logs unchanged.

## EXE v3.4.60 / DLL v3.4.39 - 2026年06月04日 00:18
### Changes
- `src__injector_exe/logmodel.cpp`: raised the visible UI log retention window from 300 rows to 10000 rows, with old rows still pruned one at a time from the head.
- `src__auth_dll/qml/main.qml`, `src__auth_dll/src/qml_embedded.h`: changed the main log view so it only auto-scrolls when it is already pinned to the bottom; scrolling upward now pauses auto-follow instead of yanking the view back down.
- `CMakeLists.txt`, `src__injector_exe/backend.cpp`: bumped the EXE-facing version to `3.4.60`.
- `src__auth_dll/CMakeLists.txt`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/auth/cloud_client_tcp.cpp`, `src__auth_dll/qml/main.qml`, `src__auth_dll/src/qml_embedded.h`: bumped Auth DLL version surfaces to `3.4.39`.
- Rebuilt both `NoteBotInjector.exe` and `NoteBotAuth.dll`.

### Reason
- The log panel was behaving like a short 300-row queue. After long runtimes, older rows were physically removed from the model, and the DLL log view also forced itself back to the bottom on every content-height/count change. This build keeps a much larger scrollback window and preserves manual upward scrolling while new log lines arrive.

## EXE v3.4.59 / DLL v3.4.38 - 2026年05月29日 04:18
### Changes
- `tools__project_helpers/build/filter_map_dll.py`: fixed Auth DLL map filtering so every build preserves the latest full linker map as `build__auth_dll_cache/NoteBotAuth.full.latest.map`.
- `tools__project_helpers/build/filter_map_dll.py`: added the protected helper object files to the official filtered map and keeps `NBVmp_*` symbols from the real linker map instead of relying on hand-entered `.vmp` names.
- `src__auth_dll/CMakeLists.txt`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/auth/cloud_client_tcp.cpp`, `src__auth_dll/qml/main.qml`, `src__auth_dll/src/qml_embedded.h`: bumped Auth DLL version surfaces to `3.4.38`.
- Rebuilt only `NoteBotAuth.dll`; EXE remains `3.4.59`.

### Reason
- VMProtect selection must come from real map rows with real linked addresses. The previous map filtering discarded the protected helper object files, which forced unsafe manual `.vmp` entries and could make VMProtect fail to resolve or crash.

## EXE v3.4.59 / DLL v3.4.37 - 2026年05月29日 04:14
### Changes
- `src__auth_dll/CMakeLists.txt`: changed the Auth DLL linker map configuration to emit a VMP-friendly map with debug symbols preserved and COMDAT folding/reference stripping disabled for protected helper discovery.
- `src__auth_dll/CMakeLists.txt`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/auth/cloud_client_tcp.cpp`, `src__auth_dll/qml/main.qml`, `src__auth_dll/src/qml_embedded.h`: bumped Auth DLL version surfaces to `3.4.37`.
- Rebuilt only `NoteBotAuth.dll`; EXE remains `3.4.59`.

### Reason
- The VMP workflow must use real `.map` symbols and addresses. Hand-entered `.vmp` functions can crash or fail to resolve, so this build starts moving Auth DLL protection back to a map-driven flow.

## EXE v3.4.59 / DLL v3.4.35 - 2026年05月28日 23:49
### Changes
- `src__auth_dll/src/crypto/vmp_defs.h`: added no-inline and named VMP marker macros so normal builds stay no-op while `NB_VMP_BUILD` can expose stable VMProtect regions later.
- `src__auth_dll/src/protected/*`: added protected Auth DLL helper modules for auth token decisions, signed DLL metadata parsing, download integrity checks, injection planning, shared auth input building, and custom-name patch byte generation.
- `src__auth_dll/src/api.cpp`, `src__auth_dll/src/auth/cloud_client_tcp.cpp`, `src__auth_dll/src/name_injector.cpp`: moved sensitive decisions into `NBVmp_*` helpers while keeping exported API names, TCP actions, HTTP signed-URL download flow, and WinAPI outer calls compatible.
- `tools__project_helpers/vmp_plan/*.ps1`: updated VMP automation to prefer `NBVmp_` map symbols, support mutation entries, and skip the old large exported wrapper functions for Auth DLL protection.
- `src__auth_dll/CMakeLists.txt`, `src__auth_dll/qml/main.qml`, `src__auth_dll/src/qml_embedded.h`: bumped Auth DLL version surfaces to `3.4.35`.
- Rebuilt only `NoteBotAuth.dll`; EXE remains `3.4.59`.

### Reason
- Auth DLL needs a cleaner shape before VMProtect automation. Large exported wrappers are too coarse and risky to protect directly, so this build splits the sensitive logic into stable `NBVmp_*` functions that can be individually selected for mutation, virtualization, or stronger protection without changing the public client protocol.

## EXE v3.4.59 / DLL v3.4.34 - 2026年05月28日 23:06
### Changes
- `src__auth_dll/qml/main.qml`, `src__auth_dll/src/qml_embedded.h`: restored the visible injector UI wording around process count, process list actions, custom-name editor, and injection button states back to English.
- `src__auth_dll/src/name_injector.cpp`: restored custom-name result and error messages back to English.
- `src__auth_dll/src/api.cpp`, `src__auth_dll/src/auth/cloud_client_tcp.cpp`, `src__auth_dll/CMakeLists.txt`: bumped Auth DLL version surfaces to `3.4.34`.
- Rebuilt only `NoteBotAuth.dll`; EXE remains `3.4.59`.

### Reason
- The Chinese UI labels looked visually worse in the compact HUD layout. This build keeps the requested launcher/process/custom-name/cooldown wording in English and fixes the remaining custom-name messages that could still appear in the log/status area.

## EXE v3.4.59 / DLL v3.4.33 - 2026年05月28日 23:02
### Changes
- `CMakeLists.txt`, `src__injector_exe/backend.cpp`: bumped the EXE-facing version to `3.4.59`.
- `src__auth_dll/CMakeLists.txt`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/auth/cloud_client_tcp.cpp`, `src__auth_dll/qml/main.qml`, `src__auth_dll/src/qml_embedded.h`: bumped Auth DLL version surfaces to `3.4.33`.
- `src__auth_dll/qml/main.qml`, `src__injector_exe/backend.cpp`, `src__auth_dll/src/api.cpp`: restored the first batch of injector/process/custom-name/cooldown labels and logs back to English.
- Rebuilt both `NoteBotInjector.exe` and `NoteBotAuth.dll`.

### Reason
- This was the first pass for reverting the selected Chinese UI labels back to English after the localization pass made the UI feel worse.

## EXE v3.4.58 / DLL v3.4.32 - 2026年05月28日 22:25
### Changes
- `src__injector_exe/updater.cpp`: fixed the Auth DLL signed-URL refresh branch. HTTP 401/403 now forces a fresh `auth_dll_bootstrap` ticket on the next retry instead of reusing the expired URL.
- `CMakeLists.txt`, `src__injector_exe/backend.cpp`: bumped the EXE-facing version to `3.4.58`. Auth DLL remains `3.4.32`.
- Rebuilt only `NoteBotInjector.exe`; `NoteBotAuth.dll` was not rebuilt again.

### Reason
- After the successful `3.4.57` build, the retry code still had one edge case where an expired URL could be retried without renewing the ticket. This build makes URL expiry recovery match the intended two-step flow: service ticket first, direct HTTP download second.

## EXE v3.4.57 / DLL v3.4.32 - 2026年05月28日 21:36
### Changes
- `src__injector_exe/updater.cpp/h`: rebuilt the EXE Auth DLL updater into a bootstrap flow. Startup now sends one encrypted `auth_dll_bootstrap` request to `notebot-api.fucku.top`, receives Auth DLL metadata plus a short-lived signed URL, then downloads directly from that URL.
- `src__injector_exe/updater.cpp`: switched EXE Auth DLL HTTP download to WinHTTP with `WINHTTP_ACCESS_TYPE_NO_PROXY`, validates size, SHA-256 and MD5, logs only the URL host, and retries up to 3 times. If the signed URL is expired or returns 401/403, it requests a fresh ticket before retrying.
- `src__injector_exe/backend.cpp`: changed startup update handling to compare the local main/cache Auth DLL by SHA-256 first and MD5 second, then sync only one verified DLL into `%LOCALAPPDATA%\NoteBotInjector\NoteBotAuth.dll`.
- `src__auth_dll/src/auth/cloud_client_tcp.cpp`: changed the Auth DLL WinHTTP downloader to `WINHTTP_ACCESS_TYPE_NO_PROXY`, so overlay/Auth DLL HTTP downloads do not use the Windows system proxy.
- Server `/home/william/NoteBot-API/src/handler.js`: added `auth_dll_bootstrap`, reusing the existing HMAC token check and returning `download_mode=url`, `download_url`, `expires_in`, `dll_name`, `dll_md5`, `dll_sha256`, and `dll_size` in one service response.
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`, `src__auth_dll/src/api.cpp`, `src__auth_dll/qml/main.qml`, `src__auth_dll/src/qml_embedded.h`: bumped EXE to `3.4.57`, Auth DLL to `3.4.32`, and regenerated embedded QML.
- Rebuilt both `NoteBotInjector.exe` and `NoteBotAuth.dll`.

### Reason
- The EXE should be a long-lived bootstrapper, not a place where server business protocol keeps growing. The new boundary keeps EXE startup to one signed-ticket request plus one direct no-proxy HTTP download, while Auth DLL keeps owning authorization, diagnosis, overlay download, and injection.

## EXE v3.4.56 / DLL v3.4.31 - 2026-05-28 20:30
### Changes
- `src__auth_dll/src/auth/cloud_client_tcp.cpp`: replaced the Auth DLL HTTP download path with synchronous WinHTTP. Overlay/auth DLL downloads no longer depend on `QNetworkAccessManager` inside a worker `std::thread`, fixing the stall after `download_url host=...`.
- `src__auth_dll/src/auth/cloud_client_tcp.cpp`: progress now advances from the server-provided `dll_size`, so the progress bar is updated while bytes are being read instead of sitting at 0.
- `src__auth_dll/qml/main.qml`, `src__auth_dll/src/qml_embedded.h`: changed the user-facing DLL UI labels back to Chinese and regenerated the embedded QML.
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`: enabled MSVC `/utf-8`; bumped EXE to `3.4.56` and Auth DLL to `3.4.31`.
- Rebuilt both `NoteBotInjector.exe` and `NoteBotAuth.dll`.

### Reason
- Local curl proved the signed Qiniu URL downloads correctly, so the stuck progress was in the client HTTP implementation, not the server file. The DLL was doing async Qt networking from a normal worker thread and could hang before `http status` was logged. The English UI came from the current DLL QML text plus stale embedded QML, so both source QML and embedded QML were corrected.

## v3.4.55 / DLL v3.4.30 - 2026-05-28 20:28
### Changes
- `src__injector_exe/backend.cpp`: extended the stale-DLL guard to the HTTP download failure branch. Once the server reports an auth DLL update, a failed download now stops initialization instead of falling through to the old local DLL.
- `CMakeLists.txt`, `src__injector_exe/updater.cpp`: bumped EXE version and User-Agent to `3.4.55`.
- Rebuilt only `NoteBotInjector.exe`; `NoteBotAuth.dll` remains `3.4.30`.

### Reason
- The server rejects stale auth DLL hashes before `auth_v2`, `diagnose_v2`, and `inject_report`. If the client knows an update is required but cannot download it, loading the old DLL only leads to the same `upgrade_required` failure later.

## v3.4.54 / DLL v3.4.30 - 2026-05-28 20:24
### Changes
- `src__injector_exe/backend.cpp`: fixed the remaining stale-DLL path. When the downloaded auth DLL cannot replace `%LOCALAPPDATA%\NoteBotInjector\NoteBotAuth.dll`, initialization now stops in-place and emits `initializationFailed()` instead of calling `finishInitialization()`.
- `CMakeLists.txt`, `src__injector_exe/updater.cpp`: bumped EXE version and User-Agent to `3.4.54`.
- Rebuilt only `NoteBotInjector.exe`; `NoteBotAuth.dll` remains `3.4.30`.

### Reason
- Remote server verification showed `diagnose_v2`, `auth_v2`, and `inject_report` all reject stale auth DLL hashes through `checkAuthDllPolicy()`. The previous build still had one failure branch that could continue into `loadAuthDll()` after a sync failure, so this build makes that branch truly stop.

## v3.4.53 / DLL v3.4.30 - 2026-05-28 20:10
### Changes
- `src__injector_exe/main.cpp`: added a single-instance guard with `Local\NoteBotInjector.SingleInstance`, preventing multiple launcher windows from locking the local auth DLL at the same time.
- `src__injector_exe/backend.cpp`: if the downloaded `NoteBotAuth.dll` cannot replace `%LOCALAPPDATA%\NoteBotInjector\NoteBotAuth.dll`, startup now stops instead of loading the stale DLL.
- `CMakeLists.txt`, `src__injector_exe/updater.cpp`: bumped EXE version and User-Agent to `3.4.53`.
- Rebuilt only `NoteBotInjector.exe`; `NoteBotAuth.dll` remains `3.4.30` and still lives only under `build__auth_dll_cache`.

### Reason
- The client had downloaded the server-required auth DLL, but the main local DLL path was locked by another running injector instance. The old code logged the sync failure and then continued loading the stale DLL, so `diagnose_v2` reported `upgrade_required`.

## v3.4.52 / DLL v3.4.30 - 2026-05-28 19:55
### Changes
- `CMakeLists.txt`, `src__auth_dll/CMakeLists.txt`: bumped `NoteBotInjector` to `3.4.52` and `NoteBotAuth` to `3.4.30`, then rebuilt through the renamed official cache/output layout.
- `src__injector_exe/`, `src__auth_dll/`, `qml__injector_exe/`, `resources__injector_exe/`: kept the explicit folder naming scheme so each directory says whether it belongs to the EXE, DLL, QML fallback, or EXE resources.
- `build_clean.bat`, `build_dll.bat`, `build_all.bat`, `tools__project_helpers/build/*`: verified the new `build__injector_exe_cache`, `build__auth_dll_cache`, and `dist__release_artifacts` paths by running a clean build.
- `src__auth_dll/qml/main.qml`, `src__auth_dll/src/qml_embedded.h`, `src__auth_dll/src/api.cpp`, `src__auth_dll/src/auth/cloud_client_tcp.cpp`: aligned DLL-facing version strings to `3.4.30`.

### Reason
- The root directory names were still too vague after the first cleanup. This build locks in the clearer `component__purpose` layout and proves the renamed source, resource, cache, helper, and release-artifact paths actually build.

## v3.4.51 / DLL v3.4.29 - 2026-05-28 18:42
### Changes
- `src/updater.cpp`: migrated the EXE-side `NoteBotAuth.dll` updater from old TCP-appended DLL bytes to the new signed `download_url` HTTP GET flow.
- `src/updater.cpp`: added retry-on-failure behavior that requests a fresh token and signed URL on each attempt, then validates downloaded size, SHA-256, and MD5 before writing with `QSaveFile`.
- `src/updater.cpp`: logs only the download URL host, HTTP status, downloaded size, and hash comparison values, avoiding full temporary token URLs.
- `CMakeLists.txt`: bumped `NoteBotInjector` from `3.4.50` to `3.4.51`.

### Reason
- The server now returns encrypted JSON metadata only for `dll_download_secure`; it no longer appends the 4-byte DLL length plus raw DLL bytes. The EXE updater still waited for that old TCP payload before loading `NoteBotAuth.dll`, so startup failed at 100% with `等待 DLL 数据超时`.

## v3.4.50 / DLL v3.4.29 - 2026-05-28 18:30
### Changes
- `auth_dll/CMakeLists.txt`, `auth_dll/src/api.cpp`, `auth_dll/src/auth/cloud_client_tcp.cpp`, `auth_dll/qml/main.qml`, `auth_dll/src/qml_embedded.h`: bumped the DLL-facing version from `3.4.28` to `3.4.29`.
- Rebuilt the DLL through the normal `C:\NB\build_dll` output path instead of the temporary `auth_dll\build_dll` test path.
- Kept the signed-URL DLL download migration from `3.4.28`: TCP fetches encrypted JSON metadata only, then HTTP downloads from `download_url` with size, SHA-256, and MD5 verification.

### Reason
- The previous build used the right source changes but landed first in the test build directory. This rebuild corrects the official artifact location so downstream packaging and upload scripts read `C:\NB\build_dll\NoteBotAuth.dll`.

## v3.4.50 / DLL v3.4.28 - 2026-05-28 18:16
### Changes
- `auth_dll/src/auth/cloud_client_tcp.cpp`: rebuilt both overlay DLL and auth DLL download paths around the new server contract: TCP now only fetches encrypted JSON metadata, then the client downloads through the server-signed `download_url`.
- `auth_dll/src/auth/cloud_client_tcp.cpp`: added HTTP GET retry handling that refreshes the signed URL on timeout, HTTP failure, expired token responses, size mismatch, SHA-256 mismatch, or MD5 mismatch; full token URLs are no longer printed to logs.
- `auth_dll/CMakeLists.txt`, `auth_dll/src/api.cpp`, `auth_dll/src/auth/cloud_client_tcp.cpp`, `auth_dll/qml/main.qml`, `auth_dll/src/qml_embedded.h`: bumped DLL-facing version strings from `3.4.27` to `3.4.28` and regenerated embedded QML.

### Reason
- The server no longer appends raw DLL bytes after the TCP JSON response. The old client kept waiting for a 4-byte DLL length and cancelled injection after receiving valid metadata, so downloads had to move to signed Qiniu HTTP URLs with local size/hash verification.

## v3.4.50 / DLL v3.4.27 - 2026-05-18 00:08
### Changes
- `qml/main.qml`: restored the bootstrap shell to the original fixed-anchor splash layout instead of centering the whole logo/progress/log block as one column.
- `src/backend.cpp`: removed the long 400ms startup delay, added immediate bootstrap log/status entries, and starts the update check after a short first-frame delay.
- `CMakeLists.txt`: EXE version `3.4.49` -> `3.4.50`.

### Reason
- The temporary EXE-only shell felt shifted upward because the log panel participated in column centering. Logs also appeared late because initialization intentionally waited before writing update status.

## v3.4.49 / DLL v3.4.27 - 2026-05-17 23:35
### Changes
- `qml/main.qml`: removed the EXE-embedded business UI completely. The EXE now only contains a bootstrap/update shell with logo, status text, thin progress bar, and mini log.
- Removed EXE QML startup calls that could trigger scanning or reveal the old Inject/process/license layout before the DLL UI takes over.
- `CMakeLists.txt`: EXE version `3.4.48` -> `3.4.49`.

### Reason
- The EXE should not own product UI or injection controls. Its job is to launch, update, load the DLL, and get out of the DLL's way.

## v3.4.48 / DLL v3.4.27 - 2026-05-17 23:25
### Changes
- `src/main.cpp`: hides the old EXE bootstrap window before loading DLL QML, and restores it only if DLL QML loading fails.
- `auth_dll/qml/main.qml` and `qml/main.qml`: splash visibility now requires `backend.initializing`, preventing an already-finished DLL UI handoff from showing a second splash frame.
- `CMakeLists.txt`: EXE version `3.4.47` -> `3.4.48`.

### Reason
- Loading DLL QML creates a new root `Window`. The old handoff briefly showed both old and new windows, causing overlapping animation and a second window-like layer.

## v3.4.47 / DLL v3.4.27 - 2026-05-17 22:58
### Changes
- `src/backend.cpp`: the DLL QML handoff now happens after `initializing` is cleared and download progress is reset, so the DLL UI no longer re-enters the splash/download state.
- `auth_dll/qml/main.qml` and `qml/main.qml`: added a `Component.onCompleted` fallback so if the DLL UI is loaded after initialization finishes, it jumps straight into the main UI instead of replaying the splash.
- `CMakeLists.txt`: EXE version `3.4.46` -> `3.4.47`.

### Reason
- The previous handoff let the new DLL QML observe stale initialization/download state and flash back to the big progress splash. This version keeps the visible transition clean.

## v3.4.46 / DLL v3.4.27 - 2026-05-17 22:48
### Changes
- `src/main.cpp`: removed startup-time `loadAuthDll()` and now always boots with the EXE embedded QML first.
- `src/backend.h/.cpp`: added `dllUiReady(qml)` so the EXE switches to DLL-owned QML only after update check, sync, and DLL loading finish.
- `src/backend.cpp`: removed the `%LOCALAPPDATA%/NoteBotInjector/dlls/NoteBotAuth.dll` load priority. Normal runtime loading now uses one primary DLL path: `%LOCALAPPDATA%/NoteBotInjector/NoteBotAuth.dll`, then the EXE directory fallback.
- `src/backend.cpp`: update sync is expected to happen before `LoadLibrary`, so copy failures are logged as real sync errors instead of hidden behind the old cache-next-launch workaround.
- `CMakeLists.txt`: EXE version `3.4.45` -> `3.4.46`.

### Reason
- The previous DLL-owned QML migration loaded `NoteBotAuth.dll` too early, which locked the DLL before the updater tried to replace it. The EXE is now a cleaner bootstrap/updater shell: show built-in UI, check/update DLL, load the single prepared DLL, then hand the visible layout to DLL QML.

## v3.4.45 / DLL v3.4.27 - 2026-05-17 22:38
### Changes
- `CMakeLists.txt`: EXE version `3.4.44` -> `3.4.45`.
- `auth_dll/CMakeLists.txt` / `auth_dll/src/api.cpp`: DLL version `3.4.26` -> `3.4.27`.
- Rebuilt EXE and DLL after reverting the splash progress bar to the original thin style.

### Reason
- Keeps the user-facing progress bar appearance unchanged while preserving real download progress reporting during slow DLL updates.

## v3.4.44 / DLL v3.4.26 - 2026-05-17 22:20
### Changes
- `src/backend.cpp`: `loadAuthDll()` now prefers `%LOCALAPPDATA%/NoteBotInjector/dlls/NoteBotAuth.dll` before the main copied DLL path, so a downloaded cached DLL can be used on the next launch even if the main path is locked.
- `src/backend.cpp`: update sync no longer logs an error when the main DLL path is locked by the running process; it keeps the cached DLL and reports that it will take effect on next launch.
- `auth_dll/qml/main.qml` and `qml/main.qml`: reverted the splash progress bar appearance to the original thin `200x3` style while keeping the real download-progress binding.
- `CMakeLists.txt`: EXE version `3.4.43` -> `3.4.44`.
- `auth_dll/CMakeLists.txt` / `auth_dll/src/api.cpp`: DLL version `3.4.25` -> `3.4.26`.

### Reason
- The EXE loads `NoteBotAuth.dll` early for DLL-owned QML, so copying over the same main DLL file during update can fail due to the file being locked. Loading from the cached `dlls/` copy avoids the repeated update/copy failure loop.
## v3.4.43 - 2026-05-17 21:57
### Changes
- `auth_dll/qml/main.qml` and `qml/main.qml`: startup splash progress now switches to real `backend.downloadProgress` while DLL updates are downloading.
- The startup progress bar is wider and thicker (`320x8`), includes a subtle moving highlight during download, and shows `正在下载 DLL  xx%` instead of looking stuck at the generic init step.
- `src/backend.cpp`: sets `downloadProgress` to `0` before a required DLL download starts and `100` after the download completes, so slow downloads visibly move from the beginning.
- `CMakeLists.txt`: EXE version `3.4.42` -> `3.4.43`.
- `auth_dll/CMakeLists.txt` / `auth_dll/src/api.cpp`: DLL version normalized forward to `3.4.25` because `nb_get_version()` was already ahead of the CMake value.

### Reason
- Slow DLL downloads previously held the startup bar at the coarse init step, making the app look frozen. The splash now gives clear download feedback from 0% to 100%.
## v3.4.42 - 2026-05-17 21:28
### Changes
- `auth_dll/include/notebot_auth.h`: added the generic `nb_call(action, json, out_json, out_size)` DLL bridge.
- `auth_dll/src/api.cpp`: implemented `nb_call` with JSON responses for baseline actions: `ping`, `get_status`, `get_version`, `set_key`, `get_key`, `verify`, `activate`, `diagnose`, `reset`, `download`, `download_secure`, `inject`, `inject_async`, `self_check`, `log`, and `state`.
- `src/backend.h/.cpp`: exposed `backend.callDll(action, json)`, `backend.callDllAsync(action, json)`, and `backend.appendLog(msg)` to QML.
- `src/backend.cpp`: resolves `nb_call` as an optional DLL export so older DLLs can still load, while newer DLLs get the generic bridge.
- `CMakeLists.txt`: EXE version `3.4.41` -> `3.4.42`.
- `auth_dll/CMakeLists.txt`: DLL version `3.4.21` -> `3.4.22`.

### Reason
- This turns the EXE into a long-lived shell: future DLL-owned QML can add buttons and actions by calling `backend.callDll(...)` without requiring a new EXE for each new feature.
## v3.4.41 - 2026-05-17 20:45
### Changes
- `src/main.cpp`: load UI QML from `NoteBotAuth.dll` first through `backend.getQmlFromDll()` and `engine.loadData()`.
- Fallback remains `qrc:/qml/main.qml` if DLL QML is unavailable, so the EXE still has a built-in recovery UI.
- `auth_dll/qml/main.qml`: synced with the current EXE `qml/main.qml` so this migration keeps the visible layout unchanged.
- `CMakeLists.txt`: EXE version `3.4.40` -> `3.4.41`.
- `auth_dll/CMakeLists.txt`: DLL version `3.4.20` -> `3.4.21`.

### Reason
- This makes the EXE a stable shell while the DLL owns the main QML layout, allowing future layout updates through `NoteBotAuth.dll` without replacing the EXE.

## v3.4.40 — 2026年05月12日

### 变更内容
- **拆分 `win32injector.cpp`：EXE 进程管理 vs DLL 注入逻辑彻底分离**
  - 新建 `src/win32process.h` + `src/win32process.cpp`：负责 `findAllProcesses()`、`getWindowForPid()`、`bringToFront()`（EXE 专用）
  - `src/win32injector.cpp` 只保留 `injectDll()`（CreateRemoteThread + LoadLibraryW），供 DLL 侧调用
  - `src/backend.cpp`：改 `#include "win32injector.h"` 为 `#include "win32process.h"`，所有 `Win32Injector::` 调用改为 `Win32Process::`
  - `CMakeLists.txt`（EXE）：SOURCES 替换 `src/win32injector.cpp` 为 `src/win32process.cpp`；HEADERS 新增 `src/win32process.h`
- **修复 DLL 日志回调 `s_dllLogCallback`**，DLL 内部日志正确转发到 UI 面板
  - 通过 `s_backendInstance` + `QMetaObject::invokeMethod` + `emit _logSignal()` 异步发回主线程
  - 解决之前日志只存变量不发 UI 的问题
- **移除 `LoadLibraryExW` 预加载测试**，修复 VMP 加壳 DLL 注入失败
  - 原代码在 `CreateRemoteThread` 前用 `LOAD_LIBRARY_AS_DATAFILE` 预读 PE 头，被 VMP 壳检测到映射行为后拒绝加载
  - 删掉预加载，只保留 PE machine 检查（读文件句柄而非映射），VMP DLL 注入恢复正常
- **修复 `build_fix.py` 的 `shlex.split()` 反斜杠转义问题**
  - 默认 `posix=True` 把 Windows 路径 `C:\QtBuild\...` 的 `\` 当转义字符吃掉，导致 LNK1181
  - 改为 `shlex.split(content, posix=False)`，保留反斜杠分隔符
- **修复 `build_fast.bat` 的 errorlevel 检测**
  - `build_fix.py` 成功但 `sys.exit(4294967295)` 在 cmd 里变成非零 errorlevel，bat 错误报告失败
  - 改为直接检查 `build\NoteBotInjector.exe` 文件存在性，不再依赖 `%errorlevel%`
- `CMakeLists.txt`：版本号 v3.4.37 → v3.4.40

### 根因 / 背景
- EXE 和 DLL 编译分离后，改注入逻辑只需要重编 DLL，不需要重编 EXE，构建速度大幅提升
- VMP 壳会检测 `LoadLibraryExW(..., LOAD_LIBRARY_AS_DATAFILE)` 的内存映射行为并拒绝加载，旧版注入器没有这一步所以成功
- `shlex.split()` 是 POSIX shell 解析器，默认处理反斜杠为转义字符，不适用于 Windows 路径

## v3.4.37 — 2026年05月12日

### 变更内容
- **注入冷却时间从 30 秒缩短到 5 秒**
  - `auth_dll/src/api.cpp`：`nb_inject_async()` 中所有 cooldown 分支统一调整
  - 状态保持时间：3 秒 → 2 秒（让用户看到成功/失败提示）
  - 冷却等待时间：27 秒 → 3 秒
  - 总 cooldown：30 秒 → 5 秒
- `CMakeLists.txt`：版本号 v3.4.36 → v3.4.37

### 根因 / 背景
- 用户反馈注入失败后要等太久才能再次点击，30 秒冷却严重影响调试效率
- 5 秒足够看到结果提示，又不会让用户等得不耐烦

## v3.4.36 — 2026年05月12日

### 变更内容
- **新增 `build_fast.bat` 增量构建脚本**，日常小改动几秒完成
  - 不清 build/ 和 build_dll/ 目录，直接调用 ninja 增量编译
  - DLL 没改就跳过，EXE 只重编改动的那几个 cpp
  - 构建失败时自动调用 `build_fix.py` 修复 RSP 超长问题
- **修改 `build_now.bat`**，顶部加注释说明全量构建的适用场景
- `CMakeLists.txt`：版本号 v3.4.35 → v3.4.36

### 根因 / 背景
- 每次改一两个文件都要等几分钟，因为 build_now.bat 强制删掉 build/ 重新 cmake + 全量编译
- 增量构建利用 ninja 的依赖跟踪，只编译真正改动的文件，速度提升 10-50 倍
- 全量构建（build_now.bat）只在 CMakeLists.txt 改了、第一次构建、或缓存脏了的时候用

## v3.4.35 — 2026年05月12日

### 变更内容
- **修复 DLL 日志回调 `s_dllLogCallback`**，DLL 内部的日志现在能正确显示在 UI 日志面板上
  - `src/backend.cpp`：原实现只把日志存到 `g_dllLogBuffer` 变量，没有发到 UI
  - 改为通过 `s_backendInstance` + `QMetaObject::invokeMethod` + `emit _logSignal()` 将日志异步发回主线程
  - 同时把 `s_backendInstance` 的声明提前到 `s_dllLogCallback` 之前，避免未声明标识符错误
- `CMakeLists.txt`：版本号 v3.4.34 → v3.4.35

### 根因 / 背景
- 点击注入按钮后 DLL 内部的日志（如"未激活"、"诊断失败"、"DLL 版本为空"等）完全看不到，导致调试注入没反应的问题时毫无头绪
- 日志回调是个空壳，注释写"通过信号发回 UI"但根本没实现，属于明显的遗漏

## v3.4.33 — 2026年05月12日

### 变更内容
- **EXE 独立完成 TCP 版本检查和 DLL 下载**，完全不依赖 DLL
  - 新建 `src/updater.h` + `src/updater.cpp`，从 `cloud_client_tcp.cpp` 提取 `checkUpdate()` 和 `downloadDllSecure()` 核心逻辑
  - `CMakeLists.txt`：新增 `src/updater.cpp` 和 `auth_dll/src/crypto/crypto_utils.cpp` 到 SOURCES，添加 `auth_dll/src/crypto` 到 include path
- **调整启动顺序**：检查更新/下载 → 加载 DLL → 本地验证
  - `initializeAsync()` 不再先 `loadAuthDll()`，直接调用 `checkDllUpdateAsync()`
  - `checkDllUpdateAsyncInternal()` 改用 `Updater` 类代替 `m_funcs->fn_check_update` / `m_funcs->fn_download_secure`
  - `finishInitialization()` 修改：如果 `m_funcs` 为 nullptr 也直接加载 DLL（不再是仅热替换）
- `CMakeLists.txt`：版本号 v3.4.32 → v3.4.33

### 根因 / 背景
- 阶段 1（v3.4.32）虽然 QML 移到了 EXE，但 `initializeAsync()` 仍然先 `loadAuthDll()` 再检查更新，违背了"EXE 先独立检查更新、最后才加载 DLL"的设计理念
- 阶段 2 把 TCP 通信逻辑（AES-GCM 加密、Token 验证、TCP 传输）完全抽到 EXE 侧，DLL 只在最后才被加载
- 实现真正的期望流程：显示 Splash → 检查更新 → 下载（如需）→ 加载 DLL → 运行

## v3.4.32 — 2026年05月12日

### 变更内容
- **QML 从 DLL 移到 EXE**，启动页面不再依赖 DLL
  - `qml/main.qml` 从 `auth_dll/qml/main.qml` 同步到 `C:/NB/qml/main.qml`
  - `app.qrc` 加入 `qml/main.qml`，由 EXE 自己打包
  - `main.cpp` 去掉前置 `loadAuthDll()` 和 `getQmlFromDll()`，直接加载 `qrc:/qml/main.qml`
- **调整启动顺序**：加载 DLL → 检查更新/下载 → 热替换 DLL → 本地验证
  - `initializeAsync()` 中先加载 DLL，再调用 `checkDllUpdateAsync()`
  - 新增 `finishInitialization()`，在更新检查完成后：卸载旧 DLL → 加载新 DLL → 本地验证
  - 下载新 DLL 后不再需要重启，自动热替换
- `src/backend.h`：新增 `finishInitialization()` 声明
- `CMakeLists.txt`：版本号 v3.4.31 → v3.4.32

### 根因 / 背景
- 之前 QML 内嵌在 DLL 中，EXE 必须先加载 DLL 才能显示窗口，违背了"EXE 包含所有依赖、先检查更新再加载 DLL"的设计理念
- 现在 Splash 页面完全不依赖 DLL，DLL 只在版本检查和下载阶段才被加载

## v3.4.31 — 2026年05月12日

### 变更内容
- `src/backend.cpp`：`initializeAsync()` 拆分 Splash 显示和真实进度逻辑
  - 先设置 `m_initializing = true` 让 Splash 淡入
  - 再用 `QTimer::singleShot(400)` 等淡入动画（350ms）完成后才开始真实进度
  - 解决之前 Splash 淡入和进度推进并行、用户看到时进度已过半的问题
- `src/main.cpp`：`singleShot` 延迟从 400ms 改回 0ms（延迟已集中到 backend.cpp 内部）
- `CMakeLists.txt`：版本号 v3.4.30 → v3.4.31

### 根因 / 背景
- v3.4.30 的改法只是把 `main.cpp` 的 singleShot 从 100ms 延到 400ms，但 `initializeAsync()` 一旦触发就同时做两件事：启动 Splash 淡入 + 开始真实逻辑
- 结果 Splash 还没完全显示出来，进度就已经走到 30 了
- 现在改成真正的顺序执行：Splash 淡入完成 → 才开始加载授权模块

## v3.4.30 — 2026年05月12日

### 变更内容
- `src/main.cpp`：延后初始化启动时机，等 Splash 淡入完成后再开始进度条增长
  - `QTimer::singleShot` 延迟从 100ms 改为 400ms
  - Splash 淡入动画时长 350ms，400ms 时动画已完成，进度条从零开始清晰可见
- `CMakeLists.txt`：版本号 v3.4.29 → v3.4.30

### 根因 / 背景
- 100ms 时 Splash 还没完全显示出来，进度条已经开始走了，用户看不到从零开始的过程
- 延后到 400ms，淡入动画结束后再启动真实初始化流程

## v3.4.29 — 2026年05月12日

### 变更内容
- `src/backend.cpp`：启动进度条改为从零平滑增长
  - `initializeAsync()` 初始进度从 5 改为 0，本地验证前跳到 15，检查更新前跳到 30
  - `checkDllUpdateAsyncInternal()` 增加中间状态步进：30 → 60 → 70 → 90 → 100
  - 每一步都伴随状态文字更新，用户能直观看到卡在哪个环节
- `CMakeLists.txt`：版本号 v3.4.28 → v3.4.29

### 根因 / 背景
- 之前进度条直接跳到 20、40、100，中间长时间没动静，用户以为卡死了
- 改为平滑递增后，视觉上始终在动，体验更好

## v3.4.28 — 2026年05月12日

### 变更内容
- `auth_dll/src/auth/offline_cache.h/.cpp`：离线缓存密钥从机器指纹改为 licenseKey 派生
  - `DeriveCacheKey()` 参数从 `FixedHash32 machineHash` 改为 `QString licenseKey`
  - 盐值从 `"NBAuthOfflineCacheV1"` 升级为 `"NBAuthOfflineCacheV2"`，避免旧缓存被误读
  - 实现从 `SHA256(salt || machineHash)` 改为 `SHA256(salt || licenseKeyUtf8)`
  - `SaveOfflineCache()` / `LoadOfflineCache()` 签名同步更新
- `auth_dll/src/auth/cloud_client_tcp.cpp`：`authenticateOffline()` 不再计算机器指纹
  - 去掉 `ComputeMachineHash()` 调用
  - 直接传 `licenseKey` 给 `LoadOfflineCache()`
- `auth_dll/src/auth/license_token.h/.cpp`：清理遗留机器码校验函数
  - 删除 `TokenIsValidForMachine()` 声明和实现
  - `LicenseToken::machineHash[32]` 字段保留（不改 wire format 长度）
- `auth_dll/CMakeLists.txt`：版本号 v3.4.19 → v3.4.20
- `CMakeLists.txt`：版本号 v3.4.27 → v3.4.28

### 根因 / 背景
- 离线缓存以前用机器指纹做加密密钥，用户换 CPU/主板/BIOS 时指纹会变，缓存解密失败，被迫重新联网激活，增加客服成本
- 改为 licenseKey 派生后，机器怎么变都不影响离线缓存的有效性
- 保留 `ComputeMachineHash()` 在联网激活中的调用（仅用于服务端日志记录）

## v3.4.27 — 2026年05月12日

### 变更内容
- `auth_dll/qml/main.qml`：修复 `startScanning()` 在初始化完成前被错误触发
  - 根因：`Item` 默认 `visible: true`，绑定 `visible: opacity > 0` 在 QML 加载瞬间将 `visible` 从 `true` 改为 `false`，触发 `onVisibleChanged`，导致 `startScanning()` 在 `initializeAsync()` 开始前就被调用
  - 修复：将触发逻辑从 `onVisibleChanged` 改为 `Connections { target: backend; onInitializationFinished: ... }`，确保只在异步初始化完成后才启动主界面
- `CMakeLists.txt`：版本号 v3.4.26 → v3.4.27

### 根因 / 背景
- Qt Quick `Item` 的 `visible` 默认值是 `true`，即使显式写了 `visible: opacity > 0`，在组件创建瞬间 `visible` 仍然是 `true`，绑定生效后才变为 `false`。这个从 true → false 的变化会触发 `onVisibleChanged`
- `Connections` 监听信号的方式不受属性默认值影响，只在 `initializationFinished()` 信号发射时执行一次

## v3.4.26 — 2026年05月12日

### 变更内容
- `auth_dll/qml/main.qml`：修复 splash 与主界面重叠 + `startScanning()` 重复调用
  - 将触发主界面显示的时机从 `onOpacityChanged` 改为 `onVisibleChanged`
  - 添加 `!splashDone` 保护，防止 `backend.startScanning()` 和 `mainReveal.start()` 被重复触发
  - 现在 splash 完全淡出（`visible: opacity > 0` 变为 false）后，主界面才开始淡入，不再重叠
- `src/main.cpp`：修复 `QFile::open()` 返回值丢弃的编译警告（`[[nodiscard]]`）
- `CMakeLists.txt`：版本号 v3.4.25 → v3.4.26

### 根因 / 背景
- 旧方案 `onOpacityChanged` 在 `opacity` 绑定值瞬间变为 0 时就触发，此时 `Behavior` 淡出动画刚开始，主界面淡入动画也同时启动，导致两者重叠约 350ms
- `startScanning()` 重复调用可能是因为 `opacity` 在 `Behavior` 动画过程中有微小抖动，导致 `onOpacityChanged` 被触发多次

## v3.4.25 — 2026年05月12日

### 变更内容
- **启动器页面重构** — 解决 10 秒黑屏，显示真实初始化进度：
  - `src/main.cpp`：调整启动顺序 — 先 `loadAuthDll()` 加载 DLL → 创建 QML 引擎 → 加载 DLL QML（窗口立即显示）→ `QTimer::singleShot` 触发异步初始化
  - `src/backend.h`：新增 `initializing`/`initStep`/`initStatus` 三个 Q_PROPERTY + 信号，供 QML 绑定
  - `src/backend.cpp`：
    - 拆分 `initialize()` 为同步 `loadAuthDll()` + 异步 `initializeAsync()`
    - 新增 `checkDllUpdateAsync()` / `checkDllUpdateAsyncInternal()` — 后台线程执行同步 TCP 调用（`fn_check_update` / `fn_download_secure`），避免阻塞 UI
    - 新增 `setInitStatus()` 统一更新进度状态
    - 新增 `logFromThread()` 线程安全日志 — 通过 `QMetaObject::invokeMethod` 把日志传回主线程的 `LogModel`
    - 修改 `doLocalVerify()` — 去掉内部的 `QTimer::singleShot(500, checkDllUpdate)`，更新检查统一由 `initializeAsync()` 管理
  - `auth_dll/qml/main.qml`：
    - 删除固定 `SequentialAnimation` 假进度动画
    - `splash` 的 `opacity` 直接绑定 `backend.initializing` — 初始化中显示，完成后淡出
    - `progressBar.width` 绑定 `backend.initStep`
    - `splashStatus.text` 绑定 `backend.initStatus`
    - 新增迷你日志列表（`ListView` 绑定 `logModel`）— 启动期间实时显示 `[INIT]`/`[UPD]` 日志
    - `onOpacityChanged` 中初始化完成后自动调用 `backend.startScanning()` + `mainReveal.start()`
- `CMakeLists.txt`：版本号 v3.4.24 → v3.4.25

### 根因 / 背景
- 黑屏根因：`main.cpp` 中 `backend.initialize()`（加载 DLL + 本地验证 + 更新检查）在 `QQmlApplicationEngine` 创建之前同步执行，导致 `app.exec()` 启动事件循环前没有任何窗口渲染
- 假进度根因：DLL QML 的 `SequentialAnimation` 是固定 1.2 秒的装饰动画，文字硬编码为 KERNEL32.DLL → LoadLibraryW → ... → READY，完全不反映真实初始化进度
- 方案：先显示窗口，后台异步初始化，通过 QML 属性绑定让 splash 成为"真实进度仪表盘"

## v3.4.23 — 2026年05月11日

### 变更内容
- `auth_dll/src/api.cpp`：
  - `nb_get_qml()` 开头显式调用 `qInitResources_app()`，强制触发 Qt 资源注册（解决静态 Qt + DLL 时全局构造函数未执行的问题）
  - 增加调试日志：读取失败时打印 `file.errorString()`，并枚举所有可用 qrc 资源
  - 读取成功时打印字节数

## v3.4.22 — 2026年05月11日

### 变更内容
- 修复 **QML 界面加载失败**（DLL 加载成功但 rootObjects 为空）：
  - 根因：EXE 和 DLL 各自**静态链接 Qt**，导致 Qt 资源表是**两份独立的副本**。DLL 加载时通过全局构造函数注册了 `qrc:/qml/main.qml`，但 EXE 的 QML 引擎查找的是 EXE 自己的资源表，看不到 DLL 里的条目
  - 修复：新增 `nb_get_qml()` C API —— DLL 内部用自己的 `QFile` 读取自己的 qrc 资源，把 QML 文本内容通过缓冲区返回给 EXE
  - EXE 改用 `engine.loadData(qmlData)` 加载 QML，绕过跨 DLL 的 Qt 资源系统隔离
- `auth_dll/include/notebot_auth.h`：新增 `nb_get_qml(char* buf, int buf_size)` 声明
- `auth_dll/src/api.cpp`：实现 `nb_get_qml()` —— `QFile(":/qml/main.qml")` 读取，返回字节数
- `src/backend.cpp/h`：
  - `AuthDllFuncs` 新增 `fn_get_qml` 解析
  - `Backend` 新增公共方法 `getQmlFromDll()`：调用 DLL 的 `nb_get_qml` 获取完整 QML 文本
- `src/main.cpp`：加载 QML 时优先走 `backend.getQmlFromDll()` + `engine.loadData()`，失败再回退到 `engine.load(QUrl)`
- `CMakeLists.txt` / `api.cpp`：版本号更新为 v3.4.22

### 根因 / 背景
- 静态链接 Qt + DLL 资源是 Qt 的已知陷阱：每个二进制独立一份资源注册表
- `engine.load("qrc:/qml/main.qml")` 在 EXE 侧永远找不到 DLL 里的资源，除非动态链接 Qt 或使用显式资源注册
- `loadData()` 方案是最小侵入的修复：不需要改 Qt 链接方式，也不需要改 QML 文件位置

## v3.4.21 — 2026年05月11日

### 变更内容
- 修复 **EXE 点击无反应** 的致命问题：
  - 根因：QML 文件已移到 DLL 资源中，但 `Backend` 构造函数用 `QTimer::singleShot(100ms)` 延迟加载 DLL，导致 `QQmlApplicationEngine` 在 DLL 加载前尝试加载 `qrc:/qml/main.qml` 失败，直接 `return 1` 退出
  - 修复：`main.cpp` 中创建 `Backend` 后立即调用 `backend.initialize()` 同步加载 DLL，确认成功后再创建 QML 引擎
- `src/backend.h/cpp`：
  - 新增公共方法 `initialize()`：同步加载 DLL + 本地验证
  - `loadAuthDll()` 各失败分支新增 `MessageBoxW` 弹窗诊断（DLL 缺失 / LoadLibrary 失败 / 导出函数解析失败）
  - `checkDllUpdate()` 下载失败时弹窗提示具体原因（网络问题 / Token 验证失败 / 磁盘不足）
- `src/main.cpp`：
  - QML 加载失败时弹窗提示"界面加载失败"及排查建议
  - 日志文件路径：`%TEMP%\notebot_injector_debug.log`
- `CMakeLists.txt` / `api.cpp`：版本号更新为 v3.4.21

### 根因 / 背景
- EXE 点击后没有任何窗口、没有任何弹窗、没有任何日志，用户完全不知道发生了什么
- 异步加载 DLL 的设计在 QML 迁移到 DLL 资源后产生竞态条件
- 缺乏弹窗诊断意味着任何启动失败都表现为"静默崩溃"

## v3.4.20 — 2026年05月11日

### 变更内容
- `auth_dll/src/crypto/crypto_utils.h/cpp`：新增 `GenerateDllDownloadToken()` —— 使用 SecureRandomBytes 生成 16 字节 nonce，拼接 nonce || timestamp || command 后做 HMAC-SHA256
- `NoteBot/server-node/src/config.js`：新增 `DLL_DOWNLOAD_PSK_HEX` 和 `DLL_NONCE_CACHE_SIZE`
- `NoteBot/server-node/src/crypto.js`：新增 `verifyDllToken(nonce, timestamp, signature, command)` —— 5 分钟时间窗口、nonce 防重放缓存、timingSafeEqual 防时序攻击
- `NoteBot/server-node/src/handler.js`：
  - 新增 `dll_update_check` 命令：扫描 `dlls/auth/` 目录，返回最新版本号/文件名/MD5/大小
  - 新增 `dll_download_secure` 命令：先验证 Token，通过后返回 DLL 二进制
- `auth_dll/src/auth/cloud_client_tcp.h/cpp`：
  - `checkUpdate()` 完全实现：发送 `dll_update_check`（Token 验证，无需 licenseKey）
  - 新增 `downloadDllSecure()`：发送 `dll_download_secure`（Token 验证）
  - 新增 `buildTokenRequest()` 辅助函数：统一生成 nonce + timestamp + HMAC-SHA256 签名
  - 内嵌 `g_dllDownloadPsk[32]`，与服务端 `DLL_DOWNLOAD_PSK_HEX` 一致
- `auth_dll/include/notebot_auth.h`：新增 `nb_download_secure()` C API 声明
- `auth_dll/src/api.cpp`：
  - `nb_check_update()` 完全实现：调用 CloudClient 检查更新，有更新返回 NB_OK + 版本号 + MD5
  - 新增 `nb_download_secure()` 实现：调用 CloudClient 安全下载，支持进度回调和 MD5 校验
  - 版本号硬编码更新为 "3.4.20"
- `src/backend.cpp/h`：
  - `AuthDllFuncs` 新增 `fn_download_secure` 解析
  - 新增 `checkDllUpdate()`：启动后自动检查 NoteBotAuth.dll 更新，发现新版本则下载到 `%LOCALAPPDATA%\NoteBotInjector\dlls\`，然后复制到 `NoteBotAuth.dll`，提示"重启后生效"
  - `doLocalVerify()` 成功后延迟 500ms 触发 `checkDllUpdate()`
- `CMakeLists.txt`：版本号更新为 v3.4.20

### 根因 / 背景
- EXE 需要终身不变，所有业务逻辑（包括 UI 绘制）在 DLL 中
- DLL 本身需要热更新能力：启动时自动检测服务端是否有新版本，下载后替换，下次启动生效
- Token 验证防止外网不明连接滥用 DLL 下载接口：只有持有正确 PSK + 算法才能通过服务端验证
- 机器绑定方案已废弃（`diagnose.js` 不再检查 machine_hash），本地授权文件改用 key-bound 加密

## v3.4.19 — 2026年05月11日

### 变更内容
- `build/NoteBotInjector.map`：精简为仅25个核心函数（注入器+授权链路），剔除所有Qt模板实例化、STL内部、lambda展开器、unwind元数据
- 新增 `filter_map.py`：构建后自动过滤MAP，保留关键函数（`doActivate`/`doInject`/`authenticate`/`downloadDll`/`AesGcmEncrypt`等25个）
- `build_now.bat`：ninja直接成功时自动调用 `filter_map.py`
- `build_fix.py`：移除重复filter调用，由 `build_now.bat` 统一调度
- `CMakeLists.txt`：版本号更新为 v3.4.19

### 根因 / 背景
- VMP导入完整MAP（24万行）会被Qt/STL噪声淹没，关键函数难以定位
- 精简MAP直接暴露授权核心路径，加壳时精准选择虚拟化目标
- 自动过滤避免每次构建后手动操作

## v3.4.18 — 2026年05月11日

### 变更内容
- `NoteBot/server-node/src/config.js`：`USE_PROXY_PROTOCOL` 改为 `true`，新增 `EVENTS_FILE` 路径
- `NoteBot/server-node/src/handler.js`：全面改写——
  - PROXY Protocol v1 现已启用，获取 FRP 透传的真实客户端 IP
  - 新增事件日志系统，每次 auth/diagnose/dll_download/sysinfo 写入 `data/events.json`（含时间戳、密钥、IP、操作类型、结果）
  - 所有控制台日志显示真实 IP
  - 新增 `sysinfo` 命令处理
- `src/auth/cloud_client_tcp.h/cpp`：新增 `sendSystemInfo()` 方法，静默上报 CPU/GPU/RAM/OS
- `src/backend.cpp/h`：新增 `reportSystemInfo()` —— 读注册表取 CPU 型号、EnumDisplayDevices 取 GPU、GlobalMemoryStatusEx 取内存、注册表取 Windows 版本，激活成功后后台静默上报
- `src/backend.cpp` / `qml/main.qml` / `CMakeLists.txt`：版本号统一更新为 v3.4.18

### 根因 / 背景
- FRP 节点开启 `proxy_protocol_version = v1` 后，服务端必须启用 PROXY Protocol 解析才能看到真实 IP
- 运维需要知道"谁、什么时候、用什么密钥、从哪里"连接了服务器
- 系统信息上报方便排查客户环境问题

## v3.4.17 — 2026年05月11日

### 变更内容
- `NoteBot/server-node/src/handler.js`：DLL 下载改为两段式协议——先发 AES 加密的 JSON 元数据（不含 dll_data），后跟 `[4B长度][原始 DLL 二进制]`。去除 base64 编码和 zlib 压缩
- `src/auth/cloud_client_tcp.cpp`：`downloadDll()` 重构读取流程——读完 JSON 元数据后检查 `dll_size` 字段，>0 则继续读原始 DLL 二进制，=0 则走旧 base64 兼容路径。socket 推迟到全部读完再关
- `src/backend.cpp` / `qml/main.qml` / `CMakeLists.txt`：版本号统一更新为 v3.4.17

### 根因 / 背景
- VMP 加壳后的 DLL 压缩率仅 87.5%，deflate 无效。瓶颈回到 base64 膨胀（13.4MB→17.8MB）和 AES 对大块数据的开销
- 新协议 DLL 以原始二进制传输（13.4MB 就是 13.4MB），去掉 33% base64 膨胀，传输量减少约 4.4MB
- **需同步部署服务端，旧客户端连新服务端/新客户端连旧服务端会走 base64 兼容路径**

## v3.4.16 — 2026年05月10日

### 变更内容
- `NoteBot/server-node/src/handler.js`：`handleDllDownload()` 读取 DLL 后增加 zlib raw deflate 压缩（Qt `qUncompress` 兼容格式），再 base64 编码返回。日志新增压缩率（如 10MB→5MB）
- `src/auth/cloud_client_tcp.cpp`：`downloadDll()` 在 base64 解码后增加 `qUncompress()` 解压步骤
- `src/backend.cpp` / `qml/main.qml` / `CMakeLists.txt`：版本号统一更新为 v3.4.16

### 根因 / 背景
- 用户反馈 DLL 下载速度慢（10MB 约 1%/秒），实际传输的 13.4MB 数据经过 FRP 隧道太慢
- PE/DLL 文件 deflate 压缩率通常 40-60%，10MB 可压到 4-6MB，传输量砍半
- **需要同步部署服务端，否则客户端解压会失败**

## v3.4.15 — 2026年05月10日

### 变更内容
- `src/auth/cloud_client_tcp.h/cpp`：新增 `setProgressCallback` + `m_progressCallback`，`downloadDll()` 读取循环中按已读字节百分比实时回调进度
- `src/backend.cpp`：注入流程挂载下载进度回调，映射到进度条 10-75% 区间（之前是假的 20% 瞬间跳到 80%）
- `qml/main.qml`：进度条从状态面板底部挪到注入按钮正上方（`Layout.fillHeight` spacer 之前），高度从 6→10px，百分比文字垂直居中
- `src/backend.cpp` / `qml/main.qml` / `CMakeLists.txt`：版本号统一更新为 v3.4.15

### 根因 / 背景
- DLL 下载进度条是假的——`downloadDll()` 一次阻塞读完 13MB 才返回，中间零进度反馈
- 修复后每次 `socket.read()` 后计算 `已读/总长*100%`，回调到 UI 驱动进度条平滑前进

## v3.4.13 — 2026年05月10日

### 变更内容
- `src/auth/cloud_client_tcp.cpp` `authenticate()`：重构响应解析逻辑，先读 `status` 字段再按实际情况返回 `AuthResult`，不再把所有非 ok 响应统一报 `InvalidSignature`
  - `expired` → `AuthResult::Expired`
  - `invalid` → `AuthResult::InvalidKey`
  - `revoked` → `AuthResult::InvalidKey`
  - 只有真正的 ok+解析失败才报 `ServerError`
- `src/backend.cpp` / `qml/main.qml` / `CMakeLists.txt`：版本号统一更新为 v3.4.13

### 根因 / 背景
- 服务端日志清清楚楚返回 `expired`（密钥过期），但客户端 UI 显示"服务端响应签名无效"——完全牛头不对马嘴
- 根因同 v3.4.12 的诊断错误覆盖问题但发生在激活/auth 流程：`authenticate()` 调用 `parseResponse()`，后者判断 `status != "ok"` 就返回 false，前者无条件把 false 映射为 `InvalidSignature`
- 本次修复后用户能看到真正的失败原因（过期/无效/吊销），而不是一律被翻译成"签名无效"

## v3.4.12 — 2026年05月10日

### 变更内容
- `NoteBot/server-node/src/diagnose.js`：彻底去掉 `machine_hash` 依赖，不再要求客户端发送机器指纹，不再做设备绑定检查
- `src/auth/cloud_client_tcp.cpp`：错误消息映射不再把所有 `status=invalid` 覆盖为"Token 签名无效"，保留服务端返回的原始错误消息
- `src/backend.cpp` / `qml/main.qml` / `CMakeLists.txt`：版本号统一更新为 v3.4.12

### 根因 / 背景
- 客户端 v3.4.8 已停止发送 `machine_hash` 到 diagnose 接口，但服务端 `diagnose.js` 仍在强制校验 `machine_hash` 长度（必须 64 位 hex），客户端不发就返回 `机器指纹无效`
- 客户端错误映射把 `status=invalid` 统一覆盖成"Token 签名无效"，导致用户看到的错误信息与实际根因完全牛头不对马嘴
- 本次修复彻底对齐：服务端不再要机器指纹、不再做设备绑定、不再返回 device_mismatch；客户端如实展示服务端原始错误消息
- **重要：需重新部署服务端 `diagnose.js` 到 notebot-api.fucku.top 才能生效**

## v3.4.11 — 2026年05月10日

### 变更内容
- `src/backend.cpp` / `qml/main.qml` / `CMakeLists.txt`：版本号统一更新为 v3.4.11
- 构建生成 `NoteBotInjector.exe`

### 根因 / 背景
- v3.4.10 代码已落地但未构建产出 exe，本次构建产出包含 v3.4.10 全部修复（兼容诊断重试 + 旧票自动洗票）+ v3.4.9 预留区清零修复

## v3.4.10 — 2026年05月10日

### 变更内容
- `src/auth/cloud_client_tcp.h` / `cloud_client_tcp.cpp`：`diagnose()` 新增兼容模式，可在不改密钥、不删文件的前提下把旧票据按干净 token 重新组包发送
- `src/backend.cpp`：注入前若遇到 `Token 签名无效`，自动走一次兼容诊断重试；重试通过后立即回写修复后的 `license.dat`，不再要求手动删除本地授权文件
- `src/backend.cpp` / `qml/main.qml` / `CMakeLists.txt`：版本号统一更新为 v3.4.10

### 根因 / 背景
- 用户明确不要手动删 `license.dat`，因此修复不能只停留在“以后新票别再写坏”，还必须把已经写坏的旧票当场救回来
- 旧票的问题本质是 token 预留区混进了脏字节，导致服务端看到的“正文”与原始签名不匹配；但这些预留区本来就不该有业务含义，所以可以在客户端本地清零后重组票据再验一次
- 这次修复等于给旧通行证加了一个自动洗票机：先试原票，票脏了就现场擦干净、复核通过后再把干净票写回本地

## v3.4.9 — 2026年05月10日

### 变更内容
- `src/auth/license_file_manager.cpp`：写入本地 `license.dat` 前强制清零 `LicenseToken` 里的预留字节，避免把内存脏数据一起加密落盘
- `src/auth/cloud_client_tcp.cpp`：解析激活响应后统一清零 token 预留字节，保证后续诊断时发回服务端的 token 与签名对应的原文一致
- `src/backend.cpp`：每次启动本地验证前先清空缓存授权数据，避免读旧缓存把坏票据继续带进注入链
- `src/backend.cpp` / `qml/main.qml` / `CMakeLists.txt`：版本号统一更新为 v3.4.9

### 根因 / 背景
- 当前故障不是密钥输错，也不是客户机环境问题，而是激活后本地保存/读取的授权票据里混入了不该参与签名的脏字节
- 服务端 diagnose 验签时只认最原始那 108 字节 token 正文；只要客户端本地票据里的预留区被带上随机内存垃圾，就会出现“正文和章对不上”，最终报 `Token 签名无效`
- 这次修复等于先把票据上的空白栏全部擦干净再盖章、再存档，避免自己电脑也被这套链路卡死

## v3.4.8 — 2026年05月10日

### 变更内容
- `src/auth/mutual_verify.cpp`：`auth.dat` 路径改为硬编码 `%LOCALAPPDATA%\NoteBotInjector\dlls\auth.dat`，不再使用 Qt `AppLocalDataLocation` 动态计算
- `src/auth/license_file_manager.cpp`：文件格式升至 v3，`licenseKey` 明文存于文件头，AES key 由 licenseKey 派生，彻底去掉机器指纹依赖
- `src/auth/cloud_client_tcp.cpp` / `cloud_client_tcp.h`：`diagnose()` 去掉 `machineHash` 参数，不再发送 `machine_hash` 字段
- `src/backend.cpp` / `qml/main.qml` / `CMakeLists.txt`：版本号统一更新为 v3.4.8

### 根因 / 背景
- 客户干净机注入成功但授权确认超时：EXE 侧用 Qt 动态路径写 `auth.dat`，DLL 侧用硬编码路径读，两边路径不一致导致 DLL 永远找不到文件，永远不写 `OKAN` 确认魔数
- 之前只改了 `backend.cpp` 的日志版本号，底部 UI 文本和工程版本号仍停留在 3.4.7，所以你看到发过去还是 3.4.7
- 机器绑定去除：彻底移除 `machineHash` 依赖，本地授权文件只认密钥内容，不再因机器指纹漂移失效

## v3.4.7 — 2026年05月10日 18:31

### 变更内容
- `src/auth/machine_id.cpp`：机器指纹改为只取 CPU CPUID + 主板/机型注册表信息，去掉计算机名、系统目录、C 盘卷序列号
- `src/backend.cpp`：DLL 获取逻辑改为按 MD5 判断是否需要重下
  - 新增本地文件 MD5 计算
  - 本地 DLL 存在时，只有本地 MD5 与服务端 MD5 一致才跳过下载
  - 同名 DLL 内容变化时自动删除旧文件并重新下载
  - 下载完成后追加一次本地 MD5 自检，失败则删除文件并取消注入
- `src/auth/cloud_client_tcp.h/cpp`：诊断结果新增 `dllMd5`，DLL 下载接口新增期望 MD5 校验
- `qml/main.qml`：底部版本号更新为 v3.4.7
- `CMakeLists.txt`：项目版本更新为 v3.4.7
- `NoteBot/server-node/src/diagnose.js`：诊断接口返回 `dll_md5`，并在服务端预先校验指定 DLL 是否存在
- `NoteBot/server-node/src/handler.js`：DLL 下载响应新增 `dll_md5`

### 根因 / 背景
- 用户反馈同一台机器会随机触发 `device_mismatch`，需要把机器指纹改成更简单、更稳定、普通权限可读的方案
- 用户反馈 DLL 自动更新只看文件名不可靠，同名替换内容时客户端会一直吃旧文件
- 这次改完后，客户端判断 DLL 是否需要更新不再看“箱子名字”，而是看“箱子里的货号（MD5）”

## v3.4.3 — 2026年05月10日

### 变更内容
- `src/backend.cpp`：`doInject()` 改为使用缓存授权数据，不再重复读 license.dat
  - 新增 `m_lastFileData` 缓存完整的 `LicenseFileData`
  - `doActivate()` 和 `doLocalVerify()` 成功后写入缓存
  - `doInject()` 直接使用缓存，解决"读取本地授权文件失败"的报错
- `src/backend.h`：新增 `m_lastFileData` 成员变量
- `src/backend.cpp`：版本号更新为 v3.4.3
- `qml/main.qml`：底部版本号更新为 v3.4.3

### 根因 / 背景
- 用户反馈激活成功后注入时报"读取本地授权文件失败，注入已取消"
- 根因：`doInject()` 重复调用 `readLicenseFile()` 读文件，与 `doLocalVerify()` 的读取可能因竞态/缓存不一致导致失败
- 修复：激活和本地验证成功后缓存完整授权数据，注入时直接用缓存，不再读文件

## v3.4.2 — 2026年05月10日

### 变更内容
- **DLL 共享内存授权验证**：DLL（overlay.dll）加载时强制验证共享内存 HMAC 签名
  - `src/auth/shared_auth_block.h`：新增 `expiresAt` 字段，version 升为 4
  - `src/auth/mutual_verify.h/cpp`：`writeAuth()` 新增 `expiresAt` 参数，HMAC 输入增加 expiresAt
  - `src/backend.cpp`：`doInject()` 写入共享内存时传入 `expiresAt`
  - `NoteBot/src/auth/dll_auth.cpp`：HMAC 输入增加 expiresAt，新增过期时间检查
  - `NoteBot/src/main.cpp`：`WorkerThread` 开头调用 `DllAuth::VerifyTokenFromSharedMemory()`，验证失败立即卸载 DLL
- **服务端 key 处理修复**：`notebot_auth_server.py` 修复 `\x00` 填充字符导致密钥查不到的问题
  - `process_auth`、`process_diagnose` 改用 `replace('\x00', '')` 清理密钥
- 修复 `license_file_manager.cpp` 读取密钥时 `\x00` 未清理的问题
- `src/backend.cpp`：版本号更新为 v3.4.2
- `qml/main.qml`：底部版本号更新为 v3.4.2
- `NoteBot/build_overlay.bat`：版本号更新为 v6.2.1

### 根因 / 背景
- 用户反馈普通注入器也能直接注入 overlay.dll 绕过授权验证，DLL 端缺少共享内存验证
- 用户要求 DLL 有过期机制，防止授权过期后继续使用
- 服务端 `process_diagnose` 返回"密钥不存在"，根因是 license.dat 中 32 字节固定长度 key 含有 `\x00` 填充，Python 的 `strip()` 不去掉 `\x00`

## v3.4.1 — 2026年05月10日

### 变更内容
- `qml/main.qml`：修复密钥文本超出输入框的问题
  - TextInput 添加 `maximumLength: 32` 和 `clip: true`
- `qml/main.qml`：统一按钮装饰线颜色
  - 底部装饰线颜色从 `Qt.rgba(0.8, 0.35, 0.95, 0.9)` 改为 `accentPurple`（与顶部一致）
- `src/backend.cpp`：版本号更新为 v3.4.1
- `qml/main.qml`：底部版本号更新为 v3.4.1

### 根因 / 背景
- 用户反馈密钥过长时会溢出输入框边界
- 用户反馈按钮上下装饰线颜色不一致

## v3.4.0 — 2026年05月09日

### 变更内容
- **激活绑定机制**：从"每次启动联网验证"改为"一次激活，本地验证+注入时诊断"
  - 新增 `src/auth/license_file_manager.h/cpp`：本地授权文件加密系统
    - 文件存储于 `%LOCALAPPDATA%/NoteBotInjector/license.dat`
    - AES-256-GCM 加密，密钥由 HmacSha256(salt, machine_hash) 派生
    - 文件包含 LicenseToken(108B) + Signature(256B) + machine_hash(32B) + CRC32 校验
    - 换机器无法解密（machine_hash 不匹配）
  - `src/backend.cpp` `doActivate()`：联网激活成功后写入本地授权文件
  - `src/backend.cpp` `doLocalVerify()`：启动时读取本地文件，无需联网即可验证
  - `src/backend.cpp` `doDiagnose()`：注入前联网诊断，服务端验证设备绑定
  - `src/backend.cpp` `resetActivation()`：重置激活状态（删除本地文件）
  - `src/auth/cloud_client_tcp.h/cpp`：新增 `diagnose()` 方法，TCP 协议发送 token+signature+machine_hash
  - `NoteBot/server/notebot_auth_server.py`：新增 `process_diagnose()` 和 `bound_machine_hash` 字段
    - 首次 diagnose 自动绑定设备
    - 后续 diagnose 检查设备是否匹配，返回 device_mismatch 阻止注入
- **QML UI 改造**：
  - `qml/main.qml`：按钮文字从"验证密钥"改为"激活密钥"，已激活后显示"已激活"
  - `qml/main.qml`：密钥输入框锁定（isKeyLocked 时 readOnly + enabled=false + opacity=0.5）
  - `qml/main.qml`：状态面板颜色逻辑从"已授权"改为"已激活"
  - `qml/main.qml`：注入按钮启用条件从 `licenseStatus === "已授权"` 改为 `isActivated`
- `src/backend.h`：新增 `isActivated`、`isKeyLocked` QML 属性
- `CMakeLists.txt`：添加 `license_file_manager.cpp`
- `src/backend.cpp`：版本号更新为 v3.4.0
- `qml/main.qml`：底部版本号更新为 v3.4.0

### 根因 / 背景
- 用户要求彻底重构授权机制：密钥与设备绑定、本地文件存储、防止多设备共用
- 服务端压力降低（激活一次后不再每次启动联网）
- 用户体验提升（断网后仍可启动和注入，只要本地文件有效）

## v3.3.1 — 2026年05月09日

### 变更内容
- `qml/main.qml`：修复侧边栏状态区 `Column` 使用 `Layout.fillWidth` 导致宽度为 0 的问题
  - 状态区 `Column` 的 `Layout.fillWidth: true` 改为 `width: parent.width`
  - 自动选最新版按钮的 `Layout.fillWidth: true` 改为 `width: parent.width`
  - 根因：`Column` 不是 Layout 元素，不支持 `Layout` 附加属性
- `src/backend.cpp`：版本号更新为 v3.3.1
- `qml/main.qml`：底部版本号更新为 v3.3.1

### 根因 / 背景
- v3.3.0 中密钥输入框和日志窗口因布局属性误用导致不可见

## v3.3.0 — 2026年05月09日

### 变更内容
- **密钥输入系统**：侧边栏新增 LICENSE KEY 输入框，支持密码显示/隐藏切换
  - 密钥保存到 QSettings，下次启动自动读取
  - 输入后自动触发服务器验证
  - 移除硬编码密钥
- **开发者模式**：Dev tier 用户显示 MODULE 手动选择接口，普通用户自动隐藏
  - 普通用户自动调用 `pickLatest()` 获取最新版 DLL
- **日志窗口扩大**：日志区域从 180px 改为 `Layout.fillHeight`，与进程列表各占约 50%
- **注入按钮 5 秒冷却**：点击注入后启动 5 秒冷却定时器，按钮显示 "COOLDOWN..."
- **自动更新框架**：新增 `UpdateChecker::checkUpdate()` 接口（stub 实现，待服务端对接）
- **加密下载框架**：新增 `UpdateDownloader` 类，支持 HTTPS 下载 + AES-GCM 解密到临时文件
- **日志过滤机制**：`LogModel` 新增 `appendDebug()` 方法，敏感信息只写文件不显示 UI
- `src/backend.h`：新增 `licenseKey`、`isDevMode`、`injectCooldown` QML 属性
- `src/backend.cpp`：版本号更新为 v3.3.0
- `qml/main.qml`：底部版本号更新为 v3.3.0
- `CMakeLists.txt`：添加 `update_downloader.cpp`

### 根因 / 背景
- 为正式营业做准备，补全商业软件必备功能：用户密钥管理、功能分级、体验优化

## v3.2.6 — 2026年05月09日

### 变更内容
- `src/auth/mutual_verify.cpp`：重写为 HMAC-SHA256 方案，放弃 RSA 双向校验
  - `writeAuth` 改为计算 HMAC-SHA256 tag（tier + capabilities + timestamp 共 13 bytes 输入）
  - `verifyResponse` 直接返回 true（DLL 自行验证 HMAC tag，EXE 无需等待）
- `src/auth/mutual_verify.h`：移除 dllPubKey 相关 API，简化接口
- `src/auth/shared_auth_block.h`：与 DLL 同步 v3 格式（magic=0x4E424156，HMAC tag 字段）
- `src/auth/shared_hmac_key.h`：新增，32 bytes 编译期嵌入的共享 HMAC key
- `src/backend.cpp`：调用 `writeAuth(tier, capabilities)` 替代 `writeToken`
- `src/backend.cpp`：版本号字符串更新为 v3.2.6
- `qml/main.qml`：底部版本号文字更新为 v3.2.6

### 根因 / 背景
- v3.2.5 之前的 RSA 双向校验在 16 次构建中始终失败（STATUS_INVALID_SIGNATURE / NTE_BAD_SIGNATURE 等）
- 根本原因是 Python cryptography 与 Windows CNG 跨平台签名字节序/填充模式不兼容
- 改用 HMAC-SHA256 共享密钥方案，双方使用同一套代码计算 tag，彻底消除跨平台差异

## v3.2.5 — 2026年05月09日

### 变更内容
- `src/auth/cloud_client_tcp.cpp`：修复 Token 解析逻辑，适配服务端新的二进制 token wire 格式
  - `parseResponse` 改为解析 `token_wire` hex 字符串，直接 `memcpy` 填充 `LicenseToken` 结构体（108 bytes）
  - 去掉逐个 JSON 字段解析（`token` dict 已废弃）
  - 增加 `tierReserved` 和 `reserved` 字段清零（防止服务端未清零导致结构体不一致）
- `src/backend.cpp`：版本号字符串更新为 v3.2.5
- `qml/main.qml`：底部版本号文字更新为 v3.2.5

### 根因 / 背景
- v3.2.4 注入后 DLL 双向校验超时，诊断发现 DLL 日志显示 "Token 验证失败"
- 根因：服务端对 JSON 字符串签名，DLL 验证的是二进制结构体数据，RSA 验签对象不一致导致永远失败
- 修复：服务端改为对二进制 token 签名，EXE 客户端直接透传二进制数据，DLL 保持验证逻辑不变

## v3.2.4 — 2026年05月09日

### 变更内容
- `src/auth/mutual_verify.h/cpp`：修复 MutualVerifier 的移动语义，防止注入后 EXE 闪退
  - 新增移动构造函数和移动赋值运算符，移动后将源对象的 m_hMap/m_pView 置空
  - 禁用拷贝构造和拷贝赋值
  - 根因：默认移动构造对原始指针只是复制，导致原始对象和 lambda 中的对象都持有相同句柄；doInject 返回时原始对象析构关闭共享内存，lambda 中 verifyResponse 访问已解除映射的内存 → 崩溃
- `src/backend.cpp`：版本号字符串更新为 v3.2.4
- `qml/main.qml`：底部版本号文字更新为 v3.2.4

### 根因 / 背景
- v3.2.3 测试中注入 DLL 成功但注入器闪退，DLL 继续运行
- 诊断发现 MutualVerifier 被 std::move 捕获到 lambda 后，原始对象在 doInject 返回时析构，错误地关闭了共享内存，导致注入线程中的 verifyResponse 访问无效内存

## v3.2.3 — 2026年05月09日

### 变更内容
- **EXE-DLL 双向校验闭环打通（Phase 4-5）**：
  - `src/auth/mutual_verify.cpp`：新增 `MutualVerifier::writeToken()`，把验证通过的 Token 写入共享内存
  - `src/auth/dll_pubkey_embedded.cpp`：嵌入 DLL 公钥（DER 格式，294 bytes），用于 EXE 侧验证 DLL 响应签名
  - `src/backend.cpp` `doInject()`：完整串联验证流程——创建共享内存 → 写入 challenge → 写入 Token → 注入 DLL → 等待 DLL 响应 → 用 DLL 公钥验签 → 日志输出结果
  - `src/backend.cpp` `doAuthVerify()`：验证成功时缓存 token 和 sig，供注入时写入共享内存
  - `src/backend.h`：新增 `m_lastToken`、`m_lastSig`、`m_authSuccess` 成员变量
- `src/backend.cpp`：版本号字符串更新为 v3.2.3
- `qml/main.qml`：底部版本号文字更新为 v3.2.3

### 根因 / 背景
- 云授权体系的核心闭环：EXE 在线验证后将 token 写入共享内存，DLL 注入后读取并验签，再用私钥响应 challenge，EXE 验证通过后放行
- 验证失败只记录日志、不杀目标进程，符合约束

## v3.2.2 — 2026年05月09日

### 变更内容
- `qml/main.qml`：修复滚轮完全无法滚动的问题
  - v3.2.1 改用 `SmoothedAnimation` 时误删了 `wheelAnim.start()`，导致滚轮事件只设置了目标值但没有启动动画
  - 加回 `wheelAnim.start()`，滚轮恢复正常平滑滚动
- `src/backend.cpp`：版本号字符串更新为 v3.2.2
- `qml/main.qml`：底部版本号文字更新为 v3.2.2

### 根因 / 背景
- `SmoothedAnimation` 和 `NumberAnimation` 一样都需要显式调用 `start()` 才会执行
- v3.2.1 的修改中把 `.start()` 漏掉了，滚轮事件被接收但 contentY 从未改变

## v3.2.1 — 2026年05月09日

### 变更内容
- `qml/main.qml`：滚轮动画改为 `SmoothedAnimation`，解决顿挫感
  - 去掉 `NumberAnimation`（每次滚轮事件触发一次固定时长动画，连续滚动时一抽一抽）
  - 改用 `SmoothedAnimation`（`velocity: 1200`），持续跟踪目标值，连续滚轮时无缝衔接
- `qml/main.qml`：点击日志空白区自动取消文本选中
  - `MouseArea` 增加 `onPressed: logView.forceActiveFocus()`，点击非文本区域时焦点离开 TextEdit，选中自动消失
- `src/backend.cpp`：版本号字符串更新为 v3.2.1
- `qml/main.qml`：底部版本号文字更新为 v3.2.1

### 根因 / 背景
- v3.2.0 的 `NumberAnimation` 在快速连续滚轮时产生明显顿挫，因为每次事件都重启一个 250ms 的独立动画
- `SmoothedAnimation` 专为跟踪动态目标设计，目标值变化时不会生硬重启，而是平滑过渡
- 用户反馈点击空白处无法取消之前选中的文本，体验差；通过焦点转移让 TextEdit 失去焦点即可自动清除选中

## v3.2.0 — 2026年05月09日

### 变更内容
- `qml/main.qml`：日志滚轮恢复平滑过渡动画
  - 在 `MouseArea.onWheel` 中不再直接修改 `contentY`，改由 `NumberAnimation` 驱动
  - 动画时长 250ms，`Easing.OutCubic`，模拟惯性滚动手感
  - 空白区长按拖拽仍被 `interactive: false` 彻底禁用
- `src/backend.cpp`：版本号字符串更新为 v3.2.0
- `qml/main.qml`：底部版本号文字更新为 v3.2.0

### 根因 / 背景
- v3.1.9 的 `interactive: false` 方案虽然解决了拖拽问题，但滚轮变成了生硬跳变，失去了原有的平滑惯性感
- 用户要求保留滚轮平滑过渡，因此在 MouseArea 里用 NumberAnimation 接管滚轮位移

## v3.1.9 — 2026年05月09日

### 变更内容
- `qml/main.qml`：彻底禁用日志区域的拖拽滚动（第二版）
  - 去掉 `pressDelay: 600000` 方案，改用 `interactive: false` 从根本上关闭 Flickable 拖拽
  - 内部增加 `MouseArea`（`z: -1`），手动处理 `onWheel` 事件实现滚轮滚动
  - 保留右侧 `ScrollBar` 拖拽滚动和 `TextEdit` 的文本选中复制
- `src/backend.cpp`：版本号字符串更新为 v3.1.9
- `qml/main.qml`：底部版本号文字更新为 v3.1.9

### 根因 / 背景
- v3.1.8 的 `pressDelay: 600000` 方案仍然无法完全阻止长按空白区拖动，用户反馈体验极差
- `interactive: false` 是 Flickable 的原生开关，关闭后拖拽彻底消失；滚轮通过 MouseArea 手动接管，ScrollBar 拖拽不受影响

## v3.1.8 — 2026年05月09日

### 变更内容
- `qml/main.qml`：完全禁用日志区域的拖拽滚动
  - `pressDelay` 从 200 改为 600000（10分钟），ListView 永远不会进入拖拽模式
  - 仅保留两种滚动方式：鼠标滚轮、拖拽右侧 ScrollBar
  - TextEdit 的文本选中不受影响，点击即可选中复制

### 根因 / 背景
- 用户反馈 200ms 的 pressDelay 仍然会导致误触发拖拽，体验不好
- 直接给 pressDelay 设一个极大值，effectively 禁用拖拽，同时保留滚轮和滚动条

## v3.1.7 — 2026年05月09日

### 变更内容
- `qml/main.qml`：修复日志滚轮不能用的问题
  - 去掉 `ListView.interactive` 绑定（该方案会同时禁用滚轮）
  - 改用 `pressDelay: 200`——ListView 延迟 200ms 才开始检测拖拽，这段时间内鼠标事件先给 delegate 里的 `TextEdit`
  - 滚轮事件不受影响，始终可正常滚动
  - 200ms 内按住拖动 → TextEdit 选中文本；超过 200ms 后拖动 → ListView 拖拽滚动

### 根因 / 背景
- v3.1.6 的 `interactive: false` 方案虽然解决了选中问题，但连带禁用了鼠标滚轮
- `pressDelay` 是 Qt Flickable 的原生属性，只延迟拖拽检测，不影响滚轮和点击

## v3.1.6 — 2026年05月09日

### 变更内容
- `qml/main.qml`：修复日志无法选中的问题
  - delegate 里的 `Text` 改回 `TextEdit`（`readOnly: true; selectByMouse: true`）
  - 点击日志文本时 `TextEdit` 获得焦点，`ListView.interactive` 自动设为 false（禁用滚动），此时可以正常鼠标拖拽选中文本、Ctrl+C 复制
  - 点击日志区域外的地方，`TextEdit` 失去焦点，`ListView` 恢复滚动
- `src/logmodel.h/cpp`：加 `lineAppended` 信号（备用）

### 根因 / 背景
- `ListView` 的 Flickable 行为会在鼠标按下时抢占事件，导致 delegate 里的 `TextEdit` 接收不到鼠标事件
- 通过焦点状态联动控制 `ListView.interactive`，实现"选中文本时禁用滚动，平时可滚动"

## v3.1.5 — 2026年05月09日

### 变更内容
- `server/notebot_auth_server.py`：密钥改为"首次激活才开始倒计时"
  - 表结构改 `duration_days`（有效期天数）+ `activated_at`（首次激活时间）+ `expires_at`（计算出的过期时间）
  - 首次请求时设置 `activated_at=now`，`expires_at=now+duration_days*86400`；后续请求只检查是否过期，不重新计时
  - 去掉 `issued_at` 和 `max_machines`（不限制机器数）
- `src/auth/cloud_client_tcp.cpp`：去掉离线验证 fallback
  - 连接失败或响应超时直接返回 `NetworkError`，不再尝试读本地缓存
  - 去掉 `SaveOfflineCache` 调用
- `src/backend.cpp`：去掉 `SuccessOffline` 状态分支和对应日志

### 根因 / 背景
- 用户要求密钥购买后随时可激活，激活后才开始倒计时（不是创建时就固定过期时间）
- 用户保证服务器持续在线，不需要离线断网验证

### 注意
- **服务端数据库表结构变了**，请删除旧版 `NoteBot/server/auth.db`，重新创建数据库并插入测试密钥

## v3.1.4 — 2026年05月09日

### 变更内容
- `qml/main.qml`：修复日志不自动滚动到底部的问题
  - 把 `contentY = contentHeight - height` 计算改为 `positionViewAtEnd()`，更可靠
- `qml/main.qml`：修复日志文本无法选中的问题
  - 日志项从 `Text` 改为 `TextEdit`（`readOnly: true; selectByMouse: true`），支持鼠标拖拽选中和 Ctrl+C 复制

### 根因 / 背景
- 日志不自动滚底：用户需要手动拖滚动条才能看到最新日志
- 日志无法复制：调试时想把错误信息复制出来分析，但 `Text` 元素不支持选中

## v3.1.3 — 2026年05月09日

### 变更内容
- `src/auth/cloud_client_tcp.cpp`：修复响应密文读取为 0 字节的问题
  - 读取循环改为先检查 `socket.bytesAvailable()`，缓冲区有数据直接读，避免服务端 `close()` 后 RST 导致 `waitForReadyRead` 失败
  - 循环内增加详细超时诊断：已读字节数、bytesAvailable、socketError、errorString

### 根因 / 背景
- 服务端日志显示正常发送 870 字节响应，但客户端读取长度前缀（870）后，while 循环里读到的密文长度为 0
- 原因：服务端发完数据立刻 `writer.close()`，TCP 连接被 RST，Qt 的 `waitForReadyRead` 在循环里返回 false 超时
- 修复：有数据时直接用 `read()` 消费缓冲区，不依赖 `waitForReadyRead`

## v3.1.2 — 2026年05月09日

### 变更内容
- `src/auth/cloud_client_tcp.h/cpp`：诊断日志全部改为回调机制，`setLogCallback()` 把每一步详细状态实时传回 UI
- `src/backend.cpp`：`doAuthVerify()` 中给 `CloudClient` 挂上回调，所有验证细节直接显示在注入器日志面板
- `server/notebot_auth_server.py`：服务端 `handle_client` 加逐行诊断（连接/读取长度前缀/解密/处理/加密响应/发送/断开，每一步都打印）

### 根因 / 背景
- 客户端显示"网络错误"但看不到具体卡在哪一步，因为 GUI 程序没有控制台，qDebug/qInfo 都输出到虚空
- 服务端也只有"已连接/已断开"两行，异常被吞掉，无法定位问题
- 改成回调回传后，注入器面板能直接看到"正在连接...""TCP连接成功""发送XX字节""等待响应..."等每一步状态
## EXE v3.6.42 / DLL v3.5.50 - 2026年06月14日 13:35
### Changes
- `qml__injector_exe/main.qml`: 把注入按钮上方那块多行吐槽卡片 `skinPreviewBtn` 临时隐藏，同时把高度压成 `0`，避免继续占位抢视觉焦点。
- `qml__injector_exe/main.qml`: 关闭这块卡片原本点击会触发的怒气爆闪特效，`triggerAngerBurst()` 现在直接清空状态并停止动画，不再炸全屏。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.42 / 30642`，并重新完成 EXE 构建。

### Reason
- 这轮目标很单纯，就是先把这块过于显眼、情绪值过高的入口从用户视野里拿掉，让当前发版界面更干净，先服务“能快速发给用户”这件事。
- 根目录原 `build_all.bat` 在本机被正在运行的 `NoteBotInjector.exe` 占用绊了一下，导致老缓存目录里的链接结果不可信；因此这次补了一套独立缓存目录重新编译，确保 `3.6.42` 的新 EXE 真正落盘。
## EXE v3.6.43 / DLL v3.5.50 - 2026年06月14日 13:39
### Changes
- `qml__injector_exe/main.qml`: 把注入按钮上方那块多行吐槽卡片 `skinPreviewBtn` 保持隐藏，并继续让它不占布局高度、不响应点击。
- `qml__injector_exe/main.qml`: 怒气爆闪入口继续保持关闭，`triggerAngerBurst()` 不再触发任何全屏爆闪动画。
- `CMakeLists.txt`, `resources__injector_exe/app_icon.rc`, `src__injector_exe/backend.cpp`: 主程序版本前推到 `3.6.43 / 30643`，并重新完成 EXE 正式构建，`dist__release_artifacts\\NoteBotInjector.exe` 已更新。

### Reason
- 这轮是在确认 UI 改动已经落地后，补一版真正可发的正式构建，目标是让发布目录里的 EXE 也同步成为最新版本，而不是只停留在单独缓存目录里。





