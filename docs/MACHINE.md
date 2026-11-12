# The jam PC

Recorded 2026-11-12 by the setup session (`setup/auto.md`).

| Item | Value |
|---|---|
| Windows | 11 Pro, build 26100 |
| CPU | Intel Core i7-11800H (8 cores, laptop) |
| RAM | 32 GB |
| Graphics | NVIDIA GeForce RTX 3060 Laptop GPU, driver 32.0.16.1692 (2026-09-04); Parsec Virtual Display Adapter |
| Disk 0 | Samsung MZVL2512HCJQ 512 GB NVMe SSD: `C:` (476 GB NTFS), **18 GB free at setup** |
| Disk 1 | Samsung 980 PRO 1 TB NVMe SSD: one 930 GB non-Windows partition (the other OS), no drive letter. Not touched. |
| Address | 192.168.100.25 (Ethernet, gateway 192.168.100.1) |
| Windows user | Marwan |

## Installed

| Tool | Version | Note |
|---|---|---|
| Epic Games Launcher | 1.3.210.0 (winget reinstalled the 20.3.3 MSI) | |
| Unreal Engine | 5.9.0 Preview (installing) | `C:\Epic Games\UE_5.9`; Core, Templates and Feature Packs, Engine Source; no symbols, no extra platforms; 33 GB |
| Visual Studio Community 2022 | 17.14.45 | Desktop C++, .NET desktop, **Game development with C++**; MSVC 14.44.35207; Windows SDK 10.0.26100 and 10.0.19041 |
| Git / Git LFS | 2.56.0 / 3.3.0 | |
| 7-Zip | 26.06 | |
| OBS Studio | 28.1.2 | winget upgrade failed (installer exit 6, files in use); 28.1.2 records fine |
| Sunshine | 2026.914 | service running, web UI on 47990 |
| Claude Code | 2.1.322 | |

## Decisions

| Finding | Decision |
|---|---|
| 18 GB free on `C:`, the only Windows drive | Cleared by the `disk-cleanup` session with Marwan: 147 GB free before the engine and VS update. |
| Internet about 10-18 Mbps down (Ethernet link negotiates at only 100 Mbps) | The 13 GB engine download takes 2-3 hours. Plan big downloads (Fab assets, engine updates) early. |
| Installers and the Launcher's folder step need UAC; prompts cancel themselves after about 2 minutes | Raise them only while Marwan watches through Moonlight. |
| GPU driver from 2026-09 | Current enough. |
| NVIDIA RTX card | RTX award possible (DLSS). |
| RAM 32 GB | No limit noted; still close the editor while packaging if memory runs short. |
