# KFI userspace ABI

The canonical ABI header is `include/uapi/linux/kfi.h`. ABI 1.2 uses a common
16-byte request header containing `struct_size`, `flags`, and `request_id`.
Published ioctl structure sizes are frozen for ABI 1.x; compatible extensions
must consume reserved fields rather than changing `sizeof`.

## Negotiation

Clients open `/dev/kfi` or a configured private procfs endpoint, then call:

1. `KFI_IOC_GET_VERSION`
2. `KFI_IOC_GET_CAPS`
3. `KFI_IOC_GET_RUNTIME_INFO`

The major ABI version must match. Clients must test capability bits before
using optional commands.

| Structure | Size |
|---|---:|
| `kfi_request_header` | 16 |
| `kfi_version` | 72 |
| `kfi_caps` | 128 |
| `kfi_open_process` | 64 |
| `kfi_close_session` | 64 |
| `kfi_runtime_info` | 256 |
| `kfi_memory_io` | 64 |
| `kfi_enumerate` | 64 |
| `kfi_thread_entry` | 64 |
| `kfi_map_entry` | 320 |
| `kfi_visibility_control` | 64 |

All callers must zero output and reserved fields. Unknown flags return
`EINVAL`; a mismatched `struct_size` returns `EMSGSIZE`.

## Transport capabilities

`KFI_CAP_TRANSPORT_CHAR` and `KFI_CAP_TRANSPORT_PROC_PRIVATE` report transports
that registered successfully. `KFI_CAP_TRANSPORT_PROC_HIDDEN` is reported only
when the optional procfs enumeration filter is active.

## Session lifecycle

`KFI_IOC_OPEN_PROCESS` accepts a positive PID and returns an opaque
`session_id`. IDs are scoped to the client associated with the opened file.
Sessions use kernel references so in-flight operations can finish while another
thread closes the ID.

## Memory transfer

`KFI_IOC_READ_MEMORY` and `KFI_IOC_WRITE_MEMORY` combine a session ID, remote
address, userspace buffer, requested byte count, and completed byte count. Each
ioctl is bounded by `kfi_caps.max_io_size`; the C++ SDK splits larger requests.
`MemoryTransferError` preserves cumulative progress on failure.

## Paged enumeration

`KFI_IOC_ENUM_THREADS` and `KFI_IOC_ENUM_MAPS` use `kfi_enumerate`.
`capacity` is limited to `KFI_ENUM_MAX_ENTRIES`; `returned` records the number
of entries copied; `next_cursor` is passed into the next request.
`KFI_ENUM_RESULT_END` terminates the sequence.

Thread cursors are ordinal and represent a best-effort view while threads are
created or exit. `KFI_THREAD_FLAG_LEADER` marks the thread-group leader.

Map cursors are virtual addresses. The next cursor is the end address of the
last returned VMA, which keeps pagination stable for a locked VMA walk.
`KFI_MAP_FLAG_PATH_TRUNCATED` reports file paths that exceeded the fixed output
field.

Unknown ioctl numbers return `ENOTTY`; missing processes return `ESRCH`.
