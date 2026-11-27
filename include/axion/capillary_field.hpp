#pragma once
#include "capillary_course.hpp"
#include "material_context.hpp"
#include "material_signal.hpp"

namespace axion::life::capillary {
constexpr Word contextual = 128, regional_synapses = 256, catalytic_surface = 512, foraging_ports = 1024,
               fixed_appetite = 2048;
struct Settings {
    Word mode = 0, shift = 2, mineral = 32, raw_frame = 0;
};
struct Ledger {
    Count steps = 0, springs = 0, carried = 0, attended = 0, experiments = 0, changed = 0, placed = 0, lifted = 0,
          nets = 0, caught = 0, correct = 0, forecast = 0, edits = 0, spikes = 0, contrasts = 0, heat = 0,
          model_actions = 0, model_food = 0, return_nets = 0, return_food = 0, reused = 0, ambiguity_seen = 0,
          tests_due = 0, rechecked = 0, retained_tests = 0, read_context = 0, comparisons = 0, different_hand = 0,
          different_net = 0, food_added = 0, food_forgone = 0, other_food = 0, other_cost = 0, mixed_material = 0,
          pulse_added = 0, pulse_removed = 0, switched_sills = 0, one_shot = 0, active_grips = 0, enabled_bite = 0,
          foregone_bite = 0, writing = 0, reading = 0, pumped = 0, pump_effect = 0, chemical_nets = 0, writer_food = 0,
          reader_food = 0, appetitive_edits = 0, port_compared = 0, changed_preference = 0, changed_pumps = 0,
          changed_nets = 0, port_extra_food = 0, port_missed_food = 0;
};
struct Note {
    Word tick = 0, site = 0, slot = 0, born = 0, rock = 0, previous = 0, sensed = 0, actual = 0, fired = 0, hand = 0,
         forecast = absent, cast = 0, food_before = 0, food = 0, after_food = 0, energy_before = 0, after_energy = 0,
         heat = 0, cargo_before = 0, cargo_after = 0, edge_before = 0, edge_after = 0, wet = 0, law = 0, dendrites = 0,
         motor = 0;
    Tissue prior{}, learned{};
    Word incoming = 0, pad = 0, outgoing = 0, deposited = 0, threshold = 0, wants_flow = 1, filter = 0, donation = 0;
    History context_before{}, context_after{};
    Count alternatives = 0;
    Word memory_gate = 0, in_context = 0, review = 0, chosen_context = 0, geology = 0, chemistry = 0;
    Word port = 0, light = 0, choice = 0, pump = 0, basal_row = 0, contacts_before = 0, contacts_after = 0,
         food_edits = 0, food_law = 0, prior_choice = 0, shadow_cast = 0, shadow_food = 0, appetite_audit = 0,
         stock_threshold = 0, net_available = 0, mouthful = 0, pump_available = 0;
};
AX_FN Word rock(Settings law, Word seed, Word x, Word y) {
    return law.mineral < 32 ? law.mineral : (mix(seed ^ mix((x >> 4) + 13183u * (y >> 4))) % 32);
}
AX_FN Word centroid(Word x, Word y) { return (x & 2047u) | ((y & 2047u) << 11); }
AX_FN void retain(Mind &m, Word at, Word image, bool fired, Word hand, Word dye, Word &new_pair) {
    Word local = image | (Word(fired) << 6);
    if ((m.before >> 31) && ((m.before >> 8) & 0x3fffffu) == at && ((m.before ^ local) & 64)) {
        m.after = (1u << 31) | (m.before & 127) | (local << 7) | ((hand & 7) << 14) | ((dye & 15) << 17);
        new_pair = 1;
    }
    m.before = (1u << 31) | (at << 8) | local;
}
AX_FN bool valid(Mind m) {
    return (!m.before || ((m.before >> 31) && !(m.before & 0x40000080u))) &&
           (!m.after || ((m.after >> 31) && !(m.after & 0x7fe00000u) && ((m.after ^ (m.after >> 7)) & 64)));
}
struct Mark {
    Word slot = 0, born = 0, signals = 0, resources = 0;
};
struct Closing {
    Word epoch = 0, place = 0;
    Mark first{}, second{};
};
struct Film {
    Mark *marks = nullptr;
    Word side = 0, x = 0, y = 0, frames = 0;
};
struct Donor {
    Word moment = 0, position = 0;
};
struct Echo {
    Word enabled = 0, suppressed = 0, other = 0, prevented = 0, net = 0, horizon = 0, site = 0, slot = 0, born = 0,
         when = 0;
    Word influence[17]{};
};
struct ContextNote {
    Tissue tissue{};
    History before{}, after{};
    Count diagnostic = 0;
    Word tick = 0, site = 0, slot = 0, born = 0, rock = 0, gene = 0, motor = 0, coin = 0, options = 0, rows = 0,
         want = 0, valuable = 0, net_available = 0, eyes = 0, limbs = 0, stock = 0, energy = 0, hand = 0,
         other_hand = 0, cast = 0, other_cast = 0, row = 0, other_row = 0, food = 0, other_food = 0, heat = 0,
         other_heat = 0, permission = 0, review = 0, chosen = 0, signed_contact = 0;
    Word port = 0, choice = 0, pump = 0;
};
struct PumpOrigin {
    Word writer = 0, born = 0, signal = 0;
};
struct Reception {
    Word tick = 0, source = 0, destination = 0, writer = 0, writer_born = 0, reader = 0, reader_born = 0,
         writer_data = 0, line = 0, input = 0, counterfactual = 0, rock = 0, light = 0, other_light = 0, move = 0,
         other_move = 0, bare = 0, other_bare = 0, cast = 0, other_cast = 0, wants = 0, available = 0, sorting = 0,
         stock = 0, mouthful = 0, energy = 0, heat = 0, food = 0, other_food = 0;
};
struct Deliveries {
    Count pairs = 0, sole_source = 0, changed_move = 0, acquired_source = 0, acquired_response = 0, learned_both = 0,
          gained = 0, missed = 0;
};
struct PortGallery {
    PumpOrigin *origins = nullptr;
    Reception *records = nullptr;
    Deliveries *counts = nullptr;
    Word side = 0, x = 0, y = 0, capacity = 0;
};
struct Device {
    Word *waves = nullptr;
    Ledger *ledger = nullptr;
    Note *notes = nullptr;
    Word samples = 0;
    Settings law{};
    Film film{};
    Word *impulses = nullptr;
    ContextNote *context_notes = nullptr;
    Word context_sites = 0;
    PortGallery gallery{};
};
} // namespace axion::life::capillary
