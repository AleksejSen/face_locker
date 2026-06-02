# 📑 Face Locker

An automated, smart background daemon for Linux that monitors webcam presence via face recognition and locks your desktop session instantly if you step away or if an intruder is detected.

---

## ✨ Features
* **Zero-Setup Configuration**: Automatically generates a local configuration directory and template file on its first run.
* **Smart Lock Detection**: Checks active system processes and DBus pathways to avoid wasting camera resources or processing frames when your screen is already locked.
* **Instant Shutdown Handling**: Uses thread-safe condition variables to terminate instantly on `Ctrl+C` or systemd `SIGTERM`.
* **Desktop Agnostic**: Supports GNOME, KDE, Sway, i3, Hyprland, and standard systemd `loginctl` sessions.

---

## 🛠️ Building & Installing

The repository includes a comprehensive `Makefile` to compile the engine, generate an optimized Debian `.deb` installer bundle, and register system components.

### Prerequisites
Ensure your system has a C++17 compliant compiler, CMake, and OpenCV core modules installed:
```bash
sudo apt update
sudo apt install build-essential cmake libopencv-dev
```

### Installation Steps

1. **Build and Package**: Compile the source binaries and pack them directly into a fresh Debian distribution bundle.
   ```bash
   make build-deb
   ```
2. **Install**: Deploy the binary architecture along with preconfigured `systemd` installation hooks directly to your system.
   ```bash
   make install-deb
   ```
3. **Uninstall**: Cleanly purge deployed application binaries and registered services from your environment.
   ```bash
   make uninstall-deb
   ```

---

## ⚙️ Configuration & Priority Logic

Face Locker uses a multi-tiered approach to determine its runtime properties. Settings are evaluated in a specific hierarchy: **Command Line Arguments Override Configuration File Settings**.

### 1. Configuration File
On its very first launch, the daemon generates a local environment configuration file at:
📂 `~/.config/face-locker/config.env`

To set up your permanent face verification baseline, open this file and configure your absolute reference image path:
```env
# Face Locker Configuration File
REFERENCE_PICTURE="/home/alex/Pictures/my_profile.jpg"
INTERVAL=10
DEBUG_MODE=false
```

### 2. Command Line Arguments
You can explicitly override any parameters written in your `config.env` file by passing variables directly into the terminal execution string.


| Flag | Long Option | Description | Example |
| :--- | :--- | :--- | :--- |
| `-r` | `--reference_picture` | **Absolute path** to your baseline face profile image. | `-r /tmp/face.jpg` |
| `-i` | `--interval` | Frequency of webcam capture evaluations in **seconds**. | `-i 5` |
| `-d` | `--debug` | Displays active GUI windows mapping webcam loops and bounding boxes. | `-d` |

### 3. Priority Matrix Examples

* **Scenario A (Using Filesystem Configuration)**: You run `face_locker` with no arguments. It checks `~/.config/face-locker/config.env`, extracts your configured image path, and executes.
* **Scenario B (Temporary Command Override)**: Your file says `INTERVAL=10`, but you execute `face_locker -i 2`. The application will run tests every **2 seconds**, overriding the file default.
* **Scenario C (Safe Fallback Validation)**: If no profile photo path is specified in either the file or the command arguments, the app terminates safely before touching the camera hardware to prevent initialization crashes.
