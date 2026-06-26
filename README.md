# ⚡ FiveM Strafe Macro

[![Language](https://img.shields.io/badge/Language-C%2B%2B17-blueviolet.svg)](#)
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)](#)
[![Backend](https://img.shields.io/badge/Graphics-DirectX%2011-cyan.svg)](#)
[![GUI Library](https://img.shields.io/badge/GUI-Dear%20ImGui-orange.svg)](#)

A professional, high-performance, external keyboard movement macro designed specifically for **Grand Theft Auto V** and **FiveM**. Fully written in optimized **C++17**, utilizing a hardware-accelerated **Dear ImGui** interface powered by **DirectX 11** for minimum CPU/GPU overhead.

Developed with a multi-layered humanization engine and built-in stealth defenses to remain entirely undetected by server-side behavior trackers.

---

## 🚀 Key Features

### 🧠 1. Multi-Layered Humanization Engine
Unlike standard macros that use basic uniform random ranges (which are easily flaggable by server-side machine learning filters), this macro simulates realistic hand-movement biology:
*   **Gaussian Jitter (Layer 1)**: Keypress delays follow a normal distribution curve centered around your target delay.
*   **Micro-Jitter (Layer 2)**: Adds a 50% chance of a tiny delay (1-5 ms) right before keystroke switches to simulate natural skin contact delay.
*   **Periodic Attention Drift (Layer 3)**: Shifts the baseline delay slightly every 15-30 cycles to simulate focus loss or hand adjustment.
*   **Natural Stutters (Layer 4)**: Injects occasional minor hesitation pauses (3% chance of a 30-80 ms pause).
*   **Fatigue Emulation (Layer 5 - Optional)**: Simulates physical hand fatigue by gradually introducing a minor timing penalty over minutes of continuous activation.

### 🛡️ 2. Advanced Stealth & Bypass
- **Dynamic Window Class**: Class registration generates a random 12-letter class name on each launch to block basic class-name signature lookups.
- **Dynamic Title Spoofing**: Change the window title (e.g., to `chrome` or `spotify`) on the fly inside the Advanced settings to evade window search hooks.
- **Direct Scan Codes**: Simulates key inputs using low-level keyboard scan codes (`KEYEVENTF_SCANCODE`) via `SendInput()`, bypassing OS-level virtual key blocks.
- **100% External**: Runs entirely outside game memory space; does not read/write memory or hook game dlls.

### 🏃 3. Contextual Movement & Strafe Patterns
Includes **8 custom movement sequences** tailored for various tactical scenarios (siper peek, open area, sliding momentum):

| Pattern | Description | Best Use Case |
| :--- | :--- | :--- |
| **`A <-> D (Classic)`** | Rapid alternating left/right strafe. | Classic peek fights and cover shooting. |
| **`A -> D -> A (Triple)`** | Triple-tap side movement rhythm. | Unpredictable wide-swing peaking. |
| **`Strafe Left Only`** | Single directional tap looping. | Peeking left side of a wall. |
| **`Strafe Right Only`** | Single directional tap looping. | Peeking right side of a wall. |
| **`WASD` / `WDSA`** | Circular clockwise/counter-clockwise. | Evading fire in open spaces. |
| **`DSAW` / `WADS`** | Alternative circular movements. | Alternative evasive trajectories. |

- **Optional Auto-Sprint (Shift)**: Automatically holds down Left Shift while strafe cycles run to maintain GTA V's running/sliding momentum.

---

## 🛠️ Build Instructions

### Requirements
- **Windows OS** (Win 10/11)
- **Visual Studio 2019/2022** with Desktop Development with C++ (MSVC compiler)
- **CMake** (v3.20 or newer)

### Compiling (Recommended)
1. Open a terminal (PowerShell / Command Prompt) in the repository root.
2. Run the following commands to configure and compile a fully optimized, self-contained release executable:

```bash
# Generate build files
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Compile release binary
cmake --build build --config Release
```

The resulting binary `StrafeMacro.exe` will be located in `build/Release/`. Because the CMake configuration enforces **Static CRT Linkage (`/MT`)**, the executable has zero dependencies and runs as a standalone file.

---

## 📖 Configuration & Best Practices

### 1. Optimizing Leaning Animations (GTA V Physics)
- Very fast delays (e.g., under 50ms) do not give the GTA V physics engine enough time to register character side-leaning.
- **Recommended Settings**:
  - `Base Delay`: `120 ms`
  - `Min Delay`: `80 ms`
  - `Max Delay`: `160 ms`
  - `Jitter / Random`: `15% - 25%` (Enables the humanization engine).

### 2. Resolving Mouse / Trigger Conflicts
If you assign the trigger key to a mouse side button (e.g., Mouse 5) and experience dropped left-clicks (shooting) while strafing, this is caused by a hardware/driver queue conflict or game mechanic:
- **GTA V Sprint Restriction**: Holding down Shift (running) naturally blocks primary shooting/punching in GTA V. If you need to shoot instantly during a strafe, **turn off** the `Auto Sprint` checkbox.
- **Hardware Bus Conflicts**: Some mouse software (Logitech G-Hub, Razer Synapse) limits polling rate when multiple mouse buttons are held and virtual keys are spammed.
  - *Mitigation*: Map your mouse side button to a neutral keyboard key (like `F24`, `P`, or `L`) inside your mouse driver software, then set that key as the trigger in the macro.

---

## 🕵️ Safe Usage Guidelines
1. **Rename the Executable**: Always rename the compiled file (e.g., rename `StrafeMacro.exe` to `svchost_helper.exe` or `lgHub.exe`) before starting it.
2. **Run as Administrator**: If your FiveM launcher runs with administrative privileges, you must run the macro as an Administrator so Windows permits input injection.
3. **Randomization**: Always keep the `Jitter / Random` slider above `15%` when playing on public servers with active monitoring scripts.

---

## 🧑‍💻 Credits & License
- **Author**: [Nosvemos](https://github.com/)
- This project is compiled using the open-source [Dear ImGui](https://github.com/ocornut/imgui) library under the MIT License.
