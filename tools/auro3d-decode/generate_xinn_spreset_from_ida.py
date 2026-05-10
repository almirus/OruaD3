import ida_bytes
import struct


OUT = r"C:\Users\USER\IdeaProjects\Orua D3\tools\auro3d-decode\auro3d_xinn_spreset_data.inc"
SPRESET_SIZE = 7568

PRESETS = [
    ("2in6_1", 0x2A5730, 12216),
    ("2in6_2", 0x2A2750, 12241),
    ("2in6_3", 0x29F770, 12249),
    ("2in6_4", 0x29C790, 12240),
    ("5inN_1", 0x2967F0, 12217),
    ("5inN_2", 0x2997B0, 12242),
    ("5inN_3", 0x293810, 12250),
    ("5inN_4", 0x290830, 12241),
]


class Parser:
    def __init__(self, text):
        self.lines = text.replace("\r\n", "\n").replace("\r", "\n").split("\n")
        self.pos = 0

    def next_data(self):
        while self.pos < len(self.lines):
            line = self.lines[self.pos].strip()
            self.pos += 1
            if not line or line.startswith(";"):
                continue
            return line
        raise ValueError("unexpected end of preset text")

    def ints(self, n):
        parts = self.next_data().split()
        if len(parts) < n:
            raise ValueError("not enough ints")
        return [int(x, 10) for x in parts[:n]]

    def floats(self, n):
        parts = self.next_data().split()
        if len(parts) < n:
            raise ValueError("not enough floats")
        return [float(x) for x in parts[:n]]

    def int_float(self):
        parts = self.next_data().split()
        if len(parts) < 2:
            raise ValueError("not enough int/float fields")
        return int(parts[0], 10), float(parts[1])


def w32(buf, idx, val):
    struct.pack_into("<i", buf, idx * 4, int(val))


def wf32(buf, idx, val):
    struct.pack_into("<f", buf, idx * 4, float(val))


def parse_spreset(text):
    p = Parser(text)
    buf = bytearray(SPRESET_SIZE)

    a0, a1 = p.ints(2)
    w32(buf, 0, a0)
    w32(buf, 1, a0 + a1)
    for i, val in enumerate(p.floats(3)):
        wf32(buf, 2 + i, val)
    for i, val in enumerate(p.floats(2)):
        wf32(buf, 5 + i, val)

    count = a0 + a1
    for group in range(8):
        base = 7 + 192 * group
        for row in range(count):
            ival, fval = p.int_float()
            w32(buf, base + row * 2, ival)
            wf32(buf, base + row * 2 + 1, fval)
        for row in range(count):
            ival, fval = p.int_float()
            w32(buf, base + 96 + row * 2, ival)
            wf32(buf, base + 96 + row * 2 + 1, fval)

    wf32(buf, 1543, p.floats(1)[0])
    wf32(buf, 1544, p.floats(1)[0])
    for i, val in enumerate(p.floats(2)):
        wf32(buf, 1545 + i, val)
    v0, v1 = p.ints(2)
    w32(buf, 1547, v0)
    w32(buf, 1548, max(v1, 48))
    wf32(buf, 1549, p.floats(1)[0])
    wf32(buf, 1550, p.floats(1)[0])

    for block in range(16):
        base = 1551 + 11 * block
        b0, b1 = p.ints(2)
        w32(buf, base + 0, b0)
        w32(buf, base + 1, b1)
        w32(buf, base + 2, b0 + b1)
        wf32(buf, base + 3, p.floats(1)[0])
        vals = p.floats(7)
        for i, val in enumerate(vals):
            wf32(buf, base + 4 + i, val)

    for base in (1727, 1743, 1759, 1775, 1791, 1807, 1823, 1839):
        vals = p.floats(16)
        for i, val in enumerate(vals):
            wf32(buf, base + i, val)

    vals = p.ints(4)
    for i, val in enumerate(vals):
        w32(buf, 1855 + i, max(val, 48))

    for base in (1859, 1862, 1865, 1868):
        for i, val in enumerate(p.floats(3)):
            wf32(buf, base + i, val)
    try:
        vals = p.floats(3)
        for i, val in enumerate(vals):
            wf32(buf, 1871 + i, val)
    except ValueError:
        pass
    for idx in range(1874, 1879):
        try:
            wf32(buf, idx, p.floats(1)[0])
        except ValueError:
            break
    for base in (1879, 1885):
        try:
            vals = p.floats(6)
        except ValueError:
            vals = [0] * 6
        for i, val in enumerate(vals):
            wf32(buf, base + i, val)

    return bytes(buf)


def read_text(addr, size):
    data = ida_bytes.get_bytes(addr, size)
    if data is None or len(data) != size:
        raise RuntimeError("failed to read preset text at 0x%x" % addr)
    return data.decode("ascii")


def cpp_bytes(data):
    out = []
    for i in range(0, len(data), 16):
        out.append("    " + ", ".join("0x%02X" % b for b in data[i:i + 16]) + ",")
    return "\n".join(out)


parsed = []
for name, addr, size in PRESETS:
    parsed.append((name, parse_spreset(read_text(addr, size))))

with open(OUT, "w", newline="\n") as f:
    f.write("// Generated from IDA libauro.so preset cstr data. Do not edit by hand.\n\n")
    f.write("namespace {\n\n")
    for name, data in parsed:
        f.write("alignas(4) constexpr std::array<std::uint8_t, 7568> kXinnSpreset_%s = {{\n" % name)
        f.write(cpp_bytes(data))
        f.write("\n}};\n\n")
    f.write("constexpr const std::array<std::uint8_t, 7568>* kXinnSpreset2in6Portable[4] = {\n")
    f.write("    &kXinnSpreset_2in6_1, &kXinnSpreset_2in6_2, &kXinnSpreset_2in6_3, &kXinnSpreset_2in6_4,\n};\n")
    f.write("constexpr const std::array<std::uint8_t, 7568>* kXinnSpreset5inNPortable[4] = {\n")
    f.write("    &kXinnSpreset_5inN_1, &kXinnSpreset_5inN_2, &kXinnSpreset_5inN_3, &kXinnSpreset_5inN_4,\n};\n\n")
    f.write("} // namespace\n")

print("wrote", OUT, "presets", len(parsed))
