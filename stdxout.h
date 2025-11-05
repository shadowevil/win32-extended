#pragma once
//============================
// C++ Version Macros
//============================
#define CPP98_03 199711L
#define CPP11 201103L
#define CPP14 201402L
#define CPP17 201703L
#define CPP20 202002L
#define CPP23 202302L

#if defined(_MSVC_LANG)
#   define CPP_STD _MSVC_LANG
#else
#   define CPP_STD __cplusplus
#endif

#define CPP_AT_LEAST(ver) (CPP_STD >= ver)

#include <string>
#include <sstream>
#include <ostream>
#ifdef _WIN32
#   include <io.h>            // _write, STDOUT, STDERR (Windows)
#else
#   include <unistd.h>        // _write on POSIX, but on Windows use <io.h>
#endif
#include <cstddef>

namespace stdx {
#ifndef STDX_FILE_DESCRIPTOR
#define STDX_FILE_DESCRIPTOR
    enum FileDescriptor {
        STDIN = 0,
        STDOUT = 1,
        STDERR = 2
    };
#endif

    template<typename T>
    inline size_t replace_first_brace(std::string& s, const T& value) {
        size_t start = s.find('{');
        if (start == std::string::npos) return 0;
        size_t end = s.find('}', start);
        if (end == std::string::npos) return 0;

        std::ostringstream oss;
        oss << value;
        std::string rep = oss.str();
        s.replace(start, end - start + 1, rep);
        return rep.size();
    }

    inline size_t count_placeholders(const std::string& s) {
        size_t count = 0, pos = 0;
        while ((pos = s.find('{', pos)) != std::string::npos) {
            size_t end = s.find('}', pos);
            if (end != std::string::npos) { count++; pos = end + 1; }
            else break;
        }
        return count;
    }

    template<typename... Args>
    inline void Print(const std::string& message, Args&&... args) {
        std::string formatted = message;
#if CPP_AT_LEAST(CPP17)
        (replace_first_brace(formatted, std::forward<Args>(args)), ...);
#else
        using expander = int[];
        expander{ 0, (replace_first_brace(formatted, std::forward<Args>(args)), 0)... };
#endif
        _write(STDOUT, formatted.c_str(), (unsigned int)formatted.size());
    }

    template<typename... Args>
    inline void PrintLine(const std::string& message, Args&&... args) {
        std::string formatted = message;
#if CPP_AT_LEAST(CPP17)
        (replace_first_brace(formatted, std::forward<Args>(args)), ...);
#else
        using expander = int[];
        expander{ 0, (replace_first_brace(formatted, std::forward<Args>(args)), 0)... };
#endif
        formatted += "\n";
        _write(STDOUT, formatted.c_str(), (unsigned int)formatted.size());
    }

    template<typename... Args>
    inline void PrintErr(const std::string& message, Args&&... args) {
        std::string formatted = message;
#if CPP_AT_LEAST(CPP17)
        (replace_first_brace(formatted, std::forward<Args>(args)), ...);
#else
        using expander = int[];
        expander{ 0, (replace_first_brace(formatted, std::forward<Args>(args)), 0)... };
#endif
        _write(STDERR, formatted.c_str(), (unsigned int)formatted.size());
    }

    template<typename... Args>
    inline void PrintLineErr(const std::string& message, Args&&... args) {
        std::string formatted = message;
#if CPP_AT_LEAST(CPP17)
        (replace_first_brace(formatted, std::forward<Args>(args)), ...);
#else
        using expander = int[];
        expander{ 0, (replace_first_brace(formatted, std::forward<Args>(args)), 0)... };
#endif
        formatted += "\n";
        _write(STDERR, formatted.c_str(), (unsigned int)formatted.size());
    }
}