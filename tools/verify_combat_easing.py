"""Static numeric model for report 64; does not execute the original game."""
import math
import struct


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def original_numeric_model(start, end, duration, progress):
    ratio = f32(progress / duration)
    pi = f32(math.pi)
    angle = f32(f32(f32(ratio * pi) * 180.0) / pi)
    sine = f32(math.sin(angle * math.pi / 180.0))
    assert math.isfinite(sine)
    correction = f32(f32(0.0 / f32(314.1592712402344)) * sine)
    return int(f32(f32(f32(ratio + correction) * (end - start)) + start))


def main():
    count = 0
    for start, end, duration in ((10, 100, 50), (1, 30, 98)):
        for progress in range(duration):
            observed = original_numeric_model(start, end, duration, progress)
            expected = start + progress * (end - start) // duration
            assert observed == expected, (start, end, duration, progress, observed, expected)
            count += 1
    print(f'{count} interior values verified against static binary32 model; no runtime oracle')


if __name__ == '__main__':
    main()
