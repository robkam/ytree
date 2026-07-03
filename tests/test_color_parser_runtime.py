import subprocess
from pathlib import Path


def test_color_parser_accepts_theme_style_syntax(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    driver = tmp_path / "color_parser_driver.c"
    binary = tmp_path / "color_parser_driver"

    driver.write_text(
        r'''
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

typedef struct {
  const char *text;
  int expected_fg;
  int expected_bg;
} ColorCase;

static int expect_color(const ColorCase *entry) {
  int fg = -1;
  int bg = -1;

  ParseColorString(entry->text, &fg, &bg);
  if (fg != entry->expected_fg || bg != entry->expected_bg) {
    fprintf(stderr, "%s => %d,%d expected %d,%d\n", entry->text, fg, bg,
            entry->expected_fg, entry->expected_bg);
    return 1;
  }
  return 0;
}

int main(void) {
  const ColorCase cases[] = {
      {"grey on blue", 8, COLOR_BLUE},
      {"gray,black", 8, COLOR_BLACK},
      {"+grey on black", COLOR_WHITE, COLOR_BLACK},
      {"+red on +grey", 9, COLOR_WHITE},
      {"red", COLOR_RED, -1},
      {"cyan,blue", COLOR_CYAN, COLOR_BLUE},
  };
  size_t i;

  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    if (expect_color(&cases[i]) != 0)
      return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary)], cwd=repo_root, check=True)


def test_file_color_rules_preserve_profile_order(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    driver = tmp_path / "file_color_order_driver.c"
    binary = tmp_path / "file_color_order_driver"

    driver.write_text(
        r'''
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

int main(void) {
  ViewContext ctx;
  FileColorRule *head;

  memset(&ctx, 0, sizeof(ctx));
  AddFileColorRule(&ctx, "*.sh", COLOR_CYAN, COLOR_BLACK);
  AddFileColorRule(&ctx, "*.py", COLOR_GREEN, COLOR_BLACK);
  head = (FileColorRule *)ctx.file_color_rules_head;

  if (head == NULL || strcmp(head->pattern, "*.sh") != 0 ||
      head->next == NULL || strcmp(head->next->pattern, "*.py") != 0) {
    fprintf(stderr, "file color rule order was not preserved\n");
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary)], cwd=repo_root, check=True)
