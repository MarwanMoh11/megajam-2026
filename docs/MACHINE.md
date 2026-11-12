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
| Unreal Engine | none yet | 5.9 MegaJam Preview to install from the Launcher |
| Visual Studio Community 2022 | 17.6.2 | Desktop C++ and .NET desktop workloads; **Game development with C++ missing** |
| Git / Git LFS | 2.56.0 / 3.3.0 | |
| 7-Zip | 26.06 | |
| OBS Studio | 28.1.2 | winget upgrade failed (installer exit 6, files in use); 28.1.2 records fine |
| Sunshine | 2026.914 | service running, web UI on 47990 |
| Claude Code | 2.1.322 | |

## Decisions

| Finding | Decision |
|---|---|
| 18 GB free on `C:`, the only Windows drive | **Blocker.** Engine (~60-120 GB) and the VS update can't fit. Marwan decides what to clear; see DEVLOG. |
| GPU driver from 2026-09 | Current enough. |
| NVIDIA RTX card | RTX award possible (DLSS). |
| RAM 32 GB | No limit noted; still close the editor while packaging if memory runs short. |
