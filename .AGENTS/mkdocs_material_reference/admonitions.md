# Admonitions

Reference:
- https://squidfunk.github.io/mkdocs-material/reference/admonitions/
- Local: `.tmp/mkdocs-material-reference/admonitions.md`

## When to use in this repo
- Add safety notes in install/service steps.
- Highlight distro-specific caveats.
- Warn about discontinued/project-status expectations.

## Example for G15Stats docs
```markdown
!!! warning
    Run `mkdocs build --strict` before committing docs changes.

!!! note
    G15Stats may require manual hwmon sensor selection with `-t`/`-f`.
```
