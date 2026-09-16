# 最大化下交互响应验证：
# 1) 点击"维护锁定"开关后 90ms×10 连拍——观察轨道变红/圆钮移动出现在第几帧（卡顿则延迟多帧）
# 2) 点击"维护确认"开弹窗——0.3s 后截图确认弹窗已完整显示
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
    print(f"dwm {prect} zoomed={user32.IsZoomed(hwnd)}")

    def to_lp(vx, vy):
        cx = prect[0] + vx - ox
        cy = prect[1] + vy - oy
        return (cy << 16) | (cx & 0xFFFF)

    def move(vx, vy, wait=0.05):
        user32.PostMessageW(hwnd, 0x0200, 0, to_lp(vx, vy))
        time.sleep(wait)

    def click(vx, vy, wait=0.3):
        move(vx, vy)
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

    # ---- 滚到表单区 ----
    wheel(-12)
    move(1900, 500, 0.3)  # 移到空白，清除 hover

    # ---- 点击"维护锁定"开关，连拍测响应帧数 ----
    click(2001, 778, 0.0)
    for i in range(10):
        shot(hwnd, f"{base}_sw_{i}.png")
        time.sleep(0.09)
    time.sleep(0.8)
    shot(hwnd, f"{base}_sw_final.png")

    # ---- 回滚到顶部，点头像区"维护确认"开弹窗 ----
    wheel(30, vx=1900, vy=1000)
    time.sleep(0.4)
    # 头像区在页面下部，滚回顶部后重新滚 12 格
    wheel(-12)
    click(293, 1663, 0.35)
    shot(hwnd, f"{base}_modal_open.png")
    time.sleep(0.8)
    shot(hwnd, f"{base}_modal_open2.png")
    proc.kill()


if __name__ == "__main__":
    main()
