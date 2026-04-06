# Installation Tutorial

This tutorial walks through a practical Linux setup for G15Stats, including
sensor selection and a systemd service.

!!! tip
    If you only need the minimal build steps, use the shorter
    [Installation](installation.md) page.

## 1) Install dependencies

=== "Debian/Ubuntu"

    ```bash
    sudo apt-get update
    sudo apt-get install libgtop2-dev libg15daemon-client-dev libg15render-dev libyaml-dev
    ```

=== "Fedora/RHEL"

    ```bash
    sudo dnf install libgtop2-devel libg15daemon-client-devel libg15render-devel libyaml-devel
    ```

=== "Arch Linux"

    ```bash
    sudo pacman -S libgtop libg15daemon libg15render libyaml
    ```

## 2) Build and install

From the repository root:

```bash
./configure
make
sudo make install
```

Verify installation:

```bash
g15stats -h
```

## 3) Sensor setup (temperature and fan)

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

## 4) Run as a systemd service

Create `/etc/systemd/system/g15stats.service`:

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
WantedBy=multi-user.target
```

Reload and start:

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now g15stats.service
```

Check status and logs:

```bash
systemctl status g15stats.service
journalctl -u g15stats.service -b
```

## 5) Verification checklist

- `g15stats -h` prints help without errors.
- CPU/memory/network pages update on the LCD.
- Temperature and fan pages show values.
- `systemctl status g15stats.service` is `active (running)`.

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

## 6) Troubleshooting

- **No sensor values:** try explicit `-t`/`-f` IDs and inspect `/sys/class/hwmon`.
- **Wrong interface traffic:** set `-i <interface>` (for example `eth0`, `enp3s0`,
  `wlan0`).
- **Service not starting:** check `journalctl -u g15stats.service -b` for missing
  libraries or path issues.
- **No LCD output:** confirm `g15daemon` is running and your user/system can access
  the device.
