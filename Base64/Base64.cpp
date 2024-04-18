
#ifndef SKYLIB_BASE64_H
#define SKYLIB_BASE64_H

#include <iostream>
#include <unordered_map>
#include <vector>
#include <queue>

namespace Base64{
    const bool encode = 0, decode = 1;
    const short std = 1, url_safe = 2;

    class Bytes{
    public:
        friend std::ostream& operator<< (std::ostream& __out, const Bytes& __bytes){
            if (!__bytes.len){ __out << "bytes:[]"; return __out; }
            __out << "bytes:[";
            for (int i = 0; i < __bytes.len - 1; ++i) __out << __bytes.data_field[i] << ',';
            __out << __bytes.data_field[__bytes.len - 1] << ']';
            return __out;
        }

        friend std::string& operator<< (std::string& __str, const Bytes& __bytes){
            for (int i = 0; i < __bytes.len; ++i){ __str.push_back(__bytes.data_field[i]);}
            return __str;
        }
        friend std::string& operator<< (std::string&& __str, const Bytes& __bytes){
            for (int i = 0; i < __bytes.len; ++i){ __str.push_back(__bytes.data_field[i]);}
            return __str;
        }

        Bytes& operator+ (const Bytes&& __bytes){
            this->len += __bytes.len;
            this->data_field.reserve(this->len + __bytes.len);
            for (std::vector<short>::const_iterator it = __bytes.data_field.begin(); it != __bytes.data_field.end(); ++it){
                this->data_field.push_back(*it);
            }
            return *this;
        }
        Bytes& operator+ (const Bytes& __bytes){
            if (&__bytes == this) return *this;
            this->len += __bytes.len;
            this->data_field.reserve(this->len + __bytes.len);
            for (std::vector<short>::const_iterator it = __bytes.data_field.begin(); it != __bytes.data_field.end(); ++it){
                this->data_field.push_back(*it);
            }
            return *this;
        }

        short& operator[](int __index){
            __index = (__index >= 0 ? __index : (this->len + __index));
            if (__index < 0 || __index >= this->len) return this->data_field[0];
            return this->data_field[__index];
        }
        short at(int __index) const {
            __index = (__index >= 0 ? __index : (this->len + __index));
            if (__index < 0 || __index >= this->len) return -1;
            return this->data_field[__index];
        }

        Bytes(): data_field(), len(0){}

        Bytes(int __len, short int byte = 0): 
        data_field(__len, byte) { this->len = __len; }

        template<typename __NumberT>
        Bytes(const std::vector<__NumberT>& __vec): data_field() {
            this->len = __vec.size();
            this->data_field.reserve(this->len);
            for (typename std::vector<__NumberT>::const_iterator it = __vec.begin(); it != __vec.end(); ++it){
                this->data_field.push_back(static_cast<short>(*it));
            }
        }

        Bytes(const std::string& __str): data_field(){
            this->len = __str.size();
            this->data_field.reserve(this->len);
            for (std::string::const_iterator it = __str.begin(); it != __str.end(); ++it){
                this->data_field.push_back(static_cast<short>(*it));
            }
        }

        Bytes(const char* __cstr): data_field() {
            int _len = 0;
            while (__cstr[_len] != '\0'){
                this->data_field.push_back(static_cast<short>(__cstr[_len]));
                _len += 1;
            }
            this->len = _len;
        }

        Bytes(const char* __bytes, int byte_len): data_field(){
            this->data_field.reserve(byte_len);
            for (int i = 0; i < byte_len; ++i){
                this->data_field.push_back(static_cast<short>(__bytes[i]));
            }
            this->len = byte_len;
        }

        template<typename __NumberT>
        inline void push_back(__NumberT& __x){ this->data_field.push_back(__x % 256); ++this->len; }
        template<typename __NumberT>
        inline void push_back(__NumberT&& __x){ this->data_field.push_back(__x % 256); ++this->len; }

        inline void pop_back() {
            if (this->len > 0) {
                this->data_field.pop_back();
                --this->len; 
            }
        }
        inline void clear(){ this->data_field.clear(); this->len = 0; }
        inline int size() const { return this->len; }

    private:
        int len;
        std::vector<short> data_field;
    };

    class Base64Coder{
    public:
        friend std::ostream& operator<< (std::ostream& __out, const Base64Coder& __b64){
            __out << "Base64Coder Data:\n" << "Mapping:";
            for (int i = 0; i < 64; ++i){
                __out << '(' << i << ':' << __b64.mapping[i] << ')';
            }
            __out << "\nPadding: " << __b64.padding << "  Mode: " << (__b64.work_flg ? "Decode" : "Encode");
            return __out;
        }
        friend Base64Coder& operator<< (Bytes& __contain, Base64Coder& __b64){
            __contain.clear();
            __b64.contain = &__contain;
            return __b64;
        }
        void operator<< (const Bytes& __data){
            if (this->work_flg == decode){ this->b64Decode(__data); }
            else if (this->work_flg == encode){ this->cutPadding(); this->b64Encode(__data); }
        }
        void operator<< (const Bytes&& __data){
            if (this->work_flg == decode){ this->b64Decode(__data); }
            else if (this->work_flg == encode){ this->cutPadding(); this->b64Encode(__data); }
        }
        Base64Coder& operator<< (bool __mode){
            this->work_flg = __mode;
            return *this;
        }
        Base64Coder(short int __preset = std): buffer() {
            this->work_flg = encode;
            this->mapping = new char[64]();
            this->padding = '=';
            this->rmapping = NULL;

            int char_ord = 65;
            for (int i = 0; i < 64; ++i, ++char_ord){
                mapping[i] = static_cast<char>(char_ord);
                if (char_ord == 90){ char_ord = 96; }
                if (char_ord == 122){ char_ord = 47; }
            }
            if (__preset == std) { mapping[62] = '+'; mapping[63] = '/'; }
            else if (__preset == url_safe) { mapping[62] = '-'; mapping[63] = '_'; }
        }
        Base64Coder(const std::unordered_map<short, char>& __b64_list, char __padding = '='): buffer() {
            // default
            this->work_flg = encode;
            this->mapping = new char[64]();
            this->padding = __padding;
            this->rmapping = NULL;

            int char_ord = 65;
            for (short i = 0; i < 64; ++i, ++char_ord){
                mapping[i] = static_cast<char>(char_ord);
                if (char_ord == 90){ char_ord = 96; }
                if (char_ord == 122){ char_ord = 47; }
            }
            mapping[62] = '+'; mapping[63] = '/';

            // customize
            for (std::unordered_map<short, char>::const_iterator it = __b64_list.begin(); it != __b64_list.end(); ++it){
                if (it->first >= 0 && it->first < 64) { mapping[it->first] = it->second; }
            }
        }
        Base64Coder(const Base64Coder& __b64): contain(__b64.contain), buffer() {
            this->work_flg = __b64.work_flg;
            this->mapping = new char[64]();
            this->padding = __b64.padding;
            this->rmapping = new short[256]();
            for (int i = 0; i < 64; ++i){ this->mapping[i] = __b64.mapping[i]; }
            for (int i = 0; i < 256; ++i){ this->rmapping[i] = __b64.rmapping[i]; }
        }
        ~Base64Coder(){
            if (this->mapping != NULL){
                delete[] this->mapping;
                this->mapping = NULL;
            }
            if (this->rmapping != NULL){
                delete[] this->rmapping;
                this->rmapping = NULL;
            }
        }
    private:
        char* mapping;
        short* rmapping;
        char padding;
        bool work_flg;
        Bytes* contain;
        std::queue<short> buffer;

        void cutPadding(){
            while (contain->size() > 0){
                if ((*contain)[-1] == static_cast<short>(padding)){
                    contain->pop_back();
                } else break;
            }
        }
        void generateRMap(){
            this->rmapping = new short[256]();
            for (int i = 0; i < 64; ++i){
                this->rmapping[static_cast<short>(mapping[i])] = i;
            }
        }

        void splitBytes(short __byte, short ignore = 0){
            short base = (1 << 7);
            while (base){
                if (ignore > 0){
                    ignore -= 1;
                } else this->buffer.push(__byte / base);
                __byte %= base;
                base /= 2;
            }
        }
        short gatherBytes(short cnt){
            short res = 0;
            short base = (1 << (cnt - 1));
            for (int i = 0; i < cnt; ++i){
                if (this->buffer.size()){
                    res += base * (this->buffer.front());
                    this->buffer.pop();
                }
                base /= 2;
            }
            return res;
        }

        void b64Encode(const Bytes& __b){
            if (!__b.size()) return ;
            int it = 0;
            while (it < __b.size() || this->buffer.size()){
                if (this->buffer.size() < 6 && it < __b.size()){
                    short bit = __b.at(it);
                    if (bit < 0){
                        bit = bit + 129 + 127;
                    }
                    this->splitBytes(bit);
                    ++it;
                }
                contain->push_back(this->mapping[this->gatherBytes(6)]);
            }
            if (contain->size() % 4){
                short spare = (contain->size() / 4 + 1) * 4 - contain->size();
                while(spare--){ contain->push_back(static_cast<short>(padding)); }
            }
        }
        void b64Decode(const Bytes& __b) {
            if (!__b.size()) return ;
            if (this->rmapping == NULL) {
                generateRMap(); 
            }
            int blocks = __b.size() / 4;
            for (int it = 0; it < blocks; ++it){
                for (int j = 0; j < 4; ++j){
                    if (__b.at(it * 4 + j) != padding){
                        splitBytes(rmapping[__b.at(it * 4 + j)], 2); 
                    }
                }
                while (buffer.size() >= 8){
                    short bit = gatherBytes(8);
                    if (bit > 127) {
                        bit = bit - 127 - 129;
                    }
                    contain->push_back(bit);
                }
            }
            short tail = 0;
            if (buffer.size()) tail = gatherBytes(8);
            if (buffer.size() && tail){ contain->push_back(tail); }
        }
    };

} namespace b64 = Base64;

#endif /* SKYLIB_BASE64_H */