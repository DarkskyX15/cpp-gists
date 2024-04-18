/*
 * @Date: 2024-03-28 16:37:08
 * @Author: DarkskyX15
 * @LastEditTime: 2024-04-18 22:11:11
 */

#ifndef _DYNAMIC_STRING_HPP_
#define _DYNAMIC_STRING_HPP_

#include <string>
#include <vector>
#include <iostream>

namespace DynamicString
{
    
    class MixedChar{
    public:
        /// @brief 默认构造
        MixedChar() = default;
        MixedChar(char __chr);
        MixedChar(std::string::const_iterator __begin, int __len);
        MixedChar(const char* __begin, int __len);
        friend std::ostream& operator<< (std::ostream& __out, const MixedChar& __mc) {
            for (auto it : __mc.bytes) { __out.put(it); }
            return __out;
        }

        inline bool isUnicode() const { return this->is_unicode; }
        inline int getCodeData() const { return this->unicode_data; }
        inline int getSize() const { return static_cast<int>(present_size); }
        inline bool set(char __chr);
        bool set(const char* __multi_chr);
        inline char& operator[](int __index) { return bytes[__index]; };
        void operator= (char __chr) { set(__chr); }
        void operator= (const char* __multi_chr) { set(__multi_chr); }

    private:
        char present_size;
        bool is_unicode;
        int unicode_data;
        std::vector<char> bytes;

        void generateUnicode();
    };

    class MixedString{
    public:
        MixedString();
        MixedString(const char* __cstr);
        MixedString(const std::string& __str);
        MixedString(const MixedString& __ms);
        MixedString(MixedString&& __ms);
        ~MixedString();

        friend std::ostream& operator<< (std::ostream& __out, const MixedString& __ms) {
            for (auto it : __ms.str_data) {
                __out << it;
            }
            return __out;
        }

        MixedString operator+ (const MixedString& __m_str) const;
        void operator+= (const MixedString& __m_str);
        inline MixedChar& operator[] (int __index);
        inline const MixedChar& operator[] (int __index) const;
        MixedString& operator= (const MixedString& __m_str);
        MixedString& operator= (MixedString&& __m_str);

        inline MixedChar& at (int __index);
        inline const MixedChar& at (int __index) const;
        inline int size() const { return str_data.size(); }
        inline int rawSize() {
            updateRawSize();
            return raw_size;
        }
        const char* c_str();

        std::vector<MixedChar>::iterator begin() {
            return str_data.begin();
        }
        std::vector<MixedChar>::const_iterator begin() const {
            return str_data.cbegin();
        }
        std::vector<MixedChar>::iterator end() {
            return str_data.end();
        }
        std::vector<MixedChar>::const_iterator end() const {
            return str_data.cend();
        }

    private:
        int raw_size;
        char* cstr_ptr;
        std::vector<MixedChar> str_data;

        void updateRawSize();
        inline bool onBit(int n, short __pos);
    };

    /* MixedString */

    /// @brief 空构造
    MixedString::MixedString(): str_data() {
        cstr_ptr = nullptr;
        raw_size = 0;
    }

    /// @brief 混合字符串构造
    /// @param __cstr 由C风格字符串构造
    MixedString::MixedString(const char* __cstr): str_data() {
        cstr_ptr = nullptr;
        int index = 0, bit, utf_size = 0;
        while (true) {
            if (__cstr[index] == '\000') break;
            bit = static_cast<unsigned char>(__cstr[index]);
            utf_size = 0;
            if (onBit(bit, 7)) {
                utf_size = 1;
                for (int i = 1; i < 4; ++i) {
                    if (onBit(bit, 7 - i)) utf_size += 1;
                }
                str_data.push_back(MixedChar((__cstr + index), utf_size));
                index += utf_size;
            } else {
                str_data.push_back(MixedChar(__cstr[index]));
                ++index;
            }
        }
        raw_size = index;
    }

    /// @brief 混合字符串构造
    /// @param __str 由C++风格字符串构造
    MixedString::MixedString(const std::string& __str): str_data() {
        cstr_ptr = nullptr;
        int bit, utf_size = 0;
        auto it = __str.begin();
        while (true) {
            if (it == __str.end()) break;
            bit = static_cast<unsigned char>(*it);
            utf_size = 0;
            if (onBit(bit, 7)) {
                utf_size = 1;
                for (int i = 1; i < 4; ++i) {
                    if (onBit(bit, 7 - i)) utf_size += 1;
                }
                str_data.push_back(MixedChar(it, utf_size));
                it += utf_size;
            } else {
                str_data.push_back(MixedChar(*it));
                ++it;
            }
        }
        raw_size = __str.size();
    }

    MixedString::MixedString(const MixedString& __ms): str_data(__ms.str_data) {
        int c_size = __ms.str_data.size();
        raw_size = __ms.raw_size;    
        if (__ms.cstr_ptr != nullptr) {
            cstr_ptr = new char[c_size + 1]();
            for (int i = 0; i < c_size + 1; ++i) {
                cstr_ptr[i] = __ms.cstr_ptr[i];
            }
        } else cstr_ptr = nullptr;
    }

    MixedString::MixedString(MixedString&& __ms): str_data() {
        raw_size = __ms.raw_size;
        cstr_ptr = __ms.cstr_ptr;
        __ms.cstr_ptr = nullptr;
        str_data = std::move(__ms.str_data);
    }

    MixedString::~MixedString() {
        if (cstr_ptr != nullptr) {
            delete[] cstr_ptr;
            cstr_ptr = nullptr;
        }
    }

    // Public Functions

    inline MixedChar& MixedString::at (int __index) {
        return str_data[__index];
    }
    inline const MixedChar& MixedString::at (int __index) const {
        return str_data[__index];
    }

    inline MixedChar& MixedString::operator[] (int __index) {
        return str_data[__index];
    }
    inline const MixedChar& MixedString::operator[] (int __index) const {
        return str_data[__index];
    }

    const char* MixedString::c_str() {
        updateRawSize();
        if (cstr_ptr != nullptr) delete[] cstr_ptr;
        cstr_ptr = new char[raw_size + 1]();
        for (int i = 0, j = 0; i < str_data.size(); ++i) {
            for (int z = 0; z < str_data[i].getSize(); ++z, ++j) {
                cstr_ptr[j] = str_data[i][z];
            }
        }
        cstr_ptr[raw_size] = '\000';
        return cstr_ptr;
    }

    MixedString MixedString::operator+ (const MixedString& __m_str) const {
        MixedString temp(*this);
        temp.raw_size += __m_str.raw_size;
        temp.str_data.reserve(__m_str.str_data.size());
        for (auto& mc : __m_str.str_data) {
            temp.str_data.push_back(mc);
        }
        return temp;
    }

    void MixedString::operator+= (const MixedString& __m_str) {
        raw_size += __m_str.raw_size;
        str_data.reserve(str_data.size() + __m_str.str_data.size());
        for (auto& mc : __m_str.str_data) {
            str_data.push_back(mc);
        }
    }

    MixedString& MixedString::operator= (const MixedString& __m_str) {
        if (&__m_str == this) return *this;
        raw_size = __m_str.raw_size;
        str_data = __m_str.str_data;
        if (__m_str.cstr_ptr != nullptr) {
            cstr_ptr = new char[raw_size + 1]();
            for (int i = 0; i < raw_size + 1; ++i) {
                cstr_ptr[i] = __m_str.cstr_ptr[i];
            }
        } else cstr_ptr = nullptr;
        return *this;
    }

    MixedString& MixedString::operator= (MixedString&& __m_str) {
        if (&__m_str == this) return *this;
        raw_size = __m_str.raw_size;
        str_data = std::move(__m_str.str_data);
        cstr_ptr = __m_str.cstr_ptr;
        __m_str.cstr_ptr = nullptr;
        return *this;
    }

    // Private Functions

    inline bool MixedString::onBit(int n, short __pos) {
        return n & (1 << __pos);
    }

    void MixedString::updateRawSize() {
        int length = 0;
        for (auto& m_char : str_data) {
            length += m_char.getSize();
        }
        raw_size = length;
    }

    /* MixedChar */

    MixedChar::MixedChar(char __chr) {
        present_size = 1;
        is_unicode = false;
        unicode_data = static_cast<int>(__chr);
        bytes.push_back(__chr);
    }
    MixedChar::MixedChar(std::string::const_iterator __begin, int __len) {
        present_size = __len;
        is_unicode = true;
        bytes.reserve(__len);
        for (int i = 0; i < __len; ++i, ++__begin) bytes.push_back(*__begin);
        generateUnicode();
    }
    MixedChar::MixedChar(const char* __begin, int __len) {
        present_size = __len;
        is_unicode = true;
        bytes.reserve(__len);
        for (int i = 0; i < __len; ++i) bytes.push_back(__begin[i]);
        generateUnicode();
    }

    // Private Funcs
    
    void MixedChar::generateUnicode() {
        unicode_data = 0;
        unicode_data |= (bytes[0] & 0b1111);
        for (int i = 1; i < bytes.size(); ++i) {
            unicode_data <<= 6;
            unicode_data |= (bytes[i] & 0b111111);
        }
    }

    inline bool MixedChar::set(char __chr) {
        is_unicode = false;
        if (present_size < 1) {
            bytes.push_back(__chr);
        } else {
            bytes[0] = __chr;
            present_size = 1;
        }
        return true;
    }

    bool MixedChar::set(const char* __multi_chr) {
        int index = 0;
        while (true) {
            if (index > 4) return false;
            if (__multi_chr[index] == '\000') break;
            if (index >= bytes.size()) {
                bytes.push_back(__multi_chr[index]);
            } else {
                bytes[index] = __multi_chr[index];
            }
            index += 1;
        }
        if (index <= 1) is_unicode = false;
        else is_unicode = true;
        present_size = is_unicode;
        return true;
    }

} // namespace DynamicString
namespace dstring = DynamicString;

#endif /* _DYNAMIC_STRING_HPP_ */