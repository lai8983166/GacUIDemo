#ifndef VCZH_PUNK_FX
#define VCZH_PUNK_FX

/***********************************************************************
朋克动效辅助（手写，非生成）
对齐 UILIB punk-collage 主题的过渡规格：
  --ui-duration: 0.2s   --ui-ease: cubic-bezier(0.4, 0, 0.2, 1)
  按钮 hover translate(-2,-2) / active translate(2,2)
  开关滑块 translateX(20px) + 轨道变色
  输入框聚焦投影 #000 -> #E8151C
***********************************************************************/

#include "GacUI.h"

namespace punkfx
{
	using namespace vl;
	using namespace vl::presentation;
	using namespace vl::presentation::controls;
	using namespace vl::reflection::description;

	// UILIB --ui-duration: 0.2s
	constexpr vuint64_t PunkDurationMs = 200;

	// vl::Func 用 nullptr 判空不可靠（空 Func 经 operator bool 仍为真，调用即 AV），
	// 统一用 no-op lambda 作为"无完成回调"的默认值
	inline const Func<void()>& NoOp()
	{
		static Func<void()> noop = []() {};
		return noop;
	}

	// CSS cubic-bezier(0.4, 0, 0.2, 1)：x 单调可二分求参 u，再取 y(u)
	inline double PunkEase(double x)
	{
		if (x <= 0) return 0;
		if (x >= 1) return 1;
		double lo = 0, hi = 1;
		for (vint i = 0; i < 24; i++)
		{
			double u = (lo + hi) * 0.5;
			double xu = 3 * (1 - u)*(1 - u)*u*0.4 + 3 * (1 - u)*u*u*0.2 + u*u*u;
			if (xu < x) lo = u; else hi = u;
		}
		double u = (lo + hi) * 0.5;
		return 3 * (1 - u)*(1 - u)*u*0.0 + 3 * (1 - u)*u*u*1.0 + u*u*u;
	}

	inline Color PunkLerpColor(Color a, Color b, double t)
	{
		auto ch = [t](unsigned char x, unsigned char y)
		{
			return (unsigned char)(0.5 + x + ((double)y - (double)x) * t);
		};
		return Color(ch(a.r, b.r), ch(a.g, b.g), ch(a.b, b.b), ch(a.a, b.a));
	}

	// UILIB punk 主题：开关轨道 未选中 --ui-border-strong(#000) / 选中 --ui-primary(#E8151C)
	inline Color SwitchTrackColor(bool selected, bool enabled)
	{
		if (!enabled) return Color(0x9E, 0x9E, 0x9E);
		return selected ? Color(0xE8, 0x15, 0x1C) : Color(0x00, 0x00, 0x00);
	}

	// UILIB punk 主题：输入框聚焦投影 #E8151C，常态 #000
	inline Color InputShadowColor(bool focused)
	{
		return focused ? Color(0xE8, 0x15, 0x1C) : Color(0x00, 0x00, 0x00);
	}

/***********************************************************************
PunkTween：有限补间动画
每帧回调 frame(缓动后的进度 0..1)；结束时先回调 frame(1) 再回调 finished，
保证终值精确（GacUI 内建有限动画不投递最后一帧）。
***********************************************************************/

	class PunkTween
		: public virtual IDescriptable
		, public IGuiAnimation
		, public Description<PunkTween>
	{
	protected:
		vuint64_t						durationMs;
		Func<void(double)>				frame;
		Func<void()>					finished;
		vuint64_t						startMs = 0;
		bool							started = false;

		static vuint64_t				NowMs()
		{
			return vl::DateTime::UtcTime().osMilliseconds;
		}
	public:
		PunkTween(vuint64_t _durationMs, const Func<void(double)>& _frame, const Func<void()>& _finished = NoOp())
			:durationMs(_durationMs)
			, frame(_frame)
			, finished(_finished)
		{
		}

		void Start()override
		{
			started = true;
			startMs = NowMs();
		}

		void Pause()override
		{
		}

		void Resume()override
		{
		}

		void Run()override
		{
			if (!started) return;
			double t = (double)(NowMs() - startMs) / (double)durationMs;
			if (t >= 1.0)
			{
				if (frame) frame(1.0);
				if (finished) finished();
			}
			else if (t > 0)
			{
				if (frame) frame(PunkEase(t));
			}
		}

		bool GetStopped()override
		{
			return started && (double)(NowMs() - startMs) >= (double)durationMs;
		}
	};

/***********************************************************************
StartTween：启动补间并管理同槽位旧动画（新补间启动前杀掉旧的，避免叠加）
***********************************************************************/

	inline Ptr<IGuiAnimation> StartTween(
		GuiInstanceRootObject* root,
		Ptr<IGuiAnimation>* slot,
		const Func<void(double)>& frame,
		const Func<void()>& finished = NoOp(),
		vuint64_t durationMs = PunkDurationMs)
	{
		if (slot && *slot)
		{
			root->KillAnimation(*slot);
			*slot = nullptr;
		}
		auto anim = Ptr<IGuiAnimation>(new PunkTween(durationMs, frame, finished));
		root->AddAnimation(anim);
		if (slot) *slot = anim;
		return anim;
	}

/***********************************************************************
GetFxSlot：每实例动效状态槽
皮肤/应用模板类没有 USER_CONTENT 成员可放状态，借用宿主的命名对象表存放
（Value 持 Ptr，宿主析构时随之释放，无泄漏）。
***********************************************************************/

	template<typename T>
	T* GetFxSlot(GuiInstanceRootObject* root, const wchar_t* key)
	{
		// Description<T> 虚继承 DescriptableObject，必须 dynamic_cast 下行
		if (auto rawPtr = root->GetNamedObject(key).GetRawPtr())
		{
			return dynamic_cast<T*>(rawPtr);
		}
		auto slot = Ptr<T>(new T());
		root->SetNamedObject(key, ::vl::__vwsn::Box(slot));
		return slot.Obj();
	}

/***********************************************************************
LiftDriver：按钮悬停/按下的位移驱动
punk 规格：hover translate(-2,-2)、active translate(2,2)、松开/移出回 0
***********************************************************************/

	class LiftDriver : public Description<LiftDriver>
	{
	protected:
		compositions::GuiBoundsComposition*		lift = nullptr;
		Ptr<IGuiAnimation>						current;
		double									x = 0;
		double									y = 0;
		bool									hover = false;
		controls::ButtonState					state = controls::ButtonState::Normal;

		void									Apply(double nx, double ny)
		{
			x = nx;
			y = ny;
			// 四边锚定 (l,t,r,b)=(dx,dy,-dx,-dy) 恰为纯平移（不改变尺寸）。
			// 注意：-1 是 AlignmentToParent 的"未对齐"哨兵，任一边不得输出 -1；
			// 中间帧四舍五入恰逢 -1 时退为 0（单帧 1px 偏差，不可感知）
			auto edge = [](double v)
			{
				vint r = (vint)llround(v);
				return r == -1 ? 0 : r;
			};
			lift->SetAlignmentToParent(Margin(edge(nx), edge(ny), edge(-nx), edge(-ny)));
		}

		void									ApplyTarget(GuiInstanceRootObject* root)
		{
			if (!lift) return;
			double tx = 0, ty = 0;
			if (state == controls::ButtonState::Pressed)
			{
				tx = 2; ty = 2;
			}
			else if (hover)
			{
				// 控件层 GuiButton::OnMouseEnter 受 ignoreChildControlMouseEvents 限制，
				// 模板内部命中的 enter 不更新 ButtonState（见皮肤按钮 hover 失效问题），
				// 因此 hover 信号由模板 container 的 MouseEnter/MouseLeave 直接传入
				tx = -2; ty = -2;
			}
			double sx = x, sy = y;
			StartTween(root, &current, [this, sx, sy, tx, ty](double t)
			{
				Apply(sx + (tx - sx) * t, sy + (ty - sy) * t);
			});
		}
	public:
		void									Bind(compositions::GuiBoundsComposition* liftBounds)
		{
			lift = liftBounds;
			Apply(0, 0);
		}

		void									SetHover(bool value, GuiInstanceRootObject* root)
		{
			if (hover == value) return;
			hover = value;
			ApplyTarget(root);
		}

		void									OnStateChanged(controls::ButtonState value, GuiInstanceRootObject* root)
		{
			if (!lift) return;
			if (state == value) return;
			state = value;
			ApplyTarget(root);
		}
	};

/***********************************************************************
ColorDriver：SolidBackground 颜色补间（开关轨道 / 输入框聚焦投影）
***********************************************************************/

	class ColorDriver : public Description<ColorDriver>
	{
	protected:
		Ptr<elements::GuiSolidBackgroundElement>	target;
		Ptr<IGuiAnimation>						current;
		Color									now;
	public:
		void									Bind(const Ptr<elements::GuiSolidBackgroundElement>& element, Color initial)
		{
			target = element;
			now = initial;
			if (target) target->SetColor(now);
		}

		void									SetNow(Color c)
		{
			if (!target) return;
			now = c;
			target->SetColor(now);
		}

		void									TweenTo(GuiInstanceRootObject* root, Color to)
		{
			if (!target) return;
			Color from = now;
			if (from == to) return;
			StartTween(root, &current, [this, from, to](double t)
			{
				now = PunkLerpColor(from, to, t);
				target->SetColor(now);
			});
		}
	};

/***********************************************************************
SlideDriver：水平位置补间（开关圆钮 translateX；仅锚左/上边，右/下 -1）
***********************************************************************/

	class SlideDriver : public Description<SlideDriver>
	{
	protected:
		compositions::GuiBoundsComposition*		bounds = nullptr;
		Ptr<IGuiAnimation>						current;
		vint									top = 0;
		double									x = 0;
	public:
		void									Bind(compositions::GuiBoundsComposition* target, vint topOffset)
		{
			bounds = target;
			top = topOffset;
		}

		void									SetNow(double nx)
		{
			if (!bounds) return;
			x = nx;
			bounds->SetAlignmentToParent(Margin((vint)(0.5 + x), top, -1, -1));
		}

		void									TweenTo(GuiInstanceRootObject* root, double tx)
		{
			if (!bounds) return;
			double sx = x;
			if (sx == tx) return;
			StartTween(root, &current, [this, sx, tx](double t)
			{
				SetNow(sx + (tx - sx) * t);
			});
		}
	};
}

#endif
