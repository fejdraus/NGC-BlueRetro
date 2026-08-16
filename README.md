# NGC-BlueRetro — custom firmware (HW1, 4 LEDs, GameCube)

<p align="center"><img src="https://raw.githubusercontent.com/RetroScaler/NGC-BlueRetro/master/image/logo.png" width="500"/></p>

Firmware for the **RetroScaler NGC-BlueRetro** adapter — a Bluetooth controller
receiver for the Nintendo GameCube, built around the ESP32, hardware revision
**HW1**, with four per slot indicator LEDs on GPIO 14 / 32 / 33 / 25.

Prebuilt images are attached to the [releases](../../releases).

## Where this firmware comes from

1. **The original BlueRetro firmware** by
   [Jacques Gagnon (darthcloud)](https://github.com/darthcloud/BlueRetro), taken
   at upstream `master` (`e1a9831`), the final state of that project. It
   provides everything that makes the adapter work: the Bluetooth stack, the
   per vendor controller protocols and the wired console drivers.
2. **The functionality
   [RetroScaler](https://github.com/RetroScaler/NGC-BlueRetro) implemented for
   their NGC-BlueRetro adapters** — the four per slot connection indicator LEDs,
   and rumble enabled out of the box. They publish the binaries of their recent
   builds but not the sources: their repository is frozen at January 2023 code,
   while the shipped firmware is a build of upstream `ddfc31b` from June 2025.
   That behaviour is therefore reimplemented here on top of current upstream
   code.
3. **Fixes and adaptations made specifically for compatibility with this
   hardware**, listed further down.

## Features

* Up to **4 Bluetooth controllers** at once, one per GameCube port
* PS3, PS4, PS5, Xbox One S, Xbox Series X\|S, **Switch 2**, Switch Pro, Joy-Con,
  Wii / Wii U Pro, 8BitDo, Retro-Bit and generic HID devices, BR/EDR and LE
* **Rumble** enabled by default on all four ports
* Emulated **memory card** backed by the adapter flash
* Per port button mapping, turbo, deadzones and response curves, configured from
  Chrome over Bluetooth at [blueretro.io](https://blueretro.io)
* Firmware updatable over USB or over the air

## LED status

<p align="center"><img src="https://raw.githubusercontent.com/RetroScaler/NGC-BlueRetro/master/image/1.png" width="500"/></p>

| LED | GPIO | Meaning |
|---|---|---|
| Red | 14 | controller 1 connected |
| Green | 32 | controller 2 connected |
| Orange | 33 | controller 3 connected |
| White | 25 | controller 4 connected |

A LED lights up once the controller has finished HID initialisation and is
ready, and goes out when the device disconnects.

## Buttons

**EN** resets the adapter.

**BOOT**, thresholds coming from `hw_config.sw_io0_hold_thres_ms`:

| Press | Action |
|---|---|
| short | disconnect every Bluetooth device and start pairing |
| hold ~1 s | enable inquiry mode, keeping connected devices |
| hold ~3 s | restore the default configuration and clear the pairing keys |
| hold ~30 s | the above, and also erase `otadata` so the factory image boots |

The adapter flashes the port LEDs while the button is held, faster as each
threshold is crossed, so the action is picked by watching them.

## What this firmware adds over the stock one

Every device specific change sits behind a Kconfig option that defaults to `n`,
so plain upstream builds are unaffected.

* **`RETROSCALER_BLUERETRO_4LEDS_HW`** — the four per slot LEDs. They are lit
  from `bt_hid_init()` and cleared from `bt_host_reset_dev()`, which covers BLE
  only controllers as well; the stock firmware hooked three scattered call sites
  keyed on the device name read and missed them. Mutually exclusive with
  `BLUERETRO_HW2`, whose reset and port sense lines use the same pins, and
  refuses to build together with `BT_H4_TRACE` or `COVERAGE`, which claim
  GPIO 14 as an UART CTS.
* **`BLUERETRO_HW1_HOTPLUG`** — brings back the hotplug behaviour upstream
  dropped for HW1 in `9600f0d`. A port is announced to the console only once a
  Bluetooth controller is connected to that slot, so games no longer see four
  idle controllers. HW1 has no port sense lines, but hotplug never needed them:
  the port mask is derived from `BT_DEV_HID_INIT_DONE` alone and
  `nsi_port_cfg()` is pure GPIO and RMT work.
* **`BLUERETRO_GC_XBOX_FACE_LAYOUT`** — GameCube face buttons match by **name**
  for Xbox and PlayStation pads. Upstream maps by position, which swaps X and B
  on those pads, while the alternate table only matches names for Nintendo
  layout pads — leaving Xbox users without a name match in either mode.
* **`BLUERETRO_GC_TOGGLE_ON_SHARE`** — moves the face buttons mapping toggle
  from View (`PAD_MS`) to Share (`PAD_MQ`), freeing View up to be mapped to a
  regular button. `PAD_MQ` is added to `gc_mask`, otherwise
  `adapter_map_from_btn()` would drop any mapping targeting it.

Fixes applied unconditionally:

* **NSI RX channel re-arm.** A port can be disabled between RX End and TX End,
  which leaves the RMT channel with `rx_en` cleared while only TX End puts it
  back into RX, so the port would stay deaf after being enabled again. With
  hotplug that window is reachable on every controller drop.
* **Switch 2 HID init through the common dispatcher.**
  `bt_att_hid_start_next_state()` called `bt_hid_sw2_init()` directly, bypassing
  `bt_hid_init()` and everything hooked on it.
* **BDADDR load from NVS restored.** Upstream commit `f9461fa` commented the
  function out and left a hardcoded test MAC behind; that debug leftover shipped
  in `v25.10-beta` and in the stock firmware.
* Rumble enabled in the factory configuration, as the stock firmware does.

## Build variants

Both live in `configs/hw1/` and set `BLUERETRO_HW2=n`, the four LEDs and
`BLUERETRO_SYSTEM_GC`:

| Config | Difference |
|---|---|
| `gamecube_4leds` | wired ports always answer the console, as the stock firmware does |
| `gamecube_4leds_hotplug` | ports appear and disappear with the Bluetooth controllers |

## Flashing

### Driver

The adapter exposes a **CH340** USB serial bridge (`VID_1A86` / `PID_7523`).
Windows has no in-box driver, so it shows up as an unknown device until one is
installed:

* Windows Update usually has it — Device Manager, *Update driver*, *Search
  automatically*, look for `wch.cn - Ports`
* or the vendor package: <https://www.wch-ic.com/downloads/CH341SER_EXE.html>

Beware of the many mirrors serving unsigned copies of `CH341SER.EXE`.

On at least one unit the bridge corrupts data above `115200` baud: reads broke
at 8% at `460800` and at 1.6% at `921600`. Flash and dump at `115200`.

### Back up first

```
python -m esptool --chip esp32 -p COM? -b 115200 read-flash 0 0x400000 stock-4MB.bin
```

This full dump is the only way back to the stock firmware.

### NodeMCU-PyFlasher

<p align="center"><img src="https://raw.githubusercontent.com/RetroScaler/NGC-BlueRetro/master/image/nodemcu3.png" width="600"/></p>

Pick one of the `*_4MB.bin` files from the release, baud `115200`, flash mode
`DIO`, **Erase flash: no**.

### esptool

From an unpacked `*_esptool.zip`:

```
python -m esptool --chip esp32 -p COM? -b 115200 --before default-reset --after hard-reset \
  write-flash --flash-mode dio --flash-size 4MB --flash-freq 40m \
  0x1000 bootloader.bin 0x8000 partition-table.bin \
  0xd000 ota_data_initial.bin 0x10000 BlueRetro.bin
```

**Never use `erase_flash`.** The `storage` partition holds the Bluetooth link
keys, the configuration and the emulated memory card. The partition table is
identical to the stock one and the configuration migrates automatically, so all
of it survives an update as long as the flash is not erased.

## Configuration

Connect from a Chromium based browser at [blueretro.io](https://blueretro.io)
— *Advance config* for button mapping, *System manager* for reboot and factory
reset. Disconnect every controller first: the adapter only advertises over BLE
while no controller is connected.

The device shows up as `BlueRetro_GC_xxxx`, where the suffix is the last two
bytes of its Bluetooth address.

## Building

Upstream builds standalone since v25.04, no BlueRetroRoot needed:

```
docker run --rm -v "$PWD":/project -w /project \
    ghcr.io/darthcloud/idf-blueretro:v5.5.0_2024-12-02 \
    bash -c 'source $IDF_PATH/export.sh && cp configs/hw1/gamecube_4leds_hotplug sdkconfig && idf.py build'
```

## Credits

BlueRetro is by [Jacques Gagnon (darthcloud)](https://github.com/darthcloud/BlueRetro),
Apache-2.0 — see the [upstream README](https://github.com/darthcloud/BlueRetro#readme)
and the [project wiki](https://github.com/darthcloud/BlueRetro/wiki). All the
hard parts are his work; this fork only adapts it to one adapter.

The adapter, the four LEDs behaviour reimplemented here and the images above are
by [RetroScaler](https://github.com/RetroScaler/NGC-BlueRetro).
