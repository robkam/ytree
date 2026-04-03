/***************************************************************************
 *
 * src/ui/print_controller.c
 * Print UI orchestration and prompt flow
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_ui.h"
#include <ctype.h>
#include <string.h>
#include <unistd.h>

static void ClearPrintPrompt(ViewContext *ctx) {
  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);
}

static BOOL HasNonWhitespace(const char *text) {
  if (!text)
    return FALSE;
  while (*text) {
    if (!isspace((unsigned char)*text))
      return TRUE;
    text++;
  }
  return FALSE;
}

void UI_HandlePrintController(ViewContext *ctx, DirEntry *dir_entry,
                              BOOL tagged) {
  PrintConfig config;
  int term;
  int is_pipe = TRUE;
  char error_target[PATH_LENGTH + 1];
  PrintWriteStatus status;

  memset(&config, 0, sizeof(config));
  config.format = PRINT_FORMAT_RAW;
  error_target[0] = '\0';

  if (tagged && dir_entry->tagged_files == 0) {
    UI_Beep(ctx, FALSE);
    return;
  }

  ClearHelp(ctx);

  term = InputChoice(
      ctx, "Format: (R)aw, (F)ramed, (P)age break  (Esc) cancel  ", "RFP\033");
  if (term == ESC || term == '\033') {
    ClearPrintPrompt(ctx);
    return;
  }

  if (term == 'R')
    config.format = PRINT_FORMAT_RAW;
  else if (term == 'F')
    config.format = PRINT_FORMAT_FRAMED;
  else
    config.format = PRINT_FORMAT_PAGEBREAK;

  if (config.format == PRINT_FORMAT_FRAMED ||
      config.format == PRINT_FORMAT_PAGEBREAK) {
    char frame_sep[32] = "";
    if (UI_ReadString(ctx, ctx->active,
                      "Frame separator (default: ```): ", frame_sep,
                      sizeof(frame_sep) - 1, HST_PRINT_FRAME) == ESC) {
      ClearPrintPrompt(ctx);
      return;
    }
    if (frame_sep[0] == '\0') {
      snprintf(frame_sep, sizeof(frame_sep), "```");
    }
    snprintf(config.frame_separator, sizeof(config.frame_separator), "%s",
             frame_sep);
  }

  {
    char prompt[COMMAND_LINE_LENGTH + 1];
    char cwd[PATH_LENGTH + 1];
    if (getcwd(cwd, sizeof(cwd))) {
      snprintf(prompt, sizeof(prompt), "Write to (CWD: %.200s): ", cwd);
    } else {
      snprintf(prompt, sizeof(prompt), "Write to (command or >file): ");
    }

    if (UI_ReadString(ctx, ctx->active, prompt, config.print_to, PATH_LENGTH,
                      HST_FILE) != CR) {
      ClearPrintPrompt(ctx);
      return;
    }
  }

  if (!HasNonWhitespace(config.print_to)) {
    UI_Message(ctx, "No destination specified");
    ClearPrintPrompt(ctx);
    return;
  }

  endwin();
  SuspendClock(ctx);

  status = Cmd_WritePrintOutput(ctx, dir_entry, tagged, &config, &is_pipe,
                                error_target);
  if (status == PRINT_WRITE_OK && is_pipe) {
    HitReturnToContinue();
  }

  InitClock(ctx);
  touchwin(stdscr);
  wnoutrefresh(stdscr);
  doupdate();

  if (status == PRINT_WRITE_OPEN_FAILED) {
    if (is_pipe) {
      UI_Message(ctx, "execution of command*%s*failed", error_target);
    } else {
      UI_Message(ctx, "Failed to open file*%s*", error_target);
    }
  } else if (status == PRINT_WRITE_NO_DESTINATION) {
    UI_Message(ctx, "No destination specified");
  }

  ClearPrintPrompt(ctx);
}

void UI_HandlePrint(ViewContext *ctx, DirEntry *dir_entry, BOOL tagged) {
  UI_HandlePrintController(ctx, dir_entry, tagged);
}
