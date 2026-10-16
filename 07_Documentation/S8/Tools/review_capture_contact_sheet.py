"""Make labelled review thumbnails, retaining the unmodified native screenshots."""
from pathlib import Path
import math
import sys
from PIL import Image, ImageDraw

run = Path(sys.argv[1]).resolve()
paths = sorted((run / 'native').glob('*.png'))
assert paths, 'No native screenshots'
width, height, columns = 320, 204, 4
sheet = Image.new('RGB', (width * columns, height * math.ceil(len(paths) / columns)), '#202020')
draw = ImageDraw.Draw(sheet)
for index, path in enumerate(paths):
    x, y = index % columns * width, index // columns * height
    with Image.open(path) as source:
        preview = source.convert('RGB')
        preview.thumbnail((width, 180))
        sheet.paste(preview, (x, y))
    draw.text((x + 4, y + 183), path.stem, fill='white')
destination = run / 'contact-sheet.jpg'
sheet.save(destination, quality=92)
print(destination)
