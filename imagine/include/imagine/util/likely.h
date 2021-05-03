#pragma once

#define likely(E) __builtin_expect(!!(E), 1)
#define unlikely(E) __builtin_expect(!!(E), 0)
