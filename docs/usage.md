# Usage

## Basic Usage

!!! note
    For a full first-time setup with sensors and systemd, see
    [Installation Tutorial](installation-tutorial.md).

To start G15Stats with default settings:

```bash
g15stats
```

## Running as Daemon

To run G15Stats in the background (daemon mode):

```bash
g15stats -d
```

## Screen Navigation

Once G15Stats is running, you can navigate between screens using the G15 keyboard buttons:

| Button | Action |
|--------|--------|
| **L2** | Previous Screen |
| **L3** | Next Screen |
| **L4** | Alternative Screen (Doesn't work on Swap, Memory and Battery Screen) |
| **L5** | Bottom Info bar mode |

## Screen Types

G15Stats provides multiple screens for monitoring different system metrics:

1. **Summary Screen** - Overview of key system metrics
2. **CPU Screen** - Detailed CPU usage graphs
3. **Frequency Screen** - CPU core frequencies
4. **Memory Screen** - Memory usage visualization
5. **Swap Screen** - Swap space monitoring
6. **Network Screen** - Network traffic statistics
7. **Battery Screen** - Battery charge status
8. **Temperature Screen** - System temperature sensors
9. **Fan Speed Screen** - Fan speed monitoring

## Examples

### Basic monitoring with default refresh rate

```bash
g15stats
```

### Monitor specific network interface

```bash
g15stats -i eth0
```

### Use rotating sensor info bar

```bash
g15stats -ir
```

### Custom refresh interval

```bash
g15stats -r 20
```

### Run as daemon with custom settings

```bash
g15stats -d -i eth0 -r 15
```

### Disable CPU frequency monitoring

```bash
g15stats -df
```

### Force specific temperature sensor

```bash
g15stats -t 1
```

### Force specific fan speed sensor

```bash
g15stats -f 1
```

## Common command sets

| Scenario | Command |
|---|---|
| Default run | `g15stats` |
| Background service-like run | `g15stats -d -i eth0 -r 15` |
| Sensor-focused run | `g15stats -t 1 -f 1 -gt 1` |
| Hotplug-friendly CPU stats | `g15stats -vc` |

## Tips

- The default refresh interval is optimized for most use cases
- Use the `-u` flag for unicore graphs on systems with multiple CPU cores
- The `-ir` flag enables rotation of the bottom info bar content over all available sensors
- For systems with CPU hotplug support, use the `-vc` flag
