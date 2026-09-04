import subprocess
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]


def test_compact_duration_uses_only_meaningful_units(tmp_path):
    driver = tmp_path / "compact_duration.c"
    binary = tmp_path / "compact_duration"
    driver.write_text(
        r'''
#include "ytnova_defs.h"

#include <string.h>

static int matches(int seconds, const char *expected) {
  char buffer[32];

  String_FormatCompactDuration(seconds, buffer, sizeof(buffer));
  return strcmp(buffer, expected) == 0;
}

int main(void) {
  return matches(-1, "0s") && matches(8, "8s") &&
                 matches(68, "1m 08s") &&
                 matches(3723, "1h 02m 03s")
             ? 0
             : 1;
}
''',
        encoding="utf-8",
    )
    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-std=c99",
            "-Iinclude",
            str(driver),
            "src/util/string_utils.c",
            "-o",
            str(binary),
        ],
        cwd=REPO_ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True)
