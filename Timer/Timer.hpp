/*
 * @Date: 2024-04-14 01:26:19
 * @Author: DarkskyX15
 * @LastEditTime: 2024-04-14 02:04:48
 */

#ifndef _DS_TIMER_HPP_
#define _DS_TIMER_HPP_

#if __cplusplus >= 201100L

#include <chrono>
#include <string>
#include <iostream>
#include <type_traits>
#include <unordered_map>

namespace Timer {

    typedef std::chrono::microseconds us;
    typedef std::chrono::milliseconds ms;
    typedef std::chrono::nanoseconds ns;
    typedef std::chrono::seconds s;
    typedef std::chrono::time_point<std::chrono::steady_clock> timetick;

    template <class TimeInterval>
    class Timer {
        public:
        Timer() {
            if constexpr (std::is_same_v<TimeInterval, us>) {
                suffix = "us.";
            } else if constexpr (std::is_same_v<TimeInterval, ms>) {
                suffix = "ms.";
            } else if constexpr (std::is_same_v<TimeInterval, ns>) {
                suffix = "ns.";
            } else if constexpr (std::is_same_v<TimeInterval, s>) {
                suffix = "s.";
            }
        }

        /// @brief 获取开始时刻
        inline void start(const std::string& start_action) {
            t_start = std::chrono::steady_clock::now();
            start_times.insert(std::make_pair(start_action, t_start));
        }
        /// @brief 获取结束时刻并输出，单位us
        inline void end(const std::string& end_action) {
            t_end = std::chrono::steady_clock::now();
            auto iter = start_times.find(end_action);
            if (iter == start_times.end()) return ;
            interval = std::chrono::duration_cast<TimeInterval>(t_end - iter->second);
            std::cout << end_action << " took " << interval.count() << suffix << '\n';
            start_times.erase(iter);
        }

        private:
        timetick t_start, t_end;
        TimeInterval interval;
        std::unordered_map<std::string, timetick> start_times;
        const char* suffix;
    };

} /* namespace Timer */

#endif

#endif /* _DS_TIMER_HPP */