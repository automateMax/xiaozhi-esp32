import shutil
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
BOARD = ROOT / "main" / "boards" / "xiao-esp32s3-walle"


class WalleSequenceTest(unittest.TestCase):
    """The track choreography parser behind the self.walle.sequence MCP tool."""

    def test_parser_accepts_valid_and_rejects_unsafe_sequences(self):
        compiler = shutil.which("g++") or shutil.which("c++")
        if compiler is None:
            self.skipTest("needs a host C++ compiler")
        driver = textwrap.dedent(r"""
            #include <cstdio>
            #include <string>
            #include <vector>

            #include "walle_sequence.h"

            static int failures = 0;

            static void Expect(bool ok, const char* what) {
                if (!ok) {
                    std::printf("FAIL: %s\n", what);
                    failures++;
                }
            }

            static bool Parse(const char* text, int repeat, std::vector<walle::SequenceStep>& steps) {
                std::string error;
                return walle::ParseSequence(text, repeat, steps, error);
            }

            int main() {
                std::vector<walle::SequenceStep> s;

                Expect(Parse("F75:600, B70:300,L70:300,R70:300,T40/-90:1200,P:400", 1, s),
                       "all step kinds parse");
                Expect(s.size() == 6, "six steps");
                Expect(s[0].left == 75 && s[0].right == 75 && s[0].ms == 600, "forward");
                Expect(s[1].left == -70 && s[1].right == -70, "backward");
                Expect(s[2].left == -70 && s[2].right == 70, "left turns in place");
                Expect(s[3].left == 70 && s[3].right == -70, "right turns in place");
                Expect(s[4].left == 40 && s[4].right == -90 && s[4].ms == 1200, "raw tracks");
                Expect(s[5].left == 0 && s[5].right == 0 && s[5].ms == 400, "pause");

                Expect(Parse("f70:150,b70:150,", 2, s) && s.size() == 4,
                       "lower case, trailing comma and repeat");

                Expect(!Parse("", 1, s), "empty");
                Expect(!Parse("F75", 1, s), "missing duration");
                Expect(!Parse("F0:500", 1, s), "speed 0");
                Expect(!Parse("F101:500", 1, s), "speed above 100");
                Expect(!Parse("T120/0:500", 1, s), "track speed above 100");
                Expect(!Parse("F75:10", 1, s), "step too short");
                Expect(!Parse("F75:3001", 1, s), "step too long");
                Expect(!Parse("X75:500", 1, s), "unknown step");
                Expect(!Parse("F75:500,,B75:500", 1, s), "empty step in the middle");
                Expect(!Parse("F75:500x", 1, s), "junk after duration");
                Expect(!Parse("F75:500", 0, s) && !Parse("F75:500", 6, s), "repeat range");
                Expect(!Parse("F75:3000,F75:3000,F75:3000,F75:1001", 1, s), "over 10 s");
                Expect(Parse("F75:2000", 5, s) && s.size() == 5, "exactly 10 s is allowed");
                Expect(!Parse("P:20,P:20,P:20,P:20,P:20,P:20,P:20,P:20,P:20", 4, s),
                       "over 32 steps");
                Expect(s.empty(), "steps cleared on failure");

                if (failures == 0) std::printf("OK\n");
                return failures == 0 ? 0 : 1;
            }
        """)
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            source = tmp / "driver.cc"
            source.write_text(driver)
            binary = tmp / "driver"
            build = subprocess.run(
                [compiler, "-std=gnu++20", "-Wall", "-Wextra", "-Werror", f"-I{BOARD}",
                 str(source), str(BOARD / "walle_sequence.cc"), "-o", str(binary)],
                capture_output=True, text=True)
            self.assertEqual(build.returncode, 0, build.stderr)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


    def test_motor_runner_plays_steps_and_cancels_safely(self):
        """Runs the real walle_motors.cc against fake LEDC and esp_timer headers."""
        compiler = shutil.which("g++") or shutil.which("c++")
        if compiler is None:
            self.skipTest("needs a host C++ compiler")
        driver = textwrap.dedent(r"""
            #include <cstdio>
            #include <vector>

            #define private public
            #include "walle_motors.h"
            #undef private

            static int failures = 0;

            static void Expect(bool ok, const char* what) {
                if (!ok) {
                    std::printf("FAIL: %s\n", what);
                    failures++;
                }
            }

            // Channels 2/3 drive the left track, 4/5 the right (walle_motors.cc).
            static int Track(int a, int b) {
                if (g_ledc_duty[a] > 0) return 1;
                if (g_ledc_duty[b] > 0) return -1;
                return 0;
            }
            static int Left() { return Track(2, 3); }
            static int Right() { return Track(4, 5); }

            // Lets the armed timer expire, as the esp_timer task would.
            static void Expire(WalleMotors& m) {
                g_now_us += m.stop_timer_->timeout_us;
                esp_timer_fire(m.stop_timer_);
            }

            int main() {
                WalleMotors m(1, 2, 3, 4, false, false);

                // A sequence plays every step in order, then stops.
                m.RunSequence({{75, 75, 600}, {-70, 70, 300}, {0, 0, 200}});
                Expect(Left() == 1 && Right() == 1, "step 1 drives forward");
                Expect(m.stop_timer_->armed && m.stop_timer_->timeout_us == 600000, "step 1 timed");
                Expire(m);
                Expect(Left() == -1 && Right() == 1, "step 2 turns left");
                Expect(m.stop_timer_->timeout_us == 300000, "step 2 timed");
                Expire(m);
                Expect(Left() == 0 && Right() == 0, "step 3 pauses");
                Expire(m);
                Expect(Left() == 0 && Right() == 0 && !m.stop_timer_->armed, "stopped at the end");

                // Stop cancels a sequence, and its late callback changes nothing.
                m.RunSequence({{75, 75, 600}, {75, 75, 600}});
                m.Stop();
                Expect(Left() == 0 && !m.stop_timer_->armed, "stop cancels");
                esp_timer_fire(m.stop_timer_);
                Expect(Left() == 0 && Right() == 0, "a stale callback after stop does nothing");

                // A callback already running when a new command re-arms the timer
                // must not cut the new command short.
                m.DriveFor(80, 80, 1000);
                g_now_us += 1000000;  // the old timer is due...
                m.DriveFor(-80, -80, 1000);  // ...but a new move takes the lock first
                esp_timer_fire(m.stop_timer_);  // the old callback runs now
                Expect(Left() == -1 && Right() == -1, "new move survives a stale callback");
                Expire(m);
                Expect(Left() == 0 && Right() == 0, "new move stops on its own deadline");

                // Same race with a sequence: the stale callback must not skip a step.
                m.RunSequence({{75, 75, 500}, {-75, -75, 500}});
                esp_timer_fire(m.stop_timer_);
                Expect(Left() == 1, "a stale callback does not advance the sequence");
                Expire(m);
                Expect(Left() == -1, "the real deadline advances it");
                m.Stop();

                // An empty sequence just stops.
                m.DriveFor(80, 80, 1000);
                m.RunSequence({});
                Expect(Left() == 0 && !m.stop_timer_->armed, "empty sequence stops");

                if (failures == 0) std::printf("OK\n");
                return failures == 0 ? 0 : 1;
            }
        """)
        stubs = ROOT / "scripts" / "tests" / "host_stubs"
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            source = tmp / "driver.cc"
            source.write_text(driver)
            binary = tmp / "driver"
            build = subprocess.run(
                [compiler, "-std=gnu++20", "-Wall", "-Wextra", "-Werror", f"-I{stubs}", f"-I{BOARD}",
                 str(source), str(BOARD / "walle_motors.cc"), str(BOARD / "walle_sequence.cc"),
                 "-o", str(binary)],
                capture_output=True, text=True)
            self.assertEqual(build.returncode, 0, build.stderr)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
