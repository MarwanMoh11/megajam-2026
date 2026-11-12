"""Write the Nitrate colour-grading LUTs (256x16 PNG, Unreal's unwrapped 16x16x16 layout).

Run:  python3 make_luts.py            (writes into ./luts/ next to this file; stdlib only)

Each toned LUT maps a pixel's brightness onto a three-stop tone (dark -> mid -> light), with the same
gentle S-curve as the mock board. Saturated pixels (the stencil red and gold) pass through unchanged, so
danger stays red after grading. Materials are already greyscale (docs/ART_STYLE.md, "Materials"), so the
only saturated pixels left in a frame are the ones we put there on purpose.

LUT_Neutral.png changes nothing: import it first and check the frame looks identical. If it doesn't, the
channel layout below is wrong for this engine version (red = x within a tile, green = y, blue = tile index).
"""
import os, struct, zlib

TONES = {  # name: (dark, mid, light), same values as prototypes/style/board.html
    "Night":  ("#0b1420", "#5f7387", "#eef0ea"),   # K: the Rooftop
    "Amber":  ("#1a1108", "#9b7a4d", "#f6ebd3"),   # K: the Mill
    "Green":  ("#0e1712", "#6c806d", "#eef0e4"),   # K: the Railway
    "Silver": ("#0d0c0b", "#7a756d", "#f1eadb"),   # L: every level; title and menus
}
CURVE = 0.35        # blend toward an S-curve (contrast); the board uses the same value
KEEP_LO, KEEP_HI = 0.30, 0.55   # saturation range where a pixel fades from "toned" to "kept as is"


def hexrgb(h):
    return tuple(int(h[i:i + 2], 16) / 255 for i in (1, 3, 5))


def lerp(a, b, t):
    return tuple(x + (y - x) * t for x, y in zip(a, b))


def smoothstep(e0, e1, x):
    t = min(1.0, max(0.0, (x - e0) / (e1 - e0)))
    return t * t * (3 - 2 * t)


def tone(rgb, stops):
    r, g, b = rgb
    v = 0.2126 * r + 0.7152 * g + 0.0722 * b
    s = 2 * v * v if v < 0.5 else 1 - 2 * (1 - v) * (1 - v)
    v = v + (s - v) * CURVE
    d, m, l = (hexrgb(h) for h in stops)
    out = lerp(d, m, v / 0.5) if v < 0.5 else lerp(m, l, (v - 0.5) / 0.5)
    mx, mn = max(rgb), min(rgb)
    sat = 0.0 if mx == 0 else (mx - mn) / mx
    return lerp(out, rgb, smoothstep(KEEP_LO, KEEP_HI, sat))


def write_png(path, w, h, pixels):
    raw = b"".join(b"\x00" + bytes(pixels[y * w * 3:(y + 1) * w * 3]) for y in range(h))
    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def make(fn):
    px = []
    for y in range(16):
        for x in range(256):
            rgb = ((x % 16) / 15, y / 15, (x // 16) / 15)
            px.extend(round(255 * min(1, max(0, c))) for c in fn(rgb))
    return px


if __name__ == "__main__":
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "luts")
    os.makedirs(out, exist_ok=True)
    write_png(os.path.join(out, "LUT_Neutral.png"), 256, 16, make(lambda c: c))
    for name, stops in TONES.items():
        write_png(os.path.join(out, f"LUT_Nitrate_{name}.png"), 256, 16, make(lambda c, s=stops: tone(c, s)))
    print("wrote", sorted(os.listdir(out)))
