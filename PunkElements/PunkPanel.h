#pragma once

#define GAC_HEADER_USE_NAMESPACE
#include <GacUI.h>
#include <GacUI.Windows.h>

// 朋克拼贴面板：切角剪影 + 重墨描边 + 硬偏移投影 + 半调网点
// 通过 GuiDirect2DElement 直接以 Direct2D 绘制，挂载为组合的 owned element，
// 渲染在该组合所有子内容之下，投影可外溢到组合边界之外（元素渲染无裁剪）。
namespace punkui
{
	using namespace vl;
	using namespace vl::presentation;
	using namespace vl::presentation::elements;
	using namespace vl::presentation::controls;
	using namespace vl::presentation::compositions;

	struct PunkPanelStyle
	{
		Color	background = Color(255, 255, 255);		// 面板底色
		Color	borderColor = Color(0, 0, 0);			// 墨边
		Color	shadowColor = Color(0, 0, 0);			// 硬投影
		Color	dotColor = Color(0, 0, 0, 25);			// 半调点（alpha 25 ≈ 0.1）
		vint	borderWidth = 2;
		vint	shadowOffset = 6;
		vint	chamferTopRight = 0;					// 右上对角切角
		vint	chamferBottomLeft = 0;					// 左下对角切角
		vint	slant = 0;								// 平行四边形斜切（左右边同向倾斜）
		bool	halftone = false;						// 半调网点
		vint	dotSpacing = 7;
		double	dotRadius = 1.3;

		static PunkPanelStyle& Card();		// 卡片：白底半调 + 切角 + 墨边 + 6px 投影
		static PunkPanelStyle& WindowBg();	// 窗口底：牛皮纸 + 10px 半调
		static PunkPanelStyle& Badge();	// 徽标：切角 + 墨边 + 2px 投影
		static PunkPanelStyle& Alert();	// 提示条：3px 墨边 + 4px 投影 + 半调
		static PunkPanelStyle& Bar();		// 图表柱：红底 + 左下切角
		static PunkPanelStyle& Nav();		// 导航：平行四边形
	};

	class PunkPanel : public Object
	{
	private:
		PunkPanelStyle				style;
		GuiDirect2DElement*			element = nullptr;

		void						OnRendering(GuiGraphicsComposition* sender, GuiDirect2DElementEventArgs& arguments);
		bool						IsPointInside(float x, float y, float x1, float y1, float x2, float y2);
	public:
		PunkPanel(const PunkPanelStyle& _style);

		// 挂载为组合的 owned element；绘制覆盖组合整个边界
		void						AttachTo(GuiGraphicsComposition* composition);
		void						UpdateStyle(const PunkPanelStyle& _style);
		void						RequestRender(GuiControlHost* host);
	};
}
