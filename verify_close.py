# 弹窗关闭即时性验证：最大化下开弹窗 → 点取消/确认维护 → 0.35s 内应消失
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

    def click(vx, vy, wait=0.3):
        user32.PostMessageW(hwnd, 0x0200, 0, to_lp(vx, vy))
        time.sleep(0.05)
        user32.PostMessageW(hwnd, 0x0201, 1, to_lp(vx, vy))
        time.sleep(0.08)
        user32.PostMessageW(hwnd, 0x0202, 0, to_lp(vx, vy))
        time.sleep(wait)

    def wheel(notches, vx=1900, vy=1000):
        lp = ((prect[1] + vy) << 16) | ((prect[0] + vx) & 0xFFFF)
        wp = ((120 * (1 if notches > 0 else -1)) & 0xFFFF) << 16
        for _ in range(abs(notches)):
            user32.PostMessageW(hwnd, 0x020A, wp, lp)
            time.sleep(0.05)
        time.sleep(0.6)

    base = "F:/project/GacUIDemo/PunkUI/vfix"
    OPEN_BTN = (293 + 9, 1663 + 9)     # 维护确认（开弹窗）
    CANCEL_BTN = (2032 + 9, 1142 + 9)  # 弹窗 footer 取消
    CONFIRM_BTN = (2132 + 9, 1142 + 9) # 弹窗 footer 确认维护

    wheel(-12)
    move_to = click  # noqa

    # ---- 开弹窗 → 点取消 ----
    click(*OPEN_BTN, wait=0.5)
    shot(hwnd, base + "_c0_open.png")
    click(*CANCEL_BTN, wait=0.35)
    shot(hwnd, base + "_c1_after_cancel.png")
    time.sleep(1.0)
    shot(hwnd, base + "_c2_cancel_steady.png")

    # ---- 重开 → 点确认维护 ----
    click(*OPEN_BTN, wait=0.5)
    click(*CONFIRM_BTN, wait=0.35)
    shot(hwnd, base + "_c3_after_confirm.png")
    proc.kill()


if __name__ == "__main__":
    main()
