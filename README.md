# FFM-EXTERNAL-KDrivers (Neo FF)

**FFM-EXTERNAL-KDrivers** is a comprehensive, kernel-level external framework designed for Android. It leverages Kernel Patch Modules (KPM) to bypass standard Android sandboxing, allowing for privileged memory manipulation, synthetic input injection, and external overlay rendering.

> ⚠️ **Disclaimer:** This project is provided strictly for **educational, reverse engineering, and research purposes**. Utilizing kernel drivers to manipulate game memory violates the Terms of Service of most applications (including Free Fire Max) and may result in account bans. Use at your own risk.

---

## 🚀 Features

### Kernel Driver (`na_driver.kpm`)
- **Syscall Hooking:** Inline hooks the `__arm64_sys_prctl` system call using `hook_func`.
- **Covert Communication:** Uses a custom `prctl` magic (`0x4E41` / "NA") to communicate between user-space and kernel-space securely.
- **Memory Operations:** 
  - Virtual Memory Read/Write (via `access_process_vm`).
  - Physical Memory Read/Write (via `memremap`).
- **Process Management:** Retrieve target PID by package name without relying on user-space `/proc` parsing.
- **Input Injection:** Direct kernel-level touch event injection (simulates screen taps/swipes via `vfs_write` to input devices).

### External Application (`FFM-EXTERNAL`)
- **External Overlay:** Hardware-accelerated ImGui overlay for drawing ESP (Extra Sensory Perception).
- **Silent Aim:** Background thread calculating aim adjustments and injecting touch events directly into the kernel.
- **Auto-Launch:** Automatically launches the target application (`com.dts.freefiremax`) upon execution.
- **Volume Key Menu:** Opens/closes the GUI configuration menu via the physical Volume Up key.
- **Thermal Pacing:** Dynamic frame-pacing system (`std::chrono`) to prevent device overheating during long sessions.

### Magisk/APatch Module (`KPatch-Next-Module`)
- **Boot-to-Root:** Automatically loads the `.kpm` driver during the Android boot sequence.
- **Package Exclusions:** Support for an exclusion list to ensure the driver does not interfere with critical system processes.
- **Clean Uninstallation:** Automatically unhooks and removes the driver when the module is removed via Magisk/APatch.

---

## 📁 Repository Structure

```text
FFM-EXTERNAL-KDrivers/
├── #KPM DRIVER/          # Kernel-space code (The Backdoor)
│   ├── na_driver.c       # Main KPM C source code
│   └── Neo_driver.kpm    # Compiled kernel module
├── FFM-EXTERNAL/         # User-space code (The App)
│   ├── includes/         # Header files (ImGui, Drawing, Memory)
│   ├── jni/              # Android NDK build configs (Application.mk)
│   ├── src/              # Main C++ logic (main.cpp, ESP, Silent Aim)
│   ├── compile.bat       # Windows build script
│   └── compile.sh        # Linux/Mac build script
└── KPatch-Next-Module/   # Root Module Installer
    ├── module.prop       # Module metadata
    ├── service.sh        # Boot loader for the KPM
    ├── post-fs-data.sh   # Early boot setup
    └── ...               # Installation and uninstallation scripts
```

---

## ⚙️ How It Works (Architecture)

1. **Installation:** The user flashes the `KPatch-Next-Module` ZIP via Magisk, KernelSU, or APatch.
2. **Boot Sequence:** On boot, `service.sh` executes. It pushes `Neo_driver.kpm` into the kernel using the `kpatch` utility.
3. **Kernel Hook:** `na_driver.c` resolves the `__arm64_sys_prctl` symbol via `kallsyms_lookup_name` and hooks it.
4. **User-Space Execution:** The `FFM-EXTERNAL` binary is run via ADB or a terminal emulator. It requests the `prctl(0x4E41, CMD_READ_MEM, ...)` syscall.
5. **Interception:** The kernel intercepts the `prctl` call, verifies the `0x4E41` magic, reads the target game's memory, and returns the data to the external app.
6. **Rendering:** The external app processes the memory data, calculates enemy positions, and draws the ESP overlay on the screen.

---

## 🛠️ Prerequisites & Building

### Prerequisites
* A rooted Android device running an ARM64-v8a kernel.
* Root solution: **KernelSU**, **APatch**, or **Magisk** (with KPM support).
* Android NDK installed on your development machine.

### Building the External App
Navigate to the `FFM-EXTERNAL` directory and run the compilation script:

**Windows:**
```bash
compile.bat
```
**Linux/macOS:**
```bash
chmod +x compile.sh
./compile.sh
```
The compiled binary will be generated inside the `obj/local/arm64-v8a/` directory.

---

## 📜 Usage Protocol (Prctl Commands)

The driver exposes the following operations via the `0x4E41` prctl magic:

| Command ID | Name | Description |
| :--- | :--- | :--- |
| `0x01` | `CMD_READ_MEM` | Read virtual memory of a specific PID. |
| `0x02` | `CMD_WRITE_MEM` | Write to the virtual memory of a specific PID. |
| `0x03` | `CMD_READ_PHYS` | Read physical memory address. |
| `0x04` | `CMD_WRITE_PHYS` | Write to a physical memory address. |
| `0x05` | `CMD_GET_PID` | Retrieve PID via package name string. |
| `0x06` | `CMD_INJECT_TOUCH` | Inject synthetic touch coordinates (Silent Aim). |
| `0x07` | `CMD_FIND_INPUT` | Locate the display/input service process. |
| `0x08` | `CMD_GET_SCREEN` | Retrieve active screen width and height. |

---

## 🤝 Contributing & License

This project utilizes the **GPL v2** license for the kernel module components (`na_driver.c`), as it interfaces directly with the Linux kernel. The userspace C++ components are provided as-is for research purposes.

**Author:** `@cache_file` (As defined in the KPM headers)
