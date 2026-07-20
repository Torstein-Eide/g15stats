# Contributing

## Reporting Issues

You can report issues for critical bugs.

### Before Reporting

1. Check if the issue already exists
2. Verify you have all required dependencies installed
3. Try building from the latest source

### Issue Template

When reporting an issue, please include:

- **System Information**:
  - Operating system and version
  - G15 keyboard model
  - libgtop version
  - libg15daemon version

- **Steps to Reproduce**:
  1. Step 1
  2. Step 2
  3. Step 3

- **Expected Behavior**: What should happen

- **Actual Behavior**: What actually happens

- **Error Messages**: Any error messages or logs

## Building from Source

### Clone the Repository

```bash
git clone https://github.com/Torstein-Eide/g15stats.git
cd g15stats
```

### Build

```bash
./configure
make
```

### Run Tests

```bash
make check
```

## Code Style

G15Stats follows the GNU coding standards for C code.

### General Guidelines

- Use standard C library functions
- Follow existing code patterns in the project
- Use 4 spaces for indentation (no tabs)
- Keep lines under 80 characters when possible

### Naming Conventions

- **Functions**: lowercase with underscores (e.g., `g15daemon_version`)
- **Variables**: lowercase with underscores (e.g., `have_temp`)
- **Constants**: uppercase with underscores (e.g., `MAX_SCREENS`)
- **Files**: lowercase with underscores (e.g., `g15stats.c`)

### Comments

- Use C-style `/* */` for multi-line comments
- Use `//` for single-line comments
- Focus on explaining *why* rather than *what*

## Pull Requests

You can submit pull requests for:

- Critical bug fixes
- Security fixes
- Compatibility fixes for newer systems

### PR Guidelines

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit the PR with a clear description

## Contact

For questions or issues, you can reach out via email (see AUTHORS file for contact information).

## License

See the [COPYING](COPYING) file for license information.

## Acknowledgments

Thanks to all contributors who have helped make G15Stats possible over the years.
