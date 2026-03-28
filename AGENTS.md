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
```bash
# Standard C linting (if available)
# No specific linting found in project
```

## Test Commands
```bash
# No explicit test commands found in project
# Tests may be run by:
make check
```

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