# Implementation status

| Area | Status | Verification | Pending |
|---|---|---|---|
| ABI 1.1 and runtime info | Implemented | UAPI layout assertions; Python tooling tests | Target kernel build and device test |
| Per-open client and opaque sessions | Implemented | Static review | Concurrent kernel/device tests |
| Shared transport dispatcher | Implemented | Static review | Target kernel build |
| Character-device transport | Implemented | Existing generic CI build | Android device-node test |
| Private procfs transport | Implemented, default off | Generator unit tests; static review | Target kernel build and procfs ioctl test |
| C++ endpoint abstraction | Implemented | CTest target added | Linux CI execution |
| Process memory backend | Not started | None | V1.3 |
| Maps and threads | Not started | None | Later V1 milestone |
| Hardware breakpoints and events | Not started | None | Later V1 milestone |

Kernel changes in the current workspace are not yet built against a matching
Android/GKI tree and are not yet validated with `insmod` on a device.
