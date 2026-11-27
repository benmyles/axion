#pragma once
#include "grounding.hpp"

namespace axion::life::effort {
struct Genes {
    Word appetite = 0, muscle = 0, projection = 0, episode = 0;
};
struct Mind {
    Genes genes{};
    grounding::Synapse food{}, work{}, outcomes{}, changes{};
    Word experience = 0, trace = 0, account = 0;
};
struct Prospect {
    Word fruit = 0, surface = 0, steps = 0;
};
struct Estimate {
    int charge = 0;
    Word food = 0, work = 0, habit = 0;
};
struct Returns {
    Count n = 0, lost = 0, gained = 0, depleted = 0;
};
struct Ledger {
    Count meals = 0, contacts = 0, walks = 0, price = 0, pressed = 0, washed = 0, saved = 0, fresh_price = 0,
          offers = 0, trips = 0, stays = 0, settled = 0, inherited = 0, deaths = 0, credited = 0;
    Returns experiment[4]{};
};
enum Mode : Word {
    learning = 1,
    deciding = 2,
    trails = 4,
    reversing = 8,
    blinded_food = 16,
    blinded_work = 32,
    surveying = 64,
    comp = 128,
    reverse_comp = 256,
    same_comp = 512,
    scrambled = 1024,
    tamping = 2048,
    more_places = 4096,
    selective = 8192,
    fixed_survey = 16384,
    appraisal = 1u << 23,
    foreign_surface = 1u << 24,
    all = learning | deciding | trails | more_places | tamping
};
struct State {
    Word mode = all, period = 512;
    Ledger ledger{};
};
struct Field {
    Mind *minds = nullptr;
    unsigned char *beds = nullptr;
    Ledger *book = nullptr;
    Word mode = 0, period = 0;
};
struct Saved {
    Mind mind{};
    Word magic = 0x45464654, bytes = sizeof(Saved);
};
static_assert(sizeof(Mind) == 60 && sizeof(Saved) == 68);
AX_FN Genes founder(Word k) { return {mix(k + 3391), mix(k + 7117), mix(k + 12211), mix(k + 21061)}; }
AX_FN Genes child(Genes g, Word k, Word chance) {
    auto change = [&](Word w, Word salt) {
        Word a = mix(k + salt);
        return (a & 255) < chance ? w ^ (1u << ((a >> 8) & 31)) : w;
    };
    return {change(g.appetite, 3181), change(g.muscle, 4253), change(g.projection, 8669), change(g.episode, 13913)};
}
AX_FN Word remaining(const Mind &m) { return ((m.trace >> 5) & 15) | (((m.experience >> 8) & 7) << 4); }
AX_FN bool viable(const Mind &m) {
    return !(m.experience & 0xff000000u) && (remaining(m) || (!(m.trace | m.account) && !(m.experience & 0xf00u)));
}
struct Family {
    Genes genes{};
    Word eggs = 0;
};
struct Crossing {
    Word legs = 0, right = 0, stay = 0, travel = 0, fruit = 0, work = 0, optimum = 0, goal = 0, regret = 0,
         forced_stay = 0, forced_travel = 0, mixed = 0, ties = 0, changed = 0, stay_right = 0, travel_right = 0,
         stay_better = 0, travel_better = 0, saved = 0, spent = 0, would_switch = 0, switched_right = 0;
};
struct Card {
    Word id = 0, fruit_map = 0, other_map = 0, tariff = 0, new_tariff = 0, home = 0, away = 0, surface = 0, far = 0,
         near = 0, worked = 0, ate = 0, chosen = 0, ideal = 0, initial = 0, units = 0;
    Genes genes{};
    Mind brain{};
    int left = 0, right = 0;
};
enum Intervention : Word {
    lived,
    old_fruit,
    old_drag,
    empty,
    no_food,
    no_work,
    other_food,
    other_work,
    fixed_organs,
    opposite_surface,
    flat_cost,
    both_remote,
    internal_count
};
} // namespace axion::life::effort
