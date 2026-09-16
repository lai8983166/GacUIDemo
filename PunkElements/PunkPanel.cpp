#include "PunkPanel.h"
#include <vector>
#include <cmath>

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

	PunkPanelStyle& PunkPanelStyle::Table()
	{
		static PunkPanelStyle s = []
		{
			// UILIB punk .ui-table-wrap：border 3px #000 + box-shadow 6px 6px 0 #000 + 白底
			PunkPanelStyle v;
			v.background = Color(255, 255, 255);
			v.borderWidth = 3;
			v.shadowOffset = 6;
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

	PunkPanel::~PunkPanel()
	{
		if (halftoneTile) halftoneTile->Release();
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

	static D2D1_COLOR_F ToColorF(const Color& c)
	{
		return D2D1::ColorF(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
	}

	// 生成 sp×sp 单点 tile 位图（premultiplied BGRA，点心在 tile 中心，边缘 1px 抗锯齿），
	// 供 wrap 平铺画刷整块填充半调点阵。失败返回 null（下一帧重试）。
	static ID2D1Bitmap* CreateDotTile(ID2D1RenderTarget* rt, const Color& dot, int sp, float r)
	{
		std::vector<unsigned char> px((size_t)sp * sp * 4);
		float cx = sp * 0.5f;
		float cy = sp * 0.5f;
		for (int y = 0; y < sp; y++)
		{
			for (int x = 0; x < sp; x++)
			{
				float dx = x + 0.5f - cx;
				float dy = y + 0.5f - cy;
				float cov = r + 0.5f - sqrtf(dx * dx + dy * dy);
				cov = cov < 0 ? 0 : (cov > 1 ? 1 : cov);
				unsigned char a = (unsigned char)(0.5 + dot.a * cov);
				unsigned char* p = &px[((size_t)y * sp + x) * 4];
				p[0] = (unsigned char)(dot.b * a / 255);
				p[1] = (unsigned char)(dot.g * a / 255);
				p[2] = (unsigned char)(dot.r * a / 255);
				p[3] = a;
			}
		}
		ID2D1Bitmap* bitmap = nullptr;
		auto props = D2D1::BitmapProperties(
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
		if (FAILED(rt->CreateBitmap(D2D1::SizeU((UINT32)sp, (UINT32)sp), px.data(), (UINT32)sp * 4, props, &bitmap)))
		{
			return nullptr;
		}
		return bitmap;
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

		// ---- 半调网点（tile 位图 wrap 平铺，一次填充整块剪影）----
		if (style.halftone && style.dotRadius > 0 && style.dotSpacing >= 2)
		{
			int sp = (int)(style.dotSpacing + 0.5);
			if (halftoneTile
				&& (halftoneRt != rt
					|| halftoneSpacing != sp
					|| halftoneRadius != style.dotRadius
					|| halftoneColor != style.dotColor))
			{
				halftoneTile->Release();
				halftoneTile = nullptr;
			}
			if (!halftoneTile)
			{
				halftoneTile = CreateDotTile(rt, style.dotColor, sp, (float)style.dotRadius);
				halftoneRt = rt;
				halftoneSpacing = sp;
				halftoneRadius = style.dotRadius;
				halftoneColor = style.dotColor;
			}
			if (halftoneTile)
			{
				ID2D1BitmapBrush* tileBrushRaw = nullptr;
				auto tileBrushProps = D2D1::BitmapBrushProperties(
					D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP,
					D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
				if (SUCCEEDED(rt->CreateBitmapBrush(halftoneTile, tileBrushProps, &tileBrushRaw)) && tileBrushRaw)
				{
					ComPtr<ID2D1BitmapBrush> tileBrush = tileBrushRaw;
					// tile 原点平移到面板左上，点心相位与旧逐点实现一致（自 x1+sp/2 起排）
					tileBrush->SetTransform(D2D1::Matrix3x2F::Translation(x1, y1));
					rt->FillGeometry(geo.Obj(), tileBrush.Obj());
				}
			}
		}

		// ---- 墨边 ----
		if (style.borderWidth > 0 && borderBrush)
		{
			rt->DrawGeometry(geo.Obj(), borderBrush.Obj(), (FLOAT)style.borderWidth);
		}
	}

	// ============ 表格行 hover 高亮层 ============

	PunkRowHover::~PunkRowHover()
	{
	}

	void PunkRowHover::AttachTo(GuiGraphicsComposition* table)
	{
		host = table;
		element = GuiDirect2DElement::Create();
		element->Rendering.AttachMethod(this, &PunkRowHover::OnRendering);
		table->SetOwnedElement(Ptr(element));
		table->GetEventReceiver()->mouseMove.AttachLambda([this](GuiGraphicsComposition* sender, GuiEventArgs& arguments)
		{
			auto m = dynamic_cast<GuiMouseEventArgs*>(&arguments);
			if (!m) return;
			CollectRows();
			// m->y 相对表格；行位置为全局坐标，统一换算后比较
			vint gy = host->GetGlobalBounds().y1 + m->y;
			vint hit = -1;
			for (vint i = 0; i < rowCount; i++)
			{
				if (gy >= rowY[i] && gy < rowY[i] + rowH[i])
				{
					hit = i;
					break;
				}
			}
			SetHoverRow(hit);
		});
		table->GetEventReceiver()->mouseLeave.AttachLambda([this](GuiGraphicsComposition* sender, GuiEventArgs& arguments)
		{
			SetHoverRow(-1);
		});
	}

	void PunkRowHover::AddRow(GuiBoundsComposition* cell)
	{
		if (cell && rowCount < MaxRows)
		{
			rows[rowCount++] = cell;
		}
	}

	// 全局坐标每次重新收集：布局完成后才有效，且滚动/重排后保持正确
	void PunkRowHover::CollectRows()
	{
		for (vint i = 0; i < rowCount; i++)
		{
			Rect b = rows[i]->GetGlobalBounds();
			rowY[i] = b.y1;
			rowH[i] = b.Height();
		}
	}

	void PunkRowHover::SetHoverRow(vint row)
	{
		if (hoverRow == row) return;
		hoverRow = row;
		if (element && element->GetRenderer() && host)
		{
			if (auto controlHost = host->GetRelatedControlHost())
			{
				controlHost->GetGraphicsHost()->RequestRender();
			}
		}
	}

	void PunkRowHover::OnRendering(GuiGraphicsComposition* sender, GuiDirect2DElementEventArgs& arguments)
	{
		ID2D1RenderTarget* rt = arguments.rt;
		if (!rt || hoverRow < 0) return;

		// UILIB punk --ui-primary-soft: rgba(232,21,28,.16)，铺在行内容之下；
		// 行位置与渲染参数 bounds 同为全局坐标
		const Rect& b = arguments.bounds;
		float x1 = (float)b.x1;
		float y1 = (float)rowY[hoverRow];
		float x2 = (float)b.x2;
		float y2 = y1 + (float)rowH[hoverRow];

		ID2D1SolidColorBrush* brushRaw = nullptr;
		if (FAILED(rt->CreateSolidColorBrush(D2D1::ColorF(232 / 255.0f, 21 / 255.0f, 28 / 255.0f, 0x29 / 255.0f), &brushRaw)) || !brushRaw) return;
		ComPtr<ID2D1SolidColorBrush> brush = brushRaw;
		rt->FillRectangle(D2D1::RectF(x1, y1, x2, y2), brush.Obj());
	}
}
