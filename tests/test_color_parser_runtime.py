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


def test_compact_file_color_palette_rules_expand_in_profile_order(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    profile = tmp_path / "palette.conf"
    driver = tmp_path / "palette_driver.c"
    binary = tmp_path / "palette_driver"

    profile.write_text(
        """
[FILE_COLORS]
archives = red: tar,tgz,zip
scripts = +cyan on black: sh,bash
special = cyan: LINK,EXEC
""",
        encoding="utf-8",
    )

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  char pattern[32];
  int fg;
  int bg;
} CapturedRule;

static CapturedRule captured[8];
static int captured_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static void capture_parse_color(const char *color_str, int *fg, int *bg) {
  ParseColorString(color_str, fg, bg);
}

static void capture_file_color_rule(ViewContext *ctx, const char *pattern,
                                    int fg, int bg) {
  (void)ctx;
  if (captured_count >= 8)
    return;
  snprintf(captured[captured_count].pattern,
           sizeof(captured[captured_count].pattern), "%s", pattern);
  captured[captured_count].fg = fg;
  captured[captured_count].bg = bg;
  ++captured_count;
}

static int expect_rule(int index, const char *pattern, int fg, int bg) {
  if (strcmp(captured[index].pattern, pattern) != 0 ||
      captured[index].fg != fg || captured[index].bg != bg) {
    fprintf(stderr, "rule %d => %s %d,%d expected %s %d,%d\n", index,
            captured[index].pattern, captured[index].fg, captured[index].bg,
            pattern, fg, bg);
    return 1;
  }
  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = capture_parse_color;
  ctx.hook_add_file_color_rule = capture_file_color_rule;

  if (ReadProfile(&ctx, argv[1]) != 0) {
    fprintf(stderr, "ReadProfile failed\n");
    return 1;
  }

  if (captured_count != 7) {
    fprintf(stderr, "captured %d rules\n", captured_count);
    return 1;
  }

  if (expect_rule(0, "*.tar", COLOR_RED, -1) != 0 ||
      expect_rule(1, "*.tgz", COLOR_RED, -1) != 0 ||
      expect_rule(2, "*.zip", COLOR_RED, -1) != 0 ||
      expect_rule(3, "*.sh", 14, COLOR_BLACK) != 0 ||
      expect_rule(4, "*.bash", 14, COLOR_BLACK) != 0 ||
      expect_rule(5, "LINK", COLOR_CYAN, -1) != 0 ||
      expect_rule(6, "EXEC", COLOR_CYAN, -1) != 0)
    return 1;

  FreeProfileRuntimeData(&ctx);
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
            "src/cmd/profile.c",
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
    subprocess.run([str(binary), str(profile)], cwd=repo_root, check=True)
