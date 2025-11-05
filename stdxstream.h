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

#include <vector>
#include <cstdint>
#include <sstream>
#include <fstream>
#include <string>

#if CPP_AT_LEAST(CPP17)
#include <filesystem>
#endif


namespace stdx {
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
        None = 0,
        Read = 1 << 0, // open for reading
        Write = 1 << 1, // open for writing
        Append = 1 << 2, // append at end
        Truncate = 1 << 3, // clear file if exists
        Binary = 1 << 4, // binary mode
        Create = 1 << 5  // create if missing
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
#if CPP_AT_LEAST(CPP17)
        FileStream(const std::filesystem::path& path, FileMode mode) {
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
#endif

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
}