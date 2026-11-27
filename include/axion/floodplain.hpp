#pragma once
#include "linkage.hpp"
#include "braid.hpp"
#include "practice.hpp"
#include "wicks.hpp"
#include "effort.hpp"
#include "life.hpp"
#include "recollection.hpp"
#include "pods.hpp"
#include "cortex.hpp"
#include "nerve_pages.hpp"
#include "drift_marker.hpp"
#include "worksite.hpp"
#include "solvent.hpp"
#include "press.hpp"
#include "hollow.hpp"

namespace axion::life::capillary {struct Mind;struct History;}
namespace axion::life::flood {
using Byte = unsigned char;
using Half = unsigned short;
constexpr Word body_sites = 25, center = 12, call_types = 8;
struct NeuralModule {
    Word routing = 0, gate = 0;
};
struct Germ {
    Genome brain{};
    Word buds[4]{}, growth = 0, grip = 0, stomach = 0, voice = 0;
    Word pigments[4]{};
    NeuralModule module[4]{};
    Word allocation = 0, module_count = 1;
    RecollectionGenes hearing{};
};
struct Settler {
    Germ body{};
    pods::Inheritance pod{};
    GrowthGenes growth{};
    material::Genes hands{};
    brine::Genes saline{};
    siphon::Genes phonation{};
    Word inquiry_receptors = 0, inquiry_impulses = 0, has_inquiry = 0;
    clay::Law ceramic{};
};
struct Anatomy {
    Word shape = 0, limbs = 0, pores = 0, eyes = 0;
};
struct Silt {
    Half water = 0, grains = 0, east = 0, south = 0, leaf = 0, fruit = 0, litter = 0;
    Byte spore = 0, enzyme = 0;
    Half fibre_east = 0, fibre_south = 0;
    Word root = 0;
};
struct Colony {
    Anatomy body{};
    Word founder = 0, born = 0, generation = 0;
    Half energy = 0, load = 0;
    Byte heading = 0, action = 0, signal = 0, heard = 0;
    Word witnesses = 0, motions = 0, worked = 0, meals = 0, laid = 0, offspring = 0, wet_time = 0, diet[3]{};
    Half fibre = 0, cohort = 0;
    Count body_origin = 0, brain_origin = 0;
};
struct Ear {
    Word low[call_types]{}, high[call_types]{}, visits[call_types]{};
};
struct Resident {
    Germ germ{};
    Word tick = 0;
    Colony animal{};
    Ear ear{};
    Word slot = 0, extent = 0;
};
static_assert(__builtin_offsetof(Resident, extent) + 4 == sizeof(Resident));
struct Configuration {
    Word side = 64, tiles = 4, seed = 8123, density = 22, season = 128;
    Word terrain = 1, morph = 1, architecture = 1, wire = 5, predation = 1, plastic = 1, frontier = 0, weaving = 1,
         flora = 2;
};
struct Counters {
    Count light = 0, heat = 0, births = 0, deaths = 0, moves = 0, blocked = 0, harvest = 0, digested = 0, shared = 0;
    Count lifted = 0, laid = 0, displaced = 0, erosion = 0, flow = 0, transported = 0, converted = 0, seeded = 0;
    Count calls = 0, heard = 0, edits = 0, tape_edits = 0, body_edits = 0, remodelled = 0, brain_changes = 0,
          module_edits = 0;
    Count fibre_lifted = 0, fibre_laid = 0, joined = 0, supported = 0, frayed = 0;
    Count recruits = 0, plant_edits = 0, plant_investment = 0;
};
struct HearingEffect {
    Word experienced = 0, inputs = 0, spikes = 0, motor = 0, contacts = 0;
};
struct LegacyDifference {
    Word water = 0, food = 0, wetter = 0, vegetation = 0;
};
struct Balance {
    Count water = 0, clay = 0, energy = 0;
};
struct World {
    Silt *land;
    Word *slots;
    Germ *dna;
    Colony *animals;
    Tissue *nerve;
    Ear *ears;
    Byte *motor;
    Counters *account;
    pods::Garden pods{};
    Neurogenesis cortex{};
    material::Field matter{};
    material::TidalField tide{};
    material::Worksites worksites{};
    material::Sediment seams{};
    brine::Water brine{};
    siphon::Estuary siphons{};
    clay::Field clay{};
    hollow::Field hollows{};
    effort::Field outlay{};
    linkage::Field linkage{};
    practice::Field practice{};wicks::Field wicks{};braid::Field braids{};NervePages neural_pages{};drift::Surfaces drift_marks{};
    drift::BottomAccess *bottom{};
    Word shoals = 0;
    capillary::Mind *capillaries=nullptr;
    capillary::History *material_history=nullptr;
    unsigned char *chemical_contacts=nullptr;
    Word *grip_contacts=nullptr;
};
#if !defined(__CUDA__)
int command(int argc, char **argv);
#endif
static_assert(sizeof(Silt) == 24 && sizeof(Colony) == 96 && sizeof(Anatomy) == 16);
} // namespace axion::life::flood
namespace axion::life::flood {
struct BasinCensus {
    Count water = 0, clay = 0, energy = 0, alive = 0, banked = 0, walls = 0, wet = 0, leaf = 0, fruit = 0, litter = 0,
          carried = 0, area = 0, neurons = 0, heard = 0, energy_in_animals = 0, generations = 0, fibre = 0,
          mixed_faces = 0, fibre_cargo = 0, digesting = 0, conducting = 0, grains = 0, carried_grains = 0,
          excitation = 0, terminals[3]{}, macromolecules = 0;
};
} // namespace axion::life::flood
