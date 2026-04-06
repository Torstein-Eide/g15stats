# MkDocs Material Reference Cheatsheet

Use this as a fast map to high-value reference pages when editing docs.

## Core reference index
https://squidfunk.github.io/mkdocs-material/reference/

## Frequently used patterns
- Admonitions:
  https://squidfunk.github.io/mkdocs-material/reference/admonitions/
- Code blocks:
  https://squidfunk.github.io/mkdocs-material/reference/code-blocks/
- Content tabs:
  https://squidfunk.github.io/mkdocs-material/reference/content-tabs/
- Data tables:
  https://squidfunk.github.io/mkdocs-material/reference/data-tables/
- Icons and emojis:
  https://squidfunk.github.io/mkdocs-material/reference/icons-emojis/

## Page metadata/front matter
The reference index documents useful page-level metadata:
- `title`
- `description`
- `icon`
- `status`
- `subtitle`
- `template`

Use YAML front matter at the top of a Markdown file:

```yaml
---
title: Example page title
description: One-sentence summary for search/social metadata
---
```

## Notes for contributors
- Prefer simple, readable Markdown first.
- Use tabs or advanced components only when they clearly improve usability.
- Keep examples copy-paste ready.
- Validate rendering with `mkdocs serve` and run `mkdocs build --strict`.
