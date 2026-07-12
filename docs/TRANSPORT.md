# KFI transports

KFI exposes one control plane through one or more thin kernel transports. The
standard character device and the optional private procfs endpoint both create
the same `kfi_client`, call the same ioctl dispatcher, and use the same future
event stream. A command therefore has identical validation, ownership, and
error behavior regardless of which endpoint opened it.

## Character device

The default external-module build enables:

```text
/dev/kfi
```

The kernel registers a character device. Android deployments may need an
ueventd rule or a temporary owner-only `mknod`; see `ANDROID_DEPLOY.md`.

## Private procfs endpoint

The optional build flag creates a build-specific endpoint:

```text
/proc/<name>/<name>
```

It is a normal visible procfs entry with mode `0600`. It does not hook procfs
enumeration or modify directory callbacks. Control operations use the same
versioned ioctls as `/dev/kfi`. `read()` returns fixed-size event records and `poll()`/`epoll()` report event
readiness and client shutdown.

Generate matching kernel and userspace configuration before enabling it:

```sh
python3 scripts/gen_private_config.py \
  --output kernel/generated/kfi_private_config.h \
  --user-config build/generated/kfi_endpoint.json
```

The generated header is intentionally ignored by Git. The JSON file contains
the explicit endpoint specification accepted by the CLI. The endpoint name is
configuration, not a cryptographic secret.

Enable both transports with:

```sh
make -C "$KERNEL_OUT" M="$PWD/kernel" \
  KFI_TRANSPORT_CHAR=y KFI_TRANSPORT_PROC_PRIVATE=y modules
```

At least one transport must be enabled. A compiled transport that fails to
register is omitted from `GET_CAPS`; the module remains usable if another
transport registered successfully.

## Userspace endpoint selection

The SDK supports `Endpoint::Auto()`, `Endpoint::Device(path)`, and
`Endpoint::Proc(path)`. CLI syntax is:

```sh
kfi --endpoint auto version
kfi --endpoint dev:/dev/kfi version
kfi --endpoint proc:/proc/name/name version
kfi endpoint-info
```

`auto` uses `KFI_ENDPOINT` when it is set and otherwise selects `/dev/kfi`.
It never scans `/proc` to guess a generated endpoint name.

## Operation mapping

```text
open/release  client lifetime
ioctl         versioned control commands
read          batched fixed-size asynchronous events
poll/epoll    event readiness and shutdown
```
