# MkDocs Material Reference Agent Notes

These files are quick agent-facing notes for Material reference patterns used in this repository.

Source mirror (local temp copy):
- `.tmp/mkdocs-material-reference/`

Official source:
- https://github.com/squidfunk/mkdocs-material/tree/master/docs/reference

## Topics covered
- Admonitions
- Annotations
- Buttons
- Code blocks
- Content tabs
- Data tables
- Diagrams
- Footnotes
- Formatting
- Grids
- Icons, Emojis
- Images
- Lists
- Math
- Tooltips

## How to use these notes
1. Pick the topic file matching the docs pattern you want.
2. Copy the example and adapt it to the target page in `docs/`.
3. Keep examples distro-aware where relevant.
4. Validate output:

```bash
mkdocs serve
mkdocs build --strict
```

## Repo-specific guidance
- Use content tabs for distro-specific install commands.
- Use admonitions for warnings around service setup and sensors.
- Keep command examples copy-paste ready.
- Keep generated `site/` output out of commits.
