/***************************************************************************
 *
 * src/ui/ui_edit_config.c
 * Shared UI helper: open the user configuration profile in the editor.
 *
 * Ownership invariant: this helper is called from any controller that
 * handles ACTION_EDIT_CONFIG; it must not acquire any controller-private
 * state.
 *
 ***************************************************************************/

#define NO_YTREE_MACROS
#include "ytree_cmd.h"
#include "ytree_ui.h"
#include <stdio.h>
#include <stdlib.h>

void UI_OpenConfigProfile(ViewContext *ctx, DirEntry *dir_entry) {
  char profile_path[PATH_LENGTH + 1];
  const char *home;

  profile_path[0] = '\0';
  home = getenv("HOME");
  if (home && *home) {
    int written;
    written = snprintf(profile_path, sizeof(profile_path), "%s/%s", home,
                       PROFILE_FILENAME);
    if (written < 0 || written >= (int)sizeof(profile_path))
      profile_path[0] = '\0';
  }
  if (!profile_path[0])
    (void)snprintf(profile_path, sizeof(profile_path), "%s", PROFILE_FILENAME);

  if (Edit(ctx, dir_entry, profile_path) != 0) {
    MESSAGE(ctx, "Can't edit \"%s\"", profile_path);
    return;
  }

  if (ctx->core_init_ops.read_profile != NULL) {
    if (ctx->core_init_ops.read_profile(ctx, profile_path) == 0) {
      ctx->bypass_small_window =
          ParseSmallWindowSkipValue(GetProfileValue(ctx, "SMALLWINDOWSKIP"));
      if (ctx->core_init_ops.reinit_color_pairs != NULL) {
        ctx->core_init_ops.reinit_color_pairs(ctx);
      }
    }
  }
}
