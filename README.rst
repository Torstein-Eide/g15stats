G15STATS
========

A CPU/Memory/Swap/Network/Battery/Temperature/Fan Speed/CPU Frequencies usage meter for G15Daemon.

========
Features
========

* Summary Screen displays 4 or 5 indicators from CPU/Frequency/Memory/NET/Temperature/Fan Speed/Swap, along with current time.
* CPU Screen displays graphs of User/System/Nice and Idle time, along with LoadAVG and Uptime.
* Frequency Screen displays all CPU cores frequency with the total
* Memory Screen displays Memory Total & Free, and graph of Used vs Buffered+Cached Memory.
* Swap Screen displays Used, Free and Total swap space, along with the number of pages currently paged in/out.
* Network Screen displays Total bytes In/Out, history graph, Peak speed.
* Battery Status Screen displays battery charge data for up to three batteries
* Temperature Screen displays temperature status for available sensors
* Fan Speed Screen displays fan current speed for the available sensors

============
Requirements
============

- libgtop

=======
Options
=======


-i id		Gather statistics from named interface (ie -i eth0).
-d			Run in background (daemonise).
-nsa		Scale network graphs against highest speed recorded.  The default is to scale against the highest peak in the current graph.
-h			Show help
-r seconds	Set the refresh interval to seconds The seconds must be between 1 and 300. (ie -r 20)
-u			Display unicore graphs only on the CPU screen.
-t id		Force to monitor temperature sensor id on start (ie -t 1). 
			The id should point to sysfs path /sys/class/hwmon/hwmon[id]/device/temp1_input. 
			Default the sensor id is auto-detected.
-f id		Force to monitor fan speed sensor id on start (ie -f 1). The id should point 
			to sysfs path /sys/class/hwmon/hwmon[id]/device/fan1_input. 
			Default the sensor id is auto-detected.
-df			Disable monitoring CPUs frequencies.
-ir			Enable the bottom info bar content rotate cycle over all available sensors.
-vc			The cpu cores will be calculated every time (for systems with the cpu hotplug).
-gt id		Show temperature [id] in place of the maximal one on the Summary Screen with 
			the option -gt id ie -gt 1. The id should point to sysfs path /sys/class/hwmon/hwmon../device/temp[id]_input

=====
Usage
=====

Once running, the separate screens can be switched to as follows:

L2: Previous Screen
L3: Next Screen
L4: Alternative Screen (Doesn't work on Swap, Memory and Battery Screen)
L5: Bottom Info bar mode

=============
How to donate
=============

If you find this repo useful (don't forget to pay a visit to the related
repos too), you can buy me a beer:

:BTC: 3ECzX5UhcFSRv6gBBYLNBc7zGP9UA5Ppmn

:ETH: 0x7E17Ac09Fa7e6F80284a75577B5c1cbaAD566C1c
