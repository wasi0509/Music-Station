from pathlib import Path
import re

SOURCE = Path("u8g2/csrc/u8g2_fonts.c")
OUTPUT = Path("components/u8g2/cjk_fonts.c")

FONT_NAMES = [
    "u8g2_font_unifont_t_latin",
    "u8g2_font_unifont_t_chinese1",
    "u8g2_font_unifont_t_chinese2",
    "u8g2_font_unifont_t_chinese3",
    "u8g2_font_unifont_t_japanese1",
    "u8g2_font_unifont_t_japanese2",
    "u8g2_font_unifont_t_japanese3",
    "u8g2_font_unifont_t_korean1",
    "u8g2_font_unifont_t_korean2",
]

# The U8g2 source contains escaped byte sequences such as \234.
# latin-1 lets us read the file without UTF-8 decoding errors.
text = SOURCE.read_text(encoding="latin-1")

output = [
    "/* Auto-generated from U8g2 font data. */",
    '#include "u8g2.h"',
    "",
]

for name in FONT_NAMES:

    # A font looks like:
    #
    # const uint8_t font_name[12345] U8G2_FONT_SECTION("font_name") =
    #   "...."
    #   "...."
    #   "...";
    #
    # We capture only this declaration, stopping at the
    # first semicolon after the font declaration.

    pattern = (
        r"const\s+uint8_t\s+"
        + re.escape(name)
        + r"\[\d+\]\s+"
        r"U8G2_FONT_SECTION\(\""
        + re.escape(name)
        + r"\"\)\s*=\s*"
        r"(\"(?:[^\"\\]|\\.)*\"\s*)+;"
    )

    match = re.search(pattern, text, re.DOTALL)

    if not match:
        print(f"WARNING: {name} not found")
        continue

    extracted = match.group(0)

    output.append(extracted)
    output.append("")

    print(f"Extracted: {name}")

OUTPUT.parent.mkdir(parents=True, exist_ok=True)

OUTPUT.write_text(
    "\n".join(output),
    encoding="latin-1"
)

print()
print(f"Created: {OUTPUT}")
print(f"Size: {OUTPUT.stat().st_size:,} bytes")