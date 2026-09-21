# TOLOSAT Packet Utilization Standard Library

The PUS library provides telemetry and telecommand packet management together with the PUS services used by TAPAS. Public headers are exported from `include/`; internal parsing, scheduling, CRC, and table helpers remain under `components/utils/`.

The current implementation contains core TM/TC management and services 1, 3, 5, 6, 9, 11, 17, and 160.

From the parent flight-software repository:

```bash
make pus
make pus-verif
make pus-clean
```

The archive is written to `build/libs/libpus.a`.

## Configuration

Library options belong in `gen/Kconfig.options`. The parent project sources this fragment and generates `pus_autoconf.h`; the fragment is currently empty. A standalone checkout can maintain its own configuration with `make default_defconfig` and `make menuconfig`. Initialise the `tools/` submodule recursively before generating or building from a standalone checkout.

## License

This library is licensed under the Apache License 2.0. See [`LICENSE`](LICENSE)
for details.
