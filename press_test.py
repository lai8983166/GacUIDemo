# 按住按钮截图：DOWN 后动画中途与完成时各截一帧，验证 LiftDriver Pressed 位移 (+2,+2)
import ctypes
import subprocess
import sys
import time
from ctypes import wintypes
from pathlib import Path
from PIL import Image

sys.path.insert(0, str(Path(__file__).parent))
from shoot2 import find_window, get_dwm_rect, client_origin  # noqa: E402

user32 = ctypes.windll.user32
gdi32 = ctypes.windll.gdi32


class RECT(ctypes.Structure):
    _fields_ = [("left", wintypes.LONG), ("top", wintypes.LONG),
                ("right", wintypes.LONG), ("bottom", wintypes.LONG)]


class BMIH(ctypes.Structure):
    _fields_ = [("biSize", wintypes.DWORD), ("biWidth", wintypes.LONG),
                ("biHeight", wintypes.LONG), ("biPlanes", wintypes.WORD),
                ("biBitCount", wintypes.WORD), ("biCompression", wintypes.DWORD),
                ("biSizeImage", wintypes.DWORD), ("biXPelsPerMeter", wintypes.LONG),
                ("biYPelsPerMeter", wintypes.LONG), ("biClrUsed", wintypes.DWORD),
                ("biClrImportant", wintypes.DWORD)]


def shot(hwnd, path):
    rc = RECT()
    user32.GetClientRect(hwnd, ctypes.byref(rc))
    w, h = rc.right, rc.bottom
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
    print(f"saved {path}")


def main():
    user32.SetProcessDPIAware()
    exe = "F:/project/GacUIDemo/PunkUI/Bin/x64/Release/PunkUI.exe"
    proc = subprocess.Popen([exe], cwd=str(Path(exe).parent))
    time.sleep(2.6)
    hwnd = find_window(proc.pid)
    if not hwnd:
        print("ERROR: no window")
        proc.kill()
        return
    time.sleep(0.5)
    prect = get_dwm_rect(hwnd)
    ox, oy = client_origin(hwnd)

    def to_client(vx, vy):
        return (prect[0] + vx - ox, prect[1] + vy - oy)

    # 滚轮回顶部
    sx, sy = prect[0] + 1450, prect[1] + 700
    lp_screen = ((sy << 16) | (sx & 0xFFFF))
    for _ in range(30):
        user32.PostMessageW(hwnd, 0x020A, (120 & 0xFFFF) << 16, lp_screen)
        time.sleep(0.03)
    time.sleep(0.6)

    # 悬停「危险」按钮（红底, 394-471, 355-397 中心 432,376）对比「主要」(128-205)
    # 两段 move：先空白处建立 mouseEnter 链，再移入按钮
    for vx, vy, wait in [(432, 250, 0.3), (432, 376, 0.6)]:
        cx, cy = to_client(vx, vy)
        lp = (cy << 16) | (cx & 0xFFFF)
        user32.PostMessageW(hwnd, 0x0200, 0, lp)
        time.sleep(wait)
    shot(hwnd, "F:/project/GacUIDemo/PunkUI/hover_btn.png")

    # 按下不放
    user32.PostMessageW(hwnd, 0x0201, 1, lp)
    time.sleep(0.35)
    shot(hwnd, "F:/project/GacUIDemo/PunkUI/press_btn.png")
    user32.PostMessageW(hwnd, 0x0202, 0, lp)
    time.sleep(0.3)
    shot(hwnd, "F:/project/GacUIDemo/PunkUI/release_btn.png")
    proc.kill()


if __name__ == "__main__":
    main()
