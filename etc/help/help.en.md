# YtreeNova canonical help source (English)

This file is the single authored help source for contextual `F1` help and the
long-form help projections that later regenerate the manpage and
`docs/USAGE.md`.

## Topic-block schema

Every topic block in this file follows the same parser-facing contract:

1. The block starts with a level-2 heading in the exact form
   `## topic:<topic-id>`.
2. The heading is followed immediately by a fenced metadata block labelled
   `ytnova-help-meta`.
3. The metadata block contains exactly these keys, in this order:
   * `title:` — plain-text topic title.
   * `contexts:` — comma-separated stable runtime context/prompt IDs, or the
     literal `none` for link-only explainer pages.
4. The block then contains these sections in order:
   * required `### Contextual F1`
   * optional `### Explainer links`
   * required `### Long form`
5. When `### Explainer links` is present, every item uses Markdown link syntax
   with a `topic:` target, for example `- [Navigation](topic:navigation)`.
6. `### Long form` contains one or more level-4 subsections (`#### ...`).
   Their order is preserved as-authored for later projection.

Multiple runtime contexts may reuse one topic block by sharing a
comma-separated `contexts:` line. Reusable explanations stay in their own
shared topics and are linked with `topic:` links instead of being copied into
multiple blocks. Linked help must stay shallow: one or two hops from the
contextual page is the maximum intended depth.

## topic:intro
```ytnova-help-meta
title: Intro
contexts: none
```
### Contextual F1
YtreeNova keeps `F1` short and task-local. Use the contextual page for the
active surface, then follow shared explainer links only when you need more
background.

### Explainer links
- [Navigation](topic:navigation)
- [Directory mode](topic:dir)
- [File mode](topic:file)

### Long form
#### Purpose
This link-only topic introduces the canonical help set and explains why the
help system is split into concise contextual pages plus shared explainers.

#### Projection notes
Long-form outputs may use this topic as the introduction to the generated help
bundle without forcing every runtime `F1` page to repeat the same orientation
text.

## topic:navigation
```ytnova-help-meta
title: Navigation
contexts: none
```
### Contextual F1
Arrow keys, paging keys, `Home`, `End`, and `Enter` keep their usual ownership.
Contextual pages explain only the extra keys or caveats that differ from the
normal navigation baseline.

### Explainer links
- [Directory mode](topic:dir)
- [File mode](topic:file)
- [F7 preview](topic:f7)
- [F8 split](topic:f8)

### Long form
#### Baseline movement
Navigation is shared vocabulary. Context-specific help should assume this
baseline and document only the keys, limits, and ownership changes that are
special to that surface.

#### Projection notes
Generated man/usage output may expand this topic into the common navigation
reference instead of duplicating the same movement text under every mode.

## topic:dir
```ytnova-help-meta
title: Directory Help
contexts: main.dir
```
### Contextual F1
Directory help explains the live directory footer commands, tree/logging
behavior, and any mode-specific caveats that do not fit in the footer strip.

### Explainer links
- [Navigation](topic:navigation)
- [Filter](topic:filter)
- [F8 split](topic:f8)

### Long form
#### Scope
This topic owns the normal filesystem directory surface. It is the canonical
place for directory-mode footer parity and directory-specific caveats.

#### Projection notes
Generated long-form docs may merge this topic with adjacent shared explainers,
but the authored prose remains rooted here.

## topic:file
```ytnova-help-meta
title: File Help
contexts: main.file
```
### Contextual F1
File help explains the live file footer commands, file-view operations, and
file-specific caveats that are not obvious from the command strip alone.

### Explainer links
- [Navigation](topic:navigation)
- [Output](topic:output)
- [F7 preview](topic:f7)

### Long form
#### Scope
This topic owns the normal filesystem file surface and any file-only caveats
that should not be repeated in directory-mode help.

#### Projection notes
Long-form outputs may place this beside directory help while preserving the
shared link targets for repeated explanations.

## topic:archive-dir
```ytnova-help-meta
title: Archive Directory Help
contexts: main.archive-dir
```
### Contextual F1
Archive directory help mirrors the live archive-directory footer, then adds the
archive-specific caveats that differ from normal filesystem directory behavior.

### Explainer links
- [Navigation](topic:navigation)
- [Directory mode](topic:dir)
- [Output](topic:output)

### Long form
#### Scope
This topic owns archive-directory guidance. Shared directory semantics should
be linked instead of copied when archive mode only adds a small caveat set.

#### Projection notes
Generated long-form docs may group archive guidance near normal directory help,
but the authored archive-specific text stays here.

## topic:archive-file
```ytnova-help-meta
title: Archive File Help
contexts: main.archive-file
```
### Contextual F1
Archive file help mirrors the live archive-file footer and documents the
differences between archive file actions and normal filesystem file actions.

### Explainer links
- [Navigation](topic:navigation)
- [File mode](topic:file)
- [Output](topic:output)

### Long form
#### Scope
This topic owns archive-file guidance and keeps archive-only caveats out of the
normal file help page.

#### Projection notes
Long-form outputs may place archive and filesystem file topics together while
still preserving distinct topic IDs and authored ownership.

## topic:filter
```ytnova-help-meta
title: Filter Help
contexts: prompt.filter,prompt.filter-tagged
```
### Contextual F1
Filter help stays prompt-local: it explains accepted filter syntax, the default
`*` behavior, and any scope toggles that belong to the active filter prompt.

### Explainer links
- [Navigation](topic:navigation)
- [Showall](topic:showall)
- [Global](topic:global)

### Long form
#### Syntax ownership
This topic is the canonical authored home for filter syntax, examples, and
scope rules. Updating filter semantics here must be enough to refresh runtime
prompt help and later long-form outputs.

#### Projection notes
Because filter help is one of the strongest drift risks, later generators
should prefer this topic over duplicated prompt literals.

## topic:compare
```ytnova-help-meta
title: Compare Help
contexts: prompt.compare-target,prompt.compare-scope,prompt.compare-basis,prompt.compare-results
```
### Contextual F1
Compare help explains the compare flow currently in progress: target entry,
scope selection, comparison basis, or result tagging. The runtime chooses the
relevant excerpt by context mapping, not by duplicating prose in each prompt.

### Explainer links
- [Navigation](topic:navigation)
- [Directory mode](topic:dir)
- [File mode](topic:file)

### Long form
#### Flow ownership
This topic owns compare guidance for both filesystem and logged-tree flows. A
later generator/runtime mapper may project only the needed subsection for the
active compare step.

#### Projection notes
The authored source stays in one topic even when the runtime ultimately maps
several compare-step contexts into distinct contextual slices.

## topic:output
```ytnova-help-meta
title: Output Help
contexts: prompt.output-format,prompt.output-destination,prompt.output-separator
```
### Contextual F1
Output help explains the active output step: format choice, file destination,
hardcopy command, or separator prompt. Later runtime mapping chooses the
relevant slice without scattering separate authored prose stores.

### Explainer links
- [Navigation](topic:navigation)
- [File mode](topic:file)
- [Archive file](topic:archive-file)

### Long form
#### Flow ownership
This topic is the canonical home for output/export guidance, including format
terms, destination wording, and the distinction between file output and
hardcopy-oriented command entry.

#### Projection notes
Long-form projections may reuse this topic for the dedicated output/reference
section without requiring a separate authored prose file.

## topic:showall
```ytnova-help-meta
title: Showall Help
contexts: main.showall
```
### Contextual F1
Showall help explains the single-volume aggregated file view and the commands
or caveats that differ from ordinary file mode.

### Explainer links
- [Navigation](topic:navigation)
- [File mode](topic:file)
- [Global](topic:global)

### Long form
#### Scope
This topic owns Showall-specific guidance. Shared file-view text should be
linked, not duplicated, when Showall only adds a few scope caveats.

#### Projection notes
Showall and Global may later share generated fragments, but they keep separate
topic IDs so Global-only multi-volume notes remain possible.

## topic:global
```ytnova-help-meta
title: Global Help
contexts: main.global
```
### Contextual F1
Global help explains the multi-volume aggregated file view, including how it
returns to owner directories and how its scope differs from ordinary file mode.

### Explainer links
- [Navigation](topic:navigation)
- [File mode](topic:file)
- [Showall](topic:showall)

### Long form
#### Scope
This topic owns Global-specific guidance and preserves room for multi-volume
behavior that Showall does not need to explain.

#### Projection notes
The schema allows Global to share some generated output with Showall while still
retaining its own authored topic ID and distinct caveats.

## topic:f7
```ytnova-help-meta
title: F7 Preview Help
contexts: overlay.f7-dir,overlay.f7-file
```
### Contextual F1
F7 help explains preview ownership, allowed keys, blocked keys, and how the
preview overlay interacts with the underlying directory or file context.

### Explainer links
- [Navigation](topic:navigation)
- [File mode](topic:file)

### Long form
#### Scope
This topic owns F7 overlay guidance across directory-preview and file-preview
entry paths.

#### Projection notes
Later runtime mapping may choose narrower contextual excerpts, but those slices
still derive from this single authored topic.

## topic:f8
```ytnova-help-meta
title: F8 Split Help
contexts: overlay.f8-dir,overlay.f8-file
```
### Contextual F1
F8 help explains split-view ownership, inactive-panel defaults, and the keys
or caveats that only appear while split mode is active.

### Explainer links
- [Navigation](topic:navigation)
- [Directory mode](topic:dir)
- [File mode](topic:file)

### Long form
#### Scope
This topic owns split-view guidance and keeps split-specific caveats out of the
base directory and file pages.

#### Projection notes
Generated long-form docs may project this beside directory/file help while
preserving one authored split topic as the source of truth.
