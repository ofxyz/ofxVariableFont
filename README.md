# ofxVariableFont

openFrameworks addon for **variable fonts** (and static TTF/OTF): load, set design axes, kern, and render as **`ofPath`** outlines or **`drawString()`** in your coordinate units (px, mm, …).

![preview](example\preview.png)

## Features

- Variation axes with optional extrapolation
- `getGlyphPath()` / `getStringPaths()` → `ofPath`
- `drawString()`, bounds, advances, kerning
- Optional HarfBuzz GPOS; legacy kern table fallback
- Example preview: vector / hinted / raster via `ImVarFont` + Clipper2 fill

## Usage

```cpp
#include "ofxVariableFont.h"

varfont::VarFontFace face;
if (face.load("fonts/MyFont.ttf")) {
    face.applyAxes();
    for (const ofPath& p : face.getStringPaths("Hello", 0, 0, 10.f))
        p.draw();
}
```

## License

MIT