# G15Stats

A CPU/Memory/Swap/Network/Battery/Temperature/Fan Speed/CPU Frequencies usage meter for G15Daemon.

## Overview

G15Stats is a statistics monitoring application for G15 LCD displays. It provides real-time monitoring of system resources and displays them on the G15 keyboard's LCD screen.

## Features

- **Summary Screen**: Displays 4 or 5 indicators from CPU/Frequency/Memory/NET/Temperature/Fan Speed/Swap, along with current time
- **CPU Screen**: Displays graphs of User/System/Nice and Idle time, along with LoadAVG and Uptime
- **Frequency Screen**: Displays all CPU cores frequency with the total
- **Memory Screen**: Displays Memory Total & Free, and graph of Used vs Buffered+Cached Memory
- **Swap Screen**: Displays Used, Free and Total swap space, along with the number of pages currently paged in/out
- **Network Screen**: Displays Total bytes In/Out, history graph, Peak speed
- **Battery Status Screen**: Displays battery charge data for up to three batteries
- **Temperature Screen**: Displays temperature status for available sensors
- **Fan Speed Screen**: Displays fan current speed for the available sensors

## Quick Start

!!! tip
    New setup? Start with the
    [Installation Tutorial](installation-tutorial.md) for sensor and systemd steps.

```bash
# Build the project
./configure
make

# Install
make install

# Run
g15stats
```

## Navigation

Once running, the separate screens can be switched to as follows:

- **L2**: Previous Screen
- **L3**: Next Screen
- **L4**: Alternative Screen (Doesn't work on Swap, Memory and Battery Screen)
- **L5**: Bottom Info bar mode

## Warning

!!! warning "Discontinuation Notice"

    I'm discontinuing this after someone made a fuzz about a feature he decided he want a decade later.
    And as far I am concerned, Arch Linux's AUR administrators find this behavior just fine, so I'm not wasting my efforts on this anymore.
    
    I can still fix issues as I always did and help via mail, but keep in mind Arch Linux is impossible to be supported.

## Documentation Navigation

| Page | Purpose |
|---|---|
| [Installation](installation.md) | Build and installation instructions |
| [Installation Tutorial](installation-tutorial.md) | End-to-end setup with sensors and systemd |
| [Usage](usage.md) | How to run and use G15Stats |
| [Options](options.md) | Command-line options reference |
| [Screens](screens.md) | Detailed information about each screen |
| [Requirements](requirements.md) | System requirements and dependencies |
