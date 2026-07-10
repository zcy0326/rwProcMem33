# KFI userspace ABI

The canonical ABI header is `include/uapi/linux/kfi.h`. All structures are
fixed-size and include reserved zero-filled fields for compatible extension.

## Negotiation

Clients open `/dev/kfi`, then call:

1. `KFI_IOC_GET_VERSION`
2. `KFI_IOC_GET_CAPS`

The major ABI version must match. Clients must test capability bits before
using optional commands.

## Session lifecycle

`KFI_IOC_OPEN_PROCESS` accepts a positive PID and returns an opaque
`session_id`. The ID is valid only on the file descriptor that created it.
`KFI_IOC_CLOSE_SESSION` releases it. Closing the descriptor releases every
remaining session.

Unknown ioctl numbers return `-ENOTTY`; malformed requests return `-EINVAL`;
missing processes return `-ESRCH`.
