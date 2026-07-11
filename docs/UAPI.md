# KFI userspace ABI

The canonical ABI header is `include/uapi/linux/kfi.h`. All structures are
fixed-size and include reserved fields that callers must set to zero.

## Negotiation

Clients open either `/dev/kfi` or a configured private procfs endpoint, then
call:

1. `KFI_IOC_GET_VERSION`
2. `KFI_IOC_GET_CAPS`

ABI 1.2 adds request headers to every command, `KFI_IOC_GET_RUNTIME_INFO`,
and `KFI_IOC_HIDE_MODULE`. Runtime information reports the kernel release,
machine, page size, compiled profile, active capabilities, and transport
capabilities. The major ABI version must match. Clients must test capability
bits before using optional commands.

The kernel and C++ SDK assert these layouts at compile time: `kfi_version` 72
bytes, `kfi_caps` 128 bytes, `kfi_open_process` 64 bytes,
`kfi_close_session` 64 bytes, `kfi_runtime_info` 256 bytes,
`kfi_memory_io` 64 bytes, `kfi_visibility_control` 64 bytes,
`kfi_enumerate` 64 bytes, `kfi_thread_entry` 64 bytes, and `kfi_map_entry` 320 bytes.

## Transport capabilities

`KFI_CAP_TRANSPORT_CHAR` and `KFI_CAP_TRANSPORT_PROC_PRIVATE` report transports
that registered successfully for the running module. When the private proc
transport is active, `KFI_CAP_TRANSPORT_PROC_HIDDEN` confirms that its generated
procfs directory is hidden from root directory listings. `KFI_CAP_MODULE_HIDING`
indicates support for `KFI_IOC_HIDE_MODULE`. Runtime build flags report which
transports were compiled. The two values may differ when an optional transport
fails during initialization. Both endpoints use the same ioctl numbers and
structure layouts.

## Session lifecycle

`KFI_IOC_OPEN_PROCESS` accepts a positive PID and returns an opaque
`session_id`. The ID is valid only on the file descriptor that created it.
`KFI_IOC_CLOSE_SESSION` releases it. Closing the descriptor releases every
remaining session. `KFI_IOC_HIDE_MODULE` accepts a
`kfi_visibility_control` request with `KFI_VISIBILITY_FLAG_HIDE_MODULE` and
removes the loaded KFI module from the module list and sysfs representation.

`KFI_IOC_READ_MEMORY` and `KFI_IOC_WRITE_MEMORY` use the session ID together
with a remote address, a userspace buffer pointer, and a bounded request size.
The kernel returns the completed byte count in `completed_size`; a successful
partial transfer is reported with a zero ioctl return value. The current
maximum request size is exposed as `kfi_caps.max_io_size`.


## Process layout enumeration

`KFI_IOC_ENUM_THREADS` and `KFI_IOC_ENUM_MAPS` use the fixed-size
`kfi_enumerate` request. The caller supplies an output array, its capacity,
and a cursor. The kernel returns the number of entries, a next cursor, and
`KFI_ENUM_RESULT_END` when enumeration is complete. A request is limited to
`KFI_ENUM_MAX_ENTRIES`; userspace repeats requests until the end flag is set.

Thread cursors are ordinal positions in the thread group. Map cursors are
virtual addresses and advance to the end of the last returned VMA. Both
enumerations are weakly consistent: concurrent thread or VMA changes may be
visible between pages, so callers needing a snapshot must suspend the target.
Unknown ioctl numbers return `-ENOTTY`; unknown flags, nonzero reserved fields,
and malformed requests return `-EINVAL`;
missing processes return `-ESRCH`.
