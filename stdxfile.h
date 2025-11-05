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

#if CPP_AT_LEAST(CPP17)
#include <filesystem>
#endif

#include "stdxstream.h"

namespace stdx {
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

        inline static stdx::Bytes ReadAllBytes(const std::filesystem::path& path) {
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
#ifdef _WIN32
            DWORD attr = GetFileAttributesA(path.c_str());
            return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
#else
            struct stat st;
            return (stat(path.c_str(), &st) == 0) && S_ISREG(st.st_mode);
#endif
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
#ifdef _WIN32
            WIN32_FILE_ATTRIBUTE_DATA fad;
            if (GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &fad)) {
                LARGE_INTEGER size;
                size.HighPart = fad.nFileSizeHigh;
                size.LowPart = fad.nFileSizeLow;
                return (size_t)size.QuadPart;
            }
            return 0;
#else
        std::ifstream f(path.c_str(), std::ios::binary | std::ios::ate);
        if(!f)
            return 0;
        return static_cast<size_t>(f.tellg());
#endif
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
}