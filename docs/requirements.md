# Requirements

## System Requirements

### Operating System

G15Stats is designed for Linux systems and has been tested on:

- Debian/Ubuntu
- Fedora/RHEL
- Arch Linux

### Hardware

- **G15 Keyboard**: Logitech G15 keyboard with LCD display
- **CPU**: Any modern x86/x86_64 processor
- **Memory**: Minimal memory requirements
- **Storage**: Minimal disk space for installation

## Software Dependencies

### Required Libraries

The following development packages must be installed before compilation:

| Library | Purpose | Debian/Ubuntu | Fedora/RHEL | Arch Linux |
|---------|---------|---------------|-------------|------------|
| **libgtop** | System statistics | `libgtop2-dev` | `libgtop2-devel` | `libgtop` |
| **libg15daemon_client** | G15 daemon communication | `libg15daemon-client-dev` | `libg15daemon-client-devel` | `libg15daemon` |
| **libg15render** | LCD rendering | `libg15render-dev` | `libg15render-devel` | `libg15render` |
| **libyaml** | YAML config parsing (`/etc/g15plugins/g15stats.yaml`) | `libyaml-dev` | `libyaml-devel` | `libyaml` |

### Installing Dependencies

#### Debian/Ubuntu

```bash
sudo apt-get update
sudo apt-get install libgtop2-dev libg15daemon-client-dev libg15render-dev libyaml-dev
```

#### Fedora/RHEL

```bash
sudo dnf install libgtop2-devel libg15daemon-client-devel libg15render-devel libyaml-devel
```

#### Arch Linux

```bash
sudo pacman -S libgtop libg15daemon libg15render libyaml
```

## Build Tools

The following build tools are required:

- **GCC** or compatible C compiler
- **GNU Make**
- **Autoconf** (for running `./configure`)
- **Automake** (for running `./configure`)

### Installing Build Tools

#### Debian/Ubuntu

```bash
sudo apt-get install build-essential autoconf automake
```

#### Fedora/RHEL

```bash
sudo dnf install gcc make autoconf automake
```

#### Arch Linux

```bash
sudo pacman -S base-devel
```

## Optional Features

### Temperature Monitoring

For temperature monitoring to work, your system must have:

- Hardware temperature sensors
- `hwmon` kernel module loaded
- Sensors accessible via `/sys/class/hwmon/`

### Fan Speed Monitoring

For fan speed monitoring to work, your system must have:

- Hardware fan speed sensors
- `hwmon` kernel module loaded
- Sensors accessible via `/sys/class/hwmon/`

### Battery Monitoring

For battery monitoring to work, your system must have:

- Battery hardware (laptop or UPS)
- Battery information available via `/sys/class/power_supply/`

### Network Monitoring

For network monitoring to work:

- Network interfaces must be available
- libgtop must have permission to read network statistics

## Troubleshooting Dependencies

### Missing libgtop

If you encounter errors about missing libgtop:

```bash
# Check if libgtop is installed
pkg-config --modversion libgtop-2.0

# Install if missing (Debian/Ubuntu)
sudo apt-get install libgtop2-dev
```

### Missing G15 Libraries

If you encounter errors about missing G15 libraries:

```bash
# Check if G15 libraries are installed
pkg-config --modversion g15daemon_client
pkg-config --modversion g15render

# Install if missing (Debian/Ubuntu)
sudo apt-get install libg15daemon-client-dev libg15render-dev
```

### Permission Issues

If G15Stats cannot access hardware sensors:

```bash
# Check hwmon permissions
ls -la /sys/class/hwmon/

# You may need to run g15stats with sudo or add your user to the appropriate group
```
