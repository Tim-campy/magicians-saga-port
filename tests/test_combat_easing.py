"""Static arithmetic reference checks; never an Android runtime oracle."""
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from verify_combat_easing import original_numeric_model as easing


class EasingModelTests(unittest.TestCase):
    def test_model_boundaries_and_curvature(self):
        self.assertEqual(easing(9, 4, 0, -1, -50), 9)
        self.assertEqual(easing(9, 4, 0, 0, -50), 4)
        self.assertEqual(easing(0, 1000, 100, 50, -50), 340)
        self.assertEqual(easing(100, 1000, 999, 500, -50), 407)
        self.assertEqual(easing(100, 1, 999, 555), 44)

    def test_cpp_fixture_hash(self):
        digest = 14695981039346656037
        count = 0
        for start, end in ((100, 1000), (1000, 100), (-1000, 50), (0, 9999999)):
            for duration in (199, 999):
                for amplitude in (-80, -50, 0, 80):
                    for progress in range(-1, duration + 2):
                        value = easing(start, end, duration, progress, amplitude)
                        digest = ((digest ^ (value & 0xffffffff)) * 1099511628211) & 0xffffffffffffffff
                        count += 1
        self.assertEqual(count, 19264)
        self.assertEqual(digest, 17971640406678543181)


if __name__ == '__main__':
    unittest.main()
