#!/usr/bin/env python3
"""Generate the Freely Dev bird icon as a PS4-compliant 512x512 RGB PNG."""
from pathlib import Path
import sys
from PIL import Image, ImageDraw, ImageFilter

SIZE = 512
SCALE = 3
N = SIZE * SCALE

# The silhouette follows the white flying-bird mark supplied for Freely Dev.
bird = [
    (89.4,119.2),(109.4,160.1),(144.5,195.2),(173.9,211.9),
    (153.5,199.7),(230.3,210.7),(288.3,254.8),(306.2,279.7),
    (305.0,296.8),(276.4,326.6),(260.1,327.5),(329.5,312.8),
    (357.3,291.1),(393.6,242.5),(428.3,231.5),(387.1,212.3),
    (365.4,211.5),(337.7,227.0),(316.0,249.1),(320.1,253.6),
    (304.6,242.9),(277.6,207.4),(254.0,190.3),(142.9,153.1)
]
points = [(int(x*SCALE), int(y*SCALE)) for x,y in bird]

base = Image.new("RGBA", (N,N), (0,0,0,255))
draw = ImageDraw.Draw(base)
draw.rounded_rectangle((32*SCALE,32*SCALE,480*SCALE,480*SCALE),
                       radius=104*SCALE, fill=(3,3,5,255))
glow = Image.new("RGBA",(N,N),(0,0,0,0))
ImageDraw.Draw(glow).polygon(points, fill=(255,255,255,100))
glow = glow.filter(ImageFilter.GaussianBlur(15*SCALE))
base = Image.alpha_composite(base,glow)
ImageDraw.Draw(base).polygon(points,fill=(250,250,253,255))
icon = base.convert("RGB").resize((SIZE,SIZE),Image.Resampling.LANCZOS)
output = Path(sys.argv[1] if len(sys.argv)>1 else "sce_sys/icon0.png")
output.parent.mkdir(parents=True,exist_ok=True)
icon.save(output,format="PNG",optimize=True)
check=Image.open(output)
if check.size!=(512,512) or check.mode!="RGB":
    raise SystemExit("icon0.png must be 512x512 opaque RGB")
print(f"Generated {output}: 512x512 RGB")
