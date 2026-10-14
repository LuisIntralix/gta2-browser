// Audit harness for a locally adapted copy of gta2_re's math translation units.
// This tests selected operations; it is not a game or an original-exe oracle.
#include "Fix16_Point.hpp"
#include "BitSet32.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>

Fix16 gFix16_6777CC(0);
static unsigned checks = 0;
static unsigned failures = 0;
static unsigned checksum = 2166136261u;

static void check(int actual, int expected)
{
    ++checks;
    checksum = (checksum ^ static_cast<unsigned>(actual)) * 16777619u;
    if (actual != expected)
    {
        if (failures < 10)
            std::fprintf(stderr, "check %u: got %d expected %d\n", checks, actual, expected);
        ++failures;
    }
}

int main(int argc, char** argv)
{
    if (argc > 1 && std::strcmp(argv[1], "--negative-division") == 0)
    {
        // Isolate a signed-left-shift probe for UBSan; don't hide it in normal checks.
        volatile int raw = -16384;
        Fix16 a(raw, 0), b(32768, 0);
        std::printf("%d\n", a.Divide_436A20(b).mValue);
        return 0;
    }
    for (int a = 0; a <= 64; ++a)
    {
        for (int b = 1; b <= 32; ++b)
        {
            const int ar = a * 8192 + 123;
            const int br = b * 4096 + 71;
            Fix16 x(ar, 0), y(br, 0);
            check(x.Add_408660(y).mValue, ar + br);
            check(x.Subtract_436A00(y).mValue, ar - br);
            check(x.Multiply_408680(y).mValue,
                  static_cast<int>(std::floor(static_cast<double>(ar) * br / 16384.0)));
            check(x.Divide_436A20(y).mValue,
                  static_cast<int>(static_cast<double>(ar) * 16384.0 / br));
            check(x.Negate_4086A0().mValue, -ar);
            check(x.IsLess_451670(y), ar < br);
            check(x.IsGreater_451690(y), ar > br);
            Fix16_Point p(x, y), q(y, x);
            Fix16_Point sum = p.Add_40AC50(q);
            check(sum.x.mValue, ar + br);
            check(sum.y.mValue, ar + br);
            Fix16_Point rot = p.Rotate90CCW_5605E0();
            check(rot.x.mValue, -br);
            check(rot.y.mValue, ar);
        }
    }
    for (int a = -2880; a <= 4320; ++a)
    {
        Ang16 angle(static_cast<s16>(a), static_cast<u8>(0));
        const int expected = ((a % 1440) + 1440) % 1440;
        check(angle.rValue, expected);
        check(angle.ToAng4_405680(), expected / 360);
        check(angle.GetOctant_4056A0(), expected / 180);
    }
    for (int scale = 1; scale <= 32; ++scale)
    {
        Fix16 x(3 * scale), y(4 * scale);
        Fix16_Point p(x, y);
        check(p.GetLength_453590().mValue, 5 * scale * 16384);
    }
    BitSet32 bits;
    for (int bit = 0; bit < 31; ++bit)
    {
        bits.set_bit(bit);
        check(bits.check_bit(bit), 1);
        bits.clear_bit(bit);
        check(bits.check_bit(bit), 0);
    }
    std::printf("{\"checks\":%u,\"failures\":%u,\"checksum\":%u,"
                "\"pointer_bytes\":%zu,\"wchar_bytes\":%zu,"
                "\"fix16_bytes\":%zu,\"ang16_bytes\":%zu}\n",
                checks, failures, checksum, sizeof(void*), sizeof(wchar_t),
                sizeof(Fix16), sizeof(Ang16));
    return failures ? 1 : 0;
}
