"""
https://marlinfw.org/tools/u8glib/converter.html
"""
import sys
import io

found = False
cols = 0
lines = []

name = input("Icon name: ")
print("Paste: ")

while True:
    line = input()
    if line.startswith("const unsigned char"):
        found = True
    elif line == "};":
        break
    elif found:
        if not cols:
            cols = line.count(",")
        line = line.replace("B", "0b")
        lines.append(line.strip())

cropped_lines = []
for line in lines:
    if not line.endswith(","):
        line += ","
    if line != "0b00000000," * cols:
        cropped_lines.append(line)

code = "\n".join(cropped_lines)

print("Found icon:")
print(code)

code = code.rstrip(",")
chars = code.split(",")

img = f"const uint8_t {name.lower()}_icon{len(cropped_lines)}x{cols * 8} [] PROGMEM = {{"
for col in range(cols - 1, -1, -1):
    img += "\n    "
    for i in range(0, len(chars), cols):
        img += f"{chars[col + i].strip()},"
img += "\n};"

print(f"// {name.title()} Icon")
print(img)