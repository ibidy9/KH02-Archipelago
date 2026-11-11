#pragma once
#include <stdint.h>
#define KH02_SDK_PUMP_VERSION 1u

typedef struct Kh02SdkPumpStatus {
    uint32_t size, version, pid, thread;
    uint32_t bound, sequence, sdk_commands, window_commands;
    uint32_t pending, core_status, servicing, ignored_wakes;
} Kh02SdkPumpStatus;

typedef uint32_t (*Kh02SdkPumpFn)(uint32_t, uint32_t, Kh02SdkPumpStatus *);
