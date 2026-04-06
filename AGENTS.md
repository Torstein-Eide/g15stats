```markdown
# AGENTS.md

This file contains information for agentic coding agents operating in this repository.

## Project Overview
This is a C-based statistics monitoring application for G15 LCD displays that shows CPU, Memory, Swap, Network, Temperature, Fan Speed, and Battery usage. It's built using autotools and requires libgtop development packages.

## Build Commands
```bash
# Configure and build
./configure
make

# Clean build
make clean
make distclean

# Install
make install
```

## Lint Commands
This project does not have a specific linting tool configured.

## Test Commands
The project uses `make check` to run tests.

## Code Style Guidelines

### General
- Written in C language
- Follows GNU coding standards
- Uses standard C library functions
- Uses G15 Daemon libraries for LCD display handling

### Naming Conventions
- Functions: lowercase with underscores (e.g., `g15daemon_version`)
- Variables: lowercase with underscores (e.g., `have_temp`)
- Constants: uppercase with underscores (e.g., `MAX_SCREENS`)
- File names: lowercase with underscores (e.g., `g15stats.c`)

### Imports
- Includes standard C headers first
- Then system/library headers
- Finally project-specific headers
- Uses `#include <config.h>` for configuration

### Formatting
- Indentation: 4 spaces
- No tab characters
- Function definitions and declarations should be properly spaced
- Comments use C-style `/* */` for multi-line and `//` for single-line
- Braces for control structures on same line
- Function parameters formatted with one parameter per line when long

### Error Handling
- Error checking on library calls
- Graceful degradation when features are not available
- Proper resource cleanup on exit

### Type Usage
- Uses standard C types (`int`, `char`, `float`, `double`)
- Uses `_Bool` for boolean values
- Uses `unsigned int` for counts and sizes
- Uses `const` for non-modifiable parameters and return values
- Uses `extern` for global variables

### Global Variables
- Global variables are used directly without wrapper functions
- Variables are prefixed with `g15` to distinguish them
- Properly declare global variables with `extern` in headers

### Memory Management
- Relies on system malloc/free
- Proper cleanup of resources on exit
- No smart pointers or automatic memory management

## File Structure
The main source files are:
- `g15stats.c` - Main application
- `g15stats.h` - Header file with definitions and constants
- `Makefile` - Build system using autotools
- `configure.ac` - Autoconf configuration script

## Configuration
The project uses autotools (Autoconf/Automake). It requires:
- libg15daemon_client development package
- libg15render development package  
- libgtop-2.0 development package

## Code Structure
The application is structured as a single monolithic C program with:
- Main loop managing screens and display updates
- Screen-specific drawing functions
- Thread-safe operations for data access
- Configuration handling through command-line options

## Documentation
No inline documentation comments found. The code relies on the README for usage information.

### How to Run a Single Test
1. Build the project using `make`.
2. Navigate to the test directory (if present).
3. Run the specific test using the appropriate command, typically something like `./test_name`.

### Code Style and Naming Conventions
- Follow the guidelines provided in the README.
- Use consistent naming conventions for functions, variables, and constants.

### Error Handling and Logging Conventions
- Check errors after library calls and handle them gracefully.
- Log errors using a consistent format and level (e.g., `g15_log_error("Error message")`).

### Safe Edit Boundaries and Risky Areas
- Avoid editing files in the `.git` directory.
- Be cautious when modifying global variables or critical sections of code.

### Validation Steps Before Commit/PR
1. Ensure all tests pass using `make check`.
2. Build the project using `make` to verify no compilation errors.
3. Review code changes for adherence to style guidelines and error handling.

### Cursor/Copilot Instructions, if present
- Refer to `.github/copilot-instructions.md` for any specific guidance provided by Copilot.

---

### .AGENTS/build.md

This file contains information on how to build the project using Autotools.

## Build Steps
1. **Configure**:
   ```bash
   ./configure
   ```
2. **Build**:
   ```bash
   make
   ```
3. **Clean**:
   ```bash
   make clean
   ```
4. **Distclean**:
   ```bash
   make distclean
   ```
5. **Install** (if needed):
   ```bash
   make install
   ```

## CI Workflow
The project uses GitHub Actions for continuous integration. The `.github/workflows/ci.yml` file defines the build and test steps.

---

### .AGENTS/tests.md

This file contains information on how to run tests in the project.

## Running Tests
1. Ensure you have built the project using `make`.
2. Navigate to the test directory (if present).
3. Run the specific test using the appropriate command, typically something like `./test_name`.

---

### .AGENTS/style.md

This file contains information on coding style guidelines and conventions.

## General Style
- Follow the GNU coding standards.
- Use standard C library functions.
- Ensure consistent indentation and spacing.

## Naming Conventions
- Functions: lowercase with underscores (`g15daemon_version`)
- Variables: lowercase with underscores (`have_temp`)
- Constants: uppercase with underscores (`MAX_SCREENS`)

## Imports
- Include standard C headers first.
- Include system/library headers next.
- Include project-specific headers last.

## Formatting
- Use 4 spaces for indentation.
- Avoid using tab characters.
- Keep function parameters on separate lines when long.

---

### .AGENTS/architecture.md

This file contains information on the architecture of the project.

## Main Components
The application is structured as a single monolithic C program with:
- Main loop managing screens and display updates.
- Screen-specific drawing functions.
- Thread-safe operations for data access.
- Configuration handling through command-line options.

---

### .AGENTS/notes.md

This file contains notes and additional information for agents.

## Project-Specific Pitfalls
- Avoid editing files in the `.git` directory.
- Be cautious when modifying global variables or critical sections of code.
- Ensure all tests pass before committing or creating a pull request.