[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![JUCE](https://img.shields.io/badge/Built%20with-JUCE%208-blue)](https://juce.com)
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey)]()
[![Format](https://img.shields.io/badge/Format-VST3%20%7C%20-orange)]()

## Author: **William Ashley** — AlphaAudio
## Project: *The Its Loud Tool* (VIAU-MAX)

**Purpose:** Loudness maximization, dynamics control, and real-time broadcast-style metering.

# The Its Loud Tool

A powerful loudness enhancement and monitoring utility that brings your mixes up to competitive streaming/broadcast levels while preserving punch and transparency. Built with **JUCE 8** as a VST3 (and AU/AAX compatible) plugin.

It combines perceptual auto-makeup gain, harmonic saturation (exciter), a transparent lookahead limiter, and accurate BS.1770-4 loudness metering in one cohesive rack-style interface.

## What It Does

The Its Loud Tool is designed for final-stage loudness management — perfect for mastering, streaming prep, broadcast, or live tracking where you need both loudness and clarity.

### Key Features
| Section | Function |
|---------|----------|
| **Perceptual Gain** | Smart auto-makeup that targets your chosen LUFS (Momentary-based) with smooth, musical response. |
| **Texture / Harmonic Exciter** | Oversampled asymmetric tanh saturation for adding density and presence without harshness (drive 1–5x). |
| **True Peak Limiter** | 5ms lookahead brickwall limiter with ceiling control. Protects against inter-sample peaks. |
| **Loudness Metering** | Full BS.1770 metering: **Integrated**, **Short-Term**, **Momentary** LUFS + True Peak + held peak. |
| **Monitoring Tools** | Cycle between metering modes, reset integrated loudness, export CSV snapshot, bypass. |

## Signal Flow

Stereo In → Perceptual Makeup Gain (target LUFS)  
→ Oversampled Harmonic Exciter (saturation)  
→ Lookahead True Peak Limiter (ceiling)  
→ BS.1770 Metering (final output) → Stereo Out

- **Oversampling**: 2x with high-quality FIR half-band filters.
- **Latency**: Reported to host (oversampling + lookahead).
- **Mono compatibility**: Handles mono inputs gracefully.
- All parameters use atomic variables for thread-safe UI ↔ Audio communication.

## UI Design

- **Rack-mount aesthetic** with dark chassis, side ears, and screws.
- Custom **BroadcastKnob** LookAndFeel with glowing teal accents and detailed rotary graphics.
- Large **VU-style meter** with needle, peak LED, and tick marks.
- LCD-style digital readout showing real-time Integrated / Short / Momentary LUFS, mode, and True Peak.
- Compact, resizable editor window (default **750 × 340 px**).

## Controls
- **TARGET LUFS** (-30 to -8): Desired loudness target (uses smoothed Momentary measurement).
- **TEXTURE** (1–5x): Drive amount for the harmonic exciter.
- **CEILING** (-10 to 0 dBTP): Limiter output ceiling.
- Buttons: RESET INT (Integrated), MODE (cycle metering display), EXPORT CSV, BYPASS.

## Technical Notes
- **Sample-accurate** parameter handling with atomic floats.
- Custom **BS.1770-style** K-weighted filtering for loudness calculation.
- Ring buffers for Momentary (400ms), Short-Term (3s), and Integrated measurements.
- True Peak detection with hold and decay.
- State saving is minimal (parameters are host-managed via APVTS-ready structure; full serialization can be expanded).

## Factory / Default Behavior
Starts with sensible defaults:
- Target: **-14 LUFS**
- Texture: **2.0x**
- Ceiling: **-1 dBTP**

Version 1.0 — GUI polish and additional presets/modes planned for v2.

## Building
Requires **JUCE 8** and a C++20 compiler. Standard JUCE audio plugin project.  
The compiled `.vst3` is included in the repo root for quick testing.

## Quick Install (Windows)
Copy the `theitsloudtool.vst3` folder to:  
`C:\Program Files\Common Files\VST3\`  
Rescan plugins in your DAW. It appears as **The Its Loud Tool** (or VIAU-MAX internally).

## Website & Contact
- Website: [williamashley.music](http://williamashley.music)
- Email: contact@williamashley.music

---

Copyright (c) 2026 William Ashley d/b/a William Ashley Music ( http://WilliamAshley.music )

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License (v3).

This program is distributed in the hope that it will be useful to other audio programmers and music makers. There is no WARRANTY expressed or implied.

Attribution is requested where possible. Notice of use is appreciated.

---

## Third-Party Licenses
Built using the **JUCE framework** (© Raw Material Software Limited).  
See [JUCE License](https://juce.com) for details.

VST3 is a trademark of Steinberg Media Technologies GmbH.

---

**Enjoy making it loud — transparently.**
