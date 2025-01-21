#ifndef INC_TIMER_HPP
#define INC_TIMER_HPP
#include <chrono>
#include <functional>
#include <iostream>
#include <string>

#include <ostream>

using tdurr_t = long double;

static inline auto now() {
    return std::chrono::high_resolution_clock::now();
}

class ITimer
{
public:
    ITimer(const std::string& name) : _name(name) {}
    ~ITimer() {}

    tdurr_t elapsed() const {
        return std::chrono::duration<tdurr_t>(_end - _start).count();
    }


    std::string _name;
    decltype(now()) _start, _end;
};
static std::ostream& operator<<(std::ostream& stream, const ITimer& timer) {
    return stream << "Timer \"" << timer._name << "\" : " << timer.elapsed() << "s" << std::endl;
}

// counts time from creating to destroying and saves to _dist
class ScopeTimer : public ITimer
{
    ITimer* _dist;
public:
    ScopeTimer(const std::string& name, ITimer& _dist) : ITimer(name), _dist(&_dist) {
        this->_start = now();
    }
    ScopeTimer(const std::string& name) : ITimer(name), _dist(nullptr) {
        this->_start = now();
    }
    ~ScopeTimer() {
        this->_end = now();

        if (_dist) {
            _dist->_start = this->_start;
            _dist->_end = this->_end;
            _dist->_name = this->_name;
        }
        else {
            std::cout << (ITimer&)*this;
        }
    }
};

// counts time elapsed function to execute
template<class Ret, class... Args>
class FunctionTimer : public ITimer
{
public:
    using func_t = std::function<Ret(Args...)>;
private:
    func_t _f;
public:
    FunctionTimer(const std::string& name, func_t f) : ITimer(name) {
        _f = f;
    }

    Ret Invoke(Args... arg) {
        {
            ScopeTimer sct(this->_name, *this);

            return _f(arg...);
        }
    }
};
#endif