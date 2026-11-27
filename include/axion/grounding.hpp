#pragma once
#include "life.hpp"
#include "recollection.hpp"

namespace axion::life::grounding {
using Byte = unsigned char;
using Half = unsigned short;
constexpr Word forms = 48, objects = 8, channels = 8, hypotheses = 576, bundles = hypotheses / 32, relations = 24;
constexpr Word turns = 256, utterance = 8, questions = 16, demonstrations = 64;
constexpr Byte absent = 255;
struct Retina {
    Count sites = 0, band[channels]{};
};
struct View {
    Retina before{}, after{};
};
struct Patch {
    Byte light = 0, prior = absent, site = absent;
};
struct Scene {
    Patch patch[objects]{};
    Byte n = 0, truncated = 0;
};
struct Voice {
    Count waves[utterance]{};
    Word n = 0;
};
struct Phrase {
    Byte id[utterance]{}, n = 0;
};
struct Episode {
    Scene scene{};
    Phrase heard{};
};
struct Example {
    Scene scene{};
    Phrase heard{};
    Byte answer = 0;
    Phrase prior{};
};
struct Synapse {
    Word low = 0, high = 0;
};
struct Shape {
    Byte mask = 0, value = 0;
};
struct Cell {
    Count wave = 0, stability = 0;
    Synapse receptive[bundles]{};
    Word possible[8]{}, certain[8]{};
    Word exposures = 0;
    Half viable = 0;
    Byte order = 0, reserved = 0;
};
struct Edge {
    Synapse synapse{};
    Word possible = 0, observed = 0;
};
struct Construction {
    // Fixed acoustic context. Each nonzero source selects a term from
    // the current question, the earlier proposition, or its answer.
    Phrase motif{};
    Byte available = 0, answer_slot = 0, field[3]{}, evidence = 0, yes = 0, no = 0;
};
struct Mind {
    RecollectionGenes genes{};
    Word count = 0;
    Cell word[forms + 1]{};
    Edge edge[3][forms + 1]{};
    Count fills[3]{};
    Word layout = 0;
    Episode episodes[turns]{};
    Word oldest = 0, kept = 0, experienced = 0;
    Example examples[demonstrations]{};
    Word examples_n = 0, examples_first = 0;
    Construction construction[questions]{};
    Word constructions = 0, crowded_templates = 0;
    Scene here{};
    Phrase discourse{};
    Word sights = 0, consolidated = 0, exchanges = 0;
    alignas(8) Word version = 1;
    Word extent = 0;
};
struct Vocabulary {
    Count noun[8]{}, verb[6]{}, gap[5]{}, particle[4]{};
};
struct GroundingAudit {
    Word cases = 0, names = 0, verbs = 0, full = 0, spoken = 0, expected = 0, wrong = 0, silent = 0;
    Word trials = 0, positions = 0, bindings = 0, ambiguous = 0, abstained = 0;
};
struct InterviewAudit {
    Word exact[5]{}, valid[5]{}, wrong[5]{}, ambiguous = 0, quiet = 0;
};
struct Answer {
    Count wave = 0, sites = 0, names = 0, definite = 0;
    Phrase statement{};
    Word hypotheses = 0, complete = 0, ambiguous = 0, asking = 0;
};
struct Clarification {
    Phrase prompt{}, prior{};
    Count remaining = 0;
    Word rule = 0, current = 0, yes = 0, no = 0;
};
struct RepairAudit {
    Word trials = 0, requests = 0, answered = 0, correct = 0, wrong = 0, misinformed = 0, chance1024 = 0;
};
static_assert(__builtin_offsetof(Mind, extent) + 4 == sizeof(Mind));
#if !defined(__CUDA__)
int command(int argc, char **argv);
#endif
} // namespace axion::life::grounding
