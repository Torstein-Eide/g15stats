# MkDocs Material Guide

This file is a quick reference for maintaining docs in this repository with
MkDocs Material.

## Source references
- Getting started:
  https://squidfunk.github.io/mkdocs-material/getting-started/
- Reference index:
  https://squidfunk.github.io/mkdocs-material/reference/

## Recommended local workflow
1. Install docs dependencies from the project root:

   ```bash
   pip install -r docs/requirements.txt
   ```

2. Run local preview:

   ```bash
   mkdocs serve
   ```

3. Build docs and fail on warnings before commit:

   ```bash
   mkdocs build --strict
   ```

## Docker workflow (no local Python setup)

```bash
docker pull squidfunk/mkdocs-material
docker run --rm -p 127.0.0.1:8000:8000 -v "$(pwd):/docs" squidfunk/mkdocs-material
docker run --rm -v "$(pwd):/docs" squidfunk/mkdocs-material build --strict
```

## Repo conventions for this project
- Keep navigation in `mkdocs.yml` in sync with files in `docs/`.
- Keep generated `site/` output out of commits.
- Prefer Markdown features already enabled in `mkdocs.yml`.
- If a page uses new syntax from Material reference, enable matching extensions.

## Common additions in `mkdocs.yml`
- Navigation entries under `nav`.
- `markdown_extensions` for content syntax (for example tabs, admonitions).
- Theme features under `theme.features`.
- Repository links under `repo_url` and `extra.social`.
