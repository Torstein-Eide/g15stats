# Code Blocks

Reference:
- https://squidfunk.github.io/mkdocs-material/reference/code-blocks/
- Local: `.tmp/mkdocs-material-reference/code-blocks.md`

## When to use in this repo
- All install/build commands.
- systemd unit examples.
- sensor discovery and troubleshooting commands.

## Example for G15Stats docs
```markdown
```bash
./configure
make
sudo make install
```

```ini
[Service]
ExecStart=/usr/bin/g15stats -d -i eth0 -r 15
Restart=on-failure
```
```
