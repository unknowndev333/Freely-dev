# FREELY

FREELY is an experimental PlayStation 4 homebrew diagnostics application.

## Current prototype

- Terminal-style startup screen with the FREELY banner.
- Timestamped diagnostic log messages.
- Attempts to save a session log to the app's writable data location.
- Unsupported hardware and firmware checks are marked `N/A`.

## Package

The GitHub Actions workflow creates a fake SELF `eboot.bin`, a PS4 `param.sfo`, a custom 512×512 RGB icon, and a fake PKG using the OpenOrbis toolchain.

The workflow runs `PkgTool.Core pkg_validate` and lists the resulting package entries before uploading the artifact. A successful CI validation is not a substitute for testing installation and launch on a real GoldHEN-enabled PS4.

## Safety and scope

This prototype is read-only. It does not spoof firmware versions, patch the kernel, or modify console settings.

## Toolchain

- [OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain)
- [create-fself](https://github.com/OpenOrbis/create-fself)
- [create-gp4](https://github.com/OpenOrbis/create-gp4)
