#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <type_traits>

namespace stdx {
    //============================
    // Utility methods
    //============================
    class string {
    public:
        string() = default;

        string(const char* s) { fromUtf8(std::string(s)); }
        string(const std::string& s) { fromUtf8(s); }
        string(const wchar_t* s) { fromWstr(std::wstring(s)); }
        string(const std::wstring& s) { fromWstr(s); }
        string(const std::u32string& s) : data(s.begin(), s.end()) {}

        operator std::string() const { return toUtf8(); }
        operator std::wstring() const { return toWstr(); }

		inline size_t size() const { return data.size(); }
		inline bool empty() const { return data.empty(); }

        struct utf8_t {};
        struct utf16_t {};
        struct utf32_t {};
        template<typename Encoding>
        auto c_str() const
        {
            static_assert(std::is_same_v<Encoding, utf8_t> ||
                          std::is_same_v<Encoding, utf16_t> ||
                          std::is_same_v<Encoding, utf32_t>,
				"Unsupported encoding type");

            return select_cstr(static_cast<Encoding*>(nullptr));
        }

		inline void clear() { data.clear(); }
		inline void push_back(char32_t c) { data.push_back(c); }
        inline void push_back(char c) {
            if ((unsigned char)c < 0x80) {
                data.push_back((char32_t)c);
            }
        }
        inline void push_back(wchar_t c) {
#ifdef _WIN32
            // cannot append a lone high surrogate
            if (c >= 0xD800 && c <= 0xDBFF) return; // invalid alone
            if (c >= 0xDC00 && c <= 0xDFFF) return; // invalid alone
            data.push_back((char32_t)c);
#else
            data.push_back((char32_t)c);
#endif
        }

		inline char32_t& back() { return data.back(); }

		inline void append(const stdx::string& other) {
            data.insert(data.end(), other.data.begin(), other.data.end());
        }
        inline void append(const std::string& other) {
            auto u = utf8_to_utf32(other);
            data.insert(data.end(), u.begin(), u.end());
        }
        inline void append(const std::wstring& other)
        {
#ifdef _WIN32
            std::u32string u = utf16_to_utf32(
                std::u16string(other.begin(), other.end())
            );
            data.insert(data.end(), u.begin(), u.end());
#else
            data.insert(data.end(), other.begin(), other.end());
#endif
        }

		inline void resize(size_t newSize) { data.resize(newSize); }

        inline stdx::string substr(size_t pos, size_t len) const {
            stdx::string result;
            if (pos < data.size()) {
                size_t rlen = std::min(len, data.size() - pos);
                result.data.assign(data.begin() + pos, data.begin() + pos + rlen);
            }
            return result;
		}

        inline stdx::string to_upper() const {
            stdx::string result = *this;
            for (auto& c : result.data) {
                if (c >= U'a' && c <= U'z') {
                    c = c - U'a' + U'A';
                }
            }
            return result;
		}

        inline stdx::string to_lower() const {
            stdx::string result = *this;
            for (auto& c : result.data) {
                if (c >= U'A' && c <= U'Z') {
                    c = c - U'A' + U'a';
                }
            }
            return result;
        }

        inline stdx::string replace(char32_t oldChar, char32_t newChar) const {
            stdx::string result = *this;
            for (auto& c : result.data) {
                if (c == oldChar) {
                    c = newChar;
                }
            }
            return result;
		}

		inline stdx::string replace(const stdx::string& oldStr, const stdx::string& newStr) const {
            stdx::string result;
            size_t pos = 0;
            while (pos < data.size()) {
                bool matched = true;
                if (pos + oldStr.data.size() <= data.size()) {
                    for (size_t i = 0; i < oldStr.data.size(); ++i) {
                        if (data[pos + i] != oldStr.data[i]) {
                            matched = false;
                            break;
                        }
                    }
                }
                else {
                    matched = false;
                }
                if (matched) {
                    result.data.insert(result.data.end(), newStr.data.begin(), newStr.data.end());
                    pos += oldStr.data.size();
                }
                else {
                    result.data.push_back(data[pos]);
                    pos++;
                }
            }
            return result;
        }

        inline stdx::string replace(const std::string& oldStr, const std::string& newStr) const {
            return replace(stdx::string(oldStr), stdx::string(newStr));
        }

        inline stdx::string replace(const std::wstring& oldStr, const std::wstring& newStr) const {
            return replace(stdx::string(oldStr), stdx::string(newStr));
        }

        inline bool starts_with(const stdx::string& prefix) const {
            if (prefix.data.size() > data.size()) return false;
            for (size_t i = 0; i < prefix.data.size(); ++i) {
                if (data[i] != prefix.data[i]) return false;
            }
            return true;
		}

        inline bool ends_with(const stdx::string& suffix) const {
            if (suffix.data.size() > data.size()) return false;
            size_t offset = data.size() - suffix.data.size();
            for (size_t i = 0; i < suffix.data.size(); ++i) {
                if (data[offset + i] != suffix.data[i]) return false;
            }
            return true;
		}

        inline void trim_start() {
            size_t start = 0;
            while (start < data.size() && (data[start] == U' ' || data[start] == U'\t' || data[start] == U'\n' || data[start] == U'\r')) {
                start++;
            }
            if (start > 0) {
                data.erase(data.begin(), data.begin() + start);
            }
		}

        inline stdx::string trimmed_start() const {
            stdx::string result = *this;
            result.trim_start();
            return result;
		}

        inline void trim_end() {
            if (data.empty()) return;
            size_t end = data.size() - 1;
            while (end != (size_t)-1 && (data[end] == U' ' || data[end] == U'\t' || data[end] == U'\n' || data[end] == U'\r')) {
                if (end == 0) { data.clear(); return; }
                end--;
            }
            data.erase(data.begin() + end + 1, data.end());
        }

        inline stdx::string trimmed_end() const {
            stdx::string result = *this;
            result.trim_end();
            return result;
        }

        inline void trim() {
            trim_start();
            trim_end();
		}

        inline stdx::string trimmed() const {
            stdx::string result = *this;
            result.trim();
            return result;
		}

        inline size_t length() const {
            return data.size();
		}

		inline size_t index_of(char32_t c) const {
            for (size_t i = 0; i < data.size(); ++i) {
                if (data[i] == c) return i;
            }
            return stdx::string::npos;
        }

        inline size_t last_index_of(char32_t c) const {
            for (size_t i = data.size(); i-- > 0;) {
                if (data[i] == c) return i;
            }
            return stdx::string::npos;
		}

        inline bool contains(char32_t c) const {
            return index_of(c) != stdx::string::npos;
        }

		inline bool contains(const stdx::string& str) const {
            if (str.data.size() > data.size()) return false;
            for (size_t i = 0; i <= data.size() - str.data.size(); ++i) {
                bool matched = true;
                for (size_t j = 0; j < str.data.size(); ++j) {
                    if (data[i + j] != str.data[j]) {
                        matched = false;
                        break;
                    }
                }
                if (matched) return true;
            }
            return false;
        }

        inline bool contains(const std::string& str) const {
            return contains(stdx::string(str));
		}

        inline bool contains(const std::wstring& str) const {
            return contains(stdx::string(str));
        }

		inline std::vector<stdx::string> split(char32_t delimiter) const {
            if (data.empty()) return {};
            std::vector<stdx::string> result;
            size_t start = 0;
            for (size_t i = 0; i < data.size(); ++i) {
                if (data[i] == delimiter) {
                    stdx::string part;
                    part.data.assign(data.begin() + start, data.begin() + i);
                    result.push_back(part);
                    start = i + 1;
                }
            }
            if (start < data.size()) {
                stdx::string part;
                part.data.assign(data.begin() + start, data.end());
                result.push_back(part);
            }
            return result;
        }

		inline void pad_left(size_t totalWidth, char32_t paddingChar = U' ') {
            if (data.size() >= totalWidth) return;
            size_t padSize = totalWidth - data.size();
            data.insert(data.begin(), padSize, paddingChar);
        }

		inline stdx::string pad_left(size_t totalWidth, char32_t paddingChar = U' ') const {
            stdx::string result = *this;
            result.pad_left(totalWidth, paddingChar);
            return result;
        }

        inline void pad_right(size_t totalWidth, char32_t paddingChar = U' ') {
            if (data.size() >= totalWidth) return;
            size_t padSize = totalWidth - data.size();
            data.insert(data.end(), padSize, paddingChar);
		}

        inline stdx::string pad_right(size_t totalWidth, char32_t paddingChar = U' ') const {
            stdx::string result = *this;
            result.pad_right(totalWidth, paddingChar);
            return result;
        }

        inline void insert(size_t index, const stdx::string& str) {
            if (index > data.size()) index = data.size();
            data.insert(data.begin() + index, str.data.begin(), str.data.end());
		}

        inline stdx::string insert(size_t index, const stdx::string& str) const {
            stdx::string result = *this;
            result.insert(index, str);
            return result;
		}

		inline void remove(size_t index, size_t count) {
            if (index >= data.size()) return;
            size_t rcount = std::min(count, data.size() - index);
            data.erase(data.begin() + index, data.begin() + index + rcount);
        }

        inline stdx::string remove(size_t index, size_t count) const {
            stdx::string result = *this;
            result.remove(index, count);
            return result;
		}

        inline char32_t at(size_t index) const {
            return (index < data.size()) ? data[index] : U'\0';
        }

        //============================
		// Operator overloads
        //============================
        inline stdx::string& operator+=(const std::string& rhs) {
            auto u = utf8_to_utf32(rhs);
            data.insert(data.end(), u.begin(), u.end());
            return *this;
        }

        inline stdx::string& operator+=(const std::wstring& rhs) {
#ifdef _WIN32
            auto u16 = std::u16string(rhs.begin(), rhs.end());
            auto u32 = utf16_to_utf32(u16);
            data.insert(data.end(), u32.begin(), u32.end());
#else
            data.insert(data.end(), rhs.begin(), rhs.end());
#endif
            return *this;
        }

        inline stdx::string operator+(const std::string& rhs) const {
            stdx::string out = *this;
            out += rhs;
            return out;
        }

        inline stdx::string operator+(const std::wstring& rhs) const {
            stdx::string out = *this;
            out += rhs;
            return out;
        }

        inline stdx::string operator+(const stdx::string& rhs) const {
            stdx::string out = *this;
            out.data.insert(out.data.end(), rhs.data.begin(), rhs.data.end());
            return out;
		}

        inline stdx::string operator+=(const stdx::string& rhs) {
            data.insert(data.end(), rhs.data.begin(), rhs.data.end());
            return *this;
		}

        inline bool operator==(const stdx::string& rhs) const {
            return data == rhs.data;
		}

        inline bool operator!=(const stdx::string& rhs) const {
            return data != rhs.data;
        }

        inline bool operator==(const std::string& rhs) const {
            auto u = utf8_to_utf32(rhs);
            return data.size() == u.size() && std::equal(data.begin(), data.end(), u.begin());
        }

        inline bool operator!=(const std::string& rhs) const {
            return !(*this == rhs);
        }

        inline bool operator==(const std::wstring& rhs) const {
            auto u = from_wstring(rhs);
            return data.size() == u.size() && std::equal(data.begin(), data.end(), u.begin());
        }

        inline bool operator!=(const std::wstring& rhs) const {
            return !(*this == rhs);
        }

        inline char32_t& operator[](size_t index) {
            return data[index];
        }

        inline const char32_t& operator[](size_t index) const {
            return data[index];
        }

        inline auto begin() { return data.begin(); }
        inline auto end() { return data.end(); }
        inline auto begin() const { return data.begin(); }
        inline auto end() const { return data.end(); }
        inline auto cbegin() const { return data.cbegin(); }
        inline auto cend() const { return data.cend(); }

        std::vector<char32_t> data;
    private:

        mutable std::string  buf8;
        mutable std::wstring buf16;
        mutable std::u32string buf32;

        const char32_t* select_cstr(utf32_t*) const
        {
            buf32.assign(data.begin(), data.end());
            buf32.push_back(U'\0');
            return buf32.c_str();
        }

        const char* select_cstr(utf8_t*) const
        {
            // convert UTF-32 vector to string
            buf8 = utf32_to_utf8(std::u32string(data.begin(), data.end()));
            return buf8.c_str();
        }

        const wchar_t* select_cstr(utf16_t*) const
        {
            buf16 = toWstr();
            return buf16.c_str();
        }

        // utf8 -> utf32
        void fromUtf8(const std::string& s) {
#ifdef _WIN32
            int wlen = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, 0, 0);
            if (wlen <= 0) { data.clear(); return; }
            std::wstring w(wlen, 0);
            MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], wlen);
            fromWstr(w);
#else
            std::u32string u = utf8_to_utf32(s);
            data.assign(u.begin(), u.end());
#endif
        }

        // utf16 or utf32 platform dependent
        void fromWstr(const std::wstring& w) {
#ifdef _WIN32
            data.clear();
            data.reserve(w.size());
            for (size_t i = 0; i < w.size(); ++i) {
                wchar_t c = w[i];

                if (c == 0) break; // STOP at null terminator

                if (c >= 0xD800 && c <= 0xDBFF && i + 1 < w.size()) {
                    wchar_t c2 = w[i + 1];
                    if (c2 >= 0xDC00 && c2 <= 0xDFFF) {
                        char32_t cp = 0x10000 + (((c - 0xD800) << 10) | (c2 - 0xDC00));
                        data.push_back(cp);
                        ++i;
                        continue;
                    }
                }

                data.push_back((char32_t)c);
            }
#else
            data.assign(w.begin(), w.end());
#endif
        }

        // utf32 -> utf8
        std::string toUtf8() const {
#ifdef _WIN32
            std::wstring w = toWstr();
            int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, 0, 0, 0, 0);
            std::string s(len, 0);
            WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], len, 0, 0);
            return s;
#else
            std::u32string u(data.begin(), data.end());
            return utf32_to_utf8(u);
#endif
        }

        // utf32 -> wstring
        std::wstring toWstr() const {
#ifdef _WIN32
            std::wstring w;
            w.reserve(data.size());
            for (char32_t cp : data) {
                if (cp > 0xFFFF) {
                    cp -= 0x10000;
                    wchar_t high = 0xD800 + ((cp >> 10) & 0x3FF);
                    wchar_t low = 0xDC00 + (cp & 0x3FF);
                    w.push_back(high);
                    w.push_back(low);
                }
                else {
                    w.push_back((wchar_t)cp);
                }
            }
            w.push_back(0);
            return w;
#else
            return std::wstring(data.begin(), data.end()); // direct on Linux
#endif
        }

        static std::u32string from_wstring(const std::wstring& w)
        {
#ifdef _WIN32
            return utf16_to_utf32(std::u16string(w.begin(), w.end()));
#else
            return std::u32string(w.begin(), w.end());
#endif
        }

        static std::wstring to_wstring(const std::u32string& u)
        {
#ifdef _WIN32
            auto u16 = utf32_to_utf16(u);
            return std::wstring(u16.begin(), u16.end());
#else
            return std::wstring(u.begin(), u.end());
#endif
        }

        static std::u32string utf8_to_utf32(const std::string& s)
        {
            std::u32string out;
            out.reserve(s.size());

            size_t i = 0;
            while (i < s.size())
            {
                uint32_t c = (unsigned char)s[i];
                size_t extra = 0;

                if (c < 0x80) { extra = 0; }
                else if ((c >> 5) == 0x6) { extra = 1; c &= 0x1F; }
                else if ((c >> 4) == 0xE) { extra = 2; c &= 0x0F; }
                else if ((c >> 3) == 0x1E) { extra = 3; c &= 0x07; }
                else { out.push_back(0xFFFD); i++; continue; }

                if (i + extra >= s.size()) { out.push_back(0xFFFD); break; }

                for (size_t j = 1; j <= extra; j++)
                {
                    unsigned char cc = s[i + j];
                    if ((cc >> 6) != 0x2) { out.push_back(0xFFFD); goto next; }
                    c = (c << 6) | (cc & 0x3F);
                }

                out.push_back(c);
                i += extra + 1;
                continue;

            next:
                i++;
            }

            return out;
        }

        static std::string utf32_to_utf8(const std::u32string& s)
        {
            std::string out;
            out.reserve(s.size());

            for (uint32_t cp : s)
            {
                if (cp <= 0x7F)
                {
                    out.push_back((char)cp);
                }
                else if (cp <= 0x7FF)
                {
                    out.push_back((char)(0xC0 | (cp >> 6)));
                    out.push_back((char)(0x80 | (cp & 0x3F)));
                }
                else if (cp <= 0xFFFF)
                {
                    out.push_back((char)(0xE0 | (cp >> 12)));
                    out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
                    out.push_back((char)(0x80 | (cp & 0x3F)));
                }
                else
                {
                    out.push_back((char)(0xF0 | (cp >> 18)));
                    out.push_back((char)(0x80 | ((cp >> 12) & 0x3F)));
                    out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
                    out.push_back((char)(0x80 | (cp & 0x3F)));
                }
            }

            return out;
        }

        static std::u32string utf16_to_utf32(const std::u16string& in)
        {
            std::u32string out;
            out.reserve(in.size());

            for (size_t i = 0; i < in.size(); ++i)
            {
                char16_t c = in[i];

                // high surrogate?
                if (c >= 0xD800 && c <= 0xDBFF && i + 1 < in.size())
                {
                    char16_t c2 = in[i + 1];
                    if (c2 >= 0xDC00 && c2 <= 0xDFFF)
                    {
                        char32_t cp = 0x10000 + (((c - 0xD800) << 10) | (c2 - 0xDC00));
                        out.push_back(cp);
                        ++i;
                        continue;
                    }
                }

                // lone surrogate or BMP
                out.push_back((char32_t)c);
            }

            return out;
        }

        // UTF32 -> UTF16
        static std::u16string utf32_to_utf16(const std::u32string& in)
        {
            std::u16string out;
            out.reserve(in.size());

            for (char32_t cp : in)
            {
                if (cp <= 0xFFFF)
                {
                    out.push_back((char16_t)cp);
                }
                else
                {
                    cp -= 0x10000;
                    char16_t high = 0xD800 + ((cp >> 10) & 0x3FF);
                    char16_t low = 0xDC00 + (cp & 0x3FF);
                    out.push_back(high);
                    out.push_back(low);
                }
            }

            return out;
        }

    public:
        static constexpr size_t npos = (size_t)-1;

        //============================
        // Null checkers
        //============================
        template<typename T>
        inline static bool is_null_or_empty(const T* s) {
            return !s || !s[0];
        }

        template<typename T>
        inline static bool is_null_or_empty(const std::basic_string<T>& s) {
            return s.empty();
        }

        inline static bool is_null_or_empty(const stdx::string& s) {
            return s.empty();
        }

        template<typename T>
        inline static bool is_empty_or_whitespace(const std::basic_string<T>& s) {
            for (T c : s) {
                if (!isspace((unsigned char)c)) return false;
            }
            return true;
        }

        inline static bool is_empty_or_whitespace(const stdx::string& s) {
            for (char32_t c : s) {
                if (!(c == U' ' || c == U'\t' || c == U'\n' || c == U'\r' || c == U'\f' || c == U'\v'))
                    return false;
            }
            return true;
        }

		//============================
        // Helpers
		//============================
        template<typename T>
        inline static stdx::string to_string(T value) {
			static_assert(std::is_arithmetic_v<T>, "to_string only supports arithmetic types");
            return stdx::string(std::to_string(value));
		}

        inline static stdx::string to_string(double value, int precision) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%.*f", precision, value);
            return stdx::string(buf);
        }

        inline static stdx::string to_string(float value, int precision) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%.*f", precision, value);
            return stdx::string(buf);
        }

        inline static stdx::string join(const std::vector<stdx::string>& parts, const stdx::string& delimiter) {
            if (parts.empty()) return stdx::string();
            stdx::string result = parts[0];
            for (size_t i = 1; i < parts.size(); ++i) {
                result += delimiter;
                result += parts[i];
            }
            return result;
		}

        inline static stdx::string join(const std::vector<std::string>& parts, const stdx::string& delimiter) {
            if (parts.empty()) return stdx::string();
            stdx::string result = stdx::string(parts[0]);
            for (size_t i = 1; i < parts.size(); ++i) {
                result += delimiter;
                result += stdx::string(parts[i]);
            }
            return result;
		}

        inline static stdx::string join(const std::vector<std::wstring>& parts, const stdx::string& delimiter) {
            if (parts.empty()) return stdx::string();
            stdx::string result = stdx::string(parts[0]);
            for (size_t i = 1; i < parts.size(); ++i) {
                result += delimiter;
                result += stdx::string(parts[i]);
            }
            return result;
        }
    };
}