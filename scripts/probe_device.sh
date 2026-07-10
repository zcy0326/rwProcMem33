#!/system/bin/sh
set -u

section()
{
    printf '\n[%s]\n' "$1"
}

value_or_unavailable()
{
    if [ -r "$1" ]; then
        head -n 1 "$1"
    else
        printf 'unavailable\n'
    fi
}

section identity
printf 'uname_s=%s\n' "$(uname -s 2>/dev/null || printf unavailable)"
printf 'uname_r=%s\n' "$(uname -r 2>/dev/null || printf unavailable)"
printf 'uname_m=%s\n' "$(uname -m 2>/dev/null || printf unavailable)"
printf 'proc_version='
value_or_unavailable /proc/version

section memory
if command -v getconf >/dev/null 2>&1; then
    printf 'page_size=%s\n' "$(getconf PAGE_SIZE 2>/dev/null || printf unavailable)"
else
    printf 'page_size=unavailable\n'
fi

section android
if command -v getprop >/dev/null 2>&1; then
    for prop in \
        ro.build.version.release \
        ro.build.version.sdk \
        ro.product.device \
        ro.product.vendor.device \
        ro.boot.slot_suffix \
        ro.boot.verifiedbootstate; do
        printf '%s=%s\n' "$prop" "$(getprop "$prop")"
    done
else
    printf 'getprop=unavailable\n'
fi

section kernel_config
config_pattern='CONFIG_(MODULES|MODVERSIONS|MODULE_SIG|MODULE_SIG_FORCE|CFI|CFI_CLANG|KCFI|LTO|LTO_CLANG|KPROBES|HAVE_HW_BREAKPOINT|ARM64|COMPAT|PAGE_SIZE_4KB|PAGE_SIZE_16KB|PAGE_SIZE_64KB)='
if [ -r /proc/config.gz ] && command -v zcat >/dev/null 2>&1; then
    zcat /proc/config.gz 2>/dev/null | grep -E "$config_pattern" || true
elif [ -r /sys/kernel/config.gz ] && command -v zcat >/dev/null 2>&1; then
    zcat /sys/kernel/config.gz 2>/dev/null | grep -E "$config_pattern" || true
else
    printf 'config=unavailable\n'
fi

section module_environment
if [ -r /proc/modules ]; then
    printf 'loaded_module_count=%s\n' "$(wc -l < /proc/modules | tr -d ' ')"
else
    printf 'loaded_module_count=unavailable\n'
fi
for directory in /vendor/lib/modules /vendor_dlkm/lib/modules /odm/lib/modules; do
    if [ -d "$directory" ]; then
        count=$(find "$directory" -maxdepth 1 -type f -name '*.ko' 2>/dev/null | wc -l | tr -d ' ')
        printf 'module_dir=%s count=%s\n' "$directory" "$count"
    fi
done

section kallsyms
if [ -r /proc/kallsyms ]; then
    first_address=$(awk 'NR == 1 { print $1; exit }' /proc/kallsyms 2>/dev/null)
    case "$first_address" in
        ''|0000000000000000|00000000) printf 'kallsyms=masked\n' ;;
        *) printf 'kallsyms=readable\n' ;;
    esac
else
    printf 'kallsyms=unavailable\n'
fi

section security
printf 'selinux_enforce='
value_or_unavailable /sys/fs/selinux/enforce
printf 'lockdown='
value_or_unavailable /sys/kernel/security/lockdown
