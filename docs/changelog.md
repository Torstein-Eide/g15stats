# Changelog

## WIP 4.0 (Since `95de11a`)

This is the working changelog for the in-progress 4.0 cycle.

### Highlights

- **New config foundation**: moved to YAML config (`libyaml`) with stronger parser tests and CLI/config override coverage.
- **Better testability**: added output-file mode, raw frame dumping, and readable frame conversion for automated screen verification.
- **CPU work for modern systems**: added `CPU LOAD2` with grouped rendering for high core counts and mode-based orientation switching.
- **Diagnostics improved**: added debug logs for screen/mode/submode transitions and one-shot layout debug output for CPU bars.
- **Screen model cleanup**: removed standalone `SCREEN_FREQ`; aggregate frequency path remains.
- **Docs workflow automation**: one-command scripts now generate screen/mode dumps, PNG assets, and docs page updates.
- **Project docs refresh**: migrated README to GFM (`README.md`), added better onboarding links and contributor credits.

### Current Focus

- Stabilize screen behavior and visual consistency across CPU modes.
- Continue lint cleanup and keep warnings low.
- Keep docs/screens assets in sync with current screen layout and IDs.

## Legacy Releases

### 1.9.8

- Fork and some fixes, added scripts from gentoo.

### SVN 538 (1.9.7)

- Improve Summary Screen Fan / Temperature sensors bar calculation.

### SVN 534 (1.9.6)

- Improve Summary Screen with 3 and 6 core CPU.

### SVN 532 (1.9.5)

- Improve Summary Screen multi bar (temperature and network).
- Fix compilation with the newer version of the glibc.
- Improve refresh interval causing -nan error with the newer kernel on very fast machines due to too often glibtop probing.
- Fix possible -nan error on the fast screens switch.
- Change refresh interval with the option `-r` seconds (between 1 and 300).

### SVN 531 (1.9.4)

- Fix time info.
- Improve Fan / Temperature screens leading labels position.
- Improve Fan / Temperature screens height calculation.
- Improve Summary Screen multi bar (temperature and network).

### SVN 530 (1.9.3)

- Activate bottom info bar content rotation with option `-ir`.
- Change bottom info bar content to the component default one.
- Activate variable CPU count with option `-vc`.
- Improve fan bottom info.

### SVN 520 (1.9.2)

- Disable monitoring CPU frequencies with option `-df`.
- Improve Summary Screen without CPU frequencies.

### SVN 519

- Change default mode for Frequency and Summary screens.
- Improve Summary Screen for CPUs with more than 7 cores.

### SVN 518 (1.91)

- Fix Summary, Frequency and CPU screens for some CPU core count variants.
- Improve Summary Screen.

### SVN 517

- Improve battery screen deactivation logic.

### SVN 516

- Refactor temperature and fan speed logic into shared flow.

### SVN 515 (1.90)

- Fix temperature/fan sensor-lost handling.
- Fix screen cycle.

### SVN 514

- Many fixes and cleanup.
- Change version to 1.90.
- Use proper sysfs variable for CPU frequency.
- Add new Summary Screen indicators.
- Add network current traffic info in the bottom line.
- Refactor CPU variants into separate additional screens: Summary, CPU and CPU Freq.
- Add new fan speed screen.
- Reset info bar content to screen default after screen change.
- Control network screen scaling with `L4`.
- Force startup temperature sensor id with `-t <id>`.
- Force startup fan speed sensor id with `-f <id>`.
- Force selected temperature in Summary Screen with `-gt <id>`.
- On Fan/Temp screens, current sensor is controlled with `L4` (for systems with multiple sensors).
- Every screen has its own mode (except Swap, Memory and Battery).

### SVN 513

- Improve bottom line and keywatch logic.

### SVN 512

- Add CPU screen variants for unicore processors.
- Add CPU frequency info (when available).
- Enable battery screen when available.
- Cleanup.

### SVN 511

- Improve sensor autodetect.
- Cleanup.

### SVN 510

- Add temperature sensor autodetect.
- Add forced user sensor id with option `-s <id>`.

### SVN 509

- Temperature sensors depend on lm-sensors sysfs filesystem.

### SVN 505

- Bugfix: render bottom line on Memory Screen once.

### SVN 504

- Bottom row information rotation switched with `L5`.
- 3 additional CPU graph variants switched with `L4` (multicore only at that time).
- New default CPU screen (multicore only at that time).
- Display unicore graphs only on CPU screen with option `-u` / `--unicore`.

### SVN (pre-504)

- Improve response time when switching screens.
- Add battery status monitoring (courtesy of Pieter De Wit).
- Detect incorrect network interface and disable network screen (fixes reported 100% CPU case).

### 1.0

- Change net scale from absolute to relative, with option to revert to absolute (`-nsa`, `--net-scale-absolute`).

### 0.1

- Initial release.
