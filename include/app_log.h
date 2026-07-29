#pragma once

#include <Arduino.h>

#ifndef APP_LOG_LEVEL
#define APP_LOG_LEVEL 2
#endif

#define APP_LOG_ERROR 1
#define APP_LOG_INFO  2
#define APP_LOG_DEBUG 3
#define APP_LOG_TRACE 4

#if APP_LOG_LEVEL >= APP_LOG_ERROR
#define LOG_E(tag, fmt, ...) Serial.printf("[E][%s] " fmt "\n", tag, ##__VA_ARGS__)
#else
#define LOG_E(tag, fmt, ...)
#endif

#if APP_LOG_LEVEL >= APP_LOG_INFO
#define LOG_I(tag, fmt, ...) Serial.printf("[I][%s] " fmt "\n", tag, ##__VA_ARGS__)
#else
#define LOG_I(tag, fmt, ...)
#endif

#if APP_LOG_LEVEL >= APP_LOG_DEBUG
#define LOG_D(tag, fmt, ...) Serial.printf("[D][%s] " fmt "\n", tag, ##__VA_ARGS__)
#else
#define LOG_D(tag, fmt, ...)
#endif

#if APP_LOG_LEVEL >= APP_LOG_TRACE
#define LOG_T(tag, fmt, ...) Serial.printf("[T][%s] " fmt "\n", tag, ##__VA_ARGS__)
#else
#define LOG_T(tag, fmt, ...)
#endif
