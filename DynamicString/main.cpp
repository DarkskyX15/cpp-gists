/*
 * @Date: 2024-03-18 14:25:41
 * @Author: DarkskyX15
 * @LastEditTime: 2024-03-28 16:51:40
 */
// # include "tui_include.h"
# include "DynamicString.hpp"

int main(){
    std::string inp; std::cin >> inp; 
    dstring::MixedString ms(inp);
    for (int i = 0; i < ms.len(); ++i) {
        std::cout << ms[i] << '\n';
    }
    return 0;
}