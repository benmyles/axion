#pragma once
#include "life.hpp"

namespace axion::life::material {
using Byte = unsigned char;
using Half = unsigned short;
struct Grain {
    Byte faces = 0, substance = 0, charge = 0, marks = 0;
};
struct Genes {
    Word muscle = 0, innate = 0, receptors = 0, practice = 0, observation = 0, release = 0;
};
struct Worker {
    Genes genes{};
    Half predicts[8]{}, seeks[8]{};
    Grain hand{};
    Word recent = 0, edits = 0, sightings = 0;
};
struct Ledger {
    Count light = 0, transfers = 0, reactions = 0, folds = 0, lifts = 0, placements = 0, turns = 0, translations = 0,
          observed = 0, edits = 0, heat = 0, exchanges = 0, thermal = 0;
};
struct Conditions {
    Word density = 48, bonded = 3, agency = 1, vision = 1;
};
struct Field {
    Grain *bed = nullptr;
    Worker *residents = nullptr;
    Ledger *ledger = nullptr;
    Byte *attachment = nullptr;
    Conditions conditions{};
    Word observing = 0;
};
struct HandImage {
    Word best[2]{}, forced[2]{}, tied[2]{}, control = 0;
};
static_assert(sizeof(HandImage) == 28);
struct Rates {
    Word light = 0, flow = 0, reaction = 0, folds = 0;
};

AX_FN Word capacity(Grain p) { return 2 + (p.substance & 15); }
AX_FN Word rotate_faces(Word ports, Word n) {
    Word shift = (n & 3) * 2;
    return ((ports | (ports << 8)) >> ((8 - shift) & 7)) & 255;
}
AX_FN Word face(Grain p, Word dir) { return (p.faces >> (2 * ((dir - Word(p.marks & 1)) & 3))) & 3; }
AX_FN bool conducts(Conditions p) { return p.bonded & 1; }
AX_FN bool hydraulic(Conditions p) { return p.bonded & 2; }
AX_FN Word obstruction(Grain a, Grain b, Word dir) {
    // A closed face presents the full cross section. Perforated ionic faces
    // pass different fractions of the current. Folding rotates these faces.
    auto face_drag = [](Grain p, Word d) {
        Word port = face(p, d), width = 1 + ((p.substance >> 4) & 3);
        return !p.faces ? 0u : port == 3 ? 0u : port == 0 ? 3 * width : port == 1 ? width : 2 * width;
    };
    return face_drag(a, dir) + face_drag(b, dir ^ 2);
}
AX_FN Word appearance(Grain grain) {
    // Surface relief is visible separately from the body's absorption band.
    Word exposed = rotate_faces(grain.faces, grain.marks & 1);
    return (grain.substance & 248u) | (Word(__builtin_popcount(exposed)) & 7u);
}
AX_FN bool bonded(Grain a, Grain b, Word dir) {
    Word x = face(a, dir), y = face(b, dir ^ 2);
    return x && y && (x == 3 || y == 3 || x != y);
}
AX_FN Word weight(Grain p) { return 1 + ((p.substance >> 4) & 3); }
AX_FN Word light_band(Grain p) { return (p.substance >> 4) & 3; }
AX_FN Word catalytic_band(Grain p) { return (p.substance >> 2) & 3; }
AX_FN Word fold(Grain &p) {
    if (!(p.substance & 128) || !p.faces)
        return 0;
    Word was = p.marks & 1, level = p.charge;
    Word bent = level + 1 >= capacity(p) ? 1 : level <= capacity(p) / 3 ? 0 : was;
    p.marks = static_cast<Byte>((p.marks & ~1u) | bent);
    return was != bent;
}
// The endpoints share a bond for one alternating face update. The only
// conserved quantity exchanged here is excitation carried by the grains.
AX_FN Word conduct(Grain &a, Grain &b, Word dir, bool open) {
    if (!open || !bonded(a, b, dir))
        return 0;
    int left = int(4 * a.charge) + int((a.substance >> 4) & 3), right = int(4 * b.charge) + int((b.substance >> 4) & 3);
    Grain &from = left >= right ? a : b, &into = left >= right ? b : a;
    if (__builtin_abs(left - right) < 4 || !from.charge || into.charge == capacity(into))
        return 0;
    --from.charge;
    ++into.charge;
    return 1;
}
AX_FN Rates excite(Grain &p, Word incident, Word substrate, Half &reagent, Half &product) {
    Rates r;
    if (!p.faces)
        return r;
    if (incident == light_band(p) && p.charge < capacity(p)) {
        ++p.charge;
        r.light = 1;
    }
    if (substrate == catalytic_band(p) && reagent && p.charge >= 1 + ((p.substance >> 5) & 3) && product <= 65533) {
        --p.charge;
        --reagent;
        product += 2;
        r.reaction = 1;
    }
    r.folds = fold(p);
    return r;
}
AX_FN bool valid(Grain p) {
    return p.faces ? p.charge <= capacity(p) && p.marks < 16 && (!(p.marks & 1) || (p.substance & 128))
                   : !p.substance && !p.charge && !p.marks;
}
AX_FN Word quantity(Grain p) { return p.faces != 0; }
struct Temper {
    Word cue = 0, change = 0, spent = 0, performed = 0;
};
AX_FN Word sense(Grain home, Grain away, Grain hand, Word dir, Word sight) {
    return quantity(home) | (quantity(away) << 1) | (quantity(hand) << 2) | (Word(bonded(home, away, dir)) << 3) |
           ((home.marks & 12) << 2) | ((sight & 3) << 6);
}
AX_FN Word inventory(Grain p, Word group) {
    Word mismatch = p.faces ^ (group * 0x55u);
    return __builtin_popcount(~(mismatch | (mismatch >> 1)) & 0x55u);
}
AX_FN bool exchange(Grain &a, Grain &b, Word dir) {
    if (!a.faces || !b.faces)
        return false;
    Word at = 2 * ((dir + 1 - (a.marks & 1)) & 3), bt = 2 * ((dir + 2 - (b.marks & 1)) & 3);
    Word change = ((a.faces >> at) ^ (b.faces >> bt)) & 3;
    Word left = a.faces ^ (change << at), right = b.faces ^ (change << bt);
    if (!change || !left || !right)
        return false;
    a.faces = static_cast<Byte>(left);
    b.faces = static_cast<Byte>(right);
    return true;
}
AX_FN Word rotate_grain(Grain &p, Word n) {
    if (!p.faces)
        return 0;
    p.faces = static_cast<Byte>(rotate_faces(p.faces, n));
    p.marks = static_cast<Byte>((p.marks & 1) | 2 | ((((p.marks >> 2) + n) & 3) << 2));
    return 1;
}
AX_FN Word retinal_change(Grain h, Grain a, Grain after, Grain beyond, Word dir) {
    Word home = quantity(h), ahead = quantity(a), next = quantity(after), far = quantity(beyond);
    Word spin = home && next ? (((after.marks >> 2) - (h.marks >> 2)) & 3) : 0;
    bool old = bonded(h, a, dir), now = bonded(after, beyond, dir);
    return (Word(next > home)) | (Word(home > next) << 1) | (Word(far > ahead) << 2) | (Word(ahead > far) << 3) |
           (Word(spin == 1) << 4) | (Word(spin == 3) << 5) | (Word(now && !old) << 6) | (Word(old && !now) << 7);
}
// Motor ports 0..7 rest, rotate right/left, lift, place, push, pull or exchange
// two terminal groups. Directed work changes orientation or occupancy. Attachment
// resistance is assessed from the pre-contact lattice. No assembly is named.
AX_FN Temper manipulate(Grain &home, Grain &away, Grain &hand, Word op, Word dir, Word links, Word far_links,
                        Word reserve, Word sight) {
    Temper t;
    t.cue = sense(home, away, hand, dir, sight);
    Grain initial = home, other = away;
    Word cost = 1;
    if (op == 1 || op == 2 || op == 3 || op == 5 || op == 7)
        cost += links;
    if (op == 3 || op == 5)
        cost += weight(home) - 1;
    if (op == 4)
        cost += weight(hand) - 1;
    if (op == 6)
        cost += weight(away) - 1;
    if (op == 6 || op == 7)
        cost += far_links;
    if (!op || reserve <= cost)
        return t;
    switch (op) {
    case 1:
    case 2:
        t.performed = rotate_grain(home, op == 1 ? 1 : 3);
        break;
    case 3:
        if (home.faces && !hand.faces) {
            hand = home;
            home = {};
            t.performed = 1;
        }
        break;
    case 4:
        if (!home.faces && hand.faces) {
            home = hand;
            hand = {};
            t.performed = 1;
        }
        break;
    case 5:
        if (home.faces && !away.faces) {
            away = home;
            home = {};
            t.performed = 1;
        }
        break;
    case 6:
        if (!home.faces && away.faces) {
            home = away;
            away = {};
            t.performed = 1;
        }
        break;
    case 7:
        t.performed = exchange(home, away, dir);
        break;
    }
    if (t.performed) {
        if (home.faces)
            home.marks |= 2;
        if (op >= 5 && away.faces)
            away.marks |= 2;
        if (hand.faces)
            hand.marks |= 2;
        t.change = retinal_change(initial, other, home, away, dir);
        t.spent = cost;
    }
    return t;
}
static_assert(sizeof(Grain) == 4 && sizeof(Genes) == 24 && sizeof(Worker) == 72);
} // namespace axion::life::material

namespace axion::life::material {
struct Component {
    Word size = 0, edges = 0, branches = 0, handled = 0, charge = 0, changed = 0;
};
struct Flux {
    Word made = 0, passed = 0;
};
} // namespace axion::life::material
