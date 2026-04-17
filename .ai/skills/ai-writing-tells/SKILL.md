---
name: ai-writing-tells
description: Scan prose for AI writing tells, report numbered findings by category with line references, wait for user approval before editing, and rescan after approved fixes.
---

# AI Writing Tells

Use this skill when the user asks to scan prose for AI writing tells, chatbot-style wording, generic polish, or minimal "deslop" editing in docs, prompts, comments, commit text, or other human-facing writing.

## Scope

- Audit the specified file, selected text, or the file the user is actively editing.
- Prefer exact line references and short quoted phrases.
- Do not edit until the user explicitly confirms which items to address.

## Workflow

1. Determine the target text.
   - If the user named a file or pasted text, use that.
   - If the user refers to "this file" or "current file", infer the active file only when it is clear from context.
   - If the target is ambiguous, ask one concise question before scanning.
2. Read only the target text.
3. Scan in this order: Vocabulary, Patterns, Formatting.
4. Use judgment. Flag only wording that feels generic, padded, promotional, or mechanically polished in context.
5. Present findings in the required output format below.
6. Ask: `Which items should I fix? (all / specific numbers / none)`
7. If the user approves fixes, edit only the approved items.
8. Re-scan after edits and report the updated findings and score.
9. If the updated score is below 8, ask: `Another round? (yes / no)`
10. Stop when the user says stop or the score reaches 8 or 9. Do not chase a 10 unless the user explicitly wants a voice rewrite.

## What To Scan For

### Vocabulary

Flag any of these when they read like filler rather than precise wording:

- `additionally` (especially at sentence start)
- `align with`
- `boasts` (meaning "has")
- `bolstered`
- `crucial`
- `comprehensive`
- `cutting-edge`
- `delve`
- `emphasizing`
- `enduring`
- `enhance`
- `encompassing`
- `features` (meaning "has")
- `foster` / `fostering`
- `garner`
- `groundbreaking`
- `highlight` (as a verb)
- `innovative`
- `interplay`
- `intricate` / `intricacies`
- `key` (as an adjective)
- `landscape` (abstract noun)
- `maintains` (meaning "has")
- `meticulous` / `meticulously`
- `nuanced`
- `offers` (meaning "has" or "gives")
- `pivotal`
- `profound`
- `renowned`
- `robust`
- `seamless` / `seamlessly`
- `showcase` / `showcasing`
- `tapestry` (abstract noun)
- `testament`
- `transformative`
- `underscore` (as a verb)
- `valuable`
- `vibrant`
- `vital`

Note: AI vocabulary shifts over time. `Delve` peaked earlier; later drafts leaned harder on words like `enhance`, `highlighting`, and `showcasing`. Flag words that feel like filler in context, not words used precisely.

### Patterns

- Significance puffery: `stands as`, `serves as`, `is a testament to`, `plays a vital role`, `underscores its importance`, `reflects broader`, `setting the stage for`, `marking the`, `shaping the`, `evolving landscape`, `indelible mark`, `deeply rooted`
- Superficial `-ing` analysis: sentences that end in dangling present participles adding vague commentary, such as `highlighting its importance`, `ensuring quality`, `reflecting the broader trend`, `contributing to the field`, `fostering innovation`
- Negative parallelisms: `not just X, but also Y`, `not only ... but ...`, `it's not ... it's ...`
- Rule of three: formulaic three-item lists where two would do
- Copula avoidance: `serves as` instead of `is`, `boasts` instead of `has`, `represents` instead of `is`, `stands as` instead of `is`, `features` instead of `has`, `maintains` instead of `has`, `offers` instead of `gives`
- Em dash overuse: too many em dashes where commas, parentheses, or periods would read more naturally
- Elegant variation: cycling through fancy synonyms just to avoid repeating a direct word
- Promotional tone: `nestled in`, `in the heart of`, `diverse array`, `rich heritage`, `natural beauty`, `commitment to`
- Vague attribution: `experts argue`, `observers note`, `industry reports suggest`, `observers cite`
- Despite-challenges formula: `Despite ... faces challenges ...` followed by an optimistic recovery
- Didactic disclaimers: `it's important to note`, `worth noting`, `it's crucial to remember`
- Formulaic headings: `Challenges and Future Prospects`, `Legacy and Impact`, `Significance and Influence`

### Formatting

- Curly quotes or apostrophes in contexts that expect straight quotes
- Markdown syntax leaking into non-Markdown text, such as stray `**bold**` or `[links](url)`
- Excessive boldface used only for emphasis
- Inline-header vertical lists where every item starts with a bold label and colon even though a plain list would be clearer

## Required Output

- Group findings by category.
- Use continuous numbering across categories.
- Include line numbers and a short quote or phrase for each finding.
- Omit empty categories if there are no findings there.
- Always include `Summary` and `Human Writing Score`.

```markdown
## AI Writing Tells Found

### Vocabulary
1. Line X: "delve" - consider a simpler alternative
2. Line X: "fostering" - filler verb

### Patterns
3. Line X: Negative parallelism - "not just X, but Y"
4. Line X: Significance puffery - "serves as a testament"

### Formatting
5. Line X: Curly quotes in a code context

### Summary
X items found across Y categories.

### Human Writing Score: X/10
```

Then ask: `Which items should I fix? (all / specific numbers / none)`

## Scoring Guide

- `10`: Reads like a specific human wrote it and has a clear voice.
- `9`: Clean. No real tells.
- `8`: One or two minor patterns, but nothing most readers would notice.
- `7`: Competent but generic. A trained eye would catch several tells.
- `6`: Obviously polished by AI. Multiple vocabulary or pattern flags.
- `5` or below: Reads like a chatbot wrote it.

Do not inflate scores. A score of 10 is not the goal.

## Fix Rules

- Replace flagged words with simpler, more direct alternatives.
- Cut superficial `-ing` phrases instead of rewriting them into new filler.
- Flatten negative parallelisms into direct statements.
- Break rule-of-three lists if the third item adds nothing.
- Replace `serves as` or `stands as` with `is` where appropriate.
- Change only the approved items. Do not refactor surrounding text.

## Rescan Rules

- After fixing, re-scan the same text and report the updated findings and score.
- If the score is below 8, ask: `Another round? (yes / no)`
- Do not auto-fix another round without confirmation.

Tell list adapted from [Wikipedia: Signs of AI writing](https://en.wikipedia.org/wiki/Wikipedia:Signs_of_AI_writing), licensed under [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/). Modified and extended for use as a code editor skill.

This file is licensed under [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/).
