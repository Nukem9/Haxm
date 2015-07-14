#pragma once

#include <Windows.h>
#include <stdint.h>

#define EINVAL 1
#define EFAULT 2
#define ENOSYS 3
#define ENODEV 4
#define ENXIO  5
#define ENOSPC 6
#define ENOMEM 7
#define EINTR  8
#define EAGAIN 9

#define assert
#define dprint printf

#define CONFIG_WIN32
#define CONFIG_SOFTFLOAT
#define TARGET_X86_64

typedef unsigned long long target_ulong;

typedef unsigned long long target_phys_addr_t;
typedef unsigned long long ram_addr_t;

typedef uint8_t uint8;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float float32;
typedef double float64;