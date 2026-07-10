# KFI V1 design

KFI V1 is a kernel-assisted instrumentation backend. The first implementation
establishes a stable ABI and object ownership model before moving memory and
hardware-breakpoint code out of the two upstream modules.

## Trust and ownership model

- Opening `/dev/kfi` requires `CAP_SYS_PTRACE`.
- Every successful `open()` allocates an independent `kfi_client`.
- Sessions are stored in that client's IDR and represented in userspace by an
  integer ID. Kernel pointers never cross the ABI.
- Closing the file releases only that client's sessions.
- Android deployments must additionally restrict the device with a dedicated
  SELinux domain and an owner-only device-node mode.

## Implemented slice

- Versioned UAPI 1.0.
- `GET_VERSION` and accurate capability negotiation.
- `OPEN_PROCESS` and `CLOSE_SESSION` with opaque, per-client handles.
- C++17 userspace library and CLI commands: `version`, `caps`, `attach`.

Capability bits are only advertised after their implementation is wired into
the unified module. This prevents a new userspace client from accidentally
calling legacy operations.

## Legacy isolation

The upstream `rwProcMem33Module/` and `hwBreakpointProcModule/` directories are
kept unchanged as migration references. Their pointer-valued handles, global
breakpoint state, module-list manipulation, procfs hiding, credential mutation,
and CFI-patching paths are not linked into `kernel/kfi.ko` and are not exposed
through the KFI ABI.

## Next slices

1. Add session lookup/refcount helpers and bounded memory I/O.
2. Add maps and thread enumeration.
3. Replace global breakpoint storage with client-owned opaque IDs.
4. Add a preallocated event ring and `poll()` support.
5. Add per-breakpoint actions after event delivery is stable.
