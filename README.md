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
