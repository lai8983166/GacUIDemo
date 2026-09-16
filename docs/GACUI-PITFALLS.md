# GacUI 踩坑记录（PunkUI 实战）

PunkUI 开发过程中踩平的 GacUI 坑。适用：GacUI Release 仓库（`F:/project/GacUIDemo/gacui`），
x64 + Direct2D 全窗口重绘模式，XML 资源 + code-behind（GacGen）工作流。
行号以当前 checkout 为准，升级后可能漂移，函数名不会。

## 布局与命中测试

### 1. `AlignmentToParent` 的 -1 哨兵

- **现象**：位移/尺寸计算后某个组合"突然不渲染"或布局全乱，无任何报错。
- **根因**：`SetAlignmentToParent(Margin)` 中任一边**恰为 -1 表示该边自由（不对齐）**。
  手写舍入 `(vint)(0.5 + nx)` 在 `nx = -2` 时恰好截断出 -1，把"对齐边"静默变成"自由边"。
- **修复**：浮点转整数用 `llround`；算出恰为 -1 的边退化为 0。
- 反向用法：刻意让某边自由（如趋势柱顶边自由实现"高度自适应、底对齐"）就靠 top:-1。

### 2. `GuiButton` 默认忽略子组合命中

- **现象**：皮肤/模板命中深层组合时，按钮 hover、Pressed 状态**全部静默失效**。
- **根因**：`GuiButton::ignoreChildControlMouseEvents` 默认 `true`
  （`Import/GacUI.h:13354`）。控件的 `OnMouseEnter`/`OnMouseDown` 要求
  `eventSource == boundsComposition`，否则 `mousePressingDirect/Indirect` 都不置位，
  整个处理体被跳过。组合级 hit 链派发无此过滤。
- **修复**：模板构造期拿不到宿主（`GetRelatedControl() == null`），须在**组合级
  mouseEnter 回调里惰性** `SetIgnoreChildControlMouseEvents(false)`。

### 3. 布局后位置一律用 `GetGlobalBounds()`

- **现象**：hover 命中判断恒失败、覆盖层画错位置，静默无报错。
- **根因**：
  - `GetBounds()` **不存在**（C2039）；
  - `GetExpectedBounds()` 只在 `GuiBoundsComposition` 上有，且是**相对父组合**的坐标——
    XML `<Cell>` 里 ref.Name 的组合父是 `GuiCellComposition`，直接拿来当相对 Table
    的坐标会全错；
  - `GuiCellComposition` 本身没有公开的 bounds 访问器。
- **正解**：`GetGlobalBounds()`（`Import/GacUI.cpp:4506`）返回全局坐标，与 owned
  element 渲染参数 `GuiDirect2DElementEventArgs::bounds` **同一坐标系**——画覆盖层
  和命中判断都用它最稳，不需要手工累加父链偏移。
  布局完成前它是未初始化值；且滚动/重排后会变，**每次 mouseMove 重新收集**，不要懒缓存。

## 动画与生命周期

### 4. 构造期 `AddAnimation` 永不启动

- **现象**：控件构造函数里挂的动画停在 pending 状态，实测不会跑。
- **根因**：窗口 attach 之前动画系统没有驱动源。
- **修复**：初始状态同步**直写终态**，判据 `GetRelatedControlHost()` 非空（已挂树）。

### 5. 补间 frame 回调里不能用变换后的参数判断结束

- **现象**：弹窗关闭动画播完但面板不消失，表现恰似"性能卡顿导致关不上"。
- **根因**：关闭补间传 `ApplyModal(1.0 - t)`，其内部参数在终帧恰为 0，
  写 `if (t >= 1.0) SetVisible(false)` **永远不成立**。
- **修复**：收尾动作（隐藏/释放资源）一律放 **finished 回调**——
  PunkTween 保证先 `frame(1.0)` 再 `finished`，判断必须用补间自己的 t。

### 6. Run 前最大化会被还原

- **现象**：`ShowMaximized()` 在 `Run()` 前调用无效，窗口以原始尺寸出现；
  改用 `SetBounds(工作区)` 又出现超屏（4K 屏实测 4800x2182 > 3840x2160）。
- **根因**：
  - `WindowService::Run`（`Import/GacUI.Windows.cpp:2107`）内部 `Show()`
    即 `ShowWindow(SW_SHOWNORMAL)`（`GacUI.Windows.cpp:1684`），把之前的最大化状态
    还原回原始尺寸；
  - 未显示的窗口做 `SetBounds` 时 DPI 缩放语义错位（逻辑/物理换算按未显示状态计算）。
- **正解**：装 `INativeWindowListener`，在 `Opened()`（`WM_SHOWWINDOW` 内同步派发，
  `GacUI.Windows.cpp:590`，晚于 Run 的 Show）里调 `ShowMaximized()`。

## 渲染性能

### 7. 全窗口重绘模式下禁逐点绘制

- **现象**：全屏半调网点逐点 `FillEllipse`（10px 网格 × 2560x1440 ≈ 37k 次/帧），
  动画单帧数百 ms，**所有**补间肉眼卡顿（点开关也卡）。
- **修复**：`sp×sp` 单点 tile 位图（premultiplied BGRA，点心在 tile 中心）+
  `ID2D1BitmapBrush(D2D1_EXTEND_MODE_WRAP, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR)`
  + brush transform 对齐相位，一次 `FillGeometry` 填整块剪影。
  tile 按（RT 指针, 参数组）缓存，RT 重建（resize/设备丢失）即失效重建。
- **注意**：枚举实名是 `D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR`，
  不是 `_NEAREST`（后者不存在，C2039）。

## 合成鼠标测试方法（PostMessage 无干扰验证）

要求不抢真实桌面焦点/光标时的 UI 自动化验证套路：

- 窗口消息全部 `PostMessageW`（`WM_MOUSEMOVE` 0x0200 / `WM_LBUTTONDOWN` 0x0201 /
  `WM_LBUTTONUP` 0x0202 / `WM_MOUSEWHEEL` 0x020A），lParam 用客户区物理像素；
  截图用 `PrintWindow`，不 `SetCursorPos`、不抢焦点。
- **WM_MOUSELEAVE 污染**：真实光标在窗外时，合成 move 后系统会补发
  WM_MOUSELEAVE（约 200ms 内到达），清除刚合成的 hover 态。
  对策：move **循环保活**（每 ~20ms 一次）+ 连拍截图。
- 采样坐标不要估：先截全页，从截图量出目标元素的物理像素位置再打。
- 残留进程占用 exe 导致 LNK1104：先 `taskkill /F /IM PunkUI.exe` 再构建。
