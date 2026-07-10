# Build

## Kernel module

Use the target Android/Linux kernel build tree and its matching toolchain:

```sh
make -C "$KERNEL_OUT" M="$PWD/kernel" modules
```

For Android GKI, build against the exact device KMI and symbol list. The new
module uses exported kernel APIs and does not require disabling CFI/KCFI.

## Userspace

On Linux or with the Android NDK toolchain:

```sh
cmake -S user -B build/user -DCMAKE_BUILD_TYPE=Release
cmake --build build/user
```

The resulting binary is `build/user/kfi`.
