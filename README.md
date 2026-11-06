# FREELY — Developer Test

FREELY is an experimental PlayStation 4 homebrew diagnostics project.

## Goals

- Display a terminal-style startup banner and timestamped diagnostic messages.
- Report only information that the program can actually query.
- Save diagnostic reports in a writable application data location.
- Mark unsupported checks as `N/A` rather than inventing results.

## Safety and scope

This project is read-only. It does not spoof firmware versions, modify system settings, patch the kernel, or change console identity.

## Build status

This repository is an early development scaffold. A PS4 PKG must not be considered ready until the source has been built with the OpenOrbis toolchain, packaged, and tested on suitable hardware.

## Toolchain

OpenOrbis PS4 Toolchain: https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain
