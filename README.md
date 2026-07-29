<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 900 300" width="900" height="300">
  <defs>
    <linearGradient id="bg" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" stop-color="#0f172a"/>
      <stop offset="100%" stop-color="#1e1b4b"/>
    </linearGradient>
    <linearGradient id="accent" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" stop-color="#22d3ee"/>
      <stop offset="100%" stop-color="#a855f7"/>
    </linearGradient>
    <filter id="softShadow" x="-20%" y="-20%" width="140%" height="140%">
      <feDropShadow dx="0" dy="4" stdDeviation="6" flood-color="#000" flood-opacity="0.35"/>
    </filter>
  </defs>

  <!-- Icon: rounded terminal window -->
  <g filter="url(#softShadow)">
    <rect x="30" y="30" width="240" height="240" rx="48" fill="url(#bg)"/>
    <rect x="30" y="30" width="240" height="240" rx="48" fill="none" stroke="url(#accent)" stroke-width="4"/>

    <!-- window dots -->
    <circle cx="66" cy="66" r="7" fill="#f87171"/>
    <circle cx="90" cy="66" r="7" fill="#fbbf24"/>
    <circle cx="114" cy="66" r="7" fill="#34d399"/>

    <!-- prompt chevron -->
    <path d="M62 130 L100 160 L62 190" fill="none" stroke="url(#accent)" stroke-width="12" stroke-linecap="round" stroke-linejoin="round"/>

    <!-- speed/smart spark -->
    <path d="M150 110 L128 165 L152 165 L138 210 L192 148 L164 148 Z" fill="url(#accent)"/>

    <!-- underline cursor -->
    <rect x="110" y="205" width="90" height="10" rx="5" fill="url(#accent)" opacity="0.85"/>
  </g>

  <!-- Wordmark -->
  <text x="310" y="150" font-family="'JetBrains Mono','Fira Code',monospace" font-size="72" font-weight="700" fill="#0f172a">smart</text>
  <text x="310" y="150" font-family="'JetBrains Mono','Fira Code',monospace" font-size="72" font-weight="700" fill="url(#accent)" dx="0">
    <tspan dx="0"></tspan>
  </text>
  <text x="514" y="150" font-family="'JetBrains Mono','Fira Code',monospace" font-size="72" font-weight="700" fill="url(#accent)">fetch</text>
  <text x="310" y="190" font-family="'Inter','Segoe UI',sans-serif" font-size="24" letter-spacing="4" fill="#64748b">SYSTEM INFO, FASTER</text>
</svg>
<img width="900" height="300" alt="smartfetch-logo" src="https://github.com/user-attachments/assets/73e2fa3a-8b4c-4ee1-9009-5cceacd9302a" />
<h1 align="center">SmartFetch</h1>

<p align="center">
A lightweight and fast system information tool written in C.
</p>

<p align="center">
  <img src="https://img.shields.io/github/license/Saul-Goodman6/SmartFetch?style=for-the-badge" alt="License">
  <img src="https://img.shields.io/github/stars/Saul-Goodman6/SmartFetch?style=for-the-badge" alt="Stars">
  <img src="https://img.shields.io/github/forks/Saul-Goodman6/SmartFetch?style=for-the-badge" alt="Forks">
  <img src="https://img.shields.io/github/issues/Saul-Goodman6/SmartFetch?style=for-the-badge" alt="Issues">
</p>

---

## Features

- **Blazing Fast:** Built with pure C for minimal resource usage and maximum performance.
- **Dynamic ASCII Art:** Automatically detects your Linux distribution (`Fedora`, `Arch`, `Debian`, etc.) and renders the corresponding logo.
- **System Insights:** Displays CPU specs, real-time temperatures, RAM usage, display resolution, disk space, and OS age.
- **Easy Installation:** Includes an automated setup script for system-wide binary availability.

---

## Installation

Clone the repository and run the installation script:

```bash
git clone https://github.com/Saul-Goodman6/SmartFetch.git
cd SmartFetch
chmod +x install.sh
./install.sh
```
## Usage


Once installed, simply execute:
``` sfetch ```

## Build With
Bash
sfetch
🛠️ Built With
C Language
GCC Compiler
Linux System APIs & Utilities

## Example Of the project

<img width="1366" height="768" alt="sfetch_in_fedora" src="https://github.com/user-attachments/assets/2ac6b5e4-b9fc-4436-9bea-db0b390622e0" />
