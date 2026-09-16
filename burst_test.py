# hover 连拍：move 进入按钮后每 ~90ms 拍一帧，共 8 帧
# 捕获 lift 动画出现/保持/被 WM_MOUSELEAVE 回退的完整生命周期
import ctypes
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from press_test import shot  # noqa: E402
from shoot2 import find_window, get_dwm_rect, client_origin  # noqa: E402

user32 = ctypes.windll.user32


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

    def move(vx, vy, wait):
        cx, cy = to_client(vx, vy)
        lp = (cy << 16) | (cx & 0xFFFF)
        user32.PostMessageW(hwnd, 0x0200, 0, lp)
        time.sleep(wait)
        return lp

    # 滚轮回顶部
    sx, sy = prect[0] + 1450, prect[1] + 700
    lp_screen = ((sy << 16) | (sx & 0xFFFF))
    for _ in range(30):
        user32.PostMessageW(hwnd, 0x020A, (120 & 0xFFFF) << 16, lp_screen)
        time.sleep(0.03)
    time.sleep(0.6)

    # 移到空白处稳定，再进入危险按钮，连拍
    move(432, 180, 0.8)
    move(432, 250, 0.05)
    move(432, 376, 0.02)
    for i in range(8):
        shot(hwnd, f"F:/project/GacUIDemo/PunkUI/burst_{i}.png")
        time.sleep(0.09)
    proc.kill()


if __name__ == "__main__":
    main()
