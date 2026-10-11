import os
import shutil
import subprocess
import tempfile
import unittest
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1].parent
TESTS = ROOT / "scripts" / "tests"
BOARD = ROOT / "main" / "boards" / "xiao-esp32s3-walle"


def find_lvgl():
    """LVGL 9.5 sources: $LVGL_DIR, or the copy the IDF component manager fetched."""
    candidates = [os.environ.get("LVGL_DIR"), ROOT / "managed_components" / "lvgl__lvgl"]
    for candidate in candidates:
        if candidate and (Path(candidate) / "lvgl.h").is_file():
            return Path(candidate)
    return None


class WalleFaceHostTest(unittest.TestCase):
    def test_face_renders_correctly_on_host(self):
        lvgl = find_lvgl()
        compiler_c = shutil.which("gcc") or shutil.which("cc")
        compiler_cxx = shutil.which("g++") or shutil.which("c++")
        if lvgl is None or compiler_c is None or compiler_cxx is None:
            self.skipTest("needs LVGL sources (set LVGL_DIR or run an IDF build first) and a host C/C++ compiler")

        defines = ["-DLV_CONF_SKIP", "-DLV_MEM_SIZE=(512*1024U)", "-DLV_COLOR_DEPTH=16"]
        includes = [f"-I{lvgl}", f"-I{lvgl.parent}", f"-I{TESTS / 'host_stubs'}", f"-I{BOARD}"]
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            sources = sorted((lvgl / "src").rglob("*.c"))

            def compile_c(index_source):
                index, source = index_source
                obj = tmp / f"lvgl_{index}.o"
                subprocess.run([compiler_c, "-c", "-O1", "-w", *defines, *includes, str(source), "-o", str(obj)],
                               check=True, capture_output=True)
                return obj

            with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
                objects = list(pool.map(compile_c, enumerate(sources)))

            binary = tmp / "walle_face_host_test"
            cxx_flags = ["-std=gnu++20", "-O1", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter"]
            build = subprocess.run(
                [compiler_cxx, *cxx_flags, *defines, *includes,
                 str(TESTS / "walle_face_host_test.cc"), str(BOARD / "walle_face.cc"),
                 *map(str, objects), "-lm", "-o", str(binary)],
                capture_output=True, text=True)
            self.assertEqual(build.returncode, 0, build.stderr)

            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=600)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
