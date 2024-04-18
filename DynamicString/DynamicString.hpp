/*
 * @Date: 2024-03-28 16:37:08
 * @Author: DarkskyX15
 * @LastEditTime: 2024-03-28 16:46:25
 */

#ifndef _DSTRING_HPP_
#define _DSTRING_HPP_

#include <string>
#include <vector>
#include <iostream>

namespace DynamicString
{
    
    class MixedChar{
    public:
        MixedChar() = default;
        MixedChar(char __chr);
        MixedChar(std::string::const_iterator __begin, int __len);
        MixedChar(const char* __begin, int __len);
        friend std::ostream& operator<< (std::ostream& __out, const MixedChar& __mc) {
            for (auto it = __mc.bytes.begin(); it != __mc.bytes.end(); ++it) {
                __out << *it;
            }
            return __out;
        }

        inline bool isUnicode() const { return this->is_unicode; }
        inline int getUnicodeData() const { return this->unicode_data; }
        inline int getSize() const { return this->bytes.size(); }
        inline bool set(char __chr);
        inline bool set(const char* __multi_chr);
        inline char& operator[](int __index) { return bytes[__index]; };
        void operator= (char __chr) { set(__chr); }
        void operator= (const char* __multi_chr) { set(__multi_chr); }

    private:
        bool is_unicode;
        int unicode_data;
        std::vector<char> bytes;

        void generateUnicode();
    };

    class MixedString{
    public:
        MixedString(const char* __cstr);
        MixedString(const std::string& __str);
        MixedString(const MixedString& __ms);
        ~MixedString();

        friend std::ostream& operator<< (std::ostream& __out, const MixedString& __ms) {
            for (auto it = __ms.str_data.begin(); it != __ms.str_data.end(); ++it) {
                __out << *it;
            }
            return __out;
        }

        MixedString& operator+ (const MixedString& __mstr);
        inline MixedChar& operator[] (int __index);
        inline MixedChar& chrAt (int __index);
        inline int wid() { return width; }
        inline int len() { return length; }
        inline int rawSize() { return size; }
        const char* c_str();

    private:
        int width, length, size;
        char* cstr_ptr;
        std::vector<int> width_map; 
        std::vector<MixedChar> str_data;

        inline bool onBit(int n, short __pos);
    };

    /* MixedString */

    // Constructors
    MixedString::MixedString(const char* __cstr) : 
    width_map(), str_data() {
        width = 0; cstr_ptr = nullptr;
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
                width_map.push_back(str_data.size());
                width_map.push_back(str_data.size());
                str_data.push_back(MixedChar((__cstr + index), utf_size));
                index += utf_size;
                width += 2;
            } else {
                str_data.push_back(MixedChar(__cstr[index]));
                width_map.push_back(str_data.size() - 1);
                ++ index; ++ width;
            }
        }
        size = index;
        length = str_data.size();
    }

    MixedString::MixedString(const std::string& __str) : 
    width_map(), str_data() {
        width = 0; cstr_ptr = nullptr;
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
                width_map.push_back(str_data.size());
                width_map.push_back(str_data.size());
                str_data.push_back(MixedChar(it, utf_size));
                it += utf_size;
                width += 2;
            } else {
                str_data.push_back(MixedChar(*it));
                width_map.push_back(str_data.size() - 1);
                ++ it; ++ width;
            }
        }
        size = __str.size();
        length = str_data.size();
    }

    MixedString::MixedString(const MixedString& __ms) : 
    width_map(__ms.width_map), str_data(__ms.str_data) {
        width = __ms.width;
        length = __ms.length;
        size = __ms.size;
        if (__ms.cstr_ptr != nullptr) {
            cstr_ptr = new char[__ms.size + 1]();
            for (int i = 0; i < __ms.size + 1; ++i) {
                cstr_ptr[i] = __ms.cstr_ptr[i];
            }
        } else cstr_ptr = nullptr;
    }

    MixedString::~MixedString() {
        if (cstr_ptr != nullptr) {
            delete[] cstr_ptr;
            cstr_ptr = nullptr;
        }
    }

    // Public Functions

    inline MixedChar& MixedString::chrAt (int __index) {
        return str_data[width_map[__index]];
    }

    inline MixedChar& MixedString::operator[] (int __index) {
        return str_data[__index];
    }

    const char* MixedString::c_str() {
        if (cstr_ptr != nullptr) delete[] cstr_ptr;
        cstr_ptr = new char[size + 1]();
        for (int i = 0, j = 0; i < length; ++i) {
            for (int z = 0; z < str_data[i].getSize(); ++z, ++j) {
                cstr_ptr[j] = str_data[i][z];
            }
        }
        cstr_ptr[size] = '\000';
        return cstr_ptr;
    }

    MixedString& MixedString::operator+ (const MixedString& __mstr) {
        width_map.reserve(__mstr.width);
        for (int i = 0; i < __mstr.width; ++i) {
            width_map.push_back(__mstr.width_map[i] + length);
        }
        size += __mstr.size;
        width += __mstr.width;
        length += __mstr.length;
        str_data.reserve(__mstr.length);
        for (int i = 0; i < __mstr.length; ++i) {
            str_data.push_back(__mstr.str_data[i]);
        }
        return *this;
    }

    // Private Functions
    inline bool MixedString::onBit(int n, short __pos) {
        return (n & (1 << __pos)) ? true : false;
    }

    /* MixedChar */
    MixedChar::MixedChar(char __chr) {
        is_unicode = false;
        unicode_data = -1;
        bytes.push_back(__chr);
    }
    MixedChar::MixedChar(std::string::const_iterator __begin, int __len) {
        is_unicode = true;
        bytes.reserve(__len);
        for (int i = 0; i < __len; ++i, ++__begin) bytes.push_back(*__begin);
        generateUnicode();
    }
    MixedChar::MixedChar(const char* __begin, int __len) {
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
        if (is_unicode) return false;
        else bytes[0] = __chr;
        return true;
    }

    inline bool MixedChar::set(const char* __multi_chr) {
        if (!is_unicode) return true;
        else {
            int index = 0;
            while (true) {
                if (index >= 4) return false;
                if (__multi_chr[index] == '\000') break;
                bytes[index] = __multi_chr[index];
                index += 1;
            }
            bytes.resize(index);
        }
        return true;
    }

} // namespace DynamicString
namespace dstring = DynamicString;

#endif /* _DSTRING_HPP_ */