# Scanning and Hardware

[English README](../README.md) | [中文 README](../README.zh-CN.md)

This guide explains how I2C Detector accesses Linux adapters and how to interpret
its results. For dependencies, builds, and packaging, see the
[README](../README.md#development-environment). For thread ownership and state
transitions, see the [architecture overview](00-overview.md) (Chinese).

## Platform Behavior

| Environment | Behavior |
| --- | --- |
| CM0 ARM64 Linux device | Uses the device display backend and scans real `/dev/i2c-*` adapters. |
| Linux SDL2 desktop simulator | Uses an SDL window and the same real Linux scanner. |
| macOS or another non-Linux host | Displays `I2C scanning is only available on Linux devices`; no hardware scan or demo results. |
| Unit and render tests | Supply synthetic reports without probing hardware; render tests do not need an SDL window. |

The scanner selects its platform implementation at compile time with
`__linux__`. The simulator display option does not disable device access.
Scanning itself does not use the network or require `i2c-tools`.

## Linux Setup

The kernel must have an enabled I2C controller and expose it through `i2c-dev`.
On Raspberry Pi OS, enable the required I2C interface in the system's hardware
configuration, then reboot if requested. If `i2c-dev` is supplied as a module,
load it with:

```sh
sudo modprobe i2c-dev
```

Loading this module does not enable a disabled controller or configure board
pins. Check the device's board documentation for the appropriate controller,
pin assignment, supply voltage, common ground, and pull-ups.

Inspect the exposed adapters and the current user's groups:

```sh
ls -l /dev/i2c-*
id
```

The application opens each node with read/write access. On systems where the
nodes belong to the `i2c` group and that group has read/write permission, add the
account that will run the application to that group, for example for the current
login account:

```sh
sudo usermod -aG i2c "$(id -un)"
```

Log out and back in before launching again. If the nodes use another group or
are recreated without suitable permissions, configure access through the
system's device/udev policy. The `i2c` declaration in `lpm.toml` describes the
application's requirement; the application package does not install a permission
rule or enable the kernel interface.

Opening the page immediately scans every discovered adapter. Choose hardware
that permits probing before launch: SMBus Quick write and Byte read transactions
can change the state of some devices. Restricting the address range alone does
not make a scan harmless, and selecting a bus in the interface does not restrict
which buses the next scan probes.

## Using the Interface

1. Open I2C Detector. It discovers adapters and starts a scan automatically.
2. Wait for the report. The status displays the total number of detected addresses
   across all discovered buses, plus the number of unavailable buses, if any.
3. Select a `/dev/i2c-N` entry to see that bus's address map and count or error.
4. Click **Scan** after changing connections or access permissions to rediscover
   and rescan all adapters.

During a rescan, the previous report remains visible and its buses can still be
selected. The Scan button is disabled until the new report arrives. Results are
published as a complete report, without per-address progress. The previous bus
selection is preserved by bus number when it still exists; otherwise the first
bus in numeric order is selected.

There is no periodic automatic rescan. Results are kept in memory for the current
session; the application does not save a scan history or provide export controls.

## Reading the Address Map

The matrix always contains 128 cells, arranged in 8 rows and 16 columns from
`00` to `7F`. Each cell shows a full hexadecimal 7-bit address; for example, `3C`
means `0x3C`, not an 8-bit address containing the read/write bit.

| Appearance | Meaning |
| --- | --- |
| Green cell with white text | The probe succeeded, or the kernel reported the address as busy/claimed. |
| White cell with dark text | No detection is recorded for this address in the displayed report. Check the scan and bus status before interpreting it. |
| Muted cell | Outside the probe range: `0x00`–`0x02` and `0x78`–`0x7F` are not probed. |

Only `0x03`–`0x77` are probed, inclusive. A white cell does not prove that no
physical device exists: the adapter or device may not support the transactions,
the bus may be unavailable, or no scan may have completed yet.

The interface labels counts as “devices,” but they count detected addresses.
An address already claimed by a driver contributes to the same count as a
successful probe. The scanner does not identify device models, distinguish
claimed addresses with a separate `UU` marker, or detect devices sharing an
address. Address cells are informational and do not open register or device
detail controls.

## Probe Sequence

The [system scanner](../src/pages/i2c_detector/i2c_scanner.cpp) performs these
steps on one worker thread:

1. Enumerate `/dev` entries named `i2c-` followed by an accepted nonnegative bus
   number, then sort and deduplicate the numbers.
2. Open each `/dev/i2c-N` with `O_RDWR | O_CLOEXEC`. If opening fails, record that
   bus's error and continue with the next bus.
3. For each address from `0x03` to `0x77`, request `I2C_SLAVE`. If it fails with
   `EBUSY`, mark the address detected without further probing. Other failures
   leave the address unmarked.
4. If selecting the address succeeds, try `I2C_SMBUS_QUICK` with
   `I2C_SMBUS_WRITE`. If it succeeds, mark the address detected.
5. If the Quick transaction fails, try `I2C_SMBUS_BYTE` with `I2C_SMBUS_READ`.
   Mark the address detected only if that transaction succeeds.
6. Close each adapter, then return the completed report to the main thread.

The implementation uses Linux `ioctl` directly. It does not query `I2C_FUNCS`,
use `I2C_SLAVE_FORCE`, choose an address-specific probe method, or expose
individual transaction errors. It also does not set an application-level ioctl
timeout. Probe failure may therefore reflect unsupported operations as well as
an absent or unresponsive device.

There is no application setting or environment variable for selecting a scan
backend, device directory, bus allowlist, address range, or scan interval.
Fake scanners are injected in C++ tests rather than selected at runtime.

## Troubleshooting

| Symptom | Explanation and action |
| --- | --- |
| `I2C scanning is only available on Linux devices` | Expected on non-Linux hosts. Use the fixture tests for UI development, or run on Linux for hardware scanning. |
| `No I2C buses found` | No matching adapter nodes were discovered. Check the controller configuration, `i2c-dev`, and `/dev/i2c-*`, then rescan. |
| `Bus unavailable: Permission denied` | The running account cannot open the selected node for reading and writing. Check its ownership, mode, and group membership. |
| Other `Bus unavailable: ...` errors | The node was found but could not be opened. Check that the adapter is still present and inspect system/driver logs. Other buses are still scanned. |
| Status includes `unavailable` | At least one bus failed to open. Select those buses to read their individual errors; scan completion does not mean every bus was usable. |
| `Cannot inspect /dev: ...` | Discovery could not open `/dev`. Check the process's device namespace and access permissions. |
| Scan completes with zero detected addresses | Verify the selected bus, power, wiring, address format, and adapter/device SMBus support. The application cannot distinguish all transaction failures from absent devices. |
| A green address is managed by a kernel driver | Expected: `I2C_SLAVE` returning `EBUSY` counts as detected. No force option is used to bypass the driver. |
| Changes do not appear while scanning | The previous report remains visible until the entire scan completes. There is no incremental result display. |
| Scanning or closing takes a long time | The worker may be waiting for a kernel transaction. Cancellation is checked between buses and addresses and does not interrupt an active `ioctl`; inspect the adapter and driver. |

Fix the underlying condition and click **Scan** to refresh discovery and results.
For build and dependency errors, use the [README troubleshooting table](../README.md#troubleshooting).

## Validation Limits

`lpm test` runs four hardware-independent tests. The Model and ViewModel tests
exercise report handling; render tests verify named controls, selected colors,
bus changes, scan button state, and snapshot output at `568 x 1232` and
`1232 x 568`. Their fixture has address `0x20` on bus 1 and addresses `0x3C` and
`0x68` on bus 4.

These tests do not exercise Linux adapter discovery or `ioctl` calls. Changes to
the scanner need separate verification on suitable Linux hardware, including
permission failures and known connected devices. Passing a render test also does
not check every text boundary, scrolling path, or live size change; see the
[UI guidelines](01-user-interface.md) (Chinese).
