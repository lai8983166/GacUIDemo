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

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int CmdShow)
{
	return SetupWindowsDirect2DRenderer();
}

void GuiMain()
{
	{
		FileStream fileStream(L"UIRes/PunkUI.bin", FileStream::ReadOnly);
		GetResourceManager()->LoadResourceOrPending(fileStream);
	}
	punkui::MainWindow window;
	window.MoveToScreenCenter();
	GetApplication()->Run(&window);
}
