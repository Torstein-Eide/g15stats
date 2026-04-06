# Diagrams

Reference:
- https://squidfunk.github.io/mkdocs-material/reference/diagrams/
- Local: `.tmp/mkdocs-material-reference/diagrams.md`

## When to use in this repo
- Show startup flow (configure -> build -> install -> run).
- Show service lifecycle (enable/start/status/logs).

## Example for G15Stats docs
```markdown
```mermaid
flowchart LR
  A[configure] --> B[make]
  B --> C[make install]
  C --> D[g15stats -d]
```
```
