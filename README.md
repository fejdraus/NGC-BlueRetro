# Fork: RetroScaler NGC-BlueRetro (HW1, 4 LEDs, GameCube)

This firmware is built from three parts:

1. **The original BlueRetro firmware** by Jacques Gagnon (darthcloud), taken at
   upstream `master` (`e1a9831`), the final state of the project. It provides
   everything that actually makes the adapter work: the Bluetooth stack, the
   controller protocols and the wired console drivers.
2. **The functionality [RetroScaler](https://github.com/RetroScaler/NGC-BlueRetro)
   implemented for their NGC-BlueRetro adapters** — the four per slot connection
   indicator LEDs, and rumble enabled out of the box. RetroScaler publishes the
   binaries of their recent builds but not the sources (their repository is
   frozen at January 2023 code, while the shipped firmware is a build of
   upstream `ddfc31b` from June 2025), so that behaviour is reimplemented here
   on top of the current upstream code.
3. **Fixes and adaptations made specifically for compatibility with this
   hardware**, listed below: the LED hooks reworked so BLE only controllers are
   covered, hotplug brought back for HW1, the GameCube face buttons layout, the
   mapping toggle moved off View, and a few upstream defects repaired.

Target hardware: **RetroScaler NGC-BlueRetro**, ESP32, hardware revision
**HW1**, GameCube, four indicator LEDs on GPIO 14 / 32 / 33 / 25.

Prebuilt images are attached to the [releases](../../releases).

## Build variants

Both live in `configs/hw1/`, and both enable `BLUERETRO_HW2=n`, the 4 LEDs and
`BLUERETRO_SYSTEM_GC`:

| Config | Difference |
|---|---|
| `gamecube_4leds` | wired ports always answer the console, as the vendor firmware does |
| `gamecube_4leds_hotplug` | a port is announced only once a Bluetooth controller is connected to that slot |

## What this fork adds

All of it sits behind Kconfig options that default to `n`, so upstream builds
are bit for bit unaffected.

* **`RETROSCALER_BLUERETRO_4LEDS_HW`** — the four per slot LEDs. Lit from
  `bt_hid_init()` and cleared from `bt_host_reset_dev()`, which also covers BLE
  only controllers; the vendor firmware hooked three scattered call sites keyed
  on the device name read and missed them. Mutually exclusive with
  `BLUERETRO_HW2`, whose reset and port sense lines use the same pins, and
  refuses to build together with `BT_H4_TRACE` or `COVERAGE`, which claim
  GPIO 14 as an UART CTS.
* **`BLUERETRO_HW1_HOTPLUG`** — brings back the hotplug behaviour upstream
  dropped for HW1 in `9600f0d`. HW1 has no port sense lines, but hotplug never
  needed them: the port mask is derived from `BT_DEV_HID_INIT_DONE` alone and
  `nsi_port_cfg()` is pure GPIO and RMT work. Also restores the pre-`9600f0d`
  `nsi_init()` behaviour of starting silent.
* **`BLUERETRO_GC_XBOX_FACE_LAYOUT`** — GameCube face buttons match by **name**
  for Xbox and PlayStation pads. Upstream maps by position, which swaps X and B
  on those pads, while the alternate table only matches names for Nintendo
  layout pads, leaving Xbox users with no name match in either mode.
* **`BLUERETRO_GC_TOGGLE_ON_SHARE`** — moves the face buttons mapping toggle
  from View (`PAD_MS`) to Share (`PAD_MQ`), freeing View up to be mapped to a
  regular button. `PAD_MQ` is added to `gc_mask`, otherwise
  `adapter_map_from_btn()` would drop any mapping targeting it.

Unconditional fixes:

* **NSI RX channel re-arm** — a port can be disabled between RX End and TX End,
  which leaves the RMT channel with `rx_en` cleared while only TX End puts it
  back into RX, so the port would stay deaf after being enabled again. With
  hotplug that window is reachable on every controller drop.
* **Switch 2 HID init through the common dispatcher** —
  `bt_att_hid_start_next_state()` called `bt_hid_sw2_init()` directly, bypassing
  `bt_hid_init()` and everything hooked on it.
* **BDADDR load from NVS restored** — `f9461fa` commented the function out and
  left a hardcoded test MAC behind; that debug leftover shipped in `v25.10-beta`
  and in the vendor firmware.
* Rumble enabled in the factory config, as the vendor firmware does.

## Flashing

The adapter exposes a **CH340** USB serial bridge (`VID_1A86` / `PID_7523`).
Windows has no in-box driver for it, so the adapter shows up as an unknown
device until one is installed:

* Windows Update usually has it — Device Manager, *Update driver*, *Search
  automatically*, look for `wch.cn - Ports`
* or the vendor package: <https://www.wch-ic.com/downloads/CH341SER_EXE.html>

Beware of the many mirrors serving unsigned copies of `CH341SER.EXE`.

On at least one unit the bridge corrupts data above `115200` baud: reads broke
at 8% at `460800` and at 1.6% at `921600`. Flash and dump at `115200`.

## Building

Upstream builds standalone since v25.04, no BlueRetroRoot needed:

```
docker run --rm -v "$PWD":/project -w /project \
    ghcr.io/darthcloud/idf-blueretro:v5.5.0_2024-12-02 \
    bash -c 'source $IDF_PATH/export.sh && cp configs/hw1/gamecube_4leds_hotplug sdkconfig && idf.py build'
```

## Credits

BlueRetro is by [Jacques Gagnon (darthcloud)](https://github.com/darthcloud/BlueRetro),
Apache-2.0. All the hard parts are his work; this fork only adapts it to one
adapter.

The adapter itself, and the four LEDs behaviour reimplemented here, are by
[RetroScaler](https://github.com/RetroScaler/NGC-BlueRetro).

Everything below is the original upstream README.

---

**This repository has been archived and is no longer under active development or maintenance.**

```
After 6 years working on BlueRetro, the time has come for me to move on
and focus on my family and my real job.

The code remains available as-is for reference, learning, and community use,
but no new features, bug fixes, or pull requests will be accepted.

Thank you to everyone who contributed, tested, reported issues,
or supported the project over the years.
Your involvement made BlueRetro what it is today.

If you are looking to build upon this work, please feel free to
fork the repository in accordance with the license.

Thank you,
Jacques Gagnon
```

# BlueRetro

<p align="center"><img src="/static/PNGs/BRE_Logo_Color_Outline.png" width="600"/></p>
<br>
<p align="justify">BlueRetro is a multiplayer Bluetooth controllers adapter for various retro game consoles & computers. Lost or broken controllers? Reproduction too expensive? Need those rare and obscure accessories? Just use the Bluetooth devices you already got! The project is open source hardware & software under the CERN-OHL-P-2.0 & Apache-2.0 licenses respectively. It's built for the popular ESP32 chip. Wii, Switch, PS3, PS4, PS5, Xbox One, Xbox Series X|S & generic HID Bluetooth (BR/EDR & LE) devices are supported. Parallel 1P (Computers, NeoGeo, Supergun, JAMMA, Handheld, etc), Parallel 2P (Atari 2600/7800, Master System, Computers, etc), NES, PCE / TG16, Mega Drive / Genesis, SNES, CD-i, 3DO, Jaguar, Saturn, PSX, PC-FX, JVS (Arcade), Virtual Boy, N64, Dreamcast, PS2, GameCube & Wii extension are supported with simultaneous 4+ players using a single adapter.</p>

## READ THIS FIRST
* [Project documentation](https://github.com/darthcloud/BlueRetro/wiki)

## Need help?
* [Open a GitHub discussion](https://github.com/darthcloud/BlueRetro/discussions)

## Makers sponsoring BlueRetro
Buying BlueRetro adapters from these makers helps support the continued development of the BlueRetro firmware!\
Thanks to all sponsors!

* [Laser Bear Industries](https://www.laserbear.net)
* [Humble Bazooka](https://www.humblebazooka.com)
* [RetroOnyx](https://www.retroonyx.com/)
* [RetroTime](https://8bitmods.com/retrotime)

## Community Contribution
* BlueRetro PS1/2 Receiver by [mi213](https://twitter.com/mi213ger): 3D printed case & PCB for building DIY PS1/2 dongle.\
  https://github.com/Micha213/BlueRetro-PS1-2-Receiver
* N64 BlueRetro Mount by [reventlow64](https://twitter.com/reventlow): 3d printed mount for ESP32-DevkitC for N64.\
  https://www.prusaprinters.org/prints/90275-nintendo-64-blueretro-bluetooth-receiver-mount
* BlueRetro Adapter Case by [Sigismond0](https://twitter.com/Sigismond0): 3d printed case for ESP32-DevkitC.\
  https://www.prusaprinters.org/prints/116729-blueretro-bluetooth-controller-adapter-case
* BlueRetro AIO by [pmgducati](https://github.com/pmgducati): BlueRetro Through-hole base and cable PCBs.\
  https://github.com/pmgducati/Blue-Retro-AIO-Units
* BlueRetro HW2 internal guides by [Nostalgic Indulgences](https://twitter.com/nosIndulgences): Internal install guides\
  https://github.com/nostalgic-indulgences/BlueRetro_Internal_Installation
* BlueRetro latency test by [GamingNJncos](https://twitter.com/GamingNJncos): Documentation on how to run BlueRetro latency test\
  https://github.com/GamingNJncos/BLE-3D-Saturn-Public/tree/main/BlueRetro_Latency_Testing
* BR4N64 by [TharathielCB](https://github.com/TharathielCB): Internal BlueRetro Flex-PCB for Nintendo 64\
  https://github.com/TharathielCB/BR4N64
* BlueMemCard by [ChrispyNugget](https://github.com/ChrispyNugget): Replacement PCB for PSX memory card that incorporates PicoMemcard and BlueRetro support\
  https://github.com/ChrispyNugget/BlueMemCard
* BlueRetro HW2 QSB for GameCube by [Arthrimus](https://github.com/Arthrimus): Internal Blueretro PCB with CurrentTrigger for GameCube\
  https://github.com/Arthrimus/BlueRetro-HW2-GameCube

<br><p align="center"><img src="https://cdn.hackaday.io/images/4560691598833898038.png" height="200"/></p>
