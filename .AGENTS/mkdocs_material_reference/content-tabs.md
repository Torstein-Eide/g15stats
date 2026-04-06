# Content Tabs

Reference:
- https://squidfunk.github.io/mkdocs-material/reference/content-tabs/
- Local: `.tmp/mkdocs-material-reference/content-tabs.md`

## When to use in this repo
- Distro package install differences.
- Alternative workflows (`uv`, `venv`, `docker`).

## Example for G15Stats docs
```markdown
=== "Debian/Ubuntu"
    ```bash
    sudo apt-get install libgtop2-dev libg15daemon-client-dev libg15render-dev
    ```

=== "Fedora"
    ```bash
    sudo dnf install libgtop2-devel libg15daemon-client-devel libg15render-devel
    ```
```
