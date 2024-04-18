# include "Base64.cpp"
# include <iostream>
using namespace std;

int main(){
    string input; cin >> input;

    b64::Base64Coder b64coder(b64::url_safe);
    b64::Bytes raw(input), coded, decoded;

    // 编码
    coded << b64coder << b64::encode << b64::Bytes(input);
    // 字节数组转字符串
    cout << (string() << coded) << '\n';

    // 解码
    decoded << b64coder << b64::decode << coded;
    // 字节数组转字符串
    cout << (string() << decoded) << '\n';
    
    return 0;
}