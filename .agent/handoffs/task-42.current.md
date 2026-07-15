## Task

- Title: Refine In-App Help Text
- Acceptance target: portable, low-noise footer/help contract with non-redundant alternates, VI-aware runtime key labels, and prompt/help wording that explains special syntax and Ctrl-only tagged/search semantics.
- Completion objective: finish the remaining Task 42 runtime/docs batch that makes current in-app guidance match the portable contract already recorded in `docs/ROADMAP.md`.

## Selected work family

- Family: portable footer/help wording across runtime prompt/help surfaces
- Why this family now: the branch already records the roadmap contract; the next adjacent in-scope surfaces are the live footer/help strings, the prompt help paths for special syntax, and the canonical docs that explain those runtime surfaces.
- PR: https://github.com/robkam/ytreenova/pull/402

## Inventory

| Surface | Status | Notes |
| --- | --- | --- |
| `docs/ROADMAP.md` Task 42 contract | addressed | Recorded on this branch in `docs(roadmap): record portable footer help contract`. |
| `docs/SPECIFICATION.md` help/footer contract | addressed | Replaced stale held-`Ctrl` footer language with the portable prompt/`F1` contract. |
| `src/ui/display.c` built-in footer/help strips | addressed | File-mode footer/help rows now swap VI-sensitive keys to the runtime tokens shown to users. |
| `src/ui/display_utils.c` command-strip renderer | intentionally unchanged | Existing renderer already handled mixed-case key tokens once the strip data was corrected. |
| `src/ui/interactions.c` archive / execute / tagged-search prompt wording | addressed | Added prompt-side `F1` help plus clearer execute/search wording for `{}` and Ctrl-only tagged semantics. |
| `src/ui/input_line.c` prompt help/hints infrastructure | intentionally unchanged | Existing `UI_ReadStringWithHelp()` path was reused as-is. |
| `src/ui/compare_request.c` prompt help model | intentionally unchanged | Existing contextual prompt-help implementation is the reference pattern for Task 42 prompt help. |
| `etc/ytnova.1.md` canonical user docs | addressed | Synced portable footer/help wording, prompt `F1` help, and `{}` / tagged-search semantics. |
| `docs/USAGE.md` generated usage doc | addressed | Regenerated from `etc/ytnova.1.md`. |
| `etc/ytnova.conf` sample config comments | addressed | Added the VI/runtime-footer caveat to the legacy `[MENU]` override notes. |
| `src/core/default_profile_template.h` generated default config template | addressed | Kept the starter profile note aligned with `etc/ytnova.conf`. |
| `tests/test_command_strip_visibility.py` | addressed | Existing focused footer regression file rerun green after the VI-key footer change. |
| focused prompt-help regression test surface | addressed | Added `tests/test_help_text_contract.py` covering VI footer tokens plus archive / execute / tagged-search prompt help. |

## Validation

- Red: `source .venv/bin/activate && pytest -q tests/test_help_text_contract.py`
- Green: `source .venv/bin/activate && pytest -q tests/test_help_text_contract.py`
- Green: `source .venv/bin/activate && pytest -q tests/test_command_strip_visibility.py`
- Green: `source .venv/bin/activate && pytest -q tests/test_security_shell_paths.py tests/test_help_text_contract.py`
- Green after static-analyzer remediation: `source .venv/bin/activate && pytest -q tests/test_security_shell_paths.py tests/test_help_text_contract.py tests/test_command_strip_visibility.py`
- Green: `make clean && make`
