#pragma once

#include <string>
#include <cstring>     // strlen
#include <cstddef>     // size_t
#ifdef _WIN32
#   include <io.h>        // _read, STDIN (Windows)
#else
#   include <unistd.h>    // _read on POSIX, but on Windows use <io.h>
#endif
#include "stdxout.h"
#include "stdxstring.h"

namespace stdx {
#ifndef STDX_FILE_DESCRIPTOR
#define STDX_FILE_DESCRIPTOR
    enum FileDescriptor {
        STDIN = 0,
        STDOUT = 1,
        STDERR = 2
    };
#endif

    inline void CaptureInput(char* buf, size_t cap) {
        int n = _read(STDIN, buf, (unsigned int)(cap - 1));
        buf[(n > 0 ? n : 0)] = 0;
        size_t len = strlen(buf);
        if (len && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) buf[len - 1] = 0;
    }

    inline void CaptureInput(std::string& outStr, size_t cap) {
        outStr.resize(cap);
        int n = _read(STDIN, &outStr[0], (unsigned int)(cap - 1));
        if (n > 0) outStr[n] = 0; else outStr[0] = 0;
        if (!outStr.empty() && (outStr.back() == '\n' || outStr.back() == '\r')) outStr.pop_back();
        if (!outStr.empty() && outStr.back() == '\r') outStr.pop_back();
        outStr.resize(strlen(outStr.c_str()));
    }

    inline void CaptureInput(stdx::string& outStr, size_t cap)
    {
        std::string tmp;
        tmp.reserve(cap);

        std::vector<char> buf(cap);
        int n = _read(STDIN, buf.data(), (unsigned int)(cap - 1));

        if (n > 0)
            tmp.assign(buf.data(), n);
        else
            tmp.clear();

        // trim CR/LF
        while (!tmp.empty() && (tmp.back() == '\n' || tmp.back() == '\r'))
            tmp.pop_back();

        outStr = stdx::string(tmp);
    }

    inline void PromptInput(const char* prompt, std::string& outStr, size_t cap) { Print(prompt); CaptureInput(outStr, cap); }
    inline void PromptInput(const char* prompt, char* buf, size_t cap) { Print(prompt); CaptureInput(buf, cap); }
	inline void PromptInput(const char* prompt, stdx::string& outStr, size_t cap) { Print(prompt); CaptureInput(outStr, cap); }
}