from __future__ import annotations

import subprocess
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]


def _compile_driver(tmp_path: Path, name: str, source: str, objects: list[str], *, libs: list[str] | None = None) -> Path:
    driver = tmp_path / f"{name}.c"
    binary = tmp_path / name
    driver.write_text(source, encoding="utf-8")
    command = ["cc", "-D_GNU_SOURCE", "-Iinclude", str(driver), *objects, "-o", str(binary)]
    if libs:
        command.extend(libs)
    subprocess.run(command, cwd=REPO_ROOT, check=True)
    return binary


def test_profile_loader_rejects_invalid_ranges_without_partial_apply(tmp_path: Path) -> None:
    valid_profile = tmp_path / "valid.conf"
    invalid_profile = tmp_path / "invalid.conf"

    valid_profile.write_text(
        """[GLOBAL]
THEME=quiet-blue
AUTO_REFRESH=3
""",
        encoding="utf-8",
    )
    invalid_profile.write_text(
        """[GLOBAL]
THEME=missing-theme
AUTO_REFRESH=8
""",
        encoding="utf-8",
    )

    binary = _compile_driver(
        tmp_path,
        "profile_robustness_driver",
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static char last_message[256];

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  va_list ap;

  (void)ctx;
  va_start(ap, fmt);
  vsnprintf(last_message, sizeof(last_message), fmt, ap);
  va_end(ap);
  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 3)
    return 2;

  memset(&ctx, 0, sizeof(ctx));
  if (ReadProfile(&ctx, argv[1]) != 0) {
    fprintf(stderr, "baseline profile failed\n");
    return 1;
  }

  if (strcmp(GetProfileValue(&ctx, "THEME"), "quiet-blue") != 0 ||
      strcmp(GetProfileValue(&ctx, "AUTO_REFRESH"), "3") != 0) {
    fprintf(stderr, "baseline values not applied\n");
    return 1;
  }

  last_message[0] = '\0';
  if (ValidateProfileFile(&ctx, argv[2]) != 1) {
    fprintf(stderr, "invalid profile was not rejected by validation\n");
    return 1;
  }
      if (strstr(last_message, "AUTO_REFRESH") == NULL ||
          strstr(last_message, "line 3") == NULL) {
    fprintf(stderr, "validation message was not actionable: %s\n", last_message);
    return 1;
  }

  last_message[0] = '\0';
  if (ReadProfile(&ctx, argv[2]) != 1) {
    fprintf(stderr, "invalid profile was not rejected during load\n");
    return 1;
  }
  if (strcmp(GetProfileValue(&ctx, "THEME"), "quiet-blue") != 0 ||
      strcmp(GetProfileValue(&ctx, "AUTO_REFRESH"), "3") != 0) {
    fprintf(stderr, "invalid load partially applied runtime state\n");
    return 1;
  }
  if (strstr(last_message, "AUTO_REFRESH") == NULL) {
    fprintf(stderr, "load message missing key detail: %s\n", last_message);
    return 1;
  }

  FreeProfileRuntimeData(&ctx);
  return 0;
}
''',
        [
            "src/cmd/profile.c",
            "src/ui/color.c",
            "src/util/atomic_file.c",
            "src/util/memory_utils.c",
            "src/util/string_utils.c",
        ],
        libs=["-DCOLOR_SUPPORT", "-lncursesw", "-ltinfo"],
    )

    subprocess.run([str(binary), str(valid_profile), str(invalid_profile)], cwd=REPO_ROOT, check=True)


def test_history_loader_rejects_malformed_lines_without_partial_apply(tmp_path: Path) -> None:
    history_file = tmp_path / "broken.hst"
    history_file.write_text("0:0:keep-me\n1:2:broken-pinned-flag\n", encoding="utf-8")

    binary = _compile_driver(
        tmp_path,
        "history_robustness_driver",
        r'''
#include "ytnova_cmd.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static char last_message[256];

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  va_list ap;

  (void)ctx;
  va_start(ap, fmt);
  vsnprintf(last_message, sizeof(last_message), fmt, ap);
  va_end(ap);
  return 0;
}

static void free_history_list(ViewContext *ctx) {
  History *node = ctx->history_head;

  while (node != NULL) {
    History *next = node->next;
    free(node->hst);
    free(node);
    node = next;
  }
  ctx->history_head = NULL;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 2;

  memset(&ctx, 0, sizeof(ctx));
  InsHistory(&ctx, "existing-entry", HST_GENERAL);

  last_message[0] = '\0';
  if (ReadHistory(&ctx, argv[1]) != 1) {
    fprintf(stderr, "malformed history was not rejected\n");
    free_history_list(&ctx);
    return 1;
  }
  if (ctx.history_head == NULL || strcmp(ctx.history_head->hst, "existing-entry") != 0 ||
      ctx.history_head->next != NULL) {
    fprintf(stderr, "malformed history partially changed runtime state\n");
    free_history_list(&ctx);
    return 1;
  }
  if (strstr(last_message, "line 2") == NULL || strstr(last_message, "pinned") == NULL) {
    fprintf(stderr, "history rejection message was not actionable: %s\n", last_message);
    free_history_list(&ctx);
    return 1;
  }

  free_history_list(&ctx);
  return 0;
}
''',
        [
            "src/util/history_utils.c",
            "src/util/atomic_file.c",
            "src/util/memory_utils.c",
        ],
    )

    subprocess.run([str(binary), str(history_file)], cwd=REPO_ROOT, check=True)


def test_atomic_writer_preserves_previous_file_on_interrupted_replace(tmp_path: Path) -> None:
    target = tmp_path / "config.conf"
    target.write_text("stable-before\n", encoding="utf-8")

    binary = _compile_driver(
        tmp_path,
        "atomic_writer_driver",
        r'''
#include "ytnova_defs.h"
#include <stdio.h>
#include <string.h>

static int failing_writer(FILE *fp, void *user_data) {
  (void)user_data;

  if (fputs("partial-after\n", fp) == EOF)
    return -1;
  return -1;
}

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

int main(int argc, char **argv) {
  char buffer[128];
  FILE *fp;

  if (argc != 2)
    return 2;

  if (AtomicFileWrite(argv[1], failing_writer, NULL) == 0) {
    fprintf(stderr, "atomic write unexpectedly succeeded\n");
    return 1;
  }

  fp = fopen(argv[1], "r");
  if (fp == NULL) {
    fprintf(stderr, "failed to reopen target file\n");
    return 1;
  }
  if (fgets(buffer, sizeof(buffer), fp) == NULL) {
    fclose(fp);
    fprintf(stderr, "failed to read preserved target file\n");
    return 1;
  }
  fclose(fp);

  if (strcmp(buffer, "stable-before\n") != 0) {
    fprintf(stderr, "target file was not preserved: %s\n", buffer);
    return 1;
  }

  return 0;
}
''',
        ["src/util/atomic_file.c", "src/util/memory_utils.c"],
    )

    subprocess.run([str(binary), str(target)], cwd=REPO_ROOT, check=True)


def test_empty_history_save_clears_previous_file_contents(tmp_path: Path) -> None:
    target = tmp_path / "history.hst"
    target.write_text("0:0:stale-entry\n", encoding="utf-8")

    binary = _compile_driver(
        tmp_path,
        "empty_history_save_driver",
        r'''
#include "ytnova_cmd.h"
#include <stdio.h>

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;
  FILE *fp;
  int ch;

  if (argc != 2)
    return 2;

  memset(&ctx, 0, sizeof(ctx));
  if (SaveHistory(&ctx, argv[1]) != 0) {
    fprintf(stderr, "empty history save failed\n");
    return 1;
  }

  fp = fopen(argv[1], "r");
  if (fp == NULL) {
    fprintf(stderr, "history file was not created\n");
    return 1;
  }
  ch = fgetc(fp);
  fclose(fp);

  if (ch != EOF) {
    fprintf(stderr, "history file still contains stale data\n");
    return 1;
  }

  return 0;
}
''',
        [
            "src/util/history_utils.c",
            "src/util/atomic_file.c",
            "src/util/memory_utils.c",
        ],
    )

    subprocess.run([str(binary), str(target)], cwd=REPO_ROOT, check=True)
