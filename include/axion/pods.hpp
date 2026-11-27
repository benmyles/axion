#pragma once
#include "life.hpp"

namespace axion::life::pods {
using Byte = unsigned char;
using Half = unsigned short;

// Each bouton stores eight competing two-bit synapses, one per temporal lag.
struct Genes {
    Word plastic = 0, timing = 0, readout = 0, sense = 0;
};
struct Circuit {
    Half contact[32]{};
    signed char support[2][8]{};
    Count history = 0;
};
struct Gut {
    Byte food[3][8]{}, chaff[8]{};
};
struct FrameGenes {
    Word routes = 0, learning = 0, consolidation = 0;
};
struct Frame {
    Word anchors = 0, certainty = 0, signatures = 0, switches = 0;
    signed char evidence[4]{};
    Byte reliable = 0, active = 0, known = 0, unexplained = 0;
};
struct Inheritance {
    Genes law{};
    FrameGenes frame{};
};
struct Animal {
    Genes genes{};
    Circuit brain{};
    Gut gut{};
    FrameGenes frame_genes{};
    Frame frame{};
    Word emission = 0;
};
struct Physics {
    Word lag = 6, period = 96, variable = 1, credit = 1, background = 30, cut_cohort = 0;
};
struct Census {
    Count bites = 0, opened = 0, swallowed = 0, returned = 0, waste = 0, overlaps = 0, mature = 0, edges = 0,
          withheld = 0;
};
struct MotorImage {
    Word ties[4]{}, senses = 0, control = 0;
};
static_assert(sizeof(MotorImage) == 24);
struct Surface {
    Byte winding = 0;
};
struct Audit {
    Word before = 0, surprise = 0, live[2]{}, yoked[2]{}, cleared[2]{}, displaced[2]{}, grasps = 0, opened = 0,
         coincident = 0, delivered = 0, held = 0, alone = 0, shadow[2]{};
};
struct Transfer {
    Word unseen = 0, initial = 0, before = 0, live = 0, yoked = 0, silent = 0, clamped = 0, transplant = 0, foil = 0,
         other = 0, distant = 0, avail = 0, changed = 0, anchored = 0;
};
struct LocalAudit {
    Word alive = 0, arrived = 0, live = 0, cold = 0, changed = 0, gained = 0, lost = 0;
};
struct Embryo {
    Genes law{};
    FrameGenes frame{};
    Word lineage = 0, living = 0;
};
struct RaftBudget {
    Count initial = 0, light = 0, stock = 0, waste = 0, heat = 0, reserve = 0, offspring = 0, provision = 0;
    Count bites = 0, opened = 0, held = 0, isolated = 0, edits = 0, adults = 0;
};
struct Garden {
    Animal *animal = nullptr;
    Surface *skin = nullptr;
    Census *ledger = nullptr;
    Physics physics{};
    Word observing = 0;
};
static_assert(sizeof(Genes) == 16 && sizeof(Circuit) == 88 && sizeof(Gut) == 32 &&
              __builtin_offsetof(Animal, frame_genes) == 136 && sizeof(Animal) == 176);
} // namespace axion::life::pods
