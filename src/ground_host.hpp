#pragma once
#include "axion/grounding.hpp"
#include "life_runtime.hpp"
namespace axion::life::grounding {
class GroundedSpeech {
    runtime::Session &lab;
    runtime::Temporary private_body, lens, working_turn;
    runtime::DevicePtr body;

  public:
    GroundedSpeech(runtime::Session &s, const Mind &initial, bool present, runtime::DevicePtr shared = 0);
    void present(const View &scene);
    bool hear(const std::string &text, std::string &answer);
    void store(Mind &out);
};
} // namespace axion::life::grounding
