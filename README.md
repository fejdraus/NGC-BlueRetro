<p align="center"><img src="https://raw.githubusercontent.com/RetroScaler/NGC-BlueRetro/master/image/logo.png" width="520" alt="NGC-BlueRetro"/></p>

<h1 align="center">NGC-BlueRetro — improved firmware</h1>

<p align="center">
  Modern <a href="https://github.com/darthcloud/BlueRetro">BlueRetro</a> firmware for the <a href="https://github.com/RetroScaler/NGC-BlueRetro">RetroScaler NGC-BlueRetro</a> GameCube adapter.<br/>
  Four slot LEDs, Switch 2 support, buttons that match by name on Xbox and PlayStation pads, optional hotplug.
</p>

<p align="center">
  <a href="../../releases/latest"><img src="https://img.shields.io/github/v/release/fejdraus/NGC-BlueRetro?label=firmware&color=2ea44f" alt="Latest release"/></a>
  <a href="../../releases"><img src="https://img.shields.io/github/downloads/fejdraus/NGC-BlueRetro/total?color=blue" alt="Downloads"/></a>
  <img src="https://img.shields.io/badge/hardware-HW1%20%C2%B7%20ESP32%20%C2%B7%204MB-informational" alt="HW1"/>
  <img src="https://img.shields.io/badge/ESP--IDF-5.5-orange" alt="ESP-IDF 5.5"/>
  <a href="LICENSE"><img src="https://img.shields.io/github/license/fejdraus/NGC-BlueRetro" alt="License"/></a>
</p>

<p align="center"><b><a href="../../releases/latest">⬇ Download the latest firmware</a></b> · <a href="#flashing">Flashing guide</a> · <a href="#whats-different-from-the-stock-firmware">What changed</a> · <a href="#building-from-source">Build from source</a></p>

---

## Why this firmware

RetroScaler ships the adapter with a build of BlueRetro from June 2025 and does not publish its sources: their repository is frozen at January 2023 code. This fork re-implements the adapter specific behaviour on top of the **final** upstream BlueRetro release and fixes the defects that affect this hardware.

| | Stock firmware | This firmware |
|---|---|---|
| BlueRetro base | `ddfc31b`, June 2025 | `e1a9831`, the final upstream release (v25.10) |
| Switch 2 controllers | yes | yes, plus the upstream fix that pairs only with real Switch 2 pads |
| Face buttons on Xbox / PlayStation pads | by position, so **X and B are swapped** | **matched by name**, with a toggle for Nintendo layout pads |
| Mapping toggle button | View, which could not be used for anything else | **Share**, View is free to map |
| Slot LEDs with BLE only controllers | stay dark | **light up correctly** |
| Wired ports | always announced, games see four idle controllers | **optional hotplug build**: ports follow the controllers |
| Custom Bluetooth address | broken upstream and in the stock build | **restored** |
| Rumble | on | on |

Everything else is upstream BlueRetro as its author left it, including the fixes that landed after the stock build: more responsive rumble, SNES analog axes and rumble, and the revert of the native Switch report experiment.

## Features

* Up to **4 Bluetooth controllers** at once, one per GameCube port
* **PS3, PS4, PS5**, **Xbox One S, Xbox Series X|S**, **Switch 2**, Switch Pro, Joy-Con, Wii / Wii U Pro, 8BitDo, Retro-Bit and generic HID gamepads, BR/EDR and Bluetooth LE
* **Rumble** on all four ports out of the box
* Emulated **memory card** stored in the adapter flash
* Per port button mapping, turbo, deadzones and response curves, configured from Chrome at [blueretro.io](https://blueretro.io)
* Two builds to choose from: **standard** and **hotplug**
* Updatable over USB with a single `.bin` file, your pairings and settings survive the update

## Which build do I need

| Build | File | Behaviour |
|---|---|---|
| **Standard** | `NGC-BlueRetro_master-4leds_4MB.bin` | Like the stock firmware: all four GameCube ports always answer the console |
| **Hotplug** | `NGC-BlueRetro_master-4leds-hotplug_4MB.bin` | A port is announced to the console only while a Bluetooth controller is connected to that slot. Games no longer see four phantom players and player 1 is always a real pad |

Pick **hotplug** unless you play games that fix the set of controllers at startup and do not look again; those may miss a pad connected later. You can switch between the two builds at any time by flashing the other one.

## LED status

<p align="center"><img src="https://raw.githubusercontent.com/RetroScaler/NGC-BlueRetro/master/image/1.png" width="500" alt="Slot LEDs"/></p>

| LED | GPIO | Meaning |
|---|---|---|
| 🔴 Red | 14 | controller 1 connected |
| 🟢 Green | 32 | controller 2 connected |
| 🟠 Orange | 33 | controller 3 connected |
| ⚪ White | 25 | controller 4 connected |

A LED lights up once the controller has finished HID initialisation and is ready, and goes out when it disconnects. The pin assignments were checked against the board schematic RetroScaler publishes, [`hardware/GC_BlueRetro.pdf`](https://github.com/RetroScaler/NGC-BlueRetro/blob/master/hardware/GC_BlueRetro.pdf): ESP32-WROOM, GameCube data lines on IO19 / IO5 / IO26 / IO27, four indicator LEDs D6–D9 behind 330R resistors, CH340T for USB.

## Buttons

<p align="center"><img src="https://raw.githubusercontent.com/RetroScaler/NGC-BlueRetro/master/image/2.png" width="500" alt="EN and BOOT buttons"/></p>

**EN** resets the adapter.

**BOOT** does different things depending on how long it is held. The port LEDs flash while the button is down, faster as each threshold is crossed, so you can pick the action by watching them:

| Press | Action |
|---|---|
| short | disconnect every Bluetooth device and start pairing |
| hold ~1 s | enable inquiry mode, keeping the connected devices |
| hold ~3 s | restore the default configuration and clear the pairing keys |
| hold ~30 s | the above, and also erase `otadata` so the factory image boots |

## Flashing

### 1. Driver

The adapter uses a **CH340** USB serial bridge (`VID_1A86` / `PID_7523`). Windows has no in-box driver, so the adapter shows up as an unknown device until one is installed:

* Windows Update usually has it: Device Manager → *Update driver* → *Search automatically*, look for `wch.cn - Ports`
* or the vendor package: <https://www.wch-ic.com/downloads/CH341SER_EXE.html>

Beware of the many mirrors serving unsigned copies of `CH341SER.EXE`.

> **Use 115200 baud.** On at least one unit the bridge corrupts data above that: reads broke at 8% at 460800 and at 1.6% at 921600.

### 2. Back up the stock firmware

```
python -m esptool --chip esp32 -p COM? -b 115200 read-flash 0 0x400000 stock-4MB.bin
```

This full dump is the only way back to the stock firmware, as RetroScaler does not publish it.

### 3a. Flash with NodeMCU-PyFlasher

<p align="center"><img src="https://raw.githubusercontent.com/RetroScaler/NGC-BlueRetro/master/image/nodemcu3.png" width="600" alt="NodeMCU-PyFlasher"/></p>

Download [NodeMCU-PyFlasher](https://github.com/marcelstoer/nodemcu-pyflasher/releases) and one of the `*_4MB.bin` files from the [release](../../releases/latest).

| Field | Value |
|---|---|
| Serial port | the `USB-SERIAL CH340` port |
| NodeMCU firmware | one of the `*_4MB.bin` files |
| Baud rate | **115200** |
| Flash mode | Dual I/O (DIO) |
| Erase flash | **no** |

### 3b. Flash with esptool

From an unpacked `*_esptool.zip`:

```
python -m esptool --chip esp32 -p COM? -b 115200 --before default-reset --after hard-reset \
  write-flash --flash-mode dio --flash-size 4MB --flash-freq 40m \
  0x1000 bootloader.bin 0x8000 partition-table.bin \
  0xd000 ota_data_initial.bin 0x10000 BlueRetro.bin
```

This variant leaves the NVS partition untouched, where ESP-IDF keeps the radio calibration.

> **Never use `erase_flash`.** The `storage` partition holds the Bluetooth pairing keys, the configuration and the emulated memory card. The partition table is identical to the stock one and the configuration migrates automatically, so all of it survives an update as long as the flash is not erased.

### Going back

Flash the dump from step 2 at `0x0`:

```
python -m esptool --chip esp32 -p COM? -b 115200 write-flash 0x0 stock-4MB.bin
```

## Configuration

Connect from a Chromium based browser at [blueretro.io](https://blueretro.io): *Advance config* for button mapping, *System manager* for reboot and factory reset. Disconnect every controller first, the adapter only advertises over BLE while no controller is connected.

The device shows up as `BlueRetro_GC_xxxx`, where the suffix is the last two bytes of its Bluetooth address.

## What's different from the stock firmware

Every device specific change sits behind a Kconfig option that defaults to `n`, so a plain upstream build is unaffected.

* **`RETROSCALER_BLUERETRO_4LEDS_HW`** — the four per slot LEDs. They are lit from `bt_hid_init()` and cleared from `bt_host_reset_dev()`, which covers BLE only controllers as well; the stock firmware hooked three scattered call sites keyed on the device name read and missed them. Mutually exclusive with `BLUERETRO_HW2`, whose reset and port sense lines use the same pins, and refuses to build together with `BT_H4_TRACE` or `COVERAGE`, which claim GPIO 14 as an UART CTS.
* **`BLUERETRO_HW1_HOTPLUG`** — brings back the hotplug behaviour upstream dropped for HW1 in `9600f0d`. A port is announced to the console only once a Bluetooth controller is connected to that slot. HW1 has no port sense lines, but hotplug never needed them: the port mask is derived from `BT_DEV_HID_INIT_DONE` alone and `nsi_port_cfg()` is pure GPIO and RMT work.
* **`BLUERETRO_GC_XBOX_FACE_LAYOUT`** — GameCube face buttons match by **name** for Xbox and PlayStation pads. Upstream maps by position, which swaps X and B on those pads, while the alternate table only matches names for Nintendo layout pads, leaving Xbox users without a name match in either mode.
* **`BLUERETRO_GC_TOGGLE_ON_SHARE`** — moves the face buttons mapping toggle from View (`PAD_MS`) to Share (`PAD_MQ`), freeing View up to be mapped to a regular GameCube button. `PAD_MQ` is added to `gc_mask`, otherwise `adapter_map_from_btn()` would drop any mapping targeting it.

Fixes applied unconditionally:

* **NSI RX channel re-arm.** A port can be disabled between RX End and TX End, which leaves the RMT channel with `rx_en` cleared while only TX End puts it back into RX, so the port would stay deaf after being enabled again. With hotplug that window is reachable on every controller drop.
* **Switch 2 HID init through the common dispatcher.** `bt_att_hid_start_next_state()` called `bt_hid_sw2_init()` directly, bypassing `bt_hid_init()` and everything hooked on it.
* **BDADDR load from NVS restored.** Upstream commit `f9461fa` commented the function out and left a hardcoded test MAC behind; that debug leftover shipped in v25.10-beta and in the stock firmware.
* Rumble enabled in the factory configuration, as the stock firmware does.

The full set of changes on top of upstream is 12 files and under 200 lines: [compare `e1a9831...main`](../../compare/e1a9831...main).

## Building from source

Upstream builds standalone since v25.04, no BlueRetroRoot needed. Both build variants live in `configs/hw1/`:

| Config | Build |
|---|---|
| `gamecube_4leds` | standard |
| `gamecube_4leds_hotplug` | hotplug |

```
git clone --recursive https://github.com/fejdraus/NGC-BlueRetro.git
cd NGC-BlueRetro
docker run --rm -v "$PWD":/project -w /project \
    ghcr.io/darthcloud/idf-blueretro:v5.5.0_2024-12-02 \
    bash -c 'source $IDF_PATH/export.sh && cp configs/hw1/gamecube_4leds_hotplug sdkconfig && idf.py build'
```

The artifacts land in `build/`: `bootloader/bootloader.bin`, `partition_table/partition-table.bin`, `ota_data_initial.bin` and `BlueRetro.bin`. To produce a single `0x0` image for NodeMCU-PyFlasher, merge them with `esptool.py merge_bin` using the offsets from the esptool command above.

## Credits

BlueRetro is by [Jacques Gagnon (darthcloud)](https://github.com/darthcloud/BlueRetro), Apache-2.0. See the [upstream README](https://github.com/darthcloud/BlueRetro#readme) and the [project wiki](https://github.com/darthcloud/BlueRetro/wiki). All the hard parts are his work; this fork only adapts it to one adapter.

The adapter, the four LEDs behaviour re-implemented here and the images above are by [RetroScaler](https://github.com/RetroScaler/NGC-BlueRetro).
