#pragma once
#include "axion/composition.hpp"
#include "life_runtime.hpp"
#include <string>

namespace axion::life::composition {
class CircuitPort {
    runtime::Session &s;
    runtime::Temporary body,port;
    runtime::DevicePtr address;Mouth emission{};
    Exchange previous{};Mouth last_question{};bool awaiting=false;
    bool lessons_changed=false;
public:
    CircuitPort(runtime::Session &session,const Mind &mind,bool present,runtime::DevicePtr shared=0);
    bool hear(const std::string &line,std::string &answer);
    void save(Mind &m);
    void cancel_question();
    const Mouth &output() const {return emission;}
};
}
