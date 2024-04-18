/*
 * @Date: 2024-03-18 14:25:41
 * @Author: DarkskyX15
 * @LastEditTime: 2024-04-18 21:52:00
 */
# include "DynamicString.hpp"

int main(){
    std::string inp;
    std::cin >> inp;
    dstring::MixedString m_string(inp);
    for (auto& mc : m_string) {
        std::cout << mc << '\n';
    }
    m_string += m_string;
    m_string += m_string;
    std::cout << m_string.c_str() << '\n';
    std::cout << m_string << '\n';
    return 0;
}