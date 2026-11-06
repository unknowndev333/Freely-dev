#!/usr/bin/env python3
"""Generate the Freely Dev bird icon as a PS4-compliant 512x512 RGB PNG."""
from pathlib import Path
import sys
from PIL import Image, ImageDraw, ImageFilter

SIZE = 512
SCALE = 3
N = SIZE * SCALE

# Smooth outline traced from the supplied white flying-bird mark.
outline = [
    (89.4,119.2),(121.3,175.2),(175.2,212.7),(127.4,200.5),
    (149.4,229.9),(172.7,246.6),(211.1,261.3),(182.9,261.7),
    (226.6,283.8),(258.9,287.0),(199.7,312.8),(118.0,332.4),
    (161.7,352.8),(194.8,355.6),(234.8,331.9),(323.8,316.0),
    (355.2,293.6),(394.0,242.5),(429.1,231.9),(389.1,212.7),
    (367.9,210.7),(345.4,220.5),(313.6,250.3),(281.7,211.1),
    (256.0,191.1),(142.5,152.7)
]

# Catmull-Rom interpolation keeps the original organic silhouette smooth.
points = []
count = len(outline)
for i in range(count):
    p0, p1 = outline[(i-1)%count], outline[i]
    p2, p3 = outline[(i+1)%count], outline[(i+2)%count]
    for step in range(12):
        t = step / 12.0
        t2, t3 = t*t, t*t*t
        x = 0.5*((2*p1[0])+(-p0[0]+p2[0])*t+(2*p0[0]-5*p1[0]+4*p2[0]-p3[0])*t2+(-p0[0]+3*p1[0]-3*p2[0]+p3[0])*t3)
        y = 0.5*((2*p1[1])+(-p0[1]+p2[1])*t+(2*p0[1]-5*p1[1]+4*p2[1]-p3[1])*t2+(-p0[1]+3*p1[1]-3*p2[1]+p3[1])*t3)
        points.append((int(x*SCALE), int(y*SCALE)))

base = Image.new("RGBA",(N,N),(0,0,0,255))
ImageDraw.Draw(base).rounded_rectangle(
    (32*SCALE,32*SCALE,480*SCALE,480*SCALE),
    radius=104*SCALE, fill=(3,3,5,255)
)
glow = Image.new("RGBA",(N,N),(0,0,0,0))
ImageDraw.Draw(glow).polygon(points,fill=(255,255,255,90))
glow = glow.filter(ImageFilter.GaussianBlur(15*SCALE))
base = Image.alpha_composite(base,glow)
ImageDraw.Draw(base).polygon(points,fill=(250,250,253,255))

output = Path(sys.argv[1] if len(sys.argv)>1 else "sce_sys/icon0.png")
output.parent.mkdir(parents=True,exist_ok=True)
base.convert("RGB").resize((SIZE,SIZE),Image.Resampling.LANCZOS).save(
    output,format="PNG",optimize=True
)
check=Image.open(output)
if check.size!=(512,512) or check.mode!="RGB":
    raise SystemExit("icon0.png must be 512x512 opaque RGB")
print(f"Generated {output}: 512x512 RGB")
