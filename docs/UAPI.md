# KFI userspace ABI

The canonical ABI header is `include/uapi/linux/kfi.h`. All structures are
fixed-size and include reserved fields that callers must set to zero.

## Negotiation

Clients open either `/dev/kfi` or a configured private procfs endpoint, then
call:

1. `KFI_IOC_GET_VERSION`
2. `KFI_IOC_GET_CAPS`

ABI 1.1 adds `KFI_IOC_GET_RUNTIME_INFO`, which reports the kernel release,
machine, page size, compiled profile, and relevant build flags. The major ABI
version must match. Clients must test capability bits before
using optional commands.

The kernel and C++ SDK assert these layouts at compile time: `kfi_version` 56
bytes, `kfi_caps` 64 bytes, `kfi_open_process` 40 bytes,
`kfi_close_session` 32 bytes, and `kfi_runtime_info` 176 bytes.

## Transport capabilities

`KFI_CAP_TRANSPORT_CHAR` and `KFI_CAP_TRANSPORT_PROC_PRIVATE` report transports
that registered successfully for the running module. Runtime build flags report
which transports were compiled. The two values may differ when an optional
transport fails during initialization. Both endpoints use the same ioctl
numbers and structure layouts.

## Session lifecycle

`KFI_IOC_OPEN_PROCESS` accepts a positive PID and returns an opaque
`session_id`. The ID is valid only on the file descriptor that created it.
`KFI_IOC_CLOSE_SESSION` releases it. Closing the descriptor releases every
remaining session.

Unknown ioctl numbers return `-ENOTTY`; unknown flags, nonzero reserved fields,
and malformed requests return `-EINVAL`;
missing processes return `-ESRCH`.
