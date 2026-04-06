# Dependencies

This document describes the dependencies required to build and run g15stats.

## Build Dependencies

### Required Libraries

The following development packages must be installed before compiling:

1. **libg15daemon_client** - G15 Daemon client library
   - Header: `g15daemon_client.h`
   - Library: `libg15daemon_client`
   - Purpose: Provides interface to G15 LCD daemon for display control

2. **libg15render** - G15 Render library
   - Header: `libg15render.h`
   - Library: `libg15render`
   - Purpose: Provides rendering functions for G15 LCD display

3. **libgtop-2.0** - GNOME Top library
   - Package: `libgtop-2.0`
   - Purpose: Provides system statistics (CPU, memory, network, swap, etc.)
   - Also requires:
     - `glib-2.0` - GLib utility library

### Build Tools

The following tools are required to build from source:

- **GCC** or compatible C compiler (`cc`)
- **GNU Make**
- **Autoconf** (for running `./configure` or `./autogen.sh`)
- **Automake**
- **Libtool** (`ltmain.sh`, `libtool`)

## Runtime Dependencies

The following libraries must be present on the system to run g15stats:

- `libg15daemon_client`
- `libg15render`
- `libgtop-2.0`
- `libglib-2.0`

## Installation by Distribution

### Debian/Ubuntu

```bash
sudo apt-get install libg15daemon-client-dev libg15render-dev libgtop2-dev
```

### Fedora/RHEL

```bash
sudo dnf install libg15daemon-client-devel libg15render-devel libgtop2-devel
```

### Arch Linux

```bash
sudo pacman -S libg15daemon libg15render libgtop
```

### openSUSE

```bash
sudo zypper install libg15daemon-client-devel libg15render-devel libgtop-devel
```

## Optional Dependencies

None currently. All features are built into the main binary.

## Verification

After installation, you can verify the dependencies are available by running:

```bash
pkg-config --exists libgtop-2.0 && echo "libgtop-2.0: OK" || echo "libgtop-2.0: MISSING"
pkg-config --exists g15daemon_client && echo "g15daemon_client: OK" || echo "g15daemon_client: MISSING"
pkg-config --exists libg15render && echo "libg15render: OK" || echo "libg15render: MISSING"
```

## Build Process

Once dependencies are installed:

```bash
./configure
make
sudo make install
```

The `configure` script will check for all required dependencies and fail with an error message if any are missing.

## Notes

- The G15 LCD display hardware is required to use this application
- G15Daemon must be running on the system for g15stats to function
- Some features (temperature, fan speed) require appropriate hardware sensors and kernel modules
