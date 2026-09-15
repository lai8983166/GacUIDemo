#include "PunkPanel.h"

using namespace vl::presentation;

namespace punkui
{
	// ============ 预设样式（对应 UILIB punk-collage 规格） ============

	PunkPanelStyle& PunkPanelStyle::Card()
	{
		static PunkPanelStyle s = []
		{
			PunkPanelStyle v;
			v.background = Color(255, 255, 255);
			v.borderWidth = 2;
			v.shadowOffset = 6;
			v.chamferTopRight = 16;
			v.chamferBottomLeft = 16;
			v.halftone = true;
			v.dotSpacing = 7;
			v.dotRadius = 1.3;
			v.dotColor = Color(0, 0, 0, 18);
			return v;
		}();
		return s;
	}

	PunkPanelStyle& PunkPanelStyle::WindowBg()
	{
		static PunkPanelStyle s = []
		{
			PunkPanelStyle v;
			v.background = Color(0xEF, 0xEA, 0xE0);
			v.borderWidth = 0;
			v.shadowOffset = 0;
			v.halftone = true;
			v.dotSpacing = 10;
			v.dotRadius = 1.6;
			v.dotColor = Color(0, 0, 0, 26);
			return v;
		}();
		return s;
	}

	PunkPanelStyle& PunkPanelStyle::Badge()
	{
		static PunkPanelStyle s = []
		{
			PunkPanelStyle v;
			v.background = Color(0x21, 0x21, 0x21);
			v.borderWidth = 2;
			v.shadowOffset = 2;
			v.chamferBottomLeft = 8;
			v.halftone = false;
			return v;
		}();
		return s;
	}

	PunkPanelStyle& PunkPanelStyle::Alert()
	{
		static PunkPanelStyle s = []
		{
			PunkPanelStyle v;
			v.background = Color(255, 255, 255);
			v.borderWidth = 3;
			v.shadowOffset = 4;
			v.halftone = true;
			v.dotSpacing = 7;
			v.dotRadius = 1.2;
			v.dotColor = Color(0, 0, 0, 15);
			return v;
		}();
		return s;
	}

	PunkPanelStyle& PunkPanelStyle::Bar()
	{
		static PunkPanelStyle s = []
		{
			PunkPanelStyle v;
			v.background = Color(0xE8, 0x15, 0x1C);
			v.borderWidth = 0;
			v.shadowOffset = 0;
			v.chamferBottomLeft = 6;
			v.halftone = false;
			return v;
		}();
		return s;
	}

	PunkPanelStyle& PunkPanelStyle::Nav()
	{
		static PunkPanelStyle s = []
		{
			PunkPanelStyle v;
			v.background = Color(0xE8, 0x15, 0x1C);
			v.borderWidth = 2;
			v.shadowOffset = 3;
			v.slant = 7;
			v.halftone = false;
			return v;
		}();
		return s;
	}

	// ============ 面板 ============

	PunkPanel::PunkPanel(const PunkPanelStyle& _style)
		: style(_style)
	{
	}

	void PunkPanel::AttachTo(GuiGraphicsComposition* composition)
	{
		element = GuiDirect2DElement::Create();
		element->Rendering.AttachMethod(this, &PunkPanel::OnRendering);
		composition->SetOwnedElement(Ptr(element));
	}

	void PunkPanel::UpdateStyle(const PunkPanelStyle& _style)
	{
		style = _style;
	}

	void PunkPanel::RequestRender(GuiControlHost* host)
	{
		if (element && element->GetRenderer() && host)
		{
			host->GetGraphicsHost()->RequestRender();
		}
	}

	bool PunkPanel::IsPointInside(float x, float y, float x1, float y1, float x2, float y2)
	{
		if (x < x1 || x > x2 || y < y1 || y > y2) return false;
		if (style.slant > 0)
		{
			float h = y2 - y1;
			if (h <= 0) return false;
			float t = (y - y1) / h;
			if (x < x1 + (float)style.slant * (1.0f - t)) return false;
			if (x > x2 - (float)style.slant * t) return false;
		}
		if (style.chamferTopRight > 0)
		{
			float c = (float)style.chamferTopRight;
			if (x > x2 - c && y < y1 + c && (x - (x2 - c)) + (y1 + c - y) > c) return false;
		}
		if (style.chamferBottomLeft > 0)
		{
			float c = (float)style.chamferBottomLeft;
			if (x < x1 + c && y > y2 - c && ((x1 + c) - x) + (y - (y2 - c)) > c) return false;
		}
		return true;
	}

	static D2D1_COLOR_F ToColorF(const Color& c)
	{
		return D2D1::ColorF(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
	}

	void PunkPanel::OnRendering(GuiGraphicsComposition* sender, GuiDirect2DElementEventArgs& arguments)
	{
		ID2D1RenderTarget* rt = arguments.rt;
		if (!rt) return;

		const Rect& b = arguments.bounds;
		vint so = style.shadowOffset;
		float x1 = (float)b.x1;
		float y1 = (float)b.y1;
		float x2 = (float)b.x2 - so;
		float y2 = (float)b.y2 - so;
		float w = x2 - x1;
		float h = y2 - y1;
		if (w <= 1 || h <= 1) return;

		// ---- 构建剪影路径 ----
		D2D1_POINT_2F pts[8];
		vint n = 0;
		if (style.slant > 0)
		{
			float s = (float)style.slant;
			pts[n++] = { x1 + s, y1 };		// 上左（内收）
			pts[n++] = { x2, y1 };			// 上右
			pts[n++] = { x2 - s, y2 };		// 下右（内收）
			pts[n++] = { x1, y2 };			// 下左
		}
		else
		{
			float ctr = (float)style.chamferTopRight;
			float cbl = (float)style.chamferBottomLeft;
			pts[n++] = { x1, y1 };					// 左上
			pts[n++] = { x2 - ctr, y1 };			// 右上（切角前）
			pts[n++] = { x2, y1 + ctr };			// 右上（切角后）
			pts[n++] = { x2, y2 };					// 右下
			pts[n++] = { x1 + cbl, y2 };			// 左下（切角前）
			pts[n++] = { x1, y2 - cbl };			// 左下（切角后）
		}

		ID2D1PathGeometry* geoRaw = nullptr;
		if (FAILED(arguments.factoryD2D->CreatePathGeometry(&geoRaw)) || !geoRaw) return;
		ComPtr<ID2D1PathGeometry> geo = geoRaw;
		ID2D1GeometrySink* sinkRaw = nullptr;
		if (FAILED(geo->Open(&sinkRaw)) || !sinkRaw) return;
		ComPtr<ID2D1GeometrySink> sink = sinkRaw;

		sink->BeginFigure(pts[0], D2D1_FIGURE_BEGIN_FILLED);
		sink->AddLines(pts + 1, (UINT32)(n - 1));
		sink->EndFigure(D2D1_FIGURE_END_CLOSED);
		sink->Close();

		// ---- 画刷 ----
		auto CreateBrush = [&](const Color& c)
		{
			ID2D1SolidColorBrush* brush = nullptr;
			rt->CreateSolidColorBrush(ToColorF(c), &brush);
			return ComPtr<ID2D1SolidColorBrush>(brush);
		};
		auto bgBrush = CreateBrush(style.background);
		auto borderBrush = CreateBrush(style.borderColor);
		auto shadowBrush = CreateBrush(style.shadowColor);
		auto dotBrush = CreateBrush(style.dotColor);

		// ---- 硬投影（右下偏移）----
		if (so > 0 && shadowBrush)
		{
			D2D1_MATRIX_3X2_F oldT;
			rt->GetTransform(&oldT);
			rt->SetTransform(D2D1::Matrix3x2F::Translation((FLOAT)so, (FLOAT)so));
			rt->FillGeometry(geo.Obj(), shadowBrush.Obj());
			rt->SetTransform(oldT);
		}

		// ---- 底色 ----
		if (bgBrush)
		{
			rt->FillGeometry(geo.Obj(), bgBrush.Obj());
		}

		// ---- 半调网点（点心在剪影内）----
		if (style.halftone && dotBrush)
		{
			float sp = (float)style.dotSpacing;
			float r = (float)style.dotRadius;
			for (float y = y1 + sp / 2; y < y2; y += sp)
			{
				for (float x = x1 + sp / 2; x < x2; x += sp)
				{
					if (IsPointInside(x, y, x1, y1, x2, y2))
					{
						rt->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), r, r), dotBrush.Obj());
					}
				}
			}
		}

		// ---- 墨边 ----
		if (style.borderWidth > 0 && borderBrush)
		{
			rt->DrawGeometry(geo.Obj(), borderBrush.Obj(), (FLOAT)style.borderWidth);
		}
	}
}
