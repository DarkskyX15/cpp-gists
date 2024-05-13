/*
 * @Date: 2024-03-11 13:37:40
 * @Author: DarkskyX15
 * @LastEditTime: 2024-05-13 13:52:32
 */
#include <string>
#include <sstream>
#include <iostream>
#include <unordered_map>
using namespace std;

unsigned short translateColorHex(char __a, char __b, const unordered_map<char, int>& __map) {
    return (__map.at(__a) << 4) | __map.at(__b);
}

int main() {
    const unordered_map<char, int> hex_map{{'0', 0}, {'1', 1}, {'2', 2}, {'3', 3}, {'4', 4}, {'5', 5},
                                     {'6', 6}, {'7', 7}, {'8', 8}, {'9', 9}, {'0', 0}, {'a', 10},
                                     {'b', 11}, {'c', 12}, {'d', 13}, {'e', 14}, {'f', 15}};
    unsigned short r, g, b;
    string buffer;
    while (true) {
        cout << ">>> ";
        getline(cin, buffer);
        
        if (buffer[0] == '#') {
            for (int i = 1; i < buffer.size(); ++i) {
                if (buffer[i] >= 'A' && buffer[i] <= 'Z') {
                    buffer[i] += 'a' - 'A';
                }
            }
            r = translateColorHex(buffer[1], buffer[2], hex_map);
            g = translateColorHex(buffer[3], buffer[4], hex_map);
            b = translateColorHex(buffer[5], buffer[6], hex_map);
        } else {
            stringstream reader(buffer);
            reader >> r >> g >> b;
        }

        int raw = 0;
        raw |= r;
        raw <<= 8;
        raw |= g;
        raw <<= 8;
        raw |= b;
        
        cout << raw << '\n';
    }
    return 0;
}