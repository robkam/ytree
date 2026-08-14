# Translators Guide

This guide explains how ytnova's user-facing help and UI text are structured so translators can work on the right source in the right way.

## Current translation surfaces

ytnova currently has two different documentation/help families plus future runtime UI localization work:

1. **Contextual F1 help**
   - Canonical source: `etc/help/f1.en.md`
   - Existing locale example: `etc/help/f1.de.md`
   - Audience: users who press `F1` while doing a task now

2. **Reference help**
   - Canonical source: `etc/help/man.en.md`
   - Existing locale example: `etc/help/man.de.md`
   - Generated outputs: `etc/ytnova.1.md`, `docs/USAGE.md`, build manpage output
   - Audience: users reading the fuller reference/manual

3. **Runtime UI strings**
   - Tracked separately under roadmap Task 61 (`gettext`/`po4a`)
   - Includes footer labels, prompts, status text, and other non-generated runtime strings

The important rule is that these families do different jobs. Do not try to force one translation to sound like the other.

## What F1 help is for

Contextual `F1` help is not a full manual. Its job is:

- answer the question for the active screen or prompt
- stay short and scannable
- point to one deeper explainer when needed
- keep the user oriented inside the help popup

When translating `F1` help, preserve that structure. If a sentence becomes longer in your language, prefer clarity and scannability over literal line-by-line similarity to English.

## The three F1 help page families

F1 topics fall into three families:

1. **Contextual pages**
   - One active runtime surface
   - Examples: directory, file, archive, preview, split, prompt, dialog

2. **Command explainers**
   - One command or concept family
   - Examples: Filter, Compare, Output, Jump, wildcard rename semantics

3. **Shared topics**
   - Cross-cutting behavior reused by many pages
   - Examples: help navigation, tagged workflow, command-line editing, VI keys, theming/customization

Do not collapse these families together in translation. If the source keeps a repeated rule in one shared topic, keep it there instead of rewording it differently on every local page.

## F1 structure rules you must preserve

Each F1 topic block has a strict schema:

- `## topic:<id>`
- metadata fence with `title:` and `contexts:`
- `### Contextual F1`
- optional `### Explainer links`
- `### Long form`

Translators:

- **translate**
  - `title:`
  - text under `### Contextual F1`
  - visible link labels in `### Explainer links`
  - long-form headings and bodies

- **do not translate**
  - `topic:<id>`
  - `contexts:`
  - `topic:` targets inside links such as `(topic:navigation)`
  - schema headings like `### Contextual F1`, `### Explainer links`, `### Long form` unless the generator/schema is explicitly updated to support localization there

The stable IDs and context mappings are runtime keys, not prose.

## Help-popup navigation vs ytnova navigation

These are different and must stay distinct in translation.

### Help-popup navigation
This is navigation inside the help popup itself:

- `Up` / `Down`
- `PgUp` / `PgDn`
- `Home` / `End`
- `Enter` / `Right`
- `Left`
- `Esc` / `Q`

### Runtime ytnova navigation
This is navigation in the actual file manager:

- tree/file movement
- jump/list-jump
- prompt editing
- split movement
- preview movement
- tagged flows

Do not blur those two layers. If the source is talking about help-popup movement, do not rewrite it as ordinary file-manager navigation.

## "User gets an answer" rule

When a user presses `F1`, they must either:

- get the answer on that opening page, or
- get a clearly signposted next hop to the right explainer/shared topic

That means translations should avoid:

- vague filler
- hiding the owning concept
- turning short command summaries into dense editorial prose

If a concept belongs to a shared topic in the source, keep the local page short and let the shared topic do the deeper teaching.

## Shared-topic coverage examples

Shared topics often cover recurring user questions such as:

- how filters work
- how jump/list-jump works
- when wildcards or rename patterns work
- search or fuzzy-matching semantics
- command-line editing
- `VI_KEYS=1`
- theming/customization
- tagged-set workflow

Treat these as reusable operator guidance, not as one-off local trivia.

## Mnemonics, labels, and key tokens

ytnova distinguishes:

- the **command label** shown to the user
- the **key token** bound to that action
- the renderer's **mnemonic emphasis**

Important translator rule:

- do **not** hardcode whole rendered footer/help strings as if they were one inseparable phrase
- do **not** manually add fake punctuation just to force a shortcut shape
- do preserve the natural localized command word

Examples in plain-text docs may use `(K)eyword` notation to describe mnemonic emphasis, but runtime UI does not literally show those parentheses to the user.

## Hint line and low-noise help UI

The help popup has a small hint line at the bottom. It is intentionally lower-noise than the main footer command strip.

Translate it to preserve:

- discoverability of cross-topic movement
- back/close cues
- short visual shape

Do not expand it into a paragraph or a full restatement of the runtime footer.

## Style guidance for F1 translation

Prefer:

- direct voice
- short sentences
- plain operator language
- consistent repeated terms
- scannable bullets

Avoid:

- filler such as "this page explains..."
- unnecessary self-reference
- mixing tutorial tone with manual/reference tone
- inventing extra synonyms for the same repeated command family

## Manual/reference translation

`etc/help/man.en.md` is the fuller reference path. `etc/help/man.de.md` follows the same topic inventory and link structure as an authored localized source. Both should stay more reference-oriented than `F1`.

Do not make the manual sound like popup help, and do not make popup help sound like a Unix manpage.

## If you need to add a new language

Until the full gettext/po4a workflow lands, a new help-language contribution should preserve:

- the exact topic inventory
- stable topic IDs
- stable `contexts:` mappings
- stable intra-help `topic:` link targets
- the same separation between contextual pages, command explainers, and shared topics
- for manual/reference locales, the same authored structure as `etc/help/man.en.md` instead of translating from generated `docs/USAGE.md` or `etc/ytnova.1.md`

If a translation seems to require changing topic IDs, context IDs, or generator schema, that is not a normal translation change; raise it as a maintainer/design issue.

## Canonical references

- `docs/SPECIFICATION.md` — help structure and runtime contract
- `docs/ROADMAP.md` — Task 43 / Task 61 planning
- `etc/help/f1.en.md` — contextual F1 source
- `etc/help/man.en.md` — reference/man source
- `etc/help/man.de.md` — localized authored reference/man source example
- `scripts/generate_help_assets.py` — generator and schema enforcement
