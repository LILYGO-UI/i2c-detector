# I2C Detector 界面与开发规范

本文说明 I2C Detector 当前页面的布局、状态展示和 UI 开发约定。项目使用与构建见
[中文 README](../README.zh-CN.md)，模块与线程关系见[架构总览](00-overview.md)，
扫描范围、设备访问和错误处理见[扫描与硬件说明](02-scanning-and-hardware.md)。

新增或修改页面、组件、布局、样式、文案或字体前，必须先检查
[`src/pages/i2c_detector/`](../src/pages/i2c_detector/)、
[`src/components/`](../src/components/) 和相关测试。下文的“必须”和“不得”是开发约束；
当前行为与现有测试覆盖分别在对应章节说明。

## 当前页面

应用目前只有一个扫描页面。打开页面后自动发起扫描，用户通过总线下拉框查看结果，
通过 **Scan** 按钮重新扫描所有发现的总线。

| 区域 | 当前内容与行为 |
| --- | --- |
| 控制卡片 | 总线下拉框、`Scan` 按钮和全局扫描状态；下拉框与按钮高度均为 56 px，展开选项使用 22 px 字体。 |
| 地址卡片 | 当前总线路径、该总线的设备数量在左侧，绿色 `Detected` 图例靠右对齐，下方显示 7 位地址矩阵；窄屏或较长错误文字按需换行，图例仍靠右。 |
| 页面主体 | 占满页面高度，无应用标题栏；两张卡片按列排列并居中，内容超出高度时纵向滚动。 |

扫描过程中按钮显示 `Scanning` 并禁用，完成后恢复 `Scan`。初次尚无总线或扫描未发现
总线时，下拉框显示 `No I2C buses` 并禁用；非扫描状态下，扫描按钮仍可再次触发扫描。有旧结果时，
新一轮扫描期间继续显示旧矩阵，总线下拉框仍可切换，因此必须结合全局状态判断结果
是否已经更新。切换下拉框只切换展示，不发起扫描。

全局状态与选中总线的摘要分别表达不同范围的信息：

| 状态 | 全局状态文字与颜色 | 地址卡片 |
| --- | --- | --- |
| 等待扫描 | `Ready to scan`，灰色 | 未选总线时显示 `7-bit address map` 和 `Hexadecimal addresses 00-7F`。 |
| 扫描中 | `Scanning available I2C buses...`，棕色 | 保留上一次结果，直到新报告到达。 |
| 扫描完成 | 如 `3 devices found on 2 buses`，绿色 | 显示选中总线的设备数量，如 `2 devices detected`。 |
| 未发现总线 | `No I2C buses found`，灰色 | 显示默认地址说明，无检测高亮。 |
| 全局失败 | 错误说明，红色 | 根据返回报告展示可用信息。 |
| 个别总线不可用 | 完成状态追加如 `; 1 unavailable`，仍为绿色 | 选中出错总线时，摘要以红色显示 `Bus unavailable: ...`。 |

### 地址矩阵语义

矩阵显示 `00`–`7F` 共 128 个十六进制地址，排列为 8 行、每行 16 个。
每个单元格直接显示两位大写地址；行号和列号不另外占用表头。

| 单元格样式 | 含义 |
| --- | --- |
| 绿色背景、白色文字、深绿边框 | 选中总线的扫描结果将该地址标记为已检测到。 |
| 白色背景、黑色文字 | 地址属于本应用的探测范围 `03`–`77`，当前没有检测高亮。 |
| 浅灰背景、灰色文字 | `00`–`02`、`78`–`7F`，属于本应用跳过的地址。 |

地址单元格支持键盘聚焦：在 `Scan` 按钮上按 `↓` 可进入地址表，随后用上下左右键
浏览相邻地址；页面和矩阵会自动滚动，使焦点地址完整可见。总线下拉框保留原生方向键
选择行为，可用 Tab 移到 `Scan`。Tab / Shift+Tab 也可在控件和地址之间切换。
单元格使用标准 LVGL 按钮加入 AppKit 默认输入组，不注册额外方向键处理；点击或按 Enter
只聚焦地址，不发起扫描、寄存器读写或设备详情操作。绿色高亮表示探测成功，
或内核报告该地址已被驱动占用；不包含芯片型号识别，也不区分这两种情况。
白色单元格在扫描未完成、总线不可用或设备未响应时都可能
出现，不能仅凭颜色判断设备是否不存在；应同时查看状态与总线摘要。

首次扫描禁用顶部控件时，焦点移到第一个可用地址 `00`，不会回绕到 `7F` 而把页面带到
底部。页面创建时将所属输入组的 refocus 策略设为 `NEXT`；扫描结果刷新不重置用户的
焦点或滚动位置。

## 架构边界与更新流程

依赖方向必须保持 `View -> ViewModel -> Model`。当前文件职责如下：

| 位置 | 职责与边界 |
| --- | --- |
| `src/pages/i2c_detector/i2c_detector_model.*` | 扫描状态、总线排序、选中总线、计数和报告更新；不依赖 LVGL，可独立测试。 |
| `src/pages/i2c_detector/i2c_scanner.*` | 扫描接口及系统实现；后台处理设备访问，向 ViewModel 交付报告。 |
| `src/pages/i2c_detector/i2c_detector_view_model.*` | 用户命令、展示文案、扫描结果接收，通过 `revision_subject()` 发布变化。 |
| `src/pages/i2c_detector/i2c_detector_view.*` | LVGL 对象、布局、事件转发、状态展示和定时轮询；不得直接修改 Model。 |
| `src/components/components.*` | 页面、卡片、按钮及语义颜色。 |
| `src/components/lv_subject.hpp` | 不可复制、不可移动的 `IntSubject` RAII 封装。 |
| `src/app.cpp` | 在 AppKit 提供的根容器下创建页面，管理 View 与 ViewModel 的创建和销毁顺序。 |

```text
打开页面 / 点击 Scan -> ViewModel::start_scan() -> Scanner 后台扫描
                                                   |
100 ms LVGL 定时器 -> ViewModel::poll() <-------------+ 返回报告
                           |
                           v
                     Model 更新报告
                           |
                           v
                  revision Subject 发布
                           |
                           v
                    View::refresh()

切换总线 -> ViewModel::select_bus() -> Model 选择 -> Subject -> View::refresh()
```

View 当前使用一个绑定到页面根对象的 revision observer，收到通知后统一刷新下拉框、
状态、摘要和所有地址单元格。扫描线程不得创建或修改 LVGL 对象，也不得直接发布
Subject；这些工作应在 LVGL 线程接收结果后完成。

新增页面时，应沿用页面目录内的 Model、ViewModel、View 分层，并补充对应测试与文档。
只有领域对象实际被多个页面共享时才创建 `src/domain/`；当前项目没有该目录或路由模块，
不得假设存在这些接口。页面私有代码应留在页面目录；承担通用样式、绑定或资源生命周期
的组件可放入 `src/components/`。纯 Model 与 UI 必须继续编译到不同依赖层。

## 颜色与组件

必须优先复用 `components.hpp` 中的语义颜色，不得在页面中重复定义同用途颜色。
以下色值对应当前组件与页面设置的实色；扫描按钮禁用时使用 50% 对象不透明度。

| 颜色角色 | 常量 | 色值 |
| --- | --- | --- |
| 页面背景、下拉框背景 | `color_page_background` | `#F2F2F7` |
| 卡片、未高亮探测地址背景 | `color_container_background` | `#FFFFFF` |
| 标题 | `color_container_title` | `#1A1A1A` |
| 正文、地址文字 | `color_text` | `#000000` |
| 边框、分隔线 | `color_border` | `#D8DDE3` |
| 主操作按钮 | `color_action` | `#20262D` |
| 检测高亮、完成状态、图例 | `color_detected` | `#167C5A` |
| 检测高亮边框 | `color_detected_border` | `#0F684B` |
| 跳过地址背景 | `color_muted_surface` | `#E9EDF1` |
| 辅助文字、默认状态 | `color_muted_text` | `#68727D` |
| 跳过地址文字 | `color_disabled_text` | `#9AA1A9` |
| 扫描中状态 | `color_scanning` | `#A45A00` |
| 错误状态、不可用总线摘要 | `color_error` | `#B42318` |

新增颜色时必须记录语义角色、场景、色值和透明度；跨页面共享的颜色应提升到
`src/components/`。不得用渐变替代明确的状态色。组件必须在调用者传入的 parent 下构建，
并明确对象、回调、observer 和 timer 的所有权。

## 字体与文本

显示文本的 UI 源文件必须包含 `<cm0/typography.h>`，并在 `lv_init()` 后通过
`lilygo_ui_font_get(size)` 获取 AppKit 字体。当前页面使用固定的三档字号：

| 字号 | 当前用途 |
| --- | --- |
| 22 px | 总线下拉框及展开选项、扫描按钮、地址卡片标题 |
| 18 px | 全局扫描状态 |
| 14 px | 总线摘要、图例、地址单元格 |

- 设置字体样式前必须确认返回指针非空；共享字体及其 fallback 链由 AppKit 管理，应用不得修改或释放。
- 相同信息层级应使用一致字号，不得根据视口连续计算字号或引入大量相近字号。AppKit 会缓存每个请求的字号，直到字体系统反初始化。
- 修复溢出应先精简文案、调整布局或设置换行与截断。当前全局状态和总线摘要使用自动换行，应保留长错误文案的展示空间。
- 不得在公共 UI 中引用 `lv_font_montserrat_*`，也不得复制、改名或随应用重新打包 AppKit 已提供的字体。

AppKit 通过 LVGL FreeType 在运行时加载 Inter、Source Han Sans CN 中文 fallback 和
Font Awesome 图标 fallback。中西文混排应共用同一字体链，不得为切换字体拆分标签，
或依赖操作系统字体发现。当前界面文案为英文；增加中文文案时也应沿用这条字体链。

仅当共享字体缺少所需字形时，才可在 `assets/fonts/` 添加应用专用字体。必须通过
`lv_freetype_font_create()` 在运行时加载，以应用字体作为对象主字体，并将其 fallback
设为同字号的 AppKit 字体。应用字体必须晚于所有引用对象释放，之后调用
`lv_freetype_font_delete()`；不得修改 AppKit 的共享字体对象。新增字体须记录来源、许可
和稳定运行时路径，并补充安装与打包规则。需要子集化时应保留可重复的生成流程。

AppKit 字体按 SIL Open Font License 1.1 分发，不得描述为“无版权字体”或单独销售。
包含字体的分发物必须保留 `NOTICE.md` 与 `OFL-1.1.txt`；本项目通过
`lilygo_ui_install_font_licenses()` 安装共享字体许可声明。

## 响应式布局

当前 `apply_layout()` 根据页面根对象的实际内容宽度设置布局，由根对象的
`LV_EVENT_SIZE_CHANGED` 触发，无须重建页面。横竖屏共用同一对象树，均为两张卡片上下排列。

| 根对象内容宽度 | 工具栏 | 卡片和地址矩阵 |
| --- | --- | --- |
| `< 480 px` | 下拉框和按钮上下排列，均占工具栏全宽。 | 卡片占主体可用宽度；矩阵固定为 500 px，通过内部横向滚动查看全部地址。 |
| `480–999 px` | 下拉框弹性填充，按钮宽 164 px，横向排列。 | 卡片占主体可用宽度，矩阵填满卡片内部可用宽度。 |
| `>= 1000 px` | 与普通宽度相同。 | 两张卡片宽度设为 1000 px，并在主体中居中；矩阵随卡片内部宽度布局。 |

窄布局的主体上、左、右边距为 12 px，底部为 28 px；其他宽度分别为 20 px 和 40 px。
两张卡片之间间距为 16 px。
地址矩阵使用 Flex 列与 Flex 行，单元格高度为 40 px，行列间距为 4 px。
矩阵滚动容器高 348 px，仅负责横向滚动；页面主体负责纵向滚动。

后续布局修改必须遵守：

- 主体使用 LVGL Flex 或 Grid，结合百分比、`LV_SIZE_CONTENT`、弹性尺寸、对齐与滚动分配空间，不得依赖大量绝对坐标。
- 固定像素用于图标、间距、边框、控件下限和明确的设计尺寸；连续尺寸变化交给 layout，结构变化才设置断点。
- 断点依据父容器实际可用宽高，不得依据设备型号、CPU 架构或展示模式；可复用组件不得假设占满 display。
- 布局更新必须可重复执行，避免重建页面，并保留业务状态、焦点、滚动位置和用户输入。
- 内容变长或出现字体回退时，文字不得重叠或意外裁切；主要操作必须可达，地址矩阵应可完整浏览。

以上是实现约定，现有两种默认渲染视口并未覆盖所有宽度与断点边界；验收时还需检查
紧凑布局及宽度临界值附近的实际可用空间。

## 对象名称与生命周期

以下名称可用于渲染测试定位，修改对象结构时应保留其语义和稳定性：

| 对象名称 | 对应对象 |
| --- | --- |
| `app_surface` | 页面根容器 |
| `i2c_body`、`i2c_controls` | 纵向滚动主体、控制卡片 |
| `i2c_bus_selector`、`i2c_scan`、`i2c_status` | 总线下拉框、扫描按钮、全局状态 |
| `i2c_address_panel`、`i2c_bus_title`、`i2c_bus_summary` | 地址卡片及总线标题、摘要 |
| `i2c_bus_details` | 总线路径、设备数量或错误、检测图例的横向容器 |
| `i2c_detected_swatch` | 绿色图例色块 |
| `i2c_address_scroller`、`i2c_address_matrix` | 矩阵横向滚动容器、矩阵主体 |
| `i2c_address_00`–`i2c_address_7f` | 地址单元格，名称使用两位小写十六进制，例如 `i2c_address_3c`；显示文字为 `3C`。 |

固定名称使用 `lv_obj_set_name_static()`；地址名称在循环中生成，通过
`lv_obj_set_name()` 复制保存，不得将临时缓冲区交给静态名称接口。

`src/app.cpp` 先创建 ViewModel，再创建 View；关闭时先调用 `View::destroy()`，
再释放 View，最后释放 ViewModel。View 不在析构函数中自动销毁页面，调用者必须遵循
该关闭流程。`destroy()` 先删除 100 ms 轮询 timer，再删除页面根对象，并清空所有缓存
对象指针和地址数组，可以重复调用。根对象删除时由 LVGL 一并移除子对象及对象绑定的 observer。

- View 必须在传入的 parent 下创建顶层对象，不得假定 parent 就是当前 screen。
- ViewModel 必须比绑定其 Subject 的 View 存活更久；Subject 应复用 RAII 封装，不得复制或移动底层 `lv_subject_t`。
- `user_data` 指向的对象必须在回调注册期间存活；非对象绑定的 observer、timer、输入监听器等必须显式注销。
- 父对象删除后不得再次删除子对象，也不得使用缓存指针。
- 后续若加入页面切换，不得在 observer 或事件回调中同步删除触发回调的页面，应延迟切换。

## 测试覆盖与验收

修改 UI 实现后，至少执行以下主机配置、构建和测试；也可使用 README 中的 `lpm test`：

```sh
cmake --preset host-simulator
cmake --build --preset host-simulator --parallel
ctest --preset host-simulator
```

| 测试 | 当前自动检查 |
| --- | --- |
| `i2cdetector-core` | 无需初始化 LVGL 的 Model 状态转换、总线排序、计数、选中总线保留、空报告和全局错误。 |
| `i2cdetector-view-model` | 使用 FakeScanner 检查启动与重复启动、结果轮询、总线切换、展示文案和 revision Subject 数值更新。 |
| `i2cdetector-render-portrait` | 在 `568 x 1232` 下执行下述渲染检查和交互。 |
| `i2cdetector-render-landscape` | 在 `1232 x 568` 下执行相同检查和交互。 |
| `i2cdetector-keyboard-landscape`、`i2cdetector-keyboard-compact` | 在 `1232 x 568` 和 `320 x 568` 下通过真实 AppKit 运行时输入链验证方向键浏览、自动滚动及 Tab 切换。 |
| 上述键盘测试的 `-no-buses`、`-error` 变体 | 验证首次扫描未发现总线或失败时的启动位置和键盘浏览。 |

[`tests/render_ui.cpp`](../tests/render_ui.cpp) 注入固定扫描报告：`/dev/i2c-1` 有 `20`，
`/dev/i2c-4` 有 `3C` 和 `68`。测试检查关键对象存在、页面与卡片背景色、汇总文字、
总线选项、选中总线的地址高亮，以及 `7F` 的灰色背景；再发送总线切换事件，检查摘要
和高亮更新，并通过扫描按钮事件检查禁用与恢复。最后确认画面不是单一颜色并写出 PPM，
另以 `.dropdown.ppm` 后缀输出展开下拉列表的画面。

主机预设下产物为 `build/host-simulator/i2cdetector-portrait.ppm` 和
`build/host-simulator/i2cdetector-landscape.ppm`。这些检查没有进行像素基准比较，也没有
自动验证控件几何位置、触摸命中、字体缺字、滚动范围或运行中的尺寸切换。两种测试均
使用固定报告，不访问真实 I2C 设备；无总线、错误和慢扫描的界面效果仍需补充检查。

[`tests/test_keyboard_navigation.cpp`](../tests/test_keyboard_navigation.cpp) 在 SDL 模拟器中
向 AppKit 创建的键盘注入按下与松开，检查从 `Scan` 向下进入地址表、逐行到达末行、
左右浏览到 `70` 和 `7F`，以及焦点地址位于纵向和横向可见范围内；同时检查 Enter
不会触发扫描、Tab / Shift+Tab 切换和向上返回控制区。首次结果延迟交付，让 AppKit
多次处理禁用控件和焦点滚动，检查扫描等待及完成后页面仍在顶部，并验证浏览地址后
报告刷新不会抢走焦点或重置滚动。这些测试不访问真实硬件。

UI 验收应检查两张渲染图，并在模拟器或设备上验证：

- 横竖屏、`480 px` 与 `1000 px` 断点附近，以及小于 `480 px` 时，卡片、下拉框、按钮和矩阵没有重叠或意外裁切。
- 横屏内容可纵向滚动，紧凑布局的矩阵可横向滚动至 `7F`；操作有足够命中区域。
- 无总线、扫描中、不可用总线和较长错误文案可以正确展示，状态色与地址颜色符合上文语义。
- 切换总线、重新扫描及尺寸变化后，结果与选择符合状态逻辑；焦点、滚动和后续新增输入状态按要求保留。
- 文字无缺字或错误回退，关闭页面后没有遗留 timer、observer、重复删除或悬空指针。

涉及 SDK、工具链、安装或字体等打包资源的实现变更，还必须完成 `cm0-cross` 构建和
Debian 打包验证。仅调整文档时应核对源码、命令、路径和链接，不需要为文字修改重新执行全量构建。
