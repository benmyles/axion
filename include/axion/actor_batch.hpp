#pragma once
#include "model.hpp"
namespace axion::life {
// Independent actors can have a different expressed layout in each role.
constexpr Word vocal_margin_report=~0u;
struct ActorBatch { Word copies,body_bytes,path_bytes,organ_bytes,eligibility=0,motor_learning=1; };
}
