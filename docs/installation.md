# Installation

## Prerequisites

Before building G15Stats, ensure you have the following dependencies installed:

- **make** and **gcc** (or another C compiler) build tools
- **autoconf**, **automake**, and **libtool** (only needed when building from
  a git checkout — see [Step 1](#step-1-generate-the-configure-script))
- **libgtop** development packages
- **libg15daemon_client** development package
- **libg15render** development package
- **libyaml** development package (YAML config parsing)

### Installing Dependencies

=== "Debian/Ubuntu"

    ```bash
    sudo apt-get update
    sudo apt-get install make gcc autoconf automake libtool libgtop2-dev libg15daemon-client-dev libg15render-dev libyaml-dev
    ```

=== "Fedora/RHEL"

    ```bash
    sudo dnf install make gcc autoconf automake libtool libgtop2-devel libg15daemon-client-devel libg15render-devel libyaml-devel
    ```

=== "Arch Linux"

    ```bash
    sudo pacman -S make gcc autoconf automake libtool libgtop libg15daemon libg15render libyaml
    ```

## Build from Source

### Step 1: Generate the configure script

`configure` and the other autotools-generated files are not checked into
git. If you're building from a git checkout (not a release tarball that
already bundles them), generate them first:

```bash
autoreconf -fi
```

### Step 2: Configure

Navigate to the project directory and run the configure script:

```bash
./configure
```

### Step 3: Build

Compile the source code:

```bash
make
```

### Step 4: Install

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

## Sensor setup (temperature and fan)

G15Stats auto-detects sensors by default. Start with plain auto-detection:

```bash
g15stats
```

If the detected values are wrong or missing, force sensor IDs.

### Find available hwmon IDs

```bash
ls -la /sys/class/hwmon/
```

Each `hwmonX` is a candidate sensor source.

### Force temperature sensor source

```bash
g15stats -t 1
```

This maps to:

```text
/sys/class/hwmon/hwmon<id>/device/temp1_input
```

### Force fan sensor source

```bash
g15stats -f 1
```

This maps to:

```text
/sys/class/hwmon/hwmon<id>/device/fan1_input
```

### Show a specific temperature on summary screen

```bash
g15stats -gt 1
```

This maps to:

```text
/sys/class/hwmon/hwmon<id>/device/temp<id>_input
```

## Run as a systemd service

Running from a source checkout without building the `.deb`? Install the
bundled unit as-is with:

```bash
./contrib/init/install-user-service.sh
```

To customize options (e.g. a specific network interface), create your own
`~/.config/systemd/user/g15stats.service` instead:

!!! warning
    Replace `eth0` in `ExecStart` with your real interface name
    (for example `enp3s0` or `wlan0`).

```ini
[Unit]
Description=G15Stats LCD monitor
After=network.target

[Service]
Type=simple
ExecStart=/usr/bin/g15stats -d -i eth0 -r 15
Restart=on-failure
RestartSec=3

[Install]
WantedBy=graphical-session.target
```

Reload and start:

```bash
systemctl --user daemon-reload
systemctl --user enable --now g15stats.service
```

Check status and logs:

```bash
systemctl --user status g15stats.service
journalctl --user -u g15stats.service -b
```

## Verification checklist

- `g15stats -h` prints help without errors.
- CPU/memory/network pages update on the LCD.
- Temperature and fan pages show values.
- `systemctl --user status g15stats.service` is `active (running)`.

## Config file (optional)

Config search order is:

1. `$G15STATS_CONFIG_FILE` (if set)
2. `~/.config/g15stats/g15stats.yaml`
3. `/etc/g15plugins/g15stats.yaml`

On first run, if both user and system config files are missing, G15Stats
creates `~/.config/g15stats/g15stats.yaml` automatically.

You can define defaults in any of those config files, for example:

```yaml
daemon: true
interface: eth0
refresh: 15
temperature: 1
fan: 1
global_temp: 1
net_scale_absolute: false
info_rotate: true
variable_cpu: false
disable_freq: false
unicore: false
output_file: ""
```

Values passed on the command line override values from this config file.

Set `output_file` to a path (or use `-o <path>`) to write raw LCD frames to a
file instead of sending them to `g15daemon`.

## Troubleshooting

- **No sensor values:** try explicit `-t`/`-f` IDs and inspect `/sys/class/hwmon`.
- **Wrong interface traffic:** set `-i <interface>` (for example `eth0`, `enp3s0`,
  `wlan0`).
- **Service not starting:** check `journalctl -u g15stats.service -b` for missing
  libraries or path issues.
- **No LCD output:** confirm `g15daemon` is running and your user/system can access
  the device.
