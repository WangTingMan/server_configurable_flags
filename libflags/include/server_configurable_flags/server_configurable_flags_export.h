#pragma once

#if defined(WIN32) || defined(_MSC_VER)

#if defined(SERVERCONFIGURABLEFLAGS_EXPORTS)
#define SERVERCONFIGURABLEFLAGS_API __declspec(dllexport)
#else
#define SERVERCONFIGURABLEFLAGS_API __declspec(dllimport)
#endif  // defined(SERVERCONFIGURABLEFLAGS_EXPORTS)

#else  // defined(WIN32)
#if defined(SERVERCONFIGURABLEFLAGS_EXPORTS)
#define SERVERCONFIGURABLEFLAGS_API __attribute__((visibility("default")))
#else
#define SERVERCONFIGURABLEFLAGS_API
#endif  // defined(SERVERCONFIGURABLEFLAGS_EXPORTS)
#endif


