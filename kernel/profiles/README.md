# Build profiles

KFI uses one source tree to build separate artifacts for each target KMI family.
The initial profile names are:

- `android12-5.10`
- `android13-5.15`
- `android14-6.1`
- `android15-6.6`
- `android16-6.12`
- `generic`

The active name is compiled into `kfi_runtime_info.profile`. Until profile
headers contain feature overrides, builds default to `generic`. A build system
may select a name with an escaped string definition, for example:

```sh
make -C "$KERNEL_OUT" M="$PWD/kernel" \
  KFI_PROFILE=android15-6.6 modules
```

Profiles describe build compatibility; runtime capability bits remain the
authoritative source for userspace feature selection.
