# Tooltips

Reference:
- https://squidfunk.github.io/mkdocs-material/reference/tooltips/
- Local: `.tmp/mkdocs-material-reference/tooltips.md`

## When to use in this repo
- Clarify uncommon terms (hwmon, daemonize, hotplug) inline.

## Example for G15Stats docs
```markdown
Use the `-vc` option for CPU hotplug support.[^1]

[^1]: Re-evaluates CPU core count each refresh cycle.
```

Rule:
- Keep critical instructions in main text, not only tooltip/footnote context.
