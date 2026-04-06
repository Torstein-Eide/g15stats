# Installation

## Prerequisites

Before building G15Stats, ensure you have the following dependencies installed:

- **libgtop** development packages
- **libg15daemon_client** development package
- **libg15render** development package

### Installing Dependencies

#### Debian/Ubuntu

```bash
sudo apt-get install libgtop2-dev libg15daemon-client-dev libg15render-dev
```

#### Fedora/RHEL

```bash
sudo dnf install libgtop2-devel libg15daemon-client-devel libg15render-devel
```

#### Arch Linux

```bash
sudo pacman -S libgtop libg15daemon libg15render
```

## Build from Source

### Step 1: Configure

Navigate to the project directory and run the configure script:

```bash
./configure
```

### Step 2: Build

Compile the source code:

```bash
make
```

### Step 3: Install

Install the application (requires root privileges):

```bash
sudo make install
```

## Clean Build

If you need to perform a clean build:

```bash
make clean
make distclean
./configure
make
```

## Uninstall

To remove the installed application:

```bash
sudo make uninstall
```

## Verification

After installation, you can verify that G15Stats is installed correctly by running:

```bash
g15stats -h
```

This should display the help message with available command-line options.
