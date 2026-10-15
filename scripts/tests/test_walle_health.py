import json
import shutil
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
BOARD = ROOT / "main" / "boards" / "xiao-esp32s3-walle"


class WalleHealthTest(unittest.TestCase):
    """Reset counters the WALL-E board adds to its OTA check-in."""

    def run_driver(self, body: str) -> str:
        compiler = shutil.which("g++") or shutil.which("c++")
        if compiler is None:
            self.skipTest("needs a host C++ compiler")
        driver = textwrap.dedent("""
            #include <cstdio>
            #include <string>

            #include "walle_health.h"

            int main() {
            %s
                return 0;
            }
        """) % body
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            (tmp / "driver.cc").write_text(driver)
            build = subprocess.run(
                [compiler, "-std=gnu++20", "-Wall", "-Wextra", "-Werror", f"-I{BOARD}",
                 str(tmp / "driver.cc"), str(BOARD / "walle_health.cc"), "-o", str(tmp / "driver")],
                capture_output=True, text=True)
            self.assertEqual(build.returncode, 0, build.stderr)
            result = subprocess.run([str(tmp / "driver")], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stderr)
            return result.stdout

    def test_counts_resets_by_kind(self):
        out = self.run_driver(r"""
            walle::HealthCounters c;
            for (const char* r : {"power_on", "brownout", "brownout", "panic", "task_wdt",
                                  "int_wdt", "wdt", "software", "other"}) {
                walle::CountBoot(c, r);
            }
            std::printf("%d %d %d %d", c.boots, c.brownouts, c.panics, c.watchdogs);
        """)
        self.assertEqual(out, "9 2 1 3")

    def test_health_is_added_to_the_board_json(self):
        out = self.run_driver(r"""
            walle::HealthCounters c{12, 4, 1, 0};
            std::printf("%s\n", walle::AppendHealthJson(
                R"({"type":"xiao-esp32s3-walle","mac":"aa:bb"})", c, "brownout").c_str());
            std::printf("%s\n", walle::AppendHealthJson("{}", c, "power_on").c_str());
            std::printf("%s\n", walle::AppendHealthJson("not json", c, "panic").c_str());
        """)
        board, empty, bad = out.splitlines()
        board = json.loads(board)
        self.assertEqual(board["type"], "xiao-esp32s3-walle")
        self.assertEqual(board["health"], {"reset_reason": "brownout", "boots": 12,
                                           "brownouts": 4, "panics": 1, "watchdogs": 0})
        self.assertEqual(json.loads(empty)["health"]["reset_reason"], "power_on")
        self.assertEqual(bad, "not json")


if __name__ == "__main__":
    unittest.main()
