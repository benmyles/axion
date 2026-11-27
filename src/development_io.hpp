#pragma once
#include "individual_io.hpp"
#include "composition_host.hpp"
#include "procedure_host.hpp"
#include <vector>

namespace axion::life {
class DevelopmentalSpeech {
    runtime::Session &lab;
    storage::AgentImage &animal;
    runtime::DevicePtr mind;
    runtime::Temporary port;
    composition::CircuitPort compositional;
    procedure::MotorSpeech procedures;
    Count turns=0,answers=0;
    bool new_home=false;
    void transact(std::vector<PulsePort> &events);
public:
    DevelopmentalSpeech(runtime::Session &s,storage::AgentImage &a,runtime::DevicePtr sequence,runtime::DevicePtr composition,runtime::DevicePtr procedures=0);
    bool hear(const std::string &line,bool fallthrough=false,std::vector<Count> *emitted=nullptr,bool inner=false);
    procedure::MotorSpeech &motor_port() {return procedures;}
    void report();
};
int developmental_interview(storage::AgentImage &animal,const std::string &source,const std::string &save);
}
