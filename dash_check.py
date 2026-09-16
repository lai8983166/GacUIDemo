# 仪表盘对齐验证：切 tab → 全页截图 → 事件表行 hover 红高亮三连拍
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

    def to_lp(vx, vy):
        cx = prect[0] + vx - ox
        cy = prect[1] + vy - oy
        return (cy << 16) | (cx & 0xFFFF)

    def click(vx, vy, wait=0.4):
        user32.PostMessageW(hwnd, 0x0200, 0, to_lp(vx, vy))
        time.sleep(0.05)
        user32.PostMessageW(hwnd, 0x0201, 1, to_lp(vx, vy))
        time.sleep(0.08)
        user32.PostMessageW(hwnd, 0x0202, 0, to_lp(vx, vy))
        time.sleep(wait)

    def move(vx, vy):
        user32.PostMessageW(hwnd, 0x0200, 0, to_lp(vx, vy))

    base = "F:/project/GacUIDemo/PunkUI/dash"

    # 切到仪表盘 tab
    click(545, 95)
    shot(hwnd, base + "_full.png")

    # 事件表数据行 hover（WM_MOUSELEAVE 约 200ms 后污染，赶前截）
    move(1900, 980)
    time.sleep(0.08)
    shot(hwnd, base + "_hover.png")
    time.sleep(0.6)
    shot(hwnd, base + "_after_leave.png")
    proc.kill()


if __name__ == "__main__":
    main()
