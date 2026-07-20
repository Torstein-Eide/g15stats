# Installation

For a full walkthrough including sensor configuration and a systemd unit, see
the [Installation Tutorial](installation-tutorial.md).

## Prerequisites

Before building G15Stats, ensure you have the following dependencies installed:

- **make** and **gcc** (or another C compiler) build tools
- **libgtop** development packages
- **libg15daemon_client** development package
- **libg15render** development package
- **libyaml** development package (YAML config parsing)

### Installing Dependencies

#### Debian/Ubuntu

```bash
sudo apt-get install make gcc libgtop2-dev libg15daemon-client-dev libg15render-dev libyaml-dev
```

#### Fedora/RHEL

```bash
sudo dnf install make gcc libgtop2-devel libg15daemon-client-devel libg15render-devel libyaml-devel
```

#### Arch Linux

```bash
sudo pacman -S make gcc libgtop libg15daemon libg15render libyaml
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
