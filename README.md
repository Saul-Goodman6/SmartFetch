<h1 align="center">SmartFetch</h1>

<p align="center">
  <img
    src="https://github.com/user-attachments/assets/2cc55b49-ffb5-4ddc-9e70-eefc43bf9135"
    alt="SmartFetch Logo"
    width="300">
</p>



<p align="center">
  A lightweight, blazing-fast system information tool written in pure C.
</p>

<p align="center">
  <img src="https://img.shields.io/github/license/Saul-Goodman6/SmartFetch?style=for-the-badge" alt="License">
  <img src="https://img.shields.io/github/stars/Saul-Goodman6/SmartFetch?style=for-the-badge" alt="Stars">
  <img src="https://img.shields.io/github/forks/Saul-Goodman6/SmartFetch?style=for-the-badge" alt="Forks">
  <img src="https://img.shields.io/github/issues/Saul-Goodman6/SmartFetch?style=for-the-badge" alt="Issues">
</p>

---

<p align="center">

  <img width="1366" height="768" alt="image" src="https://github.com/user-attachments/assets/31cc2325-4f74-4ec2-952c-351b5c446828" alt="Preview_Image" />

  
</p>

## Features

- **Blazing Fast:** Built with pure C and Direct System APIs for minimal resource usage and maximum performance.
- **Dynamic ASCII Art:** Automatically detects your Linux distribution (`Fedora`, `Arch`, `Debian`, etc.) and renders the corresponding logo.
- **Terminal Color Palette:** Displays a colorful 16-color block palette at the bottom for aesthetic screenshots.
- **Detailed System Insights:** Displays CPU specs, real-time temperatures, RAM type & usage, display resolution, disk space, GPU, and OS age/uptime.
- **Standard Build System:** Uses a clean `Makefile` for simple, scalable compilation across platforms.

---

## Build & Installation

Clone the repository and compile using `make`:

```bash```
git clone https://github.com/Saul-Goodman6/SmartFetch.git
cd SmartFetch
make
sudo make install

<img width="800" height="449" alt="sfetch" src="https://github.com/user-attachments/assets/62994abe-982b-47eb-8827-0029a0fad86b" />

## Windows Support (Beta)

```Compile using GCC / MinGW```

```gcc main.c ui.c collect_windows.c -o sfetch.exe -ladvapi32```

```sfetch.exe```
