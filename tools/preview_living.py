from pathlib import Path
from PIL import Image
p=Path("build/rc7/screens")
for f in p.glob("*.ppm"):
 if not f.name.startswith("motion-"):Image.open(f).save(f.with_suffix(".png"))
frames=[]
for n in range(36):
 out=Image.new("RGB",(960,480))
 for i in range(8):out.paste(Image.open(p/f"motion-{i}-{n:02}.ppm"),((i%4)*240,(i//4)*240))
 frames.append(out)
frames[0].save(p/"engines.gif",save_all=True,append_images=frames[1:],duration=84,loop=0)
out=Image.new("RGB",(960,240))
for i,name in enumerate(["dial","acid-menu","filter","clock"]):out.paste(Image.open(p/(name+".png")),(240*i,0))
out.save(p/"controls.png")
