include $(sort $(wildcard $(BR2_EXTERNAL_BRASS_PATH)/package/*/*.mk))

# GCC 15 (Ubuntu 25.10) defaults to C23 and promotes several legacy-C
# diagnostics to errors. Buildroot 2024.02 host packages predate those
# changes (host-m4 nodiscard, host-e2fsprogs bool typedef, host-gawk
# pointer casts, host-gmp configure tests). Keep the system compiler but
# build every host tool in C17 mode with those diagnostics as warnings.
# See BUILDING.md section 1.1.
HOSTCC += -std=gnu17 -Wno-error=incompatible-pointer-types -Wno-error=implicit-function-declaration -Wno-error=int-conversion
HOSTCXX += -std=gnu++17
