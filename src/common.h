#pragma once
#include <cstdio>
#include <cstdint>
#include <stdarg.h>

void LOG(const char* txt, ...);

void LOG_WARN(const char* txt, ...);

void LOG_ERROR(const char* txt, ...);