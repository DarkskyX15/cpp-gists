
#ifndef DS_CONFIG_HPP_
#define DS_CONFIG_HPP_

#if __cplusplus >= 200704L

#include <string>
#include <unordered_map>
#include <fstream>
#include <vector>
#include <type_traits>
#include <sstream>
#include <iomanip>

namespace cfg {

    /// @brief 分割字符串
    /// @param __str 原字符串
    /// @param __container 存储结果的容器
    /// @param __sep 分割字符
    void splitString(const std::string& __str, 
                    std::vector<std::string>& __container,
                    const char __sep = -1) {
        std::string temp;
        __container.clear();
        if (__sep < 0) {
            std::stringstream s_sep(__str, std::ios::in);
            while(s_sep >> temp) {
                __container.push_back(temp);
            }
        } else {
            long long left = 0, right = 0;
            long long send = __str.size();
            while (left < send && right < send) {
                if (__str.at(right) == __sep) {
                    temp = __str.substr(left, right - left);
                    if (temp.size()) __container.push_back(temp);
                    left = right + 1;
                }
                right += 1;
            }
            temp = __str.substr(left, right - left);
            if (temp.size()) __container.push_back(temp);
        }
    }

    class Config {
        public:
        Config(std::string __empty_msg = "None"):
        empty_msg(__empty_msg) {}

        inline std::string get(const std::string& __key) const {
            auto it = config_data.find(__key);
            if (it == config_data.end()) {
                return empty_msg;
            } else {
                return it->second;
            }
        }
        
        template<class __VT>
        void get_helper(const std::string& __key, __VT& __container) const {
            std::string result = get(__key);
            if (result == empty_msg) return;
            std::stringstream s_convert(result, std::ios::in);
            s_convert >> __container;
        }

        template<class __VT>
        inline void set(const std::string& __key, const __VT& __value) {
            std::stringstream s_convert;
            if constexpr (std::is_floating_point_v<__VT>) {
                s_convert << std::fixed << std::setprecision(6) << __value;
            } else {
                s_convert << __value;
            }
            auto it = config_data.find(__key);
            if (it == config_data.end()) {
                config_data.insert(std::make_pair(__key, s_convert.str()));
            } else {
                it->second = s_convert.str();
            }
        }

        void readFromFile(const char* __src) {
            if (hold_data) {
                failed = true;
                fail_msg = "Config instance already held data!";
                return ;
            }
            std::ifstream file_in(__src, std::ios::in);
            if (!file_in) {
                failed = true;
                fail_msg = "Cannot open file: " + std::string(__src);
            }

            std::string buffer;
            std::vector<std::string> split;
            while (std::getline(file_in, buffer)) {
                if (buffer.size() <= 0) continue;
                if (buffer[0] == '#') continue;
                split.clear();
                cfg::splitString(buffer, split, '=');
                if (split.size() <= 0) {
                    failed = true;
                    fail_msg = "Cannot interpret line: " + buffer;
                }
                if (split.size() == 1) {
                    split.push_back(empty_msg);
                }
                config_data.insert(std::make_pair(split[0], split[1]));
            }
            file_in.close();
            hold_data = true;
        }

        void saveToFile(const char* __dest) {
            if (!hold_data) {
                failed = true;
                fail_msg = "Config instance holds no data!";
            }
            std::ofstream file_out(__dest, std::ios::out);
            if (!file_out) {
                failed = true;
                fail_msg = "Cannot save file at: " + std::string(__dest);
                return ;
            }
            for (const auto& ele_pair : config_data) {
                file_out << ele_pair.first << '='
                        << ele_pair.second << '\n';
            }
            file_out.flush();
            file_out.close();
        }

        inline bool if_failed() const { return failed; }
        inline std::string get_fail_msg() const { return fail_msg; }
        inline void clean_failed() { failed = failed ? true : false; }
        operator bool() { return !failed; }

        private:
        std::unordered_map<std::string, std::string> config_data;
        const std::string empty_msg;
        std::string fail_msg;
        bool failed = false, hold_data = false;
    };

} // namespace cfg

#endif

#endif /* DS_CONFIG_HPP_ */