#define GAC_HEADER_USE_NAMESPACE
#include "UI/Source/PunkUI.h"
#include "Skin/Source/PunkSkin.h"
#include <Skins\DarkSkin\DarkSkin.h>
#define _WINSOCKAPI_
#include <Windows.h>

using namespace vl::stream;
using namespace vl::presentation;

class PunkThemePlugin : public Object, public IGuiPlugin
{
public:

	GUI_PLUGIN_NAME(Custom_PunkThemePlugin)
	{
		GUI_PLUGIN_DEPEND(GacGen_DarkSkinResourceLoader);
		GUI_PLUGIN_DEPEND(GacGen_PunkSkinResourceLoader);
	}

	void Load(bool controllerUnrelatedPlugins, bool controllerRelatedPlugins)override
	{
		// DarkSkin 提供未覆盖控件的兜底模板，PunkSkin 后注册、按控件覆盖优先
		RegisterTheme(Ptr(new darkskin::Theme));
		RegisterTheme(Ptr(new punkskin::Theme));
	}

	void Unload(bool controllerUnrelatedPlugins, bool controllerRelatedPlugins)override
	{
	}
};
GUI_REGISTER_PLUGIN(PunkThemePlugin)

// UILIB 演示按视口全屏适配：启动即最大化。
// 不能在 Run 前 ShowMaximized——WindowService::Run 以 SW_SHOWNORMAL Show 窗口，
// 会把最大化状态还原回原始尺寸；监听 Opened（WM_SHOWWINDOW，Show 内同步派发），
// 在窗口真正显示后再最大化
class StartupMaximizer : public vl::presentation::INativeWindowListener
{
private:
	vl::presentation::INativeWindow* window = nullptr;
public:
	StartupMaximizer(vl::presentation::INativeWindow* _window)
		: window(_window)
	{
	}

	void Opened()override
	{
		window->ShowMaximized();
	}
};

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int CmdShow)
{
	return SetupWindowsDirect2DRenderer();
}

void GuiMain()
{
	// 应用资源由生成的 GacGen_PunkUIResourceLoader 插件在启动时自动加载（内嵌于 exe），
	// 无需外部 bin 文件，双击 exe 即可运行。
	punkui::MainWindow window;
	StartupMaximizer maximizer(window.GetNativeWindow());
	window.GetNativeWindow()->InstallListener(&maximizer);
	GetApplication()->Run(&window);
}
