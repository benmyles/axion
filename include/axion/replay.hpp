#pragma once
#include "engine.hpp"
#include <iosfwd>

namespace axion {
void replay(std::ostream &out, Course probe, Word animal, Word trial, const Totals &observed);
}
