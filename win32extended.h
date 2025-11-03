#pragma once
#include <Windows.h>
#include <cstdint>
#include <filesystem>
#include <vector>
#include <regex>
#include <shlobj.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <utility>

#ifndef RGB
#define RGB(r,g,b)        ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
#endif

namespace stdx {
    inline std::pair<float, float> GetCursorPosition() {
        HWND hwnd = GetActiveWindow();  // Obtain the window handle from Raylib
        POINT p;
        GetCursorPos(&p);
        ScreenToClient(hwnd, &p);
        return { (float)p.x, (float)p.y };
    }

    inline bool IsLeftMouseDown() {
        return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    }

    inline bool IsLeftMouseUp() {
        return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0;
    }

    inline bool IsLeftMousePressed() {
        static bool prev = false;
        bool now = IsLeftMouseDown();
        bool pressed = (now && !prev);
        prev = now;
        return pressed;
    }

    inline bool IsRightMouseDown() {
        return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    }

    inline bool IsRightMouseUp() {
        return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) == 0;
    }

    inline bool IsRightMousePressed() {
        static bool prev = false;
        bool now = IsRightMouseDown();
        bool pressed = (now && !prev);
        prev = now;
        return pressed;
    }

    inline bool IsMiddleMouseDown() {
        return (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
    }

    inline bool IsMiddleMouseUp() {
        return (GetAsyncKeyState(VK_MBUTTON) & 0x8000) == 0;
    }

    inline bool IsMiddleMousePressed() {
        static bool prev = false;
        bool now = IsMiddleMouseDown();
        bool pressed = (now && !prev);
        prev = now;
        return pressed;
    }

    inline void SetWindowClickThrough(bool enable) {
        HWND hwnd = GetActiveWindow(); // Raylib helper to get Win32 handle
        LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

        if (enable) {
            exStyle |= WS_EX_LAYERED | WS_EX_TRANSPARENT;
            SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
            SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
        }
        else {
            exStyle &= ~WS_EX_TRANSPARENT; // keep layered but not transparent-to-clicks
            SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
        }
    }

    inline void SetWindowTopMost(bool enable) {
        HWND hwnd = GetActiveWindow(); // Raylib helper to get native Win32 HWND
        if (enable) {
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
        else {
            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }

    inline int GetTaskbarHeight() {
        HWND hwnd = GetForegroundWindow();
        if (!hwnd) return 0;

        HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(MONITORINFO) };

        if (GetMonitorInfo(hMon, &mi)) {
            int fullW = mi.rcMonitor.right - mi.rcMonitor.left;
            int fullH = mi.rcMonitor.bottom - mi.rcMonitor.top;
            int workW = mi.rcWork.right - mi.rcWork.left;
            int workH = mi.rcWork.bottom - mi.rcWork.top;

            if (workH < fullH)
                return fullH - workH; // taskbar top/bottom
            if (workW < fullW)
                return fullW - workW; // taskbar left/right
        }
        return 0;
    }

    inline void HideFromTaskbar() {
        HWND hwnd = GetActiveWindow();
        LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

        // remove "AppWindow", add "ToolWindow"
        exStyle &= ~WS_EX_APPWINDOW;
        exStyle |= WS_EX_TOOLWINDOW;

        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);

        // Apply changes
        ShowWindow(hwnd, SW_HIDE);
        ShowWindow(hwnd, SW_SHOW);
    }

#undef MessageBox
    namespace MessageBox
    {
        enum class Type {
            Info,
            Warning,
            Error
        };

        enum class Buttons {
            OK,
            OKCancel,
            YesNo,
            YesNoCancel
        };

        enum class Result {
            None,
            OK,
            Cancel,
            Yes,
            No
        };

        inline static Result Show(const std::string& title,
            const std::string& message,
            Type type = Type::Info,
            Buttons buttons = Buttons::OK)
        {
            UINT style = 0;

            // Button styles
            switch (buttons) {
            case Buttons::OK: style = MB_OK; break;
            case Buttons::OKCancel: style = MB_OKCANCEL; break;
            case Buttons::YesNo: style = MB_YESNO; break;
            case Buttons::YesNoCancel: style = MB_YESNOCANCEL; break;
            }

            // Icon styles
            switch (type) {
            case Type::Info: style |= MB_ICONINFORMATION; break;
            case Type::Warning: style |= MB_ICONWARNING; break;
            case Type::Error: style |= MB_ICONERROR; break;
            }

            int r = MessageBoxA(nullptr, message.c_str(), title.c_str(), style);
            switch (r) {
            case IDOK: return Result::OK;
            case IDCANCEL: return Result::Cancel;
            case IDYES: return Result::Yes;
            case IDNO: return Result::No;
            default: return Result::None;
            }
        }
    }

    namespace Program
    {
        enum class ExecFlags
        {
            None = 0,
            WaitForExit = 1 << 0,
            Hidden = 1 << 1,
            AsAdmin = 1 << 2
        };
        inline ExecFlags operator|(ExecFlags a, ExecFlags b) { return (ExecFlags)((int)a | (int)b); }
        inline bool operator&(ExecFlags a, ExecFlags b) { return ((int)a & (int)b) != 0; }

        inline static bool Execute(const std::filesystem::path& programPath,
            const std::vector<std::string>& args = {},
            ExecFlags flags = ExecFlags::None,
            const std::filesystem::path& workingDir = {})
        {
            if (!std::filesystem::exists(programPath))
                return false;

            std::string ext = programPath.extension().string();
            for (auto& c : ext) c = (char)tolower(c);

            std::string cmdLine = "\"" + programPath.string() + "\"";
            for (const auto& arg : args)
                cmdLine += " \"" + arg + "\"";

            if (ext == ".exe" || ext == ".bat" || ext == ".cmd") {
                STARTUPINFOA si{};
                PROCESS_INFORMATION pi{};
                si.cb = sizeof(si);

                if (flags & ExecFlags::Hidden)
                    si.dwFlags |= STARTF_USESHOWWINDOW, si.wShowWindow = SW_HIDE;

                DWORD creationFlags = 0;
                if (flags & ExecFlags::AsAdmin) {
                    ShellExecuteA(nullptr, "runas", programPath.string().c_str(),
                        args.empty() ? nullptr : cmdLine.c_str(),
                        workingDir.empty() ? nullptr : workingDir.string().c_str(), SW_SHOWNORMAL);
                    return true;
                }

                if (CreateProcessA(
                    nullptr,
                    cmdLine.data(),
                    nullptr, nullptr, FALSE,
                    creationFlags, nullptr,
                    workingDir.empty() ? nullptr : workingDir.string().c_str(),
                    &si, &pi))
                {
                    if (flags & ExecFlags::WaitForExit)
                        WaitForSingleObject(pi.hProcess, INFINITE);

                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);
                    return true;
                }
                return false;
            }

            std::string params;
            for (const auto& arg : args) {
                if (!params.empty()) params += " ";
                params += "\"" + arg + "\"";
            }

            HINSTANCE result = ShellExecuteA(
                nullptr,
                (flags & ExecFlags::AsAdmin) ? "runas" : "open",
                programPath.string().c_str(),
                params.empty() ? nullptr : params.c_str(),
                workingDir.empty() ? nullptr : workingDir.string().c_str(),
                (flags & ExecFlags::Hidden) ? SW_HIDE : SW_SHOWNORMAL);

            return reinterpret_cast<intptr_t>(result) > 32;
        }

        inline bool IsRunning(const std::string& namePattern)
        {
            HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snapshot == INVALID_HANDLE_VALUE)
                return false;

            PROCESSENTRY32 pe;
            pe.dwSize = sizeof(pe);
            std::regex pattern(namePattern, std::regex_constants::icase);

            bool found = false;
            if (Process32First(snapshot, &pe)) {
                do {
                    std::string exe = pe.szExeFile;
                    if (std::regex_search(exe, pattern)) {
                        found = true;
                        break;
                    }
                } while (Process32Next(snapshot, &pe));
            }

            CloseHandle(snapshot);
            return found;
        }

        inline bool IsRunningExact(const std::string& exeName)
        {
            HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snapshot == INVALID_HANDLE_VALUE)
                return false;

            PROCESSENTRY32 pe;
            pe.dwSize = sizeof(pe);
            bool found = false;

            if (Process32First(snapshot, &pe)) {
                do {
                    if (_stricmp(pe.szExeFile, exeName.c_str()) == 0) {
                        found = true;
                        break;
                    }
                } while (Process32Next(snapshot, &pe));
            }

            CloseHandle(snapshot);
            return found;
        }
    }
}