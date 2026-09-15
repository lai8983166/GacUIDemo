# PunkUI — 用 GacUI 复刻 UILIB 朋克拼贴风格

用 [GacUI](https://github.com/vczh-libraries/Release)（vczh 的 C++ 原生 UI 库，Direct2D 渲染）
复刻 [UILIB](https://github.com/lai8983166/UILIB)（Vue3 换肤演示项目）中的
**朋克拼贴 (P5)** 主题界面 —— Persona-5 风格：红黑白三色 + 牛皮纸半调网点 +
对角切角剪影 + 重墨描边 + 硬偏移投影。

## 界面内容（与 UILIB 一致）

- **组件展示页**：按钮（默认/主要/描边/幽灵/危险/禁用）、徽标（6 种 tone）、
  切角卡片 ×3、终端会话（彩色状态标签）、表单（输入框/文本域/开关）、
  进度条（含 80% 红色阈值刻线）、提示条 ×4（左边色条）、头像 ×3、表格
- **控制台页**：控制台页头、主设备面板（4 组大数字读数 + 12 柱红色切角趋势图）、
  系统状态栏（状态灯列表）、事件记录表
- **弹窗**：黑色 55% 遮罩 + 朋克切角卡片（维护确认）
- **窗口**：自定义标题栏（黑底 + 红色 ◆ + 4px 红下边框 + 红色关闭钮）、
  顶部导航（激活项为红色平行四边形）

## 目录结构

```
PunkUI/
├── Main.cpp                 应用入口：双主题注册（DarkSkin 兜底 + PunkSkin 覆盖）
├── PunkUI.vcxproj           MSVC v143 / x64 / C++20
├── build.bat                一键构建（GacGen 资源生成 + MSBuild）
├── PunkElements/
│   ├── PunkPanel.h/.cpp     朋克面板元素：GuiDirect2DElement 直绘
│                            （切角/半调网点/墨边/硬投影，预设 Card/WindowBg/
│                             Badge/Alert/Bar/Nav 样式）
├── Skin/                    朋克皮肤（GacUI XML 皮肤，结构仿 DarkSkin）
│   ├── Resource.xml         GacGen 配置（CppCompressed 内嵌资源）
│   ├── Index.xml            ThemeTemplates 控件模板绑定
│   ├── Style.xml            状态驱动的颜色脚本（hover/press/选中/禁用）
│   ├── Template_Window.xml  自定义标题栏窗口模板
│   ├── Template_Button.xml  按钮 + 开关（SelectableButton）模板
│   ├── Template_Text.xml    单行/多行输入框模板（聚焦投影变红）
│   ├── Template_Scroll.xml  滚动条/进度条/滚动视图模板
│   ├── Template_Misc.xml    标签模板
│   └── Source/              GacGen 生成物（勿手改，UserImpl 区除外）
├── UI/                      应用界面
│   ├── Resource.xml         GacGen 配置（外部 bin）
│   ├── MainWindow.xml       窗口：导航条 + 双页切换 + 弹窗遮罩
│   ├── ShowcasePage.xml     组件展示页（ScrollContainer）
│   ├── DashboardPage.xml    控制台页
│   ├── Templates.xml        按钮变体模板（主要/描边/幽灵/危险）+ 导航模板
│   ├── AppStyle.xml         变体按钮状态色
│   └── Source/              GacGen 生成物（MainWindow/页面模板 + UserImpl 实现）
└── UIRes/PunkUI.bin         生成的资源二进制
```

## 构建与运行

前置条件：
1. Visual Studio 2022（v143 工具集，MSVC 14.3x）
2. `../gacui` 为 [vczh-libraries/Release](https://github.com/vczh-libraries/Release) 的克隆
3. 已构建工具链（一次性）：
   ```
   msbuild gacui\Tools\Executables\Executables.sln /p:Configuration=Release /p:Platform=x86 /p:PlatformToolset=v143
   ```
   并将 `gacui-src/Test/Resources/Metadata/Reflection64.bin`、`Reflection32.bin`
   拷贝到 `gacui/Tools/Executables/Release/`（GacGen 运行所需的类型元数据）。

构建：
```
build.bat            # Release
build.bat Debug      # Debug
```

运行（工作目录必须是 PunkUI，程序读取相对路径 `UIRes/PunkUI.bin`）：
```
Bin\x64\Release\PunkUI.exe
```

## 架构说明

- **主题分层**：GacUI 的 `RegisterTheme` 支持多层叠加，`CreateStyle` 从最后注册的
  主题向前查找。本应用先注册 DarkSkin（兜底全部控件），再注册 PunkSkin（只覆盖
  窗口/按钮/开关/输入框/滚动/进度/提示等），未覆盖控件自动回落 DarkSkin。
- **朋克面板（PunkPanel）**：半调网点与切角剪影无法用内置元素表达（Polygon
  固定尺寸不拉伸），故基于 `GuiDirect2DElement`（GacUI 内置的自定义 D2D 绘制
  元素，见 `Tutorial/GacUI_Windows/Direct2DClock`）实现：路径几何 + 偏移投影 +
  网点栅格 + 描边，一次绘制完成。以 owned element 挂到组合上，渲染在该组合
  所有子内容之下，投影可外溢出组合边界（元素渲染无裁剪）。
- **按钮变体**：GacUI Button 无 variant 概念，用应用资源内的多个 ControlTemplate
  实例（`ControlTemplate="punkui::PrimaryButtonTemplate"` 等）解决。
- **粗描边**：内置 `SolidBorder` 仅 1px，N 像素描边用「黑底 + 内缩 N 像素前景」
  或嵌套 Bounds 叠加实现。
- **已知合理偏离**：GacUI 无文本 skew/rotate，标题直立加粗呈现，斜切感由红色
  斜条与平行四边形剪影承担（UILIB 最终版按钮本身也已退化为矩形+黑边+硬投影）。

## 截图

- `shot_full.png` 组件展示页
- `shot_dash2.png` 控制台页
- `shot_modal.png` 维护确认弹窗
