# FREELY development roadmap

## Phase 1 — Build foundation

- [x] Create repository and project structure.
- [x] Add an OpenOrbis-based build workflow.
- [ ] Confirm the first CI build succeeds.
- [ ] Test the resulting package on a compatible PS4.

## Phase 2 — Diagnostics

- [x] Terminal-style startup banner.
- [x] Timestamped diagnostic output.
- [x] Explicit N/A results for unsupported checks.
- [ ] Verify the writable log location on real hardware.
- [ ] Add firmware and GoldHEN detection only after verifying supported APIs.
- [ ] Add memory, storage, controller, and display checks.

## Phase 3 — Usability

- [ ] Add a menu and controller navigation.
- [ ] Add log viewing and USB export.
- [ ] Replace the temporary sample icon with original FREELY artwork.

## Safety

FREELY is intended to be read-only. It must not spoof firmware, patch the kernel, or alter console identity or system settings. Never report a check as successful unless it was actually performed.
