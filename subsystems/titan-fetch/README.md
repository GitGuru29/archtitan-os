# TitanFetch

> **Owner:** [@GitGuru29](https://github.com/GitGuru29)  
> **Subsystem:** TitanFetch  
> **Language:** C++ / Qt6  
> **Status:** ✅ Active Development

## Overview

TitanFetch is the system information display tool for ArchTitan OS — a custom neofetch-style utility with a Qt6 GUI and CLI mode. It pulls real-time hardware and OS metadata and renders it in the Titan aesthetic.

## Features

- CLI mode for terminal display
- Qt6 GUI with live hardware readout
- CPU, GPU, RAM, kernel, OS, uptime info
- Themed to ArchTitan visual identity

## Folder Structure

```
titan-fetch/
├── src/        ← Source code (link to /titanfetch-src at repo root)
├── tests/
├── docs/
├── configs/
└── README.md
```

## Build

```bash
cd src/
cmake -B build && cmake --build build
```

## Related

- Source: [`/titanfetch-src`](../../titanfetch-src/)
- Wiki: [`/wiki/TitanFetch.md`](../../wiki/TitanFetch.md)
