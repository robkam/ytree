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


def test_legacy_profile_color_sections_are_ignored(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    profile = tmp_path / "palette.conf"
    driver = tmp_path / "palette_driver.c"
    binary = tmp_path / "palette_driver"

    profile.write_text(
        """
[COLORS]
DIALOG_COLOR = white on blue

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

static int captured_colors;
static int captured_rules;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static void capture_parse_color(const char *color_str, int *fg, int *bg) {
  ParseColorString(color_str, fg, bg);
}

static void capture_update_color(const char *name, int fg, int bg) {
  (void)name;
  (void)fg;
  (void)bg;
  ++captured_colors;
}

static void capture_file_color_rule(ViewContext *ctx, const char *pattern,
                                     int fg, int bg) {
  (void)ctx;
  (void)pattern;
  (void)fg;
  (void)bg;
  ++captured_rules;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = capture_parse_color;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = capture_file_color_rule;

  if (ReadProfile(&ctx, argv[1]) != 0) {
    fprintf(stderr, "ReadProfile failed\n");
    return 1;
  }

  if (captured_colors != 0 || captured_rules != 0) {
    fprintf(stderr, "legacy color sections changed runtime colors: %d/%d\n",
            captured_colors, captured_rules);
    return 1;
  }

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


def test_theme_file_loader_maps_roles_and_palette_rules(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    theme_file = tmp_path / "themes.conf"
    driver = tmp_path / "theme_loader_driver.c"
    binary = tmp_path / "theme_loader_driver"

    theme_file.write_text(
        """
[theme sample]
background = blue
box_lines = cyan on blue
tree_lines = +white on blue
margin = dynamic_text
static_text = white on blue
dynamic_text = +white on blue
keybind = +white on blue
selection = black on +grey
dialog = black on +grey
picker = black on +grey
help = white on blue
info = +white on blue
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey on blue

[file-types sample]
archives = red: tar,zip
scripts = +cyan on black: sh
links = cyan: LINK
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
  char name[32];
  int fg;
  int bg;
} CapturedColor;

typedef struct {
  char pattern[32];
  int fg;
  int bg;
} CapturedRule;

static CapturedColor colors[32];
static int color_count;
static CapturedRule rules[8];
static int rule_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static void capture_parse_color(const char *color_str, int *fg, int *bg) {
  ParseColorString(color_str, fg, bg);
}

static void capture_update_color(const char *name, int fg, int bg) {
  if (color_count >= 32)
    return;
  snprintf(colors[color_count].name, sizeof(colors[color_count].name), "%s",
           name);
  colors[color_count].fg = fg;
  colors[color_count].bg = bg;
  ++color_count;
}

static void capture_file_color_rule(ViewContext *ctx, const char *pattern,
                                    int fg, int bg) {
  (void)ctx;
  if (rule_count >= 8)
    return;
  snprintf(rules[rule_count].pattern, sizeof(rules[rule_count].pattern), "%s",
           pattern);
  rules[rule_count].fg = fg;
  rules[rule_count].bg = bg;
  ++rule_count;
}

static int expect_color(const char *name, int fg, int bg) {
  int i;

  for (i = 0; i < color_count; ++i) {
    if (strcmp(colors[i].name, name) == 0 && colors[i].fg == fg &&
        colors[i].bg == bg)
      return 0;
  }

  fprintf(stderr, "missing color %s %d,%d\n", name, fg, bg);
  return 1;
}

static int expect_rule(int index, const char *pattern, int fg, int bg) {
  if (strcmp(rules[index].pattern, pattern) != 0 || rules[index].fg != fg ||
      rules[index].bg != bg) {
    fprintf(stderr, "rule %d => %s %d,%d expected %s %d,%d\n", index,
            rules[index].pattern, rules[index].fg, rules[index].bg, pattern,
            fg, bg);
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
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = capture_file_color_rule;

  if (ReadThemeFile(&ctx, argv[1], "sample") != 0) {
    fprintf(stderr, "ReadThemeFile failed\n");
    return 1;
  }

  if (expect_color("dynamic_text", 15, COLOR_BLUE) != 0 ||
      expect_color("tree_lines", 15, COLOR_BLUE) != 0 ||
      expect_color("margin", 15, COLOR_BLUE) != 0 ||
      expect_color("box_lines", COLOR_CYAN, COLOR_BLUE) != 0 ||
      expect_color("selection", COLOR_BLACK, COLOR_WHITE) != 0 ||
      expect_color("error", 15, COLOR_RED) != 0 ||
      expect_color("disabled", 8, COLOR_BLUE) != 0)
    return 1;

  if (rule_count != 4) {
    fprintf(stderr, "captured %d rules\n", rule_count);
    return 1;
  }

  if (expect_rule(0, "*.tar", COLOR_RED, -1) != 0 ||
      expect_rule(1, "*.zip", COLOR_RED, -1) != 0 ||
      expect_rule(2, "*.sh", 14, COLOR_BLACK) != 0 ||
      expect_rule(3, "LINK", COLOR_CYAN, -1) != 0)
    return 1;

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
            "src/cmd/theme.c",
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
    subprocess.run([str(binary), str(theme_file)], cwd=repo_root, check=True)


def test_theme_margin_defaults_to_dynamic_text_when_omitted(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    theme_file = tmp_path / "themes.conf"
    driver = tmp_path / "theme_margin_default_driver.c"
    binary = tmp_path / "theme_margin_default_driver"

    theme_file.write_text(
        """
[theme sample]
background = blue
box_lines = cyan on blue
tree_lines = +white on blue
static_text = white on blue
dynamic_text = +white on blue
keybind = +white on blue
selection = black on +grey
dialog = black on +grey
picker = black on +grey
help = white on blue
info = +white on blue
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey on blue
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
  char name[32];
  int fg;
  int bg;
} CapturedColor;

static CapturedColor colors[32];
static int color_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static void capture_update_color(const char *name, int fg, int bg) {
  if (color_count >= 32)
    return;
  snprintf(colors[color_count].name, sizeof(colors[color_count].name), "%s",
           name);
  colors[color_count].fg = fg;
  colors[color_count].bg = bg;
  ++color_count;
}

static int expect_color(const char *name, int fg, int bg) {
  int i;

  for (i = 0; i < color_count; ++i) {
    if (strcmp(colors[i].name, name) == 0 && colors[i].fg == fg &&
        colors[i].bg == bg)
      return 0;
  }

  fprintf(stderr, "missing color %s %d,%d\n", name, fg, bg);
  return 1;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = AddFileColorRule;

  if (ReadThemeFile(&ctx, argv[1], "sample") != 0) {
    fprintf(stderr, "ReadThemeFile failed\n");
    return 1;
  }

  if (expect_color("margin", 15, COLOR_BLUE) != 0)
    return 1;

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
            "src/cmd/theme.c",
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
    subprocess.run([str(binary), str(theme_file)], cwd=repo_root, check=True)


def test_theme_palette_omitted_background_inherits_theme_filename_background(
    tmp_path,
):
    repo_root = Path(__file__).resolve().parents[1]
    theme_file = tmp_path / "themes.conf"
    driver = tmp_path / "theme_background_driver.c"
    binary = tmp_path / "theme_background_driver"

    theme_file.write_text(
        """
[theme sample]
background = blue
box_lines = cyan on blue
tree_lines = +white on blue
margin = dynamic_text
static_text = white on blue
dynamic_text = +white on blue
keybind = +white on blue
selection = black on +grey
dialog = black on +grey
picker = black on +grey
help = white on blue
info = +white on blue
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey on blue

[file-types sample]
archives = red: zip
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

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;
  FileColorRule *rule;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = UpdateUIColor;
  ctx.hook_add_file_color_rule = AddFileColorRule;

  if (ReadThemeFile(&ctx, argv[1], "sample") != 0) {
    fprintf(stderr, "ReadThemeFile failed\n");
    return 1;
  }

  rule = (FileColorRule *)ctx.file_color_rules_head;
  if (rule == NULL || strcmp(rule->pattern, "*.zip") != 0 ||
      rule->fg != COLOR_RED || rule->bg != COLOR_BLUE) {
    fprintf(stderr, "file rule did not inherit theme filename background\n");
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
            "src/cmd/theme.c",
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
    subprocess.run([str(binary), str(theme_file)], cwd=repo_root, check=True)


def test_failed_theme_load_keeps_previous_file_palette(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    theme_file = tmp_path / "themes.conf"
    driver = tmp_path / "theme_failure_driver.c"
    binary = tmp_path / "theme_failure_driver"

    theme_file.write_text(
        """
[file-types sample]
archives = red: zip
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

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;
  FileColorRule *rule;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = UpdateUIColor;
  ctx.hook_add_file_color_rule = AddFileColorRule;

  AddFileColorRule(&ctx, "*.old", COLOR_GREEN, COLOR_BLACK);

  if (ReadThemeFile(&ctx, argv[1], "sample") == 0) {
    fprintf(stderr, "missing theme unexpectedly loaded\n");
    return 1;
  }

  rule = (FileColorRule *)ctx.file_color_rules_head;
  if (rule == NULL || strcmp(rule->pattern, "*.old") != 0 ||
      rule->next != NULL) {
    fprintf(stderr, "previous file palette was not retained\n");
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
            "src/cmd/theme.c",
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
    subprocess.run([str(binary), str(theme_file)], cwd=repo_root, check=True)


def test_invalid_theme_load_keeps_previous_runtime_state(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    incomplete_theme = tmp_path / "incomplete.conf"
    invalid_theme = tmp_path / "invalid.conf"
    invalid_palette_theme = tmp_path / "invalid_palette.conf"
    driver = tmp_path / "theme_atomic_failure_driver.c"
    binary = tmp_path / "theme_atomic_failure_driver"

    incomplete_theme.write_text(
        """
[theme sample]
background = blue
box_lines = cyan on blue

[file-types sample]
archives = red: zip
""",
        encoding="utf-8",
    )
    invalid_theme.write_text(
        """
[theme sample]
background = blue
box_lines = cyan on blue
tree_lines = +white on blue
margin = dynamic_text
static_text = white on blue
dynamic_text = +white on blue
keybind = +white on blue
selection = black on +grey
dialog = black on +grey
picker = black on +grey
help = white on blue
info = +white on blue
warning = black on yellow
error = white on not-a-color
search_hit = black on yellow
disabled = grey on blue

[file-types sample]
archives = red: zip
""",
        encoding="utf-8",
    )
    invalid_palette_theme.write_text(
        """
[theme sample]
background = blue
box_lines = cyan on blue
tree_lines = +white on blue
margin = dynamic_text
static_text = white on blue
dynamic_text = +white on blue
keybind = +white on blue
selection = black on +grey
dialog = black on +grey
picker = black on +grey
help = white on blue
info = +white on blue
warning = black on yellow
error = white on red
search_hit = black on yellow
disabled = grey on blue

[file-types sample]
archives = red on not-a-color: zip
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

static int color_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static void capture_update_color(const char *name, int fg, int bg) {
  (void)name;
  (void)fg;
  (void)bg;
  ++color_count;
}

static int expect_previous_state(ViewContext *ctx, const char *path) {
  FileColorRule *rule;

  color_count = 0;
  if (ReadThemeFile(ctx, path, "sample") == 0) {
    fprintf(stderr, "%s unexpectedly loaded\n", path);
    return 1;
  }
  if (color_count != 0) {
    fprintf(stderr, "%s changed %d colors\n", path, color_count);
    return 1;
  }

  rule = (FileColorRule *)ctx->file_color_rules_head;
  if (rule == NULL || strcmp(rule->pattern, "*.old") != 0 ||
      rule->fg != COLOR_GREEN || rule->bg != COLOR_BLACK || rule->next != NULL) {
    fprintf(stderr, "%s did not preserve previous file palette\n", path);
    return 1;
  }

  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 4)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = AddFileColorRule;
  AddFileColorRule(&ctx, "*.old", COLOR_GREEN, COLOR_BLACK);

  if (expect_previous_state(&ctx, argv[1]) != 0)
    return 1;
  if (expect_previous_state(&ctx, argv[2]) != 0)
    return 1;
  if (expect_previous_state(&ctx, argv[3]) != 0)
    return 1;

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
            "src/cmd/theme.c",
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
    subprocess.run(
        [
            str(binary),
            str(incomplete_theme),
            str(invalid_theme),
            str(invalid_palette_theme),
        ],
        cwd=repo_root,
        check=True,
    )


def test_theme_palette_rejects_wildcard_selectors(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    theme_file = tmp_path / "themes.conf"
    driver = tmp_path / "theme_wildcard_selector_driver.c"
    binary = tmp_path / "theme_wildcard_selector_driver"

    theme_file.write_text(
        """
[theme sample]
background = blue
box_lines = cyan on blue
tree_lines = +white on blue
static_text = white on blue
dynamic_text = +white on blue
keybind = +white on blue
selection = black on +grey
dialog = black on +grey
picker = black on +grey
help = white on blue
info = +white on blue
warning = black on yellow
error = white on red
search_hit = black on yellow
disabled = grey on blue

[file-types sample]
wildcards = red: *.log,?
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

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = UpdateUIColor;
  ctx.hook_add_file_color_rule = AddFileColorRule;

  if (ReadThemeFile(&ctx, argv[1], "sample") == 0) {
    fprintf(stderr, "wildcard selector unexpectedly loaded\n");
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
            "src/cmd/theme.c",
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
    subprocess.run([str(binary), str(theme_file)], cwd=repo_root, check=True)


def test_invalid_user_theme_catalog_blocks_packaged_fallback(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    home = tmp_path / "home"
    preferred_dir = home / ".config" / "ytnova"
    preferred_dir.mkdir(parents=True)
    (preferred_dir / "themes.conf").write_text(
        """
[theme classic-blue]
background = blue
box_lines = cyan on blue
""",
        encoding="utf-8",
    )
    driver = tmp_path / "theme_path_failure_driver.c"
    binary = tmp_path / "theme_path_failure_driver"

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int color_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static char *configured_theme(const ViewContext *ctx, const char *name) {
  (void)ctx;
  if (strcmp(name, "THEME") == 0)
    return "classic-blue";
  return NULL;
}

static void capture_update_color(const char *name, int fg, int bg) {
  (void)name;
  (void)fg;
  (void)bg;
  ++color_count;
}

int main(int argc, char **argv) {
  ViewContext ctx;
  FileColorRule *rule;

  if (argc != 2)
    return 1;
  if (setenv("HOME", argv[1], 1) != 0)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = AddFileColorRule;
  ctx.core_init_ops.get_profile_value = configured_theme;
  AddFileColorRule(&ctx, "*.old", COLOR_GREEN, COLOR_BLACK);

  if (LoadConfiguredTheme(&ctx) == 0) {
    fprintf(stderr, "invalid user theme fell through to packaged catalog\n");
    return 1;
  }
  if (color_count != 0) {
    fprintf(stderr, "invalid user theme changed %d colors\n", color_count);
    return 1;
  }

  rule = (FileColorRule *)ctx.file_color_rules_head;
  if (rule == NULL || strcmp(rule->pattern, "*.old") != 0 ||
      rule->fg != COLOR_GREEN || rule->bg != COLOR_BLACK || rule->next != NULL) {
    fprintf(stderr, "invalid user theme did not preserve previous palette\n");
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
            "src/cmd/theme.c",
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
    subprocess.run([str(binary), str(home)], cwd=repo_root, check=True)


def test_unreadable_user_theme_catalog_blocks_packaged_fallback(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    home = tmp_path / "home"
    preferred_theme = home / ".config" / "ytnova" / "themes.conf"
    preferred_theme.mkdir(parents=True)
    driver = tmp_path / "theme_unreadable_path_driver.c"
    binary = tmp_path / "theme_unreadable_path_driver"

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int color_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static char *configured_theme(const ViewContext *ctx, const char *name) {
  (void)ctx;
  if (strcmp(name, "THEME") == 0)
    return "classic-blue";
  return NULL;
}

static void capture_update_color(const char *name, int fg, int bg) {
  (void)name;
  (void)fg;
  (void)bg;
  ++color_count;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;
  if (setenv("HOME", argv[1], 1) != 0)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = AddFileColorRule;
  ctx.core_init_ops.get_profile_value = configured_theme;

  if (LoadConfiguredTheme(&ctx) == 0) {
    fprintf(stderr, "unreadable user theme catalog fell through to packaged catalog\n");
    return 1;
  }
  if (color_count != 0) {
    fprintf(stderr, "unreadable user theme changed %d colors\n", color_count);
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
            "src/cmd/theme.c",
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
    subprocess.run([str(binary), str(home)], cwd=repo_root, check=True)


def test_valid_user_theme_catalog_missing_theme_allows_packaged_fallback(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    home = tmp_path / "home"
    preferred_dir = home / ".config" / "ytnova"
    preferred_dir.mkdir(parents=True)
    (preferred_dir / "themes.conf").write_text(
        """
[theme spare]
background = black
box_lines = white on black
tree_lines = white on black
margin = dynamic_text
static_text = white on black
dynamic_text = white on black
keybind = +white on black
selection = black on white
dialog = white on black
picker = black on white
help = white on black
info = white on black
warning = black on yellow
error = white on red
search_hit = black on yellow
disabled = grey on black
""",
        encoding="utf-8",
    )
    driver = tmp_path / "theme_missing_fallback_driver.c"
    binary = tmp_path / "theme_missing_fallback_driver"

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int color_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static char *configured_theme(const ViewContext *ctx, const char *name) {
  (void)ctx;
  if (strcmp(name, "THEME") == 0)
    return "classic-blue";
  return NULL;
}

static void capture_update_color(const char *name, int fg, int bg) {
  (void)name;
  (void)fg;
  (void)bg;
  ++color_count;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;
  if (setenv("HOME", argv[1], 1) != 0)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = AddFileColorRule;
  ctx.core_init_ops.get_profile_value = configured_theme;
  AddFileColorRule(&ctx, "*.old", COLOR_GREEN, COLOR_BLACK);

  if (LoadConfiguredTheme(&ctx) != 0) {
    fprintf(stderr, "valid user catalog without requested theme blocked packaged fallback\n");
    return 1;
  }
  if (color_count == 0) {
    fprintf(stderr, "packaged fallback did not apply colors\n");
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
            "src/cmd/theme.c",
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
    subprocess.run([str(binary), str(home)], cwd=repo_root, check=True)


def test_profile_runtime_snapshot_restores_values_and_palette(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    original = tmp_path / "original.conf"
    changed = tmp_path / "changed.conf"
    driver = tmp_path / "profile_snapshot_driver.c"
    binary = tmp_path / "profile_snapshot_driver"

    original.write_text(
        """
[GLOBAL]
THEME=classic-blue
SMALLWINDOWSKIP=1
""",
        encoding="utf-8",
    )
    changed.write_text(
        """
[GLOBAL]
THEME=missing-theme
SMALLWINDOWSKIP=0
""",
        encoding="utf-8",
    )
    driver.write_text(
        r'''
#include "config.h"
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static void free_file_rules(FileColorRule *rule) {
  while (rule != NULL) {
    FileColorRule *next = rule->next;
    free(rule->pattern);
    free(rule);
    rule = next;
  }
}

int main(int argc, char **argv) {
  ViewContext ctx;
  ProfileRuntimeSnapshot *snapshot;
  FileColorRule *rule;

  if (argc != 3)
    return 2;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_add_file_color_rule = AddFileColorRule;
  AddFileColorRule(&ctx, "*.old", COLOR_GREEN, COLOR_BLACK);

  if (ReadProfile(&ctx, argv[1]) != 0) {
    fprintf(stderr, "original profile failed\n");
    return 1;
  }

  snapshot = ProfileRuntimeSnapshot_Create(&ctx);
  if (ReadProfile(&ctx, argv[2]) != 0) {
    fprintf(stderr, "changed profile failed\n");
    return 1;
  }

  if (strcmp(GetProfileValue(&ctx, "THEME"), "missing-theme") != 0) {
    fprintf(stderr, "changed profile was not applied before restore\n");
    return 1;
  }

  ProfileRuntimeSnapshot_Restore(&ctx, snapshot);
  ProfileRuntimeSnapshot_Free(snapshot);

  if (strcmp(GetProfileValue(&ctx, "THEME"), "classic-blue") != 0 ||
      strcmp(GetProfileValue(&ctx, "SMALLWINDOWSKIP"), "1") != 0) {
    fprintf(stderr, "profile values were not restored\n");
    return 1;
  }

  rule = (FileColorRule *)ctx.file_color_rules_head;
  if (rule == NULL || strcmp(rule->pattern, "*.old") != 0 ||
      rule->fg != COLOR_GREEN || rule->bg != COLOR_BLACK ||
      rule->next != NULL) {
    fprintf(stderr, "file palette was not restored\n");
    return 1;
  }

  free_file_rules((FileColorRule *)ctx.file_color_rules_head);
  ctx.file_color_rules_head = NULL;
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
    subprocess.run(
        [str(binary), str(original), str(changed)], cwd=repo_root, check=True
    )
