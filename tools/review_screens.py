"""A review sheet made from actual 240 x 240 firmware renders, not UI mockups."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

d = Path('build/screens')
for p in d.glob('*.ppm'):
    Image.open(p).resize((480, 480), Image.Resampling.NEAREST).save(p.with_suffix('.png'))
names = [
    ('Track', 'screen-1-layer1.png'),
    ('Grains', 'interval-grains.png'),
    ('Loop speed', 'interval-speed.png'),
    ('Macros', 'screen-32-macro.png'),
    ('Play along', 'screen-42-playalong.png'),
    ('Arpeggio', 'screen-43-arpeggio.png'),
]
sheet = Image.new('RGB', (3 * 512 + 32, 2 * 544 + 32), '#101010')
draw = ImageDraw.Draw(sheet)
font = ImageFont.truetype('assets/fonts/LiberationSans-Regular.ttf', 18)
for i, (title, file) in enumerate(names):
    x, y = 32 + i % 3 * 512, 32 + i // 3 * 544
    draw.text((x, y), title.upper(), font=font, fill='#a0a0a0')
    im = Image.open(d / file).convert('RGB').resize((480, 480), Image.Resampling.NEAREST)
    sheet.paste(im, (x, y + 32))
sheet.save(d / 'review-en.png')
print(d / 'review-en.png')
