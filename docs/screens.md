# Screens

G15Stats provides multiple screens for monitoring different system metrics. Each screen can be accessed using the L2 and L3 buttons on your G15 keyboard.

## Summary Screen

The Summary Screen provides a quick overview of your system's key metrics.

**Displays:**
- 4 or 5 indicators from:
  - CPU usage
  - CPU Frequency
  - Memory usage
  - Network traffic
  - Temperature
  - Fan Speed
  - Swap usage
- Current time

**Navigation:**
- L4: Alternative Screen (if available)

## CPU Screen

The CPU Screen provides detailed CPU usage information with graphical representation.

**Displays:**
- Graph of User time
- Graph of System time
- Graph of Nice time
- Graph of Idle time
- Load Average (LoadAVG)
- System Uptime

**Navigation:**
- L4: Alternative Screen

## Frequency Screen

The Frequency Screen shows CPU frequency information for all cores.

**Displays:**
- Individual CPU core frequencies
- Total/average frequency across all cores

## Memory Screen

The Memory Screen provides memory usage visualization.

**Displays:**
- Total Memory
- Free Memory
- Graph of Used Memory vs Buffered+Cached Memory

**Navigation:**
- L4: Alternative Screen

## Swap Screen

The Swap Screen monitors swap space usage.

**Displays:**
- Used swap space
- Free swap space
- Total swap space
- Number of pages currently paged in
- Number of pages currently paged out

**Navigation:**
- L4: Alternative Screen

## Network Screen

The Network Screen displays network traffic statistics.

**Displays:**
- Total bytes In
- Total bytes Out
- History graph of network activity
- Peak speed

**Navigation:**
- L4: Alternative Screen

## Battery Status Screen

The Battery Status Screen shows battery information for systems with batteries.

**Displays:**
- Battery charge data for up to three batteries
- Charge level
- Charging status

**Navigation:**
- L4: Alternative Screen

## Temperature Screen

The Temperature Screen displays temperature readings from available sensors.

**Displays:**
- Temperature status for all available sensors
- Individual sensor readings
- Maximum temperature

**Configuration:**
- Use `-t <id>` to force a specific temperature sensor
- Use `-gt <id>` to show a specific temperature on the summary screen

## Fan Speed Screen

The Fan Speed Screen shows current fan speeds.

**Displays:**
- Fan current speed for available sensors
- Individual fan RPM readings

**Configuration:**
- Use `-f <id>` to force a specific fan speed sensor

## Screen Navigation

| Button | Action |
|--------|--------|
| **L2** | Go to Previous Screen |
| **L3** | Go to Next Screen |
| **L4** | Show Alternative Screen (not available on Swap, Memory, and Battery screens) |
| **L5** | Toggle Bottom Info bar mode |

## Info Bar Rotation

When using the `-ir` option, the bottom info bar will rotate through all available sensors automatically.
