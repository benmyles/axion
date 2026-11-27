#pragma once
#include "recollection.hpp"

namespace axion::life {
struct BindingGenes { Word plastic=0,competition=0,exploration=0; };
struct BindingField { Word contacts[4]{}; };
struct Bindings {
    BindingGenes genes{};
    Word forms[4]{};
    BindingField fields[4]{};
    Word support=0,attempted_form=0,attempt=0;
};
struct RoleProjection {
    Word routes[2]{0x8421u,0x84210000u};
    Word apertures=9,events=0,missing=0;
};
struct JointField { Word contacts[2][4]{}; };
struct Conjunctions {
    RecollectionGenes gates{0,0,0};
    JointField fields[4]{};
};
// Each bit is a possible final scene given one starting scene.
struct SceneWave { unsigned short scenes[16]{}; Word active=0; };
static_assert(sizeof(Conjunctions)==140 && sizeof(SceneWave)==36);
static_assert(sizeof(Bindings)==104 && sizeof(RoleProjection)==20);
#if !defined(__CUDA__)
int binding_command(int argc,char **argv);
int conjunction_command(int argc,char **argv);
#endif
}
