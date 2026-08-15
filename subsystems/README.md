# ArchTitan OS — Subsystems

This folder contains all subsystems of ArchTitan OS. Each subsystem has an **identical folder layout** so every team member works in a predictable, consistent structure.

> **Note:** Titan Settings is a **core OS component**, not a subsystem. Its source lives at [`archtitan-settings/`](../archtitan-settings/) in the repository root — alongside `titan-hwm-source/`, `titanfetch-src/`, and `sandbox/`.

## Subsystem Index

| Subsystem | Owner | Status | Folder |
|---|---|---|---|
| Titan Hardware Manager | @GitGuru29 | ✅ Active | [`titan-hwm/`](titan-hwm/) |
| Titan Media HUD | @GitGuru29 | ✅ Active | [`titan-media-hud/`](titan-media-hud/) |
| TitanFetch | @GitGuru29 | ✅ Active | [`titan-fetch/`](titan-fetch/) |
| Titan Sandbox | @GitGuru29 | ✅ Active | [`titan-sandbox/`](titan-sandbox/) |
| Auto GPU Switcher | @GitGuru29 | 🚧 Planned | [`auto-gpu-switcher/`](auto-gpu-switcher/) |
| TITAN AI | Teammate | 🚧 In Development | [`titan-ai/`](titan-ai/) |
| TITAN Task Manager | Teammate | 🚧 In Development | [`titan-task-manager/`](titan-task-manager/) |
| TITAN Share | Teammate | 🚧 In Development | [`titan-share/`](titan-share/) |
| TITAN Mirror | Teammate | 🚧 In Development | [`titan-mirror/`](titan-mirror/) |

## Standard Subsystem Layout

Every subsystem folder follows this identical structure:

```
subsystems/<subsystem-name>/
├── src/        ← All source code for this subsystem
├── tests/      ← Unit, integration, and service tests
├── docs/       ← Subsystem-specific documentation
├── configs/    ← Default config files that ship into the OS
└── README.md   ← Overview, build instructions, dependencies
```

## Contributing

- Each member works **only inside their own subsystem folder(s)**
- Open a PR targeting `main` when your subsystem feature is ready
- Update your subsystem's `README.md` with build steps and dependencies before merging
