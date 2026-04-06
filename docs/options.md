# Command-Line Options

G15Stats supports various command-line options to customize its behavior.

## Options Reference

| Option | Description |
|--------|-------------|
| `-i <id>` | Gather statistics from named interface (e.g., `-i eth0`) |
| `-d` | Run in background (daemonise) |
| `-nsa` | Scale network graphs against highest speed recorded. The default is to scale against the highest peak in the current graph |
| `-h` | Show help message |
| `-r <seconds>` | Set the refresh interval to seconds. The seconds must be between 1 and 300 (e.g., `-r 20`) |
| `-u` | Display unicore graphs only on the CPU screen |
| `-t <id>` | Force to monitor temperature sensor id on start (e.g., `-t 1`). The id should point to sysfs path `/sys/class/hwmon/hwmon<id>/device/temp1_input`. Default the sensor id is auto-detected |
| `-f <id>` | Force to monitor fan speed sensor id on start (e.g., `-f 1`). The id should point to sysfs path `/sys/class/hwmon/hwmon<id>/device/fan1_input`. Default the sensor id is auto-detected |
| `-df` | Disable monitoring CPUs frequencies |
| `-ir` | Enable the bottom info bar content rotate cycle over all available sensors |
| `-vc` | The CPU cores will be calculated every time (for systems with CPU hotplug) |
| `-gt <id>` | Show temperature `<id>` in place of the maximal one on the summary screen. The id should point to sysfs path `/sys/class/hwmon/hwmon<id>/device/temp<id>_input` |

## Detailed Options

### Network Interface Selection (`-i`)

Specify which network interface to monitor:

```bash
g15stats -i eth0
g15stats -i wlan0
```

### Daemon Mode (`-d`)

Run G15Stats as a background daemon:

```bash
g15stats -d
```

### Network Graph Scaling (`-nsa`)

Scale network graphs against the highest speed recorded rather than the highest peak in the current graph:

```bash
g15stats -nsa
```

### Refresh Interval (`-r`)

Set a custom refresh interval between 1 and 300 seconds:

```bash
# Refresh every 15 seconds
g15stats -r 15

# Refresh every 60 seconds
g15stats -r 60
```

### Unicore Display (`-u`)

Display unicore graphs only on the CPU screen, useful for systems with multiple CPU cores:

```bash
g15stats -u
```

### Temperature Sensor Selection (`-t`)

Force monitoring of a specific temperature sensor:

```bash
g15stats -t 1
```

The sensor ID should point to the sysfs path:
```
/sys/class/hwmon/hwmon<id>/device/temp1_input
```

### Fan Speed Sensor Selection (`-f`)

Force monitoring of a specific fan speed sensor:

```bash
g15stats -f 1
```

The sensor ID should point to the sysfs path:
```
/sys/class/hwmon/hwmon<id>/device/fan1_input
```

### Disable Frequency Monitoring (`-df`)

Disable monitoring of CPU frequencies:

```bash
g15stats -df
```

### Info Bar Rotation (`-ir`)

Enable rotation of the bottom info bar content over all available sensors:

```bash
g15stats -ir
```

### CPU Hotplug Support (`-vc`)

Recalculate CPU cores every time, for systems with CPU hotplug support:

```bash
g15stats -vc
```

### Custom Temperature Display (`-gt`)

Show a specific temperature sensor on the summary screen:

```bash
g15stats -gt 1
```

The ID should point to the sysfs path:
```
/sys/class/hwmon/hwmon<id>/device/temp<id>_input
```

## Help

To display the help message with all available options:

```bash
g15stats -h
```
