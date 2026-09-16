# 按钮四态连拍：rest / hover / press / leave，验证 LiftDriver 位移
# rest=0、hover=(-2,-2)、press=(+2,+2)、leave=0
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

    base = "F:/project/GacUIDemo/PunkUI/lift"

    # 1. rest：鼠标移到页面空白处（远离按钮）
    move(432, 180, 0.8)
    shot(hwnd, base + "_rest.png")

    # 2. hover：两段 move 进入「危险」按钮中心 (432,376)
    # 注意：真实光标在窗口外，合成 move 后 ~200ms 系统会补发 WM_MOUSELEAVE，
    # 需赶在其前拍摄才能捕获 hover 中间态
    move(432, 250, 0.3)
    lp = move(432, 376, 0.15)
    shot(hwnd, base + "_hover.png")
    time.sleep(0.6)

    # 3. press：按住不放
    user32.PostMessageW(hwnd, 0x0201, 1, lp)
    time.sleep(0.5)
    shot(hwnd, base + "_press.png")

    # 4. leave：松开并移出
    user32.PostMessageW(hwnd, 0x0202, 0, lp)
    move(432, 180, 0.8)
    shot(hwnd, base + "_leave.png")
    proc.kill()


if __name__ == "__main__":
    main()
