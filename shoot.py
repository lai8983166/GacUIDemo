# 截图/交互工具：启动 PunkUI，定位 VczhWindow，截图（可选点击/滚轮）后退出。
# 点击/滚轮坐标为窗口客户区设计坐标（=GacUI 逻辑像素，窗口左上角为原点）。
# 应用进程被 Windows DPI 虚拟化拉伸，因此截图区域取 DWM 物理边界，坐标按比值换算。
import argparse
import ctypes
import subprocess
import sys
import time
from ctypes import wintypes
from pathlib import Path
from PIL import ImageGrab

user32 = ctypes.windll.user32
dwmapi = ctypes.windll.dwmapi
gdi32 = ctypes.windll.gdi32

EnumWindowsProc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

found_hwnd = None
target_pid = None


def _enum_cb(hwnd, lparam):
    global found_hwnd
    pid = wintypes.DWORD()
    user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
    if pid.value == target_pid and user32.IsWindowVisible(hwnd):
        buf = ctypes.create_unicode_buffer(256)
        user32.GetClassNameW(hwnd, buf, 256)
        if buf.value == "VczhWindow":
            found_hwnd = hwnd
            return False
    return True


def find_window(pid):
    global target_pid
    target_pid = pid
    found = None
    user32.EnumWindows(EnumWindowsProc(_enum_cb), 0)
    return found_hwnd


class RECT(ctypes.Structure):
    _fields_ = [("left", wintypes.LONG), ("top", wintypes.LONG),
                ("right", wintypes.LONG), ("bottom", wintypes.LONG)]


def get_rect(hwnd):
    r = RECT()
    user32.GetWindowRect(hwnd, ctypes.byref(r))
    return (r.left, r.top, r.right, r.bottom)


def get_dwm_rect(hwnd):
    r = RECT()
    DWMWA_EXTENDED_FRAME_BOUNDS = 9
    dwmapi.DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS,
                                 ctypes.byref(r), ctypes.sizeof(r))
    return (r.left, r.top, r.right, r.bottom)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default="F:/project/GacUIDemo/PunkUI/Bin/x64/Release/PunkUI.exe")
    ap.add_argument("--out", default="F:/project/GacUIDemo/PunkUI/shot_now.png")
    ap.add_argument("--click", action="append", default=[],
                    help="设计坐标点击，格式 x,y；可多次")
    ap.add_argument("--wheel", type=int, default=0, help="滚轮格数，正上负下")
    ap.add_argument("--wheel-at", default="600,400", help="滚轮位置(设计坐标)")
    ap.add_argument("--wait", type=int, default=2600)
    ap.add_argument("--settle", type=int, default=650, help="每次点击后等待 ms")
    args = ap.parse_args()

    user32.SetProcessDPIAware()

    proc = subprocess.Popen([args.exe], cwd=str(Path(args.exe).parent))
    time.sleep(args.wait / 1000.0)
    hwnd = find_window(proc.pid)
    if not hwnd:
        print("ERROR: no VczhWindow")
        proc.kill()
        sys.exit(1)
    user32.SetForegroundWindow(hwnd)
    # 置顶，避免其他窗口（如 QQ）盖在截图区域上
    HWND_TOPMOST = -1
    SWP_NOMOVE = 0x0002
    SWP_NOSIZE = 0x0001
    SWP_SHOWWINDOW = 0x0040
    user32.SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW)
    time.sleep(0.5)

    vrect = get_rect(hwnd)          # 调用方为 DPI-aware，此处为虚拟坐标
    prect = get_dwm_rect(hwnd)      # 物理边界
    vw = vrect[2] - vrect[0]
    vh = vrect[3] - vrect[1]
    pw = prect[2] - prect[0]
    ph = prect[3] - prect[1]
    scale = pw / float(vw)
    print(f"virtual {vrect} {vw}x{vh}  physical {prect} {pw}x{ph}  scale={scale:.4f}")

    def to_physical(vx, vy):
        return (int(round(prect[0] + vx * scale)), int(round(prect[1] + vy * scale)))

    def shot(path):
        img = ImageGrab.grab(bbox=prect, all_screens=False)
        img.save(path)
        print(f"saved {path} ({img.width} x {img.height})")

    def click(vx, vy):
        ax, ay = to_physical(vx, vy)
        user32.SetCursorPos(ax, ay)
        time.sleep(0.25)
        user32.mouse_event(2, 0, 0, 0, 0)
        user32.mouse_event(4, 0, 0, 0, 0)
        time.sleep(args.settle / 1000.0)

    def wheel(notches, vx, vy):
        ax, ay = to_physical(vx, vy)
        user32.SetCursorPos(ax, ay)
        time.sleep(0.25)
        for _ in range(abs(notches)):
            user32.mouse_event(0x0800, 0, 0, (120 * (1 if notches > 0 else -1)) & 0xFFFFFFFF, 0)
            time.sleep(0.2)
        time.sleep(0.5)

    if args.wheel:
        wx, wy = (int(v) for v in args.wheel_at.split(","))
        wheel(args.wheel, wx, wy)

    clicks = []
    for c in args.click:
        clicks.append(tuple(int(v) for v in c.split(",")))

    if clicks:
        for i, (cx, cy) in enumerate(clicks):
            click(cx, cy)
            shot(args.out.replace(".png", f"_{i + 1}.png"))
    else:
        shot(args.out)

    proc.kill()


if __name__ == "__main__":
    main()
