# Maps and threads milestone

This source milestone completes the first paged maps/thread enumeration path.

Implemented:

- `KFI_IOC_ENUM_THREADS` with ordinal cursors and leader marking.
- `KFI_IOC_ENUM_MAPS` with virtual-address cursors and VMA compatibility wrapper.
- Fixed-width thread/map UAPI entries and layout assertions.
- `Session::threads()` and `Session::maps()` in the C++ SDK.
- Pagination validation against invalid counts, unknown flags, and stalled cursors.
- `kfi threads PID` and `kfi maps PID` CLI commands.
- Userspace pagination and UAPI layout tests.

Validation performed without building `kfi.ko`:

- CMake userspace build: passed.
- CTest: 4/4 passed.
- Python tools: 10/10 passed.
- Python and shell syntax checks: passed.

Pending:

- Matching Android/GKI kernel build.
- Live process pagination tests.
- Concurrent thread/VMA mutation tests with KASAN and lockdep.
