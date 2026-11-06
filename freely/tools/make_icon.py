#!/usr/bin/env python3
"""Generate FREELY's required 512x512, opaque RGB PS4 icon0.png."""
from pathlib import Path
import sys
from PIL import Image, ImageDraw, ImageFilter

SIZE = 512
SCALE = 3
N = SIZE * SCALE

base = Image.new("RGB", (N, N), (4, 7, 6))
glow = Image.new("RGBA", (N, N), (0, 0, 0, 0))
gd = ImageDraw.Draw(glow)

# Soft green glow behind the terminal frame and prompt.
green = (0, 255, 157)
gd.rounded_rectangle((16*SCALE, 16*SCALE, 496*SCALE, 496*SCALE),
                     radius=78*SCALE, outline=(*green, 110), width=15*SCALE)
gd.line([(112*SCALE, 174*SCALE), (216*SCALE, 256*SCALE),
         (112*SCALE, 338*SCALE)], fill=(*green, 120), width=34*SCALE)
gd.rounded_rectangle((286*SCALE, 310*SCALE, 406*SCALE, 344*SCALE),
                     radius=7*SCALE, fill=(*green, 120))
glow = glow.filter(ImageFilter.GaussianBlur(18*SCALE))
base = Image.alpha_composite(base.convert("RGBA"), glow)

draw = ImageDraw.Draw(base)
draw.rounded_rectangle((20*SCALE, 20*SCALE, 492*SCALE, 492*SCALE),
                       radius=74*SCALE, outline=(*green, 255), width=7*SCALE)
draw.line([(112*SCALE, 174*SCALE), (216*SCALE, 256*SCALE),
           (112*SCALE, 338*SCALE)], fill=(*green, 255), width=34*SCALE,
          joint="curve")
draw.rounded_rectangle((286*SCALE, 310*SCALE, 406*SCALE, 344*SCALE),
                       radius=7*SCALE, fill=(*green, 255))

icon = base.convert("RGB").resize((SIZE, SIZE), Image.Resampling.LANCZOS)
output = Path(sys.argv[1] if len(sys.argv) > 1 else "sce_sys/icon0.png")
output.parent.mkdir(parents=True, exist_ok=True)
icon.save(output, format="PNG", optimize=True)

# Guard against accidentally packaging a non-compliant icon.
check = Image.open(output)
if check.size != (512, 512) or check.mode != "RGB":
    raise SystemExit("icon0.png must be a 512x512 opaque RGB PNG")
print(f"Generated {output}: {check.size[0]}x{check.size[1]} RGB")
