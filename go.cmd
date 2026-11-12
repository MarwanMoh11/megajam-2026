@echo off
rem Starts the automatic PC setup: Claude Code with all permissions and Remote Control on, given setup/auto.md.
rem Run it from an administrator terminal, after cloning this repo to C:\Jam\megajam. See README.md.
cd /d "%~dp0"
claude "Read setup/auto.md and carry it out." --dangerously-skip-permissions --remote-control jam-setup
