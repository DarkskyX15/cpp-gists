
#include "Timer.hpp"
#include <thread>
using namespace std;

int main() {
    Timer::Timer<Timer::s> tm1;
    tm1.start("Try1");
    tm1.start("Try2");
    std::this_thread::sleep_for(1s);
    tm1.end("Try");
    std::this_thread::sleep_for(1s);
    tm1.end("Try2");
    return 0;
}