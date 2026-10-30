# General Minimal C28 Program

Minimal peripheral framework for the TMS320F28374S PCS control board. Hardware requirements are held in the `Broad` configuration tables; `Drivers` applies those tables. The entry points follow the IB800U lifecycle style: `PeripheralsInit`, `PlatformInit`, `ApplicationInit`, `TaskCreate`, and `StartSystem`. The `Platform/Schedule` component provides an empty cooperative task framework. No application behavior is registered.

## Branches

- `mainline/general-minimal-c28-program` is the fixed, reviewed starting point. Do not commit to or merge into this branch during follow-on development.
- `develop/general-minimal-c28-program` is the integration branch for subsequent work. Create a short-lived feature branch from it for each change and submit that branch for review before integration.
- `pcs-minimal-20261030` preserves the original firmware commit. `general-minimal-c28-program-v1.0` preserves this named project baseline.

The power PWM outputs remain locked by one-shot trip, and the gate enable remains low. Successful compilation is not hardware acceptance. See [bring-up and verification notes](docs/minimal-bringup.md) before connecting a power stage.

## Build

Use CCS or the reproducible compiler script in `tools/build_minimal.py`. The exact local compiler path and pin-sheet verification command are documented in [docs/minimal-bringup.md](docs/minimal-bringup.md).
