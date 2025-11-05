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

//============================
// Includes
//============================
#include <cstdint>
#include <cassert>
#include <stdexcept>

#define NOMINMAX
#include <Windows.h>

#include <algorithm>
#include <unordered_map>
#include <vector>
#include <regex>
#include <shlobj.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <utility>
#include <io.h>
#include <string>

#ifndef RGB
#define RGB(r,g,b) ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
#endif

#if CPP_AT_LEAST(CPP17)
#include <filesystem>
#endif

#include "stdxstring.h"
#include "stdxstream.h"
#include "stdxfile.h"
#include "stdxout.h"
#include "stdxin.h"
#include "stdxordered_map.h"

namespace stdx {
    //============================
    // Mouse and cursor utilities
    //============================
    inline std::pair<float, float> GetCursorPosition() {
        HWND hwnd = GetActiveWindow();
        POINT p;
        GetCursorPos(&p);
        ScreenToClient(hwnd, &p);
        return { (float)p.x, (float)p.y };
    }

    inline bool IsLeftMouseDown() { return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0; }
    inline bool IsLeftMouseUp() { return !IsLeftMouseDown(); }
    inline bool IsRightMouseDown() { return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0; }
    inline bool IsRightMouseUp() { return !IsRightMouseDown(); }
    inline bool IsMiddleMouseDown() { return (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0; }
    inline bool IsMiddleMouseUp() { return !IsMiddleMouseDown(); }

    inline bool IsLeftMousePressed() { static bool p = false; bool n = IsLeftMouseDown();  bool r = (n && !p); p = n; return r; }
    inline bool IsRightMousePressed() { static bool p = false; bool n = IsRightMouseDown(); bool r = (n && !p); p = n; return r; }
    inline bool IsMiddleMousePressed() { static bool p = false; bool n = IsMiddleMouseDown(); bool r = (n && !p); p = n; return r; }

    //============================
    // Window style helpers
    //============================
    inline void SetWindowClickThrough(bool enable) {
        HWND hwnd = GetActiveWindow();
        LONG ex = GetWindowLong(hwnd, GWL_EXSTYLE);
        if (enable) {
            ex |= WS_EX_LAYERED | WS_EX_TRANSPARENT;
            SetWindowLong(hwnd, GWL_EXSTYLE, ex);
            SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
        }
        else {
            ex &= ~WS_EX_TRANSPARENT;
            SetWindowLong(hwnd, GWL_EXSTYLE, ex);
        }
    }

    inline void SetWindowTopMost(bool enable) {
        HWND hwnd = GetActiveWindow();
        SetWindowPos(hwnd, enable ? HWND_TOPMOST : HWND_NOTOPMOST,
            0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    inline int GetTaskbarHeight() {
        HWND hwnd = GetForegroundWindow();
        if (!hwnd) return 0;
        MONITORINFO mi{ sizeof(mi) };
        if (GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi)) {
            int fullH = mi.rcMonitor.bottom - mi.rcMonitor.top;
            int workH = mi.rcWork.bottom - mi.rcWork.top;
            int fullW = mi.rcMonitor.right - mi.rcMonitor.left;
            int workW = mi.rcWork.right - mi.rcWork.left;
            if (workH < fullH) return fullH - workH;
            if (workW < fullW) return fullW - workW;
        }
        return 0;
    }

    inline void HideFromTaskbar() {
        HWND hwnd = GetActiveWindow();
        LONG ex = GetWindowLong(hwnd, GWL_EXSTYLE);
        ex &= ~WS_EX_APPWINDOW;
        ex |= WS_EX_TOOLWINDOW;
        SetWindowLong(hwnd, GWL_EXSTYLE, ex);
        ShowWindow(hwnd, SW_HIDE);
        ShowWindow(hwnd, SW_SHOW);
    }

    //============================
    // MessageBox wrapper
    //============================
    #undef MessageBox
    class MessageBox {
    public:
        enum class Type { Info, Warning, Error };
        enum class Buttons { OK, OKCancel, YesNo, YesNoCancel };
        enum class Result { None, OK, Cancel, Yes, No };

        inline static Result Show(const std::string& title, const std::string& msg,
            Type type = Type::Info, Buttons buttons = Buttons::OK)
        {
            UINT style = (buttons == Buttons::OK ? MB_OK :
                buttons == Buttons::OKCancel ? MB_OKCANCEL :
                buttons == Buttons::YesNo ? MB_YESNO : MB_YESNOCANCEL);

            style |= (type == Type::Info ? MB_ICONINFORMATION :
                type == Type::Warning ? MB_ICONWARNING : MB_ICONERROR);

            int r = MessageBoxA(nullptr, msg.c_str(), title.c_str(), style);
            return r == IDOK ? Result::OK :
                r == IDCANCEL ? Result::Cancel :
                r == IDYES ? Result::Yes :
                r == IDNO ? Result::No : Result::None;
        }
    };

    //============================
    // Process execution
    //============================
    enum class ExecFlags { None = 0, WaitForExit = 1 << 0, Hidden = 1 << 1, AsAdmin = 1 << 2 };
    inline ExecFlags operator|(ExecFlags a, ExecFlags b) { return (ExecFlags)((int)a | (int)b); }
    inline bool operator&(ExecFlags a, ExecFlags b) { return ((int)a & (int)b) != 0; }

    class Program {
    public:
#if CPP_AT_LEAST(CPP17)
        inline static bool Execute(const std::filesystem::path& programPath,
            const std::vector<std::string>& args = {},
            ExecFlags flags = ExecFlags::None,
            const std::filesystem::path& workingDir = {})
        {
            if (!std::filesystem::exists(programPath)) return false;
            std::string ext = programPath.extension().string();
            for (auto& c : ext) c = (char)tolower(c);

            std::string cmd = "\"" + programPath.string() + "\"";
            for (auto& a : args) cmd += " \"" + a + "\"";

            if (ext == ".exe" || ext == ".bat" || ext == ".cmd") {
                STARTUPINFOA si{}; PROCESS_INFORMATION pi{};
                si.cb = sizeof(si);
                if (flags & ExecFlags::Hidden) { si.dwFlags |= STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE; }

                if (flags & ExecFlags::AsAdmin) {
                    ShellExecuteA(nullptr, "runas", programPath.string().c_str(),
                        args.empty() ? nullptr : cmd.c_str(),
                        workingDir.empty() ? nullptr : workingDir.string().c_str(),
                        SW_SHOWNORMAL);
                    return true;
                }

                if (CreateProcessA(nullptr, (LPSTR)cmd.c_str(), nullptr, nullptr, FALSE, 0, nullptr,
                    workingDir.empty() ? nullptr : workingDir.string().c_str(),
                    &si, &pi))
                {
                    if (flags & ExecFlags::WaitForExit) WaitForSingleObject(pi.hProcess, INFINITE);
                    CloseHandle(pi.hThread); CloseHandle(pi.hProcess); return true;
                }
                return false;
            }

            std::string params;
            for (auto& a : args) { if (!params.empty())params += " "; params += "\"" + a + "\""; }

            HINSTANCE r = ShellExecuteA(nullptr, (flags & ExecFlags::AsAdmin) ? "runas" : "open",
                programPath.string().c_str(),
                params.empty() ? nullptr : params.c_str(),
                workingDir.empty() ? nullptr : workingDir.string().c_str(),
                (flags & ExecFlags::Hidden) ? SW_HIDE : SW_SHOWNORMAL);
            return (intptr_t)r > 32;
        }
#else
        inline static bool Execute(const std::string& programPath,
            const std::vector<std::string>& args = {},
            ExecFlags flags = ExecFlags::None,
            const std::string& workingDir = "")
        {
            DWORD attr = GetFileAttributesA(programPath.c_str());
            if (attr == INVALID_FILE_ATTRIBUTES) return false;

            std::string ext;
            size_t dot = programPath.find_last_of('.');
            if (dot != std::string::npos) { ext = programPath.substr(dot); for (auto& c : ext)c = (char)tolower(c); }

            std::string cmd = "\"" + programPath + "\"";
            for (auto& a : args) cmd += " \"" + a + "\"";

            if (ext == ".exe" || ext == ".bat" || ext == ".cmd") {
                STARTUPINFOA si{}; PROCESS_INFORMATION pi{};
                si.cb = sizeof(si);
                if (flags & ExecFlags::Hidden) { si.dwFlags |= STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE; }

                if (flags & ExecFlags::AsAdmin) {
                    ShellExecuteA(nullptr, "runas", programPath.c_str(),
                        args.empty() ? nullptr : cmd.c_str(),
                        workingDir.empty() ? nullptr : workingDir.c_str(), SW_SHOWNORMAL);
                    return true;
                }

                if (CreateProcessA(nullptr, (LPSTR)cmd.c_str(), nullptr, nullptr, FALSE, 0, nullptr,
                    workingDir.empty() ? nullptr : workingDir.c_str(), &si, &pi))
                {
                    if (flags & ExecFlags::WaitForExit) WaitForSingleObject(pi.hProcess, INFINITE);
                    CloseHandle(pi.hThread); CloseHandle(pi.hProcess); return true;
                }
                return false;
            }

            std::string params;
            for (auto& a : args) { if (!params.empty())params += " "; params += "\"" + a + "\""; }

            HINSTANCE r = ShellExecuteA(nullptr, (flags & ExecFlags::AsAdmin) ? "runas" : "open",
                programPath.c_str(),
                params.empty() ? nullptr : params.c_str(),
                workingDir.empty() ? nullptr : workingDir.c_str(),
                (flags & ExecFlags::Hidden) ? SW_HIDE : SW_SHOWNORMAL);
            return (intptr_t)r > 32;
        }
#endif

        inline bool IsRunning(const std::string& patt) {
            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snap == INVALID_HANDLE_VALUE) return false;
            PROCESSENTRY32W pe{ sizeof(pe) };

            std::wregex r(std::wstring(patt.begin(), patt.end()), std::regex_constants::icase);

            bool found = false;
            if (Process32FirstW(snap, &pe)) {
                do {
                    if (std::regex_search(pe.szExeFile, r)) { found = true; break; }
                } while (Process32NextW(snap, &pe));
            }

            CloseHandle(snap);
            return found;
        }

        inline bool IsRunningExact(const std::string& exe) {
            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snap == INVALID_HANDLE_VALUE) return false;
            PROCESSENTRY32W pe{ sizeof(pe) };

            std::wstring wexe(exe.begin(), exe.end());

            bool found = false;
            if (Process32FirstW(snap, &pe)) {
                do {
                    if (_wcsicmp(pe.szExeFile, wexe.c_str()) == 0) { found = true; break; }
                } while (Process32NextW(snap, &pe));
            }

            CloseHandle(snap);
            return found;
        }

    };
} // namespace stdx
