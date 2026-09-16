# 交互截图工具（无干扰版）：启动 PunkUI，置顶后用 PostMessage 投递鼠标/滚轮消息，
# 不移动真实光标、不抢占前台焦点；坐标为相对窗口 DWM 边界的物理像素（=设计坐标，scale=1）。
import argparse
import ctypes
import subprocess
import sys
import time
from ctypes import wintypes
from pathlib import Path
from PIL import Image, ImageGrab

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


class POINT(ctypes.Structure):
    _fields_ = [("x", wintypes.LONG), ("y", wintypes.LONG)]


def get_dwm_rect(hwnd):
    r = RECT()
    dwmapi.DwmGetWindowAttribute(hwnd, 9, ctypes.byref(r), ctypes.sizeof(r))
    return (r.left, r.top, r.right, r.bottom)


def client_origin(hwnd):
    pt = POINT(0, 0)
    user32.ClientToScreen(hwnd, ctypes.byref(pt))
    return (pt.x, pt.y)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default="F:/project/GacUIDemo/PunkUI/Bin/x64/Release/PunkUI.exe")
    ap.add_argument("--out", default="F:/project/GacUIDemo/PunkUI/shot_now.png")
    ap.add_argument("--click", action="append", default=[],
                    help="设计坐标点击（PostMessage），格式 x,y；可多次")
    ap.add_argument("--wheel", type=int, default=0, help="滚轮格数，正上负下")
    ap.add_argument("--wheel-at", default="600,400")
    ap.add_argument("--wait", type=int, default=2600)
    ap.add_argument("--settle", type=int, default=650, help="每次点击后等待 ms")
    ap.add_argument("--resize", default="", help="临时改窗口客户区尺寸，格式 WxH（PrintWindow 可抓屏幕外部分）")
    ap.add_argument("--hover", default="", help="只发 WM_MOUSEMOVE 模拟悬停（格式 x,y），随后截图")
    args = ap.parse_args()

    user32.SetProcessDPIAware()

    proc = subprocess.Popen([args.exe], cwd=str(Path(args.exe).parent))
    time.sleep(args.wait / 1000.0)
    hwnd = find_window(proc.pid)
    if not hwnd:
        print("ERROR: no VczhWindow")
        proc.kill()
        sys.exit(1)
    time.sleep(0.5)

    prect = get_dwm_rect(hwnd)
    ox, oy = client_origin(hwnd)
    print(f"dwm {prect}  client_origin {ox},{oy}")

    if args.resize:
        rw, rh = (int(v) for v in args.resize.lower().split("x"))
        crc = RECT()
        user32.GetClientRect(hwnd, ctypes.byref(crc))
        # 客户区目标尺寸 - 反推窗口外框尺寸（保持左上角不动）
        wr = RECT()
        user32.GetWindowRect(hwnd, ctypes.byref(wr))
        frame_w = (wr.right - wr.left) - (crc.right - crc.left)
        frame_h = (wr.bottom - wr.top) - (crc.bottom - crc.top)
        user32.SetWindowPos(hwnd, 0, wr.left, wr.top,
                            rw + frame_w, rh + frame_h, 0x0010)  # SWP_NOACTIVATE
        time.sleep(0.5)
        prect = get_dwm_rect(hwnd)
        ox, oy = client_origin(hwnd)
        print(f"resized -> client {rw}x{rh}  dwm {prect}")

    def to_client(vx, vy):
        sx = prect[0] + vx
        sy = prect[1] + vy
        return (sx - ox, sy - oy)

    class BMIH(ctypes.Structure):
        _fields_ = [("biSize", wintypes.DWORD), ("biWidth", wintypes.LONG),
                    ("biHeight", wintypes.LONG), ("biPlanes", wintypes.WORD),
                    ("biBitCount", wintypes.WORD), ("biCompression", wintypes.DWORD),
                    ("biSizeImage", wintypes.DWORD), ("biXPelsPerMeter", wintypes.LONG),
                    ("biYPelsPerMeter", wintypes.LONG), ("biClrUsed", wintypes.DWORD),
                    ("biClrImportant", wintypes.DWORD)]

    def shot(path):
        # PrintWindow(PW_RENDERFULLCONTENT)：窗口被遮挡也能抓到内容，不干扰前台
        rc = RECT()
        ok = user32.GetClientRect(hwnd, ctypes.byref(rc))
        w, h = rc.right, rc.bottom
        print(f"shot: client {w}x{h} ok={ok} IsWindow={user32.IsWindow(hwnd)} poll={proc.poll()}")
        hdc = user32.GetWindowDC(hwnd)
        mem = gdi32.CreateCompatibleDC(hdc)
        bmp = gdi32.CreateCompatibleBitmap(hdc, w, h)
        gdi32.SelectObject(mem, bmp)
        user32.PrintWindow(hwnd, mem, 2)
        bi = BMIH(ctypes.sizeof(BMIH), w, -h, 1, 32, 0, 0, 0, 0, 0, 0)
        buf = ctypes.create_string_buffer(w * h * 4)
        gdi32.GetDIBits(mem, bmp, 0, h, buf, ctypes.byref(bi), 0)
        img = Image.frombytes("RGBA", (w, h), bytes(buf), "raw", "BGRA", 0, 1)
        img.save(path)
        gdi32.DeleteObject(bmp)
        gdi32.DeleteDC(mem)
        user32.ReleaseDC(hwnd, hdc)
        print(f"saved {path} ({w} x {h})")

    WM_LBUTTONDOWN = 0x0201
    WM_LBUTTONUP = 0x0202
    WM_MOUSEWHEEL = 0x020A

    def post_click(vx, vy):
        cx, cy = to_client(vx, vy)
        lp = (cy << 16) | (cx & 0xFFFF)
        user32.PostMessageW(hwnd, 0x0200, 0, lp)  # WM_MOUSEMOVE（补 hover 状态）
        time.sleep(0.05)
        user32.PostMessageW(hwnd, WM_LBUTTONDOWN, 1, lp)
        time.sleep(0.08)
        user32.PostMessageW(hwnd, WM_LBUTTONUP, 0, lp)
        time.sleep(args.settle / 1000.0)

    def post_wheel(notches, vx, vy):
        # WM_MOUSEWHEEL 的 lParam 是屏幕坐标
        sx = prect[0] + vx
        sy = prect[1] + vy
        lp = ((sy << 16) | (sx & 0xFFFF))
        for _ in range(abs(notches)):
            wp = ((120 * (1 if notches > 0 else -1)) & 0xFFFF) << 16
            user32.PostMessageW(hwnd, WM_MOUSEWHEEL, wp, lp)
            time.sleep(0.2)
        time.sleep(0.5)

    if args.wheel:
        wx, wy = (int(v) for v in args.wheel_at.split(","))
        post_wheel(args.wheel, wx, wy)

    clicks = [tuple(int(v) for v in c.split(",")) for c in args.click]
    if args.hover:
        hx, hy = (int(v) for v in args.hover.split(","))
        cx, cy = to_client(hx, hy)
        lp = (cy << 16) | (cx & 0xFFFF)
        user32.PostMessageW(hwnd, 0x0200, 0, lp)
        time.sleep(args.settle / 1000.0)
        shot(args.out)
    elif clicks:
        for i, (cx, cy) in enumerate(clicks):
            post_click(cx, cy)
            shot(args.out.replace(".png", f"_{i + 1}.png"))
    else:
        shot(args.out)

    proc.kill()


if __name__ == "__main__":
    main()
