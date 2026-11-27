#pragma once
#include "engine.hpp"
#include <iosfwd>

namespace axion {

void tutor(Engine &engine, Model &model, std::istream &input, std::ostream &output, bool interactive = false);
}
