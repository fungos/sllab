#pragma once

#include <bx/uint32_t.h>

typedef int32_t i32;
typedef uint32_t u32;
typedef uint8_t u8;
typedef float f32;

enum eNotInitialized {
    NotInitialized
};

static const f32 kMaxFloat = std::numeric_limits<f32>::max();
static const f32 kMinFloat = std::numeric_limits<f32>::min();
