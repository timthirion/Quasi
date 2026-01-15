/// @file platform.hpp
/// @brief Platform and compiler detection macros.
///
/// Provides compile-time detection of the target platform (macOS, Linux, Windows),
/// compiler (Clang, GCC, MSVC), and C++ feature availability. All macros use the
/// Q_ prefix to avoid conflicts with other libraries.

#pragma once

// ============================================================================
// Platform Detection
// ============================================================================

#if defined(__APPLE__) && defined(__MACH__)
    #define Q_PLATFORM_MACOS 1
    #define Q_PLATFORM_POSIX 1
    #define Q_PLATFORM_NAME "macOS"
    #define Q_SHARED_LIB_EXTENSION ".dylib"
#elif defined(__linux__)
    #define Q_PLATFORM_LINUX 1
    #define Q_PLATFORM_POSIX 1
    #define Q_PLATFORM_NAME "Linux"
    #define Q_SHARED_LIB_EXTENSION ".so"
#elif defined(_WIN32) || defined(_WIN64)
    #define Q_PLATFORM_WINDOWS 1
    #define Q_PLATFORM_NAME "Windows"
    #define Q_SHARED_LIB_EXTENSION ".dll"
    #error "Windows support not yet implemented - requires LoadLibrary/GetProcAddress"
#else
    #error "Unsupported platform"
#endif

// ============================================================================
// Compiler Detection
// ============================================================================

#if defined(__clang__)
    #define Q_COMPILER_CLANG 1
    #if defined(__apple_build_version__)
        #define Q_COMPILER_APPLE_CLANG 1
        #define Q_COMPILER_NAME "Apple Clang"
    #else
        #define Q_COMPILER_LLVM_CLANG 1
        #define Q_COMPILER_NAME "LLVM Clang"
    #endif
#elif defined(__GNUC__)
    #define Q_COMPILER_GCC 1
    #define Q_COMPILER_NAME "GCC"
#elif defined(_MSC_VER)
    #define Q_COMPILER_MSVC 1
    #define Q_COMPILER_NAME "MSVC"
#else
    #define Q_COMPILER_NAME "Unknown"
#endif

// ============================================================================
// Feature Detection
// ============================================================================

#if __cplusplus < 202002L
    #error "C++20 or later required"
#endif

#if defined(__cpp_impl_coroutine) && __cpp_impl_coroutine >= 201902L
    #define Q_HAS_COROUTINES 1
#elif defined(__cpp_coroutines)
    #define Q_HAS_COROUTINES 1
#else
    #error "Coroutine support required (C++20 or later with coroutine support)"
#endif

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
    #define Q_HAS_STD_EXPECTED 1
#else
    #define Q_HAS_STD_EXPECTED 1  // Assume available with C++23 flag
#endif

// ============================================================================
// Platform-Specific Attributes
// ============================================================================

#if defined(Q_COMPILER_GCC) || defined(Q_COMPILER_CLANG)
    #define Q_ATTRIBUTE_CONSTRUCTOR [[gnu::constructor]]
    #define Q_ATTRIBUTE_DESTRUCTOR  [[gnu::destructor]]
#else
    #define Q_ATTRIBUTE_CONSTRUCTOR
    #define Q_ATTRIBUTE_DESTRUCTOR
#endif

#if defined(Q_PLATFORM_WINDOWS)
    #define Q_EXPORT_API __declspec(dllexport)
    #define Q_IMPORT_API __declspec(dllimport)
#else
    #define Q_EXPORT_API __attribute__((visibility("default")))
    #define Q_IMPORT_API
#endif
