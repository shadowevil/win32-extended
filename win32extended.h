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
#include <sstream>
#include <fstream>

#ifndef RGB
#define RGB(r,g,b) ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
#endif

#if CPP_AT_LEAST(CPP17)
#include <filesystem>
#endif

namespace stdx {
    //============================
    // Utility methods
    //============================
    inline std::string ToLower(const std::string& s) {
        std::string res = s;
        for (auto& c : res) c = (char)tolower(c);
        return res;
	}

    inline std::string ToUpper(const std::string& s) {
        std::string res = s;
        for (auto& c : res) c = (char)toupper(c);
        return res;
	}

	inline bool Contains(const std::string& str, const std::string& substr, bool caseSensitive = true) {
        if (caseSensitive) {
            return str.find(substr) != std::string::npos;
        }
        else {
            std::string lowerStr = ToLower(str);
            std::string lowerSubstr = ToLower(substr);
            return lowerStr.find(lowerSubstr) != std::string::npos;
        }
    }

    inline bool StartsWith(const std::string& str, const std::string& prefix, bool caseSensitive = true) {
        if (prefix.size() > str.size()) return false;
        if (caseSensitive) {
            return str.compare(0, prefix.size(), prefix) == 0;
        }
        else {
            std::string lowerStr = ToLower(str.substr(0, prefix.size()));
            std::string lowerPrefix = ToLower(prefix);
            return lowerStr == lowerPrefix;
        }
	}

    inline bool EndsWith(const std::string& str, const std::string& suffix, bool caseSensitive = true) {
        if (suffix.size() > str.size()) return false;
        if (caseSensitive) {
            return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
        }
        else {
            std::string lowerStr = ToLower(str.substr(str.size() - suffix.size()));
            std::string lowerSuffix = ToLower(suffix);
            return lowerStr == lowerSuffix;
        }
    }

    inline std::string Trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\n\r");
        return s.substr(start, end - start + 1);
	}

    inline std::string TrimStart(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        return s.substr(start);
	}

    inline std::string TrimEnd(const std::string& s) {
        size_t end = s.find_last_not_of(" \t\n\r");
        if (end == std::string::npos) return "";
        return s.substr(0, end + 1);
    }

    inline std::vector<std::string> Split(const std::string& s, char delimiter) {
        std::vector<std::string> tokens;
        std::istringstream iss(s);
        std::string token;
        while (std::getline(iss, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
	}

    inline std::string Join(const std::vector<std::string>& elements, const std::string& delimiter) {
        std::ostringstream oss;
        for (size_t i = 0; i < elements.size(); ++i) {
            oss << elements[i];
            if (i < elements.size() - 1) {
                oss << delimiter;
            }
        }
        return oss.str();
	}

    inline std::string Replace(const std::string& str, const std::string& from, const std::string& to) {
        if (from.empty()) return str;
        std::string result = str;
        size_t start_pos = 0;
        while ((start_pos = result.find(from, start_pos)) != std::string::npos) {
            result.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        return result;
    }

	using Byte = uint8_t;
    class ByteBuffer {
    public:
        using value_type = Byte;

        std::vector<value_type> buffer;

        ByteBuffer() = default;
        ByteBuffer(size_t size) : buffer(size) {}
        ByteBuffer(std::vector<value_type> data) : buffer(std::move(data)) {}

        size_t size() const { return buffer.size(); }
        bool empty() const { return buffer.empty(); }
        value_type* data() { return buffer.data(); }
        const value_type* data() const { return buffer.data(); }

        value_type& operator[](size_t i) { return buffer[i]; }
        const value_type& operator[](size_t i) const { return buffer[i]; }

        auto begin() { return buffer.begin(); }
        auto end() { return buffer.end(); }
        auto begin() const { return buffer.begin(); }
        auto end() const { return buffer.end(); }
    };
    using Bytes = ByteBuffer;

    class IStream {
    public:
        virtual ~IStream() {}
        virtual size_t read(void* out, size_t count) = 0;
        virtual size_t write(const void* in, size_t count) = 0;
        virtual void seek(size_t pos) = 0;
        virtual size_t tell() const = 0;
        virtual size_t size() const = 0;
    };

    class MemoryStream : public IStream {
    public:
        ByteBuffer buffer;
        size_t position = 0;

        MemoryStream() = default;
        MemoryStream(size_t size) : buffer(size) {}

        // IStream overrides
        size_t read(void* out, size_t count) override {
            size_t n = std::min(count, buffer.size() - position);
            memcpy(out, buffer.data() + position, n);
            position += n;
            return n;
        }

        size_t write(const void* in, size_t count) override {
            if (position + count > buffer.size())
                buffer.buffer.resize(position + count);

            memcpy(buffer.data() + position, in, count);
            position += count;
            return count;
        }

        void seek(size_t pos) override {
            if (pos > buffer.size())
                throw std::out_of_range("MemoryStream::seek");
            position = pos;
        }

        size_t tell() const override {
            return position;
        }

        size_t size() const override {
            return buffer.size();
        }

        // Typed helpers like FileStream
        template<typename T>
        T read() {
            T v{};
            size_t got = read(&v, sizeof(T));
            if (got != sizeof(T)) throw std::runtime_error("MemoryStream: read truncated");
            return v;
        }

        template<typename T>
        void write(const T& v) {
            write(&v, sizeof(T));
        }

        std::string read_string(size_t len) {
            std::string s(len, '\0');
            read(&s[0], len);
            return s;
        }

        std::string read_cstring() {
            std::string s;
            char c;
            while (read(&c, 1) == 1 && c != 0) s.push_back(c);
            return s;
        }

        void write_string(const std::string& s) {
            write(s.data(), s.size());
        }

        void write_cstring(const std::string& s) {
            write(s.data(), s.size());
            char zero = 0;
            write(&zero, 1);
        }
    };

    enum class FileMode : uint32_t {
        None        = 0,
        Read        = 1 << 0, // open for reading
        Write       = 1 << 1, // open for writing
        Append      = 1 << 2, // append at end
        Truncate    = 1 << 3, // clear file if exists
        Binary      = 1 << 4, // binary mode
        Create      = 1 << 5  // create if missing
    };

    inline FileMode operator|(FileMode a, FileMode b) {
        return static_cast<FileMode>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline bool operator&(FileMode a, FileMode b) {
        return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
    }

    inline std::ios::openmode ToStdMode(FileMode m) {
        std::ios::openmode mode = static_cast<std::ios::openmode>(0);

        if (m & FileMode::Read)     mode |= std::ios::in;
        if (m & FileMode::Write)    mode |= std::ios::out;
        if (m & FileMode::Append)   mode |= std::ios::app;
        if (m & FileMode::Truncate) mode |= std::ios::trunc;
        if (m & FileMode::Binary)   mode |= std::ios::binary;

        return mode;
    }

    class FileStream : public IStream {
    public:
        mutable std::fstream file; // allow tellg/seekg in const methods
        size_t cachedSize = 0;

        FileStream(const std::string& path, FileMode mode) {
            if ((mode & FileMode::Create) && !std::ifstream(path).good()) {
                std::ofstream tmp(path, std::ios::binary);
            }

            file.open(path, ToStdMode(mode));

            if (!file)
                throw std::runtime_error("FileStream: cannot open");

            auto cur = file.tellg();
            file.seekg(0, std::ios::end);
            cachedSize = static_cast<size_t>(file.tellg());
            file.seekg(cur);
        }

        ~FileStream() override { if (file.is_open()) file.close(); }

        // IStream overrides
        size_t read(void* out, size_t count) override {
            file.read((char*)out, count);
            return (size_t)file.gcount();
        }

        size_t write(const void* in, size_t count) override {
            file.write((const char*)in, count);
            return count;
        }

        void seek(size_t pos) override {
            file.seekg(pos);
            file.seekp(pos);
        }

        size_t tell() const override {
            return static_cast<size_t>(file.tellg());
        }

        size_t size() const override {
            return cachedSize;
        }

        // Typed helpers like ByteBuffer
        template<typename T>
        T read() {
            T v{};
            size_t got = read(&v, sizeof(T));
            if (got != sizeof(T)) throw std::runtime_error("FileStream: read truncated");
            return v;
        }

        template<typename T>
        void write(const T& v) {
            write(&v, sizeof(T));
        }

        std::string read_string(size_t len) {
            std::string s(len, '\0');
            read(&s[0], len);
            return s;
        }

        std::string read_cstring() {
            std::string s;
            char c;
            while (read(&c, 1) == 1 && c != 0) s.push_back(c);
            return s;
        }

        void write_string(const std::string& s) {
            write(s.data(), s.size());
        }

        void write_cstring(const std::string& s) {
            write(s.data(), s.size());
            char zero = 0;
            write(&zero, 1);
        }
    };


    class File {
    public:
#if CPP_AT_LEAST(CPP17)
        inline static bool Exists(const std::filesystem::path& path) {
            return std::filesystem::exists(path) && !std::filesystem::is_directory(path);
        }

        inline static void Delete(const std::filesystem::path& path) {
			if (std::filesystem::is_regular_file(path))
                std::filesystem::remove(path);
		}

		inline static std::string ReadAllText(const std::filesystem::path& path) {
            std::ifstream file(path, std::ios::binary);
            if (!file) return "";
            std::ostringstream ss;
            ss << file.rdbuf();
            return ss.str();
        }

        inline static void WriteAllText(const std::filesystem::path& path, const std::string& content) {
            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            if (file) {
                file << content;
            }
        }

		inline static void AppendAllText(const std::filesystem::path& path, const std::string& content) {
            std::ofstream file(path, std::ios::binary | std::ios::app);
            if (file) {
                file << content;
            }
        }

		inline static size_t GetSize(const std::filesystem::path& path) {
            if (std::filesystem::is_regular_file(path))
                return (size_t)std::filesystem::file_size(path);
            return 0;
        }

        inline static Bytes ReadAllBytes(const std::filesystem::path& path) {
            std::ifstream file(path, std::ios::binary);
            if (!file) return {};
            file.seekg(0, std::ios::end);
            size_t size = (size_t)file.tellg();
            file.seekg(0, std::ios::beg);
            Bytes data(size);
            file.read((char*)data.data(), size);
            return data;
		}

        inline static void WriteAllBytes(const std::filesystem::path& path, const Bytes& data) {
            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            if (file) {
                file.write((const char*)data.data(), data.size());
            }
		}

#else
        inline static bool Exists(const std::string& path) {
            DWORD attr = GetFileAttributesA(path.c_str());
            return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
		}

        inline static void Delete(const std::string& path) {
            DeleteFileA(path.c_str());
        }

        inline static std::string ReadAllText(const std::string& path) {
            std::ifstream file(path, std::ios::binary);
            if (!file) return "";

            std::ostringstream ss;
            ss << file.rdbuf();
            return ss.str();
        }

        inline static void WriteAllText(const std::string& path, const std::string& content) {
			std::ofstream file(path, std::ios::binary | std::ios::trunc);
			if (file) {
				file << content;
			}
		}

        inline static void AppendAllText(const std::string& path, const std::string& content) {
            std::ofstream file(path, std::ios::binary | std::ios::app);
            if (file) {
                file << content;
            }
		}

        inline static size_t GetSize(const std::string& path) {
            WIN32_FILE_ATTRIBUTE_DATA fad;
            if (GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &fad)) {
                LARGE_INTEGER size;
                size.HighPart = fad.nFileSizeHigh;
                size.LowPart = fad.nFileSizeLow;
                return (size_t)size.QuadPart;
            }
            return 0;
		}

        inline static Bytes ReadAllBytes(const std::string& path) {
            std::ifstream file(path, std::ios::binary);
            if (!file) return {};
            file.seekg(0, std::ios::end);
            size_t size = (size_t)file.tellg();
            file.seekg(0, std::ios::beg);
            Bytes data(size);
            file.read((char*)data.data(), size);
            return data;
		}

        inline static void WriteAllBytes(const std::string& path, const Bytes& data) {
            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            if (file) {
                file.write((const char*)data.data(), data.size());
            }
        }
#endif
    };


    //============================
    // File descriptor
    //============================
    enum FileDescriptor {
        STDIN = 0,
        STDOUT = 1,
        STDERR = 2
    };

    //============================
    // String formatting helpers
    //============================
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

    //============================
    // Print utilities
    //============================
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

    //============================
    // Input capture
    //============================
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

    inline void PromptInput(const char* prompt, std::string& outStr, size_t cap) { Print(prompt); CaptureInput(outStr, cap); }
    inline void PromptInput(const char* prompt, char* buf, size_t cap) { Print(prompt); CaptureInput(buf, cap); }

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
            PROCESSENTRY32 pe{ sizeof(pe) };
            std::regex r(patt, std::regex_constants::icase);
            bool found = false;
            if (Process32First(snap, &pe)) {
                do {
                    if (std::regex_search(pe.szExeFile, r)) { found = true; break; }
                } while (Process32Next(snap, &pe));
            }
            CloseHandle(snap); return found;
        }

        inline bool IsRunningExact(const std::string& exe) {
            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snap == INVALID_HANDLE_VALUE) return false;
            PROCESSENTRY32 pe{ sizeof(pe) };
            bool found = false;
            if (Process32First(snap, &pe)) {
                do {
                    if (_stricmp(pe.szExeFile, exe.c_str()) == 0) { found = true; break; }
                } while (Process32Next(snap, &pe));
            }
            CloseHandle(snap); return found;
        }
    };

    //============================
    // ordered_map (insertion-preserving associative container)
    //============================
    template<typename K, typename V>
    class ordered_map {
    public:
        using key_type = K;
        using mapped_type = V;
        using value_type = std::pair<const K, V>;
        using size_type = size_t;
        using container_type = std::vector<std::pair<K, V>>;
        using iterator = typename container_type::iterator;
        using const_iterator = typename container_type::const_iterator;

        ordered_map() = default;
        ordered_map(const ordered_map&) = default;
        ordered_map& operator=(const ordered_map&) = default;
        ordered_map(ordered_map&&) noexcept = default;
        ordered_map& operator=(ordered_map&&) noexcept = default;

        inline V& operator[](const K& key) {
            auto it = m_index.find(key);
            if (it == m_index.end()) return push_back(key, V{})->second;
            return m_data[it->second].second;
        }

        inline V& operator[](K&& key) {
            auto it = m_index.find(key);
            if (it == m_index.end()) return push_back(std::move(key), V{})->second;
            return m_data[it->second].second;
        }

        inline V& at(const K& key) { return m_data.at(m_index.at(key)).second; }
        inline const V& at(const K& key) const { return m_data.at(m_index.at(key)).second; }

        inline std::pair<iterator, bool> insert(iterator pos, const value_type& value) {
            auto it = m_index.find(value.first);
            if (it != m_index.end()) return { m_data.begin() + it->second,false };
            auto p = m_data.insert(pos, { value.first,value.second });
            rebuild_index(); return { p,true };
        }

        inline std::pair<iterator, bool> insert(iterator pos, value_type&& value) {
            auto it = m_index.find(value.first);
            if (it != m_index.end()) return { m_data.begin() + it->second,false };
            auto p = m_data.insert(pos, std::move(value));
            rebuild_index(); return { p,true };
        }

        inline iterator push_back(const K& key, V&& value) {
            auto it = m_index.find(key);
            if (it != m_index.end()) return m_data.begin() + it->second;
            m_data.emplace_back(key, std::move(value));
            m_index[key] = m_data.size() - 1;
            return std::prev(m_data.end());
        }

        inline iterator push_back(K&& key, V&& value) {
            auto it = m_index.find(key);
            if (it != m_index.end()) return m_data.begin() + it->second;
            m_data.emplace_back(std::move(key), std::move(value));
            m_index[m_data.back().first] = m_data.size() - 1;
            return std::prev(m_data.end());
        }

        template<typename... Args>
        inline iterator emplace_back(const K& key, Args&&... args) {
            auto it = m_index.find(key);
            if (it != m_index.end()) return m_data.begin() + it->second;
            m_data.emplace_back(key, V(std::forward<Args>(args)...));
            m_index[key] = m_data.size() - 1;
            return std::prev(m_data.end());
        }

        template<typename... Args>
        inline std::pair<iterator, bool> emplace(const K& key, Args&&... args) {
            auto it = m_index.find(key);
            if (it != m_index.end()) return { m_data.begin() + it->second,false };
            m_data.emplace_back(key, V(std::forward<Args>(args)...));
            m_index[key] = m_data.size() - 1;
            return { std::prev(m_data.end()),true };
        }

        inline void erase(const K& key) {
            auto it = m_index.find(key);
            if (it == m_index.end()) return;
            size_t idx = it->second;
            m_data.erase(m_data.begin() + idx);
            m_index.erase(it);
            rebuild_index();
        }

        inline iterator erase(iterator pos) {
            if (pos == m_data.end()) return m_data.end();
            m_index.erase(pos->first);
            auto nxt = m_data.erase(pos);
            rebuild_index(); return nxt;
        }

        inline void clear() noexcept { m_data.clear(); m_index.clear(); }

        inline iterator find(const K& key) {
            auto it = m_index.find(key);
            return it == m_index.end() ? m_data.end() : m_data.begin() + it->second;
        }

        inline const_iterator find(const K& key) const {
            auto it = m_index.find(key);
            return it == m_index.end() ? m_data.end() : m_data.begin() + it->second;
        }

        inline bool contains(const K& key) const noexcept { return m_index.find(key) != m_index.end(); }
        inline size_type count(const K& key) const noexcept { return m_index.count(key); }

        inline iterator begin() noexcept { return m_data.begin(); }
        inline iterator end() noexcept { return m_data.end(); }
        inline const_iterator begin() const noexcept { return m_data.begin(); }
        inline const_iterator end() const noexcept { return m_data.end(); }
        inline const_iterator cbegin() const noexcept { return m_data.cbegin(); }
        inline const_iterator cend() const noexcept { return m_data.cend(); }

        inline bool empty() const noexcept { return m_data.empty(); }
        inline size_type size() const noexcept { return m_data.size(); }

    private:
        inline void rebuild_index() {
            m_index.clear();
            for (size_t i = 0; i < m_data.size(); ++i) m_index[m_data[i].first] = i;
        }

        container_type m_data;
        std::unordered_map<K, size_t> m_index;
    };

} // namespace stdx
