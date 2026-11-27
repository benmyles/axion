#pragma once
#include "axion/procedure.hpp"
#include "life_runtime.hpp"
#include <string>
#include <vector>

namespace axion::life::procedure {
class MotorSpeech {
    runtime::Session &s;
    runtime::Temporary memory,decoder;
    runtime::DevicePtr body;
    bool populated=false;
public:
    MotorSpeech(runtime::Session &session,const Mind &initial,bool present,runtime::DevicePtr shared=0);
    bool hear(const std::string &line,std::string &answer,std::vector<Count> *echo=nullptr);
    bool garden(const std::string &utterance,const std::string &site,bool execute,bool solve,runtime::DevicePtr agency,runtime::DevicePtr concepts,
                runtime::DevicePtr peer,runtime::DevicePtr peer_concepts,runtime::DevicePtr plots,Word eyes,Word allowed,const std::vector<Count> *incoming=nullptr);
    std::vector<Count> pending();
    bool consent(const std::vector<Count> &sounds,runtime::DevicePtr agency,runtime::DevicePtr concepts);
    void save(Mind &image);
};
}
