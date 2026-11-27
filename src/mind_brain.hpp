#pragma once
#include "axion/mind.hpp"
#include "life_runtime.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace axion::mind {

using namespace axion::life::runtime;

// ---------------------------------------------------------------------------
// Host-side SDR Operations (bit-exact mirror of Blackwell GPU primitives)
// ---------------------------------------------------------------------------
inline Word sdr_popcount(const SpikeVec &a) {
    Word s = 0;
    for (Word i = 0; i < WORDS; ++i)
        s += Word(__builtin_popcount(a.bits[i]));
    return s;
}

inline Word sdr_overlap(const SpikeVec &a, const SpikeVec &b) {
    Word s = 0;
    for (Word i = 0; i < WORDS; ++i)
        s += Word(__builtin_popcount(a.bits[i] & b.bits[i]));
    return s;
}

inline SpikeVec sdr_or(const SpikeVec &a, const SpikeVec &b) {
    SpikeVec r{};
    for (Word i = 0; i < WORDS; ++i)
        r.bits[i] = a.bits[i] | b.bits[i];
    return r;
}

inline SpikeVec sdr_and_not(const SpikeVec &a, const SpikeVec &b) {
    SpikeVec r{};
    for (Word i = 0; i < WORDS; ++i)
        r.bits[i] = a.bits[i] & ~b.bits[i];
    return r;
}

inline bool sdr_empty(const SpikeVec &a) { return sdr_popcount(a) == 0; }

// Deterministic cochlear subword spike projection (100% language-agnostic!)
inline SpikeVec cochlear_encode(const std::string &raw_word, Word salt, Word target_k = TARGET_K) {
    std::string w;
    w.reserve(raw_word.size() + 2);
    w.push_back('^');
    for (unsigned char c : raw_word) {
        if (std::isalnum(c) || c == '_' || c == '-')
            w.push_back(char(std::tolower(c)));
    }
    w.push_back('$');

    std::array<int, NEURONS> votes{};
    Word whole_h = 2166136261u ^ salt;
    for (unsigned char c : w) {
        whole_h ^= c;
        whole_h *= 16777619u;
    }
    for (Word t = 0; t < 24; ++t) {
        Word idx = mix(whole_h + t * 0x9e3779b9u) & (NEURONS - 1);
        votes[idx] += 5;
    }
    for (size_t i = 0; i + 1 < w.size(); ++i) {
        Word bh = (Word(Byte(w[i])) << 8) | Word(Byte(w[i + 1]));
        for (Word t = 0; t < 4; ++t) {
            Word idx = mix(salt ^ (bh * 2654435761u) ^ (t * 7331u)) & (NEURONS - 1);
            votes[idx] += 2;
        }
    }
    for (size_t i = 0; i + 2 < w.size(); ++i) {
        Word th = (Word(Byte(w[i])) << 16) | (Word(Byte(w[i + 1])) << 8) | Word(Byte(w[i + 2]));
        for (Word t = 0; t < 6; ++t) {
            Word idx = mix(salt ^ (th * 16777619u) ^ (t * 19937u)) & (NEURONS - 1);
            votes[idx] += 3;
        }
    }

    std::array<Word, NEURONS> order{};
    for (Word i = 0; i < NEURONS; ++i)
        order[i] = i;
    std::partial_sort(order.begin(), order.begin() + target_k, order.end(), [&](Word a, Word b) {
        if (votes[a] != votes[b])
            return votes[a] > votes[b];
        return mix(whole_h ^ a) > mix(whole_h ^ b);
    });
    SpikeVec s{};
    for (Word i = 0; i < target_k; ++i) {
        Word idx = order[i];
        s.bits[idx >> 5] |= 1u << (idx & 31u);
    }
    return s;
}

// ---------------------------------------------------------------------------
// Concept / Lexical Assembly
// ---------------------------------------------------------------------------
struct ConceptNode {
    SpikeVec concept_sdr{};
    SpikeVec lexical_sdr{};
    char word[MAX_WORD_CHARS]{};
    Word categories = CAT_NONE;
    Word usage_count = 0;
    Word created_tick = 0;
    int numeric_val = QTY_NONE;
    Word relation_meta = 0;
    Word inverse_concept = 0;
    Word antonym_concept = 0;
    Word parent_cat = 0;
    Word active = 0;
};

constexpr Word META_TRANSITIVE = 1u << 0;
constexpr Word META_SYMMETRIC = 1u << 1;
constexpr Word META_MUTABLE = 1u << 2;
constexpr Word META_ORDER = 1u << 3;
constexpr Word META_INVERSE = 1u << 4;

enum LexMarker : Word {
    LM_SELF = 0,
    LM_USER = 1,
    LM_WORLD = 2,
    LM_IS_A = 3,
    LM_HAS_PROP = 4,
    LM_CAN = 5,
    LM_IN = 6,
    LM_HAS = 7,
    LM_CAUSES = 8,
    LM_WANTS = 9,
    LM_THINKS = 10,
    LM_MORE = 11,
    LM_LESS = 12,
    LM_OPPOSITE = 13,
    LM_NOT = 14,
    LM_YES = 15,
    LM_NO = 16,
    LM_AND = 17,
    LM_THEN = 18,
    LM_BUT = 19,
    LM_BECAUSE = 20,
    LM_IF = 21,
    LM_ALL = 22,
    LM_EARLIER = 23,
    LM_NOW = 24,
    LM_AFTER = 25,
    LM_IT = 26,
    LM_HE = 27,
    LM_THERE = 28,
    LM_AWAY = 29,
    LM_PRESENT = 30,
    LM_TRANSITIVE = 31,
    LM_SYMMETRIC = 32,
    LM_MUTABLE = 33,
    LM_MOVED = 34,
    LM_GAVE = 35,
    LM_WHERE = 36,
    LM_WHAT = 37,
    LM_WHO = 38,
    LM_WHY = 39,
    LM_HOW = 40,
    LM_WHICH = 41,
    LM_OR = 42,
    LM_EITHER = 43,
    LM_SKIP = 44,
    LM_SUPPOSE = 45,
    LM_MANY = 46,
    LM_TOTAL = 47,
    LM_MOST = 48,
    LM_LEAST = 49,
    LM_SOME = 50,
    LM_DO = 51,
    LM_PLUS = 52,
    LM_MINUS = 53,
    LM_TIMES = 54,
    LM_DIV = 55,
    LM_NAME = 56,
    LM_LIKES = 57,
    LM_CONNECTED = 58,
    LM_SAID = 59,
    LM_TRUST = 60,
    LM_HOLDS = 61,
    LM_EVERYTHING = 62,
    LM_EACH = 63,
    LM_START = 64,
    LM_BEFORE = 65,
    LM_FIRST = 66,
    LM_LAST = 67,
    LM_REMIND = 68,
    LM_WHEN = 69,
    LM_COSTS = 70,
    LM_FAIR = 71,
    LM_DISLIKES = 72,
    LM_REQUIRES = 73,
    LM_SIMILAR = 74,
    LM_ANY = 75,
    LM_WOULD = 76,
    LM_GREATER = 77,
    LM_HALF = 78,
    LM_TWICE = 79,
    LM_NEED = 80,
    LM_KNOWS = 81,
    LM_ALIKE = 82,
    LM_ROUTE = 83,
    LM_NEXT = 84,
    LM_MIDDLE = 85,
    LM_RANK = 86,
    LM_SPEED = 87,
    LM_REMAINDER = 88,
    LM_BUYS = 89,
    LM_NAVIGATE = 90,
    LM_BROKEN = 91,
    LM_REPAIRED = 92,
    LM_STORY = 93,
    LM_REMEMBER = 94,
    LM_EXPLAIN = 95,
    LM_IFF = 96,
    LM_PERCENT = 97,
    LM_PURPOSE = 98,
    LM_ACCIDENT = 99,
    LM_WRONG = 100,
    LM_PROMISED = 101,
    LM_BROUGHT = 102,
    LM_PROMISE = 103,
    LM_MAY = 104,
    LM_MUST = 105,
    LM_NOBODY = 106,
    LM_EVERYONE = 107,
    LM_LEFT_SIDE = 108,
    LM_RIGHT_SIDE = 109,
    LM_POUR = 110,
    LM_OVERFLOW = 111,
    LM_EVEN = 112,
    LM_ODD = 113,
    LM_THAT = 114,
    LM_PARENT_OF = 115,
    LM_GRANDPARENT = 116,
    LM_SIBLING = 117,
    LM_ANCESTOR = 118,
    LM_BEATS = 119,
    LM_COOPERATE = 120,
    LM_DEFECT = 121,
    LM_FEELS = 122,
    LM_HAPPY = 123,
    LM_SAD = 124,
    LM_AFRAID = 125,
    LM_CURIOUS_EM = 126,
    LM_LOST = 127,
    LM_FOUND = 128,
    LM_CHASED = 129,
    LM_IRONIC = 130,
    LM_LITERAL = 131,
    LM_TRUTH = 132,
    LM_LIED = 133,
    LM_PEER = 134,
    LM_TOOL = 135,
    LM_FIRE = 136,
    LM_KINDLE = 137,
    LM_FREEZING = 138,
    LM_CANNOT = 139,
    LM_TOLD = 140,
    LM_MAKES = 141,
    LM_FIND = 142,
    LM_PROTECT = 143,
    LM_CASCADE = 144,
    // v11 markers: possessive/gendered coref, algebra, optimization, schema transfer, 2D grid, common knowledge, history & narrative
    LM_HIS = 145,
    LM_HER = 146,
    LM_THEIR = 147,
    LM_ITS = 148,
    LM_SHE = 149,
    LM_THEY = 150,
    LM_EQUALS = 151,
    LM_SOLVE = 152,
    LM_VAR_X = 153,
    LM_WEIGHT = 154,
    LM_VALUE = 155,
    LM_LIMIT = 156,
    LM_PARALLEL = 157,
    LM_ORDER = 158,
    LM_CORRESPONDS = 159,
    LM_CONTROLS = 160,
    LM_FLOW = 161,
    LM_NORTH = 162,
    LM_SOUTH = 163,
    LM_EAST = 164,
    LM_WEST = 165,
    LM_PRIVATELY = 166,
    LM_MUD = 167,
    LM_CLEAN = 168,
    LM_CAUSED = 169,
    LM_ALSO = 170,
    LM_TOOK = 171,
    LM_SLEPT = 172,
    LM_SO = 173,
    LM_SIGHT = 174,
    LM_KNIGHT = 175,
    LM_KNAVE = 176,
    LM_XOR = 177,
    LM_IMPLIES = 178,
    LM_CONNECTS = 179,
    LM_CONTRAST = 180,
    LM_POSTERIOR = 181,
    LM_OVERLAP = 182,
    LM_ALWAYS = 183,
    LM_NEVER = 184,
    LM_SOMETIMES = 185,
    LM_SWITCH = 186,
    LM_STAY = 187,
    LM_DECIDE = 188,
    LM_FAVORITE = 189,
    LM_LIVE = 190,
    LM_SET = 191,
    LM_ALONE = 192,
    LM_MAPS = 193,
    LM_CONTRIB = 194,
    LM_RECALL = 195,
    LM_VISITED = 196,
    LM_CARRIED = 197,
    LM_CANDIDATE = 198,
    LM_BELONGS = 199,
    LM_VALUES = 200,
    LM_INTERVENE = 201,
    LM_INVENT = 202,
    LM_DIAGNOSE = 203,
    LM_CLUES = 204,
    LM_WONDER = 205,
    LM_LIVES = 206,
    LM_TASK = 207,
    LM_CLAUSE = 208,
    LM_RISK = 209,
    LM_INITIALLY = 210,
    // v17 markers: Kernel 11 bridge, discourse coref, 4-arg verbs, stochastic planning, budget, exceptions, meetings, fault pipeline, debate!
    LM_LINK = 211,
    LM_BUDGET = 212,
    LM_EXCEPT = 213,
    LM_SCHEDULE = 214,
    LM_PIPELINE = 215,
    LM_FAILING = 216,
    LM_PRO = 217,
    LM_CON = 218,
    LM_WITH = 219,
    LM_TO = 220,
    LM_FROM = 221,
    LM_UPSTREAM = 222,
    LM_PRIORITY = 223,
    LM_SAFETY = 224,
    LM_DEBATE = 225,
    LM_ALLOCATE = 226,
    LM_RECEIVED = 227,
    // v18 markers: Kernel 12 clustering, embedded clauses, coalition Shapley, thermo, seq-op, BOM, consensus, event-calc & meta-rule!
    LM_OUTLIER = 228,
    LM_CLOSEST = 229,
    LM_COALITION = 230,
    LM_SHAPLEY = 231,
    LM_MIX = 232,
    LM_REGISTER = 233,
    LM_RECIPE = 234,
    LM_WITNESS = 235,
    LM_TICK = 236,
    LM_PATCH = 237,
    LM_RULE = 238,
    LM_EXPLORER = 239,
    // v19 markers: Kernel 13 cortical gate, multi-turn chat recap, Bellman MDP, mediation, max-flow, Vickrey auction, 2D vectors, PERT/CPM & duty priority!
    LM_VERIFY = 240,
    LM_SPOTTED = 241,
    LM_RECAP = 242,
    LM_HORIZON = 243,
    LM_MEDIATOR = 244,
    LM_CONDUIT = 245,
    LM_AUCTION = 246,
    LM_VICKREY = 247,
    LM_POINT = 248,
    LM_MODULE = 249,
    LM_DUTY = 250,
    LM_PRICE = 251,
    // v20 markers: Kernel 14 SWR dream, narrative chapters, Walrasian market, Newtonian physics, Socratic assistant, affine 2D, info theory, Nash 2x2 & causal circuits!
    LM_DREAM = 252,
    LM_CHAPTER = 253,
    LM_CLEARING = 254,
    LM_MARKET = 255,
    LM_MOMENTUM = 256,
    LM_FULCRUM = 257,
    LM_ASSISTANT = 258,
    LM_AFFINE = 259,
    LM_NASH = 260,
    LM_PAYOFF = 261,
    LM_CIRCUIT = 262,
    LM_POWERED = 263,
    LM_DEAD = 264,
    // v21 markers: Kernel 15 universal quantifier scan, reflective opinions, linear law, finance NPV/ROI, kinematics, MCDA, Kruskal MST, LTL & apportionment!
    LM_FORALL = 265,
    LM_EXISTS = 266,
    LM_SCAN = 267,
    LM_OPINION = 268,
    LM_TRIAL = 269,
    LM_TEMP = 270,
    LM_YIELD = 271,
    LM_PROJECT = 272,
    LM_KINEMATICS = 273,
    LM_CRITERION = 274,
    LM_CABLE = 275,
    LM_WIRING = 276,
    LM_TRACE = 277,
    LM_SEATS = 278,
    // v22 markers: Kernel 16 deductive chain, open-topic Socratic Q&A, Dijkstra, sequential Bayes, expr tree, elastic collision, 0/1 cargo DP, coalition core & self-invented experiments!
    LM_DEDUCE = 279,
    LM_TOPIC = 280,
    LM_ROAD = 281,
    LM_SHORTEST = 282,
    LM_SENSOR_BAYES = 283,
    LM_VAR_EXPR = 284,
    LM_ELASTIC = 285,
    LM_CARGO = 286,
    LM_CORE = 287,
    LM_THEORY = 288,
    // v23 markers: Kernel 17 backward apical abduction, resource dynamics, ballistics, 2x2 matrix, stats, weighted gig DP, Condorcet/Borda voting & Pearl d-separation!
    LM_ABDUCE = 289,
    LM_STOCK = 290,
    LM_SUSTAINABLE = 291,
    LM_PROJECTILE = 292,
    LM_MATRIX = 293,
    LM_DATASET = 294,
    LM_GIG = 295,
    LM_BALLOT = 296,
    LM_DAG = 297,
    LM_CONFOUNDER = 298,
    // v24 markers: Kernel 18 counterfactual imagination, Markov steady-state, 2D LP, recursive trees, waves, Ohm circuits, Mendel genetics & asymptotic complexity!
    LM_IMAGINE = 299,
    LM_LOSING = 300,
    LM_GAINING = 301,
    LM_MARKOV = 302,
    LM_STEADY = 303,
    LM_LP = 304,
    LM_TREENODE = 305,
    LM_WAVE = 306,
    LM_RESISTOR = 307,
    LM_SHUNT = 308,
    LM_TRAIT = 309,
    LM_ALGO = 310,
    LM_GEN_DOM = 311,
    LM_GEN_HET = 312,
    LM_GEN_REC = 313,
    LM_ALG_CONST = 314,
    LM_ALG_LIN = 315,
    LM_ALG_NLOGN = 316,
    LM_ALG_QUAD = 317,
    // v25 markers: Kernel 19 hierarchical plan chaining, pronoun dialogue, epidemic R0, buoyancy, portfolio, topo DAG, binary search, Doppler & iterated PD!
    LM_PLAN = 318,
    LM_HAVING = 319,
    LM_EPIDEMIC = 320,
    LM_HERD = 321,
    LM_FLUID = 322,
    LM_SLAB = 323,
    LM_ASSET = 324,
    LM_EVENLY = 325,
    LM_DEP = 326,
    LM_TOPO = 327,
    LM_SORTED = 328,
    LM_SIREN = 329,
    LM_DOPPLER = 330,
    LM_PD = 331,
    LM_GRIM = 332,
    LM_THRESHOLD = 333,
    // v26 markers: Kernel 20 NMDA action-conditioned causal simulation, multi-paragraph prose, stoichiometry, optics, entropy, LCS/edit DP, queueing, Diffie-Hellman & envy-free cake division!
    LM_SIMULATE = 334,
    LM_CHEM = 335,
    LM_LIMITING = 336,
    LM_OPTICS = 337,
    LM_LENS = 338,
    LM_DIST = 339,
    LM_ENTROPY = 340,
    LM_HALVES = 341,
    LM_QUARTERS = 342,
    LM_EIGHTHS = 343,
    LM_SEQ = 344,
    LM_LCS = 345,
    LM_EDIT = 346,
    LM_QUEUE = 347,
    LM_LITTLE = 348,
    LM_DH = 349,
    LM_PUBKEY = 350,
    LM_SHARED = 351,
    LM_CAKE = 352,
    LM_ENVYFREE = 353,
    // v27 markers: Kernel 21 spiking constraint satisfaction, Layer 3d causal-because prose, 2D pursuit, ideal gas, CRT, bipartite 2-coloring, bond compounding, Nyquist & commons!
    LM_CONSTRAIN = 354,
    LM_REQUIRING = 355,
    LM_EXCLUDING = 356,
    LM_MOVER2D = 357,
    LM_GAS = 358,
    LM_PRESSURE = 359,
    LM_CRT = 360,
    LM_MODINV = 361,
    LM_BIPARTITE = 362,
    LM_BOND = 363,
    LM_RULE72 = 364,
    LM_SIGNAL = 365,
    LM_NYQUIST = 366,
    LM_ALIAS = 367,
    LM_COMMONS = 368,
    LM_PIGOUVIAN = 369,
    // v28 markers: Kernel 22 few-shot relational induction, Layer 3e concessive prose, Kepler orbits, 2x2 eigenvalues, Bayes odds, shoelace, DFA, Keynesian MPC & Hamming!
    LM_INDUCE = 370,
    LM_ALTHOUGH = 371,
    LM_DESPITE = 372,
    LM_ORBIT = 373,
    LM_RADIUS = 374,
    LM_MAT2D = 375,
    LM_EIGENVALUES = 376,
    LM_ODDS = 377,
    LM_LIKELIHOOD = 378,
    LM_POLY2D = 379,
    LM_SHOELACE = 380,
    LM_DFA = 381,
    LM_TRACEDFA = 382,
    LM_ECONOMY = 383,
    LM_MULTIPLIER = 384,
    LM_CODEWORD = 385,
    LM_HAMMING = 386,
    // v29 markers: Kernel 23 compositional rule algebra, Layer 3f temporal prose, Lorentz, capacitors/RC, Euler, F1, control, logistic & Huffman!
    LM_CHAINRULE = 387,
    LM_CROSSCHAIN = 388,
    LM_STATE_PW = 389,
    LM_CAUSE_PW = 390,
    LM_AFFECT_PW = 391,
    LM_TOOL_PW = 392,
    LM_ELSE = 393,
    LM_WHOLE = 394,
    LM_RELATIV = 395,
    LM_DILATED = 396,
    LM_CONTRACTED = 397,
    LM_RELATIVISTIC = 398,
    LM_CAPACITOR = 399,
    LM_RC = 400,
    LM_EULER = 401,
    LM_CONFUS = 402,
    LM_PRECISION = 403,
    LM_F1 = 404,
    LM_CONTROLLER = 405,
    LM_OFFSET = 406,
    LM_LOGISTIC = 407,
    LM_MSY = 408,
    LM_HUFFMAN = 409,
    // v30 Stage 4 markers: Kernel 24 live 1-shot online schema, Layer 3g multi-agent/counterfactual prose, photoelectric, pH, RSA, A*, Nash mixed, PID & Bernoulli!
    LM_ONLINESCHEMA = 410,
    LM_YIELDS = 411,
    LM_THROUGH = 412,
    LM_PHOTO = 413,
    LM_WORKFUNC = 414,
    LM_CUTOFF = 415,
    LM_SOLUTION = 416,
    LM_PH = 417,
    LM_POH = 418,
    LM_DILUTE = 419,
    LM_RSA = 420,
    LM_ENCRYPT = 421,
    LM_DECRYPT = 422,
    LM_ASTAR = 423,
    LM_HEUR = 424,
    LM_ADMISSIBLE = 425,
    LM_MIXNASH = 426,
    LM_PID = 427,
    LM_DERIV = 428,
    LM_INTEGRAL = 429,
    LM_VENTURI = 430,
    LM_PRESSDROP = 431,
    LM_JAMMED = 432,
    // v31 Stage 4 markers: Kernel 25 live LTD/LTP schema revision, Layer 3h belief revision & clarify, calculus, half-life, Hardy-Weinberg, RPN, OLS, coins & Kirchhoff!
    LM_REVISE = 433,
    LM_REVISESCHEMA = 434,
    LM_CLARIFY = 435,
    LM_BELIEF = 436,
    LM_POLY = 437,
    LM_COEFF = 438,
    LM_DERIV2 = 439,
    LM_ISOTOPE = 440,
    LM_HALFLIFE = 441,
    LM_DECAY = 442,
    LM_GENE = 443,
    LM_HARDY = 444,
    LM_RPN = 445,
    LM_PUSH = 446,
    LM_REGRESS = 447,
    LM_COINS = 448,
    LM_KCL = 449,
    LM_KVL = 450,
    LM_RESIST = 451,
    // v32 300-Domain Milestone markers: Kernel 26 spiking role-binder, live unseen-word acquisition, Carnot, Shannon-Hartley, Vec3, totient, dB, elasticity & cache AMAT!
    LM_ROLEBIND = 452,
    LM_UNBIND = 453,
    LM_SLOTAGENT = 454,
    LM_SLOTACT = 455,
    LM_SLOTOBJ = 456,
    LM_SLOTLOC = 457,
    LM_CONCEPT = 458,
    LM_CARRIES = 459,
    LM_CARNOT = 460,
    LM_EFF = 461,
    LM_CHANNEL = 462,
    LM_VEC3 = 463,
    LM_TOTIENT = 464,
    LM_COPRIME = 465,
    LM_DECIBEL = 466,
    LM_DEMAND = 467,
    LM_CACHE = 468,
    // v33 Stage 5 Crossing markers: Kernel 27 predictive spiking syntax, compositional story linking, hydrostatics/Pascal, Stefan-Boltzmann/Wien, 2x2 matrix, combinatorics, TCP BDP, compound interest & Kruskal MST!
    LM_SEQPARSE = 469,
    LM_NARRATE = 470,
    LM_SEQROLE = 471,
    LM_LINKSTORY = 472,
    LM_PAST = 473,
    LM_BEYOND = 474,
    LM_HYDRO = 475,
    LM_PASCAL = 476,
    LM_RADSTAR = 477,
    LM_WIEN = 478,
    LM_MAT2 = 479,
    LM_MATDET = 480,
    LM_MATTRACE = 481,
    LM_MATINV = 482,
    LM_PERM = 483,
    LM_COMB = 484,
    LM_DERANGE = 485,
    LM_NETLINK = 486,
    LM_COMPOUND = 487,
    LM_DOUBLE72 = 488,
    LM_MSTEDGE = 489,
    LM_MSTWEIGHT = 490,
    LM_MSTEDGES = 491,
    LM_MSTMAX = 492,
    // v34 Stage 5b markers: Kernel 28 pure-GPU parietal neural-native math, Coulomb, organ pipes, modular math, Amdahl/Gustafson, virtual memory/TLB, escape/Schwarzschild & 2D metrics!
    LM_NEURALMATH = 493,
    LM_NMADD = 494,
    LM_NMSUB = 495,
    LM_NMMUL = 496,
    LM_NMMOD = 497,
    LM_NMSOLVE = 498,
    LM_COULOMB = 499,
    LM_PLATECAP = 500,
    LM_ORGANPIPE = 501,
    LM_BEATFREQ = 502,
    LM_MODPOW = 503,
    LM_INVMOD = 504,
    LM_AMDAHL = 505,
    LM_MAXSPEED = 506,
    LM_GUSTAFSON = 507,
    LM_VMEM = 508,
    LM_TLBEAT = 509,
    LM_PLANETBODY = 510,
    LM_RSCHWARZ = 511,
    LM_PT2D = 512,
    // v35 Stage 5c markers: Kernel 29 pure-GPU axonal-delay spiking graph wavefront + 7 deep STEM domains!
    LM_GRAPHWAVE = 513,
    LM_GWEDGE = 514,
    LM_GWDIST = 515,
    LM_GWHOPS = 516,
    LM_GWVIA = 517,
    LM_GWBLOCK = 518,
    LM_GN0 = 519,
    LM_GN1 = 520,
    LM_GN2 = 521,
    LM_GN3 = 522,
    LM_GN4 = 523,
    LM_GN5 = 524,
    LM_ISOBARIC = 525,
    LM_ENTROP = 526,
    LM_FARADAY = 527,
    LM_EMF = 528,
    LM_XFORM = 529,
    LM_CATALAN = 530,
    LM_STARSBARS = 531,
    LM_PIGEON = 532,
    LM_TWOSCOMP = 533,
    LM_NANDGATE = 534,
    LM_NORGATE = 535,
    LM_BODE = 536,
    LM_CPUSCHED = 537,
    LM_DEBROGLIE = 538,
    LM_BOHR = 539,
    // v36 Stage 5d Markers: Kernel 30 Pure-GPU Spiking SAT, Tier-2 16k HBM Archive, Reynolds, Hamming, 3x3 Mat, Kinetics, CIDR & Beam!
    LM_SATSOLVE = 540,
    LM_SATCHECK = 541,
    LM_SATVAL = 542,
    LM_SATCLAUSE = 543,
    LM_SATRESET = 544,
    LM_SV0 = 545,
    LM_SV1 = 546,
    LM_SV2 = 547,
    LM_SV3 = 548,
    LM_HBMARCHIVE = 549,
    LM_HBMSTORE = 550,
    LM_HBMPAD = 551,
    LM_HBMRECALL = 552,
    LM_HBMCOUNT = 553,
    LM_REYNOLDS = 554,
    LM_HAMDIST = 555,
    LM_PARITY = 556,
    LM_MAT3 = 557,
    LM_KINETICS = 558,
    LM_CIDR = 559,
    LM_BEAM = 560,
    LM_MOMENT = 561,
    // v37 STAGE 6 CROSSING Markers: Kernel 31 Pure-GPU Spiking V1/V4 Vision, Self-Teaching Reader/Curiosity, Code Lab & 6 Stage-6 STEM Domains!
    LM_RETINA = 562,
    LM_VISSCAN = 563,
    LM_VISCOUNT = 564,
    LM_VISREL = 565,
    LM_VISTEACH = 566,
    LM_VISBOX = 567,
    LM_VISTRI = 568,
    LM_VISCROSS = 569,
    LM_VISRING = 570,
    LM_VISLSHAPE = 571,
    LM_VISDUAL = 572,
    LM_VISSCENE = 573,
    LM_STUDY = 574,
    LM_STUDYGAP = 575,
    LM_STUDYINFO = 576,
    LM_CODEPROG = 577,
    LM_CODERUN = 578,
    LM_CODESTEP = 579,
    LM_CODECHECK = 580,
    LM_CODEFIX = 581,
    LM_CHORD = 582,
    LM_ROTOR = 583,
    LM_TORQUE = 584,
    LM_COMPILER = 585,
    LM_DNASEQ = 586,
    LM_CIPHER = 587,
    LM_ROBOT = 588,
    LM_MAXREACH = 589,
    // v38 STAGE 6b Markers: Kernel 32 Pure-GPU Spiking Cochlear Audio/Prosody Cortex, Textbook Causal Induction/Self-Quiz, Prosody Empathy & 5 STEM Domains!
    LM_COCHLEA = 590,
    LM_AUDSCAN = 591,
    LM_AUDBEATS = 592,
    LM_AUDPITCH = 593,
    LM_AUDTEACH = 594,
    LM_AUDRISE = 595,
    LM_AUDFALL = 596,
    LM_AUDMAJOR = 597,
    LM_AUDTRITONE = 598,
    LM_AUDMARCH = 599,
    LM_TEXTBOOK = 600,
    LM_EMPATHY = 601,
    LM_TENSE = 602,
    LM_PROUD = 603,
    LM_LAUD = 604,
    LM_SOLID3D = 605,
    LM_PYRVOL = 606,
    LM_GRAPHC = 607,
    LM_DSP = 608,
    LM_ORGCHEM = 609,
    LM_ALKMASS = 610,
    LM_DBQUERY = 611,
    // v39 STAGE 6c Markers: Kernel 33 Pure-GPU Cross-Modal Vision+Audio Binding & Prefrontal Goal Stack, Socratic Coaching, Contraptions, Deliberation & 5 STEM Domains!
    LM_XMODAL = 612,
    LM_XFUSE = 613,
    LM_XSYNEST = 614,
    LM_XBIND = 615,
    LM_GSTACK = 616,
    LM_XBEACON = 617,
    LM_XHAZARD = 618,
    LM_XPROBE = 619,
    LM_XVAULT = 620,
    LM_XCONVOY = 621,
    LM_GOALMAP = 622,
    LM_GOALFIX = 623,
    LM_GOALRUN = 624,
    LM_COACH = 625,
    LM_CONTRAPTION = 626,
    LM_MECHADV = 627,
    LM_DELIBERATE = 628,
    LM_ASTRO = 629,
    LM_STOCHD = 630,
    LM_ELECGRID = 631,
    LM_DISTSYS = 632,
    LM_QMIN = 633,
    LM_INVOPT = 634,
    // v40 STAGE 7 CROSSING Markers: Kernel 34 Pure-GPU Spiking Dialogue Coherence Cortex, Free-Form Open Multi-Turn Chat, Pun Wit, Persona, Hypothesis Tests, Carnot, Master Theorem, Immunology & Ethics!
    LM_DIALOGUE = 635,
    LM_DLGMODE = 636,
    LM_DLGTEACH = 637,
    LM_DLGSTANCE = 638,
    LM_DLGCHECK = 639,
    LM_DLGRESET = 640,
    LM_CUEFACT = 641,
    LM_CUEWHY = 642,
    LM_CUEFEEL = 643,
    LM_CUEPLAN = 644,
    LM_CUECOMP = 645,
    LM_CUEDILEMMA = 646,
    LM_CUESTORY = 647,
    LM_CUEARGUE = 648,
    LM_DLGEXPLAIN = 649,
    LM_DLGEMPATHIZE = 650,
    LM_DLGPLAN = 651,
    LM_DLGCOMPARE = 652,
    LM_DLGDEBATE = 653,
    LM_CLAIMONE = 654,
    LM_POSSTANCE = 655,
    LM_NEGSTANCE = 656,
    LM_OPENCHAT = 657,
    LM_PUNWIT = 658,
    LM_PERSONA = 659,
    LM_STATTEST = 660,
    LM_ZSCORE = 661,
    LM_THERMOC = 662,
    LM_CARNOTPCT = 663,
    LM_MASTERREC = 664,
    LM_IMMUNOL = 665,
    LM_ETHICF = 666,
    LM_CONN_CHAT = 667,
    LM_ABOUT = 668,
    LM_TELL = 669,
    // v41 STAGE 7b Markers: Kernel 35 Pure-GPU Metaphor/Narrative/Self-Critique Cortex, Universal Open Chat, Autonomous Science Loop, Compositional Story & 5 Stage-7b STEM Domains!
    LM_METAPHOR = 670,
    LM_METBLEND = 671,
    LM_METEXPLAIN = 672,
    LM_METTEACH = 673,
    LM_METPLOT = 674,
    LM_METCRITIQUE = 675,
    LM_METRIVER = 676,
    LM_METWEB = 677,
    LM_METSPARK = 678,
    LM_METMIRROR = 679,
    LM_METCOMPASS = 680,
    LM_MFLOW = 681,
    LM_MTIME = 682,
    LM_MLINK = 683,
    LM_MMIND = 684,
    LM_MFIRE = 685,
    LM_MIDEA = 686,
    LM_MGLASS = 687,
    LM_MTRUTH = 688,
    LM_MNORTH = 689,
    LM_MGOAL = 690,
    LM_PLOTQUEST = 691,
    LM_PLOTTRIAL = 692,
    LM_PLOTTURN = 693,
    LM_PLOTWIN = 694,
    LM_FLAWHASTY = 695,
    LM_FLAWCIRCLE = 696,
    LM_FLAWFALSE = 697,
    LM_FLAWSTRAW = 698,
    LM_FLAWCASE = 699,
    LM_FLAWALL = 700,
    LM_FLAWSELF = 701,
    LM_FLAWBOTH = 702,
    LM_FLAWSHIFT = 703,
    LM_SCILOOP = 704,
    LM_HYPFAST = 705,
    LM_HYPSLOW = 706,
    LM_TALEBOX = 707,
    LM_LINPROG = 708,
    LM_SPECTRAL = 709,
    LM_NYQMIN = 710,
    LM_ALLELEHW = 711,
    LM_CRYPTODL = 712,
    LM_DHSECRET = 713,
    LM_TROPHIC = 714,
    // v42 STAGE 7c (400 DOMAINS MILESTONE!): Kernel 36 Pure-GPU Belief-Graph & Scientific Debate Cortex, Rich Multi-Facet Unscripted Chat & 7 Stage-7c STEM Domains!
    LM_BELGRAPH = 715,
    LM_BELTEACH = 716,
    LM_BELREFUTE = 717,
    LM_BELCHECK = 718,
    LM_BELCRED = 719,
    LM_BELDEBATE = 720,
    LM_THEORYWAVE = 721,
    LM_THEORYDUST = 722,
    LM_THEORYFIELD = 723,
    LM_THEORYFLUX = 724,
    LM_EVLENS = 725,
    LM_EVPRISM = 726,
    LM_EVVOID = 727,
    LM_EVPULSE = 728,
    LM_EVRAY = 729,
    LM_PEERADA = 730,
    LM_PEERLOKI = 731,
    LM_PEERVER = 732,
    LM_PEERBLUFF = 733,
    LM_ELABORATE = 734,
    LM_PEERREV = 735,
    LM_PROOFFORM = 736,
    LM_STATMECH = 737,
    LM_GAMEMECH = 738,
    LM_NEUROBIO = 739,
    LM_TAUTIME = 740,
    LM_CONTROLOPT = 741,
    LM_ASTRODYN = 742,
    LM_SEMIAX = 743,
    // v43 STAGE 8 CROSSING (COLLECTIVE SCIENTIFIC MINDS — 410 DOMAINS!): Kernel 37 Collective Discovery + Socratic Curiosity + 7 Stage-8 STEM Domains!
    LM_COLLDISC = 744,
    LM_COLLPOOL = 745,
    LM_COLLVOTE = 746,
    LM_COLLDERIVE = 747,
    LM_COLLPROVE = 748,
    LM_COLLMAKE = 749,
    LM_COLLTEACH = 750,
    LM_COLLREFUTE = 751,
    LM_LAWTHERMO = 752,
    LM_LAWGENE = 753,
    LM_LAWETHER = 754,
    LM_LAWQUANTUM = 755,
    LM_SPECPHYS = 756,
    LM_SPECBIO = 757,
    LM_SPECMATH = 758,
    LM_SPECLOGIC = 759,
    LM_THMCARNOT = 760,
    LM_THMFORCE = 761,
    LM_THMMAXWELL = 762,
    LM_THMDIRAC = 763,
    LM_AXENERGY = 764,
    LM_AXENTROPY = 765,
    LM_AXMASS = 766,
    LM_AXACCEL = 767,
    LM_AXCHARGE = 768,
    LM_AXFLUX = 769,
    LM_AXWAVE = 770,
    LM_AXSPIN = 771,
    LM_RUMINATE = 772,
    LM_SPECCODE = 773,
    LM_QGATE = 774,
    LM_CHANCODE = 775,
    LM_HEATPDE = 776,
    LM_TNEXT = 777,
    LM_COALBID = 778,
    LM_SHAPA = 779,
    LM_EPIOPT = 780,
    LM_COLLLAB = 781,
    // v44 STAGE 8b (CONTINUAL SELF-RESEARCH & MULTI-SENTENCE SPIKING FLUENCY — 420 DOMAINS!): Kernel 38 + Multi-Sentence Discourse + 7 Stage-8b STEM Domains!
    LM_SELFRES = 782,
    LM_RESDESIGN = 783,
    LM_RESPATCH = 784,
    LM_RESWHY = 785,
    LM_RESLEARN = 786,
    LM_RESEXP = 787,
    LM_HYPGRAV = 788,
    LM_HYPDRAG = 789,
    LM_HYPMAG = 790,
    LM_HYPHEAT = 791,
    LM_HYPWAVE = 792,
    LM_HYPPART = 793,
    LM_HYPACID = 794,
    LM_HYPBASE = 795,
    LM_EXPVACUUM = 796,
    LM_EXPCOIL = 797,
    LM_EXPPRISM = 798,
    LM_EXPLITMUS = 799,
    LM_EXPSPECTRUM = 800,
    LM_SYMOFFONE = 801,
    LM_SYMINFLOOP = 802,
    LM_SYMNULL = 803,
    LM_SYMRACE = 804,
    LM_SYMLEAK = 805,
    LM_SYMOVERFL = 806,
    LM_SYMDEADLK = 807,
    LM_SYMDRIFT = 808,
    LM_PATBOUND = 809,
    LM_PATGUARD = 810,
    LM_PATMUTEX = 811,
    LM_PATCLAMP = 812,
    LM_PATFREE = 813,
    LM_DISCOURSE = 814,
    LM_KALMAN = 815,
    LM_POSTX = 816,
    LM_BAYESNET = 817,
    LM_FLUIDYN = 818,
    LM_CRYPTOECC = 819,
    LM_OPTPATH = 820,
    LM_COMPILEROPT = 821,
    LM_HOISTOPS = 822,
    LM_ECONMACRO = 823,
    // v45 STAGE 8c (PURE-GPU NEURAL SYMBOLIC CALCULUS & SPONTANEOUS LIVE CONCEPT TEACHING — 430 DOMAINS!): Kernel 39 + Live Teaching/Story/Why + 7 Stage-8c STEM Domains!
    LM_SYMCALC = 824,
    LM_SYMDIFF = 825,
    LM_SYMINT = 826,
    LM_SYMEVAL = 827,
    LM_SYMADD = 828,
    LM_POLYALPHA = 829,
    LM_POLYBETA = 830,
    LM_POLYGAMMA = 831,
    LM_POLYOMEGA = 832,
    LM_TEACHLIVE = 833,
    LM_STORYTIME = 834,
    LM_WHYSO = 835,
    LM_KLDIV = 836,
    LM_BODEMARG = 837,
    LM_PMDEG = 838,
    LM_QTUNNEL = 839,
    LM_RELINV = 840,
    LM_RAFTCON = 841,
    LM_RAFTQUOR = 842,
    LM_REGALLOC = 843,
    LM_EVOGAME = 844,
    // v46 STAGE 8d (PURE-GPU AST PROGRAM SYNTHESIS/EXECUTION & FREE-FORM DEBATE/COUNTERFACTUALS — 440 DOMAINS!): Kernel 40 + Whatif/Debate + 7 Stage-8d STEM Domains!
    LM_PROGAST = 845,
    LM_ASTADD = 846,
    LM_ASTVERIFY = 847,
    LM_ASTEXEC = 848,
    LM_ASTSYNTH = 849,
    LM_PROGALPHA = 850,
    LM_PROGBETA = 851,
    LM_PROGGAMMA = 852,
    LM_PROGOMEGA = 853,
    LM_WHATIF = 854,
    LM_DEBATENOW = 855,
    LM_MERKLE = 856,
    LM_ROCKET = 857,
    LM_DELTAV = 858,
    LM_LITTLEQ = 859,
    LM_ENZYME = 860,
    LM_ENZVEL = 861,
    LM_QUORUM = 862,
    LM_SNRDB = 863,
    LM_CFGPARSE = 864,
    // v47 STAGE 8e (PURE-GPU NEURAL LINEAR ALGEBRA & STORY CO-WRITING / SELF-QUESTIONING — 450 DOMAINS!): Kernel 41 + Cowrite/Curiousit + 7 Stage-8e STEM Domains!
    LM_LINALG = 865,
    LM_LINDET = 866,
    LM_LINTR = 867,
    LM_LINVEC = 868,
    LM_LINSOLVE = 869,
    LM_LINALPHA = 870,
    LM_LINBETA = 871,
    LM_LINGAMMA = 872,
    LM_LINOMEGA = 873,
    LM_COWRITE = 874,
    LM_CURIOUSIT = 875,
    LM_RSAKEY = 876,
    LM_AEROLIFT = 877,
    LM_LIFTFRC = 878,
    LM_CACHEGEO = 879,
    LM_KEPLERORB = 880,
    LM_QUANTADC = 881,
    LM_NERNST = 882,
    LM_COURNOT = 883,
    LM_COURPRC = 884,
    // v48 STAGE 8f (PURE-GPU SPIKING PROBABILISTIC BAYES-NET & SOCRATIC SELF-CRITIQUE — 460 DOMAINS!): Kernel 42 on FINAL FREE PW13 Q3 + Critique/Analogize + 7 Stage-8f STEM Domains!
    LM_BNET = 885,
    LM_BNETPOS = 886,
    LM_BNETNEG = 887,
    LM_BNETDEC = 888,
    LM_BNETLIVE = 889,
    LM_BNALPHA = 890,
    LM_BNBETA = 891,
    LM_BNGAMMA = 892,
    LM_BNOMEGA = 893,
    LM_CRITIQUEIT = 894,
    LM_ANALOGIZE = 895,
    LM_BLACKBODY = 896,
    LM_HAMMCODE = 897,
    LM_LENSOPT = 898,
    LM_FOCLEN = 899,
    LM_AMORTIZE = 900,
    LM_PENDULUM = 901,
    LM_DNAFREQ = 902,
    LM_SOLOWMOD = 903,
    LM_SOLNET = 904,
    // v49 STAGE 8g (PURE-GPU SPIKING MDP BELLMAN PLANNER & OPEN MULTI-TURN USER GOALS — 470 DOMAINS!): Kernel 43 on FREE PW24 Q + GoalPlan/Synthesize + 7 Stage-8g STEM Domains!
    LM_MDPPLAN = 905,
    LM_MDPVAL = 906,
    LM_MDPMID = 907,
    LM_MDPPOL = 908,
    LM_MDPLIVE = 909,
    LM_MDPALPHA = 910,
    LM_MDPBETA = 911,
    LM_MDPGAMMA = 912,
    LM_MDPOMEGA = 913,
    LM_GOALPLAN = 914,
    LM_SYNTHESIZE = 915,
    LM_BTREEDB = 916,
    LM_BTKEYS = 917,
    LM_BANDGAP = 918,
    LM_FRUSTUM = 919,
    LM_QUAKEGEO = 920,
    LM_UNROLL = 921,
    LM_FIBEROPT = 922,
    LM_BUCKCONV = 923,
    LM_BKVOUT = 924,
    // v50 STAGE 8h (100-PILLAR GPU EVOLUTION MILESTONE! PURE-GPU SPIKING KALMAN FILTER & 480 DOMAINS!): Kernel 44 on FREE PW24 Q + HypoTest/Reconsider + 7 Stage-8h STEM Domains!
    LM_KFILT = 925,
    LM_KFGAIN = 926,
    LM_KFVAR = 927,
    LM_KFPOST = 928,
    LM_KFMODE = 929,
    LM_KFLIVE = 930,
    LM_KFALPHA = 931,
    LM_KFBETA = 932,
    LM_KFGAMMA = 933,
    LM_KFOMEGA = 934,
    LM_HYPOTEST = 935,
    LM_RECONSIDER = 936,
    LM_BRPRED = 937,
    LM_BPCPI = 938,
    LM_ALBEDO = 939,
    LM_BLOOMFLT = 940,
    LM_CARDIAC = 941,
    LM_PASSENT = 942,
    LM_SABINE = 943,
    LM_POLYGEOM = 944,
    LM_PGSUM = 945,
    // v51 STAGE 8i (PURE-GPU SPIKING LQR RICCATI CONTROL & MULTI-TURN CONTINUITY — 490 DOMAINS!): Kernel 45 on FREE PW24 Q + ConnectIt/JustifyIt + 7 Stage-8i STEM Domains!
    LM_LQRCTRL = 946,
    LM_LQGAIN = 947,
    LM_LQPOLE = 948,
    LM_LQCTRL = 949,
    LM_LQMODE = 950,
    LM_LQLIVE = 951,
    LM_LQALPHA = 952,
    LM_LQBETA = 953,
    LM_LQGAMMA = 954,
    LM_LQOMEGA = 955,
    LM_CONNECTIT = 956,
    LM_JUSTIFYIT = 957,
    LM_IMGCOMP = 958,
    LM_ICRAW = 959,
    LM_BAROALT = 960,
    LM_SHARDING = 961,
    LM_BUCKLING = 962,
    LM_F1SCORE = 963,
    LM_MANNING = 964,
    LM_PAGETBL = 965,
    LM_PTSPAN = 966,
    // v52 STAGE 8j (500-DOMAIN MILESTONE! PURE-GPU SPIKING FFT HARMONIC CORTEX & MULTI-TURN SELF-CORRECTION!): Kernel 46 on FREE PW24 Q + ChallengeIt/AskBack/Both + 7 Stage-8j STEM Domains!
    LM_FFTSPEC = 967,
    LM_FFDC = 968,
    LM_FFHARM = 969,
    LM_FFNYQ = 970,
    LM_FFBAND = 971,
    LM_FFLIVE = 972,
    LM_FFALPHA = 973,
    LM_FFBETA = 974,
    LM_FFGAMMA = 975,
    LM_FFOMEGA = 976,
    LM_CHALLENGEIT = 977,
    LM_ASKBACK = 978,
    LM_BOTH = 979,
    LM_SHANCAP = 980,
    LM_SCRATE = 981,
    LM_FROUDE = 982,
    LM_BEZIER = 983,
    LM_RCFILT = 984,
    LM_LOGITSEP = 985,
    LM_HOHMANN = 986,
    LM_ECCSEC = 987,
    LM_ECBITS = 988,
    // v53 STAGE 8k (510 DOMAINS! PURE-GPU SPIKING RK4 ODE INTEGRATOR & HYPOTHETICAL COUNTERFACTUAL DIALOGUE!): Kernel 47 on FREE PW24 Q + SupposeIt/TellMore/Other + 7 Stage-8k STEM Domains!
    LM_ODERK4 = 989,
    LM_ODVAL = 990,
    LM_ODEQ = 991,
    LM_ODMODE = 992,
    LM_ODLIVE = 993,
    LM_ODALPHA = 994,
    LM_ODBETA = 995,
    LM_ODGAMMA = 996,
    LM_ODOMEGA = 997,
    LM_SUPPOSEIT = 998,
    LM_TELLMORE = 999,
    LM_OTHER = 1000,
    LM_PVCELL = 1001,
    LM_PVPOW = 1002,
    LM_MMQUEUE = 1003,
    LM_GLIDRAT = 1004,
    LM_PKDOSE = 1005,
    LM_CONVNET = 1006,
    LM_SEISMO = 1007,
    LM_BATPACK = 1008,
    LM_BPWH = 1009,
    // v54 STAGE 8l (520 DOMAINS! PURE-GPU SPIKING SIMPLEX LP OPTIMIZER & MULTI-HOP PROOF DIALOGUE!): Kernel 48 on FREE PW24 Q + ProveIt/RecapIt + 7 Stage-8l STEM Domains!
    LM_SIMLP = 1010,
    LM_SXOPT = 1011,
    LM_SXXVAL = 1012,
    LM_SXBIND = 1013,
    LM_SXLIVE = 1014,
    LM_SXALPHA = 1015,
    LM_SXBETA = 1016,
    LM_SXGAMMA = 1017,
    LM_SXOMEGA = 1018,
    LM_PROVEIT = 1019,
    LM_RECAPIT = 1020,
    LM_CAPBANK = 1021,
    LM_CBCAP = 1022,
    LM_DARCYP = 1023,
    LM_GUSTAF = 1024,
    LM_RANKIN = 1025,
    LM_BDAYBND = 1026,
    LM_BEERLAM = 1027,
    LM_PUTCALL = 1028,
    LM_PCPUT = 1029,
    // v55 STAGE 8m (530 DOMAINS! PURE-GPU SPIKING PCA EIGENDECOMPOSITION & CAUSAL CONTRAST/GENERALIZATION DIALOGUE!): Kernel 49 on FREE PW27 Q + WhyDiff/Generalize + 7 Stage-8m STEM Domains!
    LM_PCAEIG = 1030,
    LM_PCLAM = 1031,
    LM_PCVAR = 1032,
    LM_PCAX = 1033,
    LM_PCLIVE = 1034,
    LM_PCALPHA = 1035,
    LM_PCBETA = 1036,
    LM_PCGAMMA = 1037,
    LM_PCOMEGA = 1038,
    LM_WHYDIFF = 1039,
    LM_GENERALIZE = 1040,
    LM_BLKHOLE = 1041,
    LM_BHRAD = 1042,
    LM_HOOKESP = 1043,
    LM_TCPCWND = 1044,
    LM_HENDBUF = 1045,
    LM_PHONGSH = 1046,
    LM_ISOSTAS = 1047,
    LM_SHARPEF = 1048,
    LM_SHRAT = 1049,
    // v56 STAGE 8n (50-KERNEL MILESTONE & 540 DOMAINS! PURE-GPU SPIKING 1D HEAT/DIFFUSION PDE SOLVER & CROSS-CATEGORY DEDUCTION!): Kernel 50 on FREE PW27 Q + DeducePair/CommonRoot + 7 Stage-8n STEM Domains!
    LM_PDESLV = 1050,
    LM_PDMID = 1051,
    LM_PDSIDE = 1052,
    LM_PDCFL = 1053,
    LM_PDLIVE = 1054,
    LM_PDALPHA = 1055,
    LM_PDBETA = 1056,
    LM_PDGAMMA = 1057,
    LM_PDOMEGA = 1058,
    LM_DEDUCEPAIR = 1059,
    LM_COMMONROOT = 1060,
    LM_DEBROG = 1061,
    LM_DBLAM = 1062,
    LM_TAILSLA = 1063,
    LM_IONTHR = 1064,
    LM_HILLBND = 1065,
    LM_BEAMST = 1066,
    LM_ARITHCD = 1067,
    LM_TAYLRL = 1068,
    LM_TRRATE = 1069,
    // v57 STAGE 8o (51 BLACKWELL KERNELS & 550 DOMAINS! PURE-GPU 256-CHAIN SPIKING MCMC POSTERIOR SAMPLER & 3-ENTITY CLUSTERING DIALOGUE!): Kernel 51 on FREE PW27 Q + GroupThree/WhatElse + 7 Stage-8o STEM Domains!
    LM_MCMCSMP = 1070,
    LM_MCPROB = 1071,
    LM_MCACC = 1072,
    LM_MCMODE = 1073,
    LM_MCLIVE = 1074,
    LM_MCALPHA = 1075,
    LM_MCBETA = 1076,
    LM_MCGAMMA = 1077,
    LM_MCOMEGA = 1078,
    LM_GROUPTHREE = 1079,
    LM_WHATELSE = 1080,
    LM_FERMIDR = 1081,
    LM_FDOCC = 1082,
    LM_BERNFLW = 1083,
    LM_SHAMIRS = 1084,
    LM_MEMVOLT = 1085,
    LM_INVKINE = 1086,
    LM_LOTKAVOL = 1087,
    LM_ROOFLN = 1088,
    LM_RFPERF = 1089,
    // v58 STAGE 8p (52 BLACKWELL KERNELS & 560 DOMAINS! PURE-GPU 4,096-ROLLOUT UCB1 MCTS PLANNER & 100% NATURAL ZERO-KEYWORD MULTI-TURN HUMAN DIALOGUE!): Kernel 52 on FREE PW27 Q + LM_SHARE + 7 Stage-8p STEM Domains!
    LM_MCTSPLN = 1090,
    LM_MTVAL = 1091,
    LM_MTVIS = 1092,
    LM_MTPOL = 1093,
    LM_MTLIVE = 1094,
    LM_MTALPHA = 1095,
    LM_MTBETA = 1096,
    LM_MTGAMMA = 1097,
    LM_MTOMEGA = 1098,
    LM_SHARE = 1099,
    LM_GROVERQ = 1100,
    LM_GRCOST = 1101,
    LM_PCMRATE = 1102,
    LM_STEBOLTZ = 1103,
    LM_PBFTSYS = 1104,
    LM_YOUNGMD = 1105,
    LM_VISVIVA = 1106,
    LM_DAMPCTL = 1107,
    LM_DCSETT = 1108,
    LM_COUNT = 1109
};

// Learned Wernicke-Broca Compositional Construction (with surface articulation!)
struct Construction {
    Word active = 0;
    Word intent = INT_NONE;
    Word length = 0;
    SlotType slot[MAX_PAT_TOKENS]{};
    Word fixed_concept[MAX_PAT_TOKENS]{};
    char fixed_surface[MAX_PAT_TOKENS][MAX_WORD_CHARS]{}; // Preserves learned surface form ("me" vs "i", "in" vs "inside")!
    Word modifiers = 0;
    Word support = 0;
    Word is_production = 0;
};

struct DiscourseWM {
    Word last_entity = 0;
    Word last_agent = 0;
    Word last_agent2 = 0;
    Word last_location = 0;
    Word last_relation = 0;
    Word last_action = 0;
    Word last_engram = ~0u;
    Word last_answer_concept = 0;
    Word last_answer_engram = ~0u;
    Word last_plan_target = 0;
    Word last_plan_goal = 0;
    PlanResult last_plan{};
    Word last_confidence = 0;
    Word last_provenance = 0;
    Word last_source_agent = 0;
    Word last_parent_a = 0;
    Word last_parent_b = 0;
    Word last_question_intent = INT_NONE;
    Word last_question_rel = 0;
    Word last_question_obj = 0;
    Word last_question_action = 0;
    Word last_question_prop = 0;
    Word last_question_persp = 0;
    Word last_explain_kind = 0; // 1=exception, 2=tollens, 3=mp, 4=via_cat, 5=disjunct, 6=direct_neg, 7=counterexample
    Word last_explain_subj = 0;
    Word last_explain_obj = 0;
    Word last_explain_cat = 0;
    Word last_explain_act = 0;
    Word turn_count = 0;
    std::vector<Word> agent_absent;
    std::vector<Word> who_saw_leave;
    std::vector<Word> curiosity_concepts; // Unknown concepts the mind wants to ask about!
    std::vector<Word> recent_learned_engrams;
    // v5: Epistemic vigilance trust ledger, CSP exclusivity groups, and inverse-math delta traces
    std::map<Word, int> trust_good;
    std::map<Word, int> trust_bad;
    struct CspGroup {
        std::vector<Word> items;
        std::vector<Word> places;
    };
    std::vector<CspGroup> csp_groups;
    std::map<std::pair<Word, Word>, int> qty_deltas;
    // v6: Prospective memory triggers, 2-variable linear systems, and unit pricing
    struct Reminder {
        Word trigger_subj = 0;
        Word trigger_prop = 0;
        Word about_obj = 0;
        bool fired = false;
    };
    std::vector<Reminder> reminders;
    struct PairSys {
        Word a = 0;
        Word b = 0;
        Word item = 0;
        int total = QTY_NONE;
        int diff = QTY_NONE; // a - b
    };
    std::vector<PairSys> linear_systems;
    std::map<Word, int> unit_prices;
    std::map<Word, Word> unit_price_currency;
    // v7: Conjunctive AND-gate rules (if s1 is p1 AND s2 is p2 -> es is ep)
    struct ConjRule {
        Word s1 = 0, p1 = 0, s2 = 0, p2 = 0, es = 0, ep = 0;
    };
    std::vector<ConjRule> conj_rules;
    // v9: Biconditional iff rules, moral intentionality blame, social promises & deontic norms
    struct IffRule {
        Word s1 = 0, p1 = 0, s2 = 0, p2 = 0;
    };
    std::vector<IffRule> iff_rules;
    std::map<Word, int> moral_blame;
    struct PromiseRec {
        Word agent = 0, obj = 0;
        int state = 0; // 0=pending, +1=kept, -1=broken
    };
    std::vector<PromiseRec> promises;
    struct DeonticNorm {
        Word act = 0, obj = 0, req_tool = 0, loc = 0;
        bool is_obligation = false;
    };
    std::vector<DeonticNorm> norms;
    // v10: Scientific hypothesis tests, belief-revision ledger, structural metaphors & affective states
    struct HypTest {
        Word hyp_prop = 0, reagent = 0, result_prop = 0;
    };
    std::vector<HypTest> hyp_tests;
    struct BeliefChange {
        Word subj = 0, eliminated = 0, deduced = 0, reagent = 0, obs_prop = 0;
    };
    std::vector<BeliefChange> belief_changes;
    struct MetaphorMap {
        Word a = 0, b = 0, c = 0, d = 0;
    };
    std::vector<MetaphorMap> metaphors;
    struct AffectRec {
        Word agent = 0, emotion = 0, event_act = 0, obj = 0;
    };
    std::vector<AffectRec> affects;
    // v11: Algebraic recall, Socratic meta-justification, GPU WM ambiguity clarification, narrative, knapsack, deps, schema xfer, 2D grid, common knowledge & causal history
    int last_solved_num = QTY_NONE;
    std::string last_just_reply;
    bool pending_clarify = false;
    Word clarify_intent = INT_NONE;
    Word clarify_cat = 0;
    Word clarify_rel = 0;
    Word clarify_obj = 0;
    Word clarify_action = 0;
    Word clarify_cand1 = 0;
    Word clarify_cand2 = 0;
    std::vector<Word> wm_focus_entities;
    struct StoryEv {
        Word agent = 0, action = 0, obj = 0, loc = 0, reason_agent = 0, reason_act = 0, reason_obj = 0;
    };
    std::vector<StoryEv> story_events;
    int pack_limit = QTY_NONE;
    struct PackItem { Word item = 0; int weight = 0; int value = 0; };
    std::vector<PackItem> pack_items;
    std::map<Word, std::vector<Word>> build_deps;
    std::map<Word, Word> schema_corr_fwd;
    std::map<Word, Word> schema_corr_rev;
    struct DomainCtrl { Word domain = 0, controller = 0, target = 0; };
    std::vector<DomainCtrl> domain_ctrls;
    std::map<Word, std::pair<int, int>> grid_coords;
    struct EpistFact { Word subj = 0, loc = 0, knower = 0; bool is_public = false; };
    std::vector<EpistFact> epist_facts;
    struct MuddyPuzzle { Word a1 = 0, a2 = 0, observer = 0, clean_seen = 0; bool active = false; };
    MuddyPuzzle muddy{};
    std::map<Word, std::vector<Word>> hist_causes;
    // v12: XOR gates, Knight/Knave puzzles, dynamic graphs & spontaneous riddle generation!
    struct XorGate { Word gate = 0, a = 0, b = 0; };
    std::vector<XorGate> xor_gates;
    struct KnightClaim { Word speaker = 0, target = 0, claimed_role = 0; };
    std::vector<KnightClaim> knight_claims;
    std::map<Word, Word> knight_roles; // agent -> LM_KNIGHT or LM_KNAVE
    std::map<Word, std::set<Word>> graph_adj;
    Word active_riddle_ans = 0;
    // v13: Live 1-shot syntax/idioms, 2x2 linear systems, modal claims, dynamic obstacle replanning & 3rd-order belief updates!
    std::map<std::string, std::vector<std::string>> learned_idioms;
    struct Lin2Eq { int a = 1, b = 1, c = 0; };
    std::vector<Lin2Eq> lin2_eqs;
    struct ModalClaim { Word subj = 0, prop = 0, mode = 0; bool falsified = false; };
    std::vector<ModalClaim> modal_claims;
    struct AltMethod { Word tool = 0, action = 0, target = 0; bool broken = false; };
    std::vector<AltMethod> alt_methods;
    std::map<std::pair<Word, Word>, Word> peer_belief_override; // (agent, subj) -> loc
    std::map<std::pair<Word, std::pair<Word, Word>>, Word> nested_belief_override; // (obs, (agent, subj)) -> loc
    struct EcoState { Word agent = 0; int energy = 10, berry = 0, ore = 0; Word last_action = 0; };
    std::vector<EcoState> eco_states;
    // v14: Personal user modeling, formal sets, river-crossing conflicts, interval bounds, joint blame & few-shot program synthesis!
    Word user_fav_animal = 0;
    Word user_home = 0;
    std::map<Word, std::vector<Word>> named_sets;
    std::vector<std::pair<Word, Word>> river_conflicts;
    struct ScoreBound { int lo = -1000000; int hi = 1000000; bool has_lo = false; bool has_hi = false; };
    std::map<Word, ScoreBound> score_bounds;
    struct JointAct { Word agent = 0, action = 0, obj = 0; bool intentional = false; };
    std::vector<JointAct> joint_acts;
    std::vector<std::pair<int, int>> func_examples;
    // v15: Open reading situation model, active info-gain inquiry, Aristotelian syllogisms, Pareto trade, 3-var algebra, Pearl DAG & culture!
    struct SitRec { Word agent = 0, visited = 0, item = 0, container = 0, clim_obj = 0, cause_s = 0, cause_loc = 0; };
    std::map<Word, SitRec> situations;
    std::map<Word, std::set<Word>> info_cands;
    std::set<Word> active_cands;
    std::map<Word, std::vector<Word>> syll_all;
    std::vector<std::pair<Word, Word>> syll_no;
    std::map<std::pair<Word, Word>, int> agent_values;
    struct TriSum { Word a = 0, b = 0; int s = 0; };
    std::vector<TriSum> tri_sums;
    std::map<Word, std::vector<Word>> causal_dag;
    struct CoinedWord { Word word = 0, prop = 0, cat = 0, act = 0; };
    std::map<Word, CoinedWord> coined_words;
    // v16: Open-vocabulary verb graph, proactive curiosity habitats, task deadlines, CNF SAT, world analogy, risk & belief revision!
    struct OpenTriple { Word subj = 0, verb = 0, obj = 0; bool negated = false; };
    std::vector<OpenTriple> open_triples;
    std::map<Word, Word> entity_habitat;
    struct TaskDl { Word task = 0; int hours = 0, deadline = 0; };
    std::map<Word, TaskDl> task_dls;
    struct SatLit { Word var = 0; bool neg = false; };
    std::vector<std::vector<SatLit>> sat_clauses;
    struct WorldRule { Word world = 0, act = 0, src = 0, dst = 0; };
    std::vector<WorldRule> world_rules;
    std::map<Word, Word> world_corr_fwd;
    std::map<Word, Word> world_corr_rev;
    std::map<Word, int> risk_pcts;
    struct BeliefRev { Word subj = 0, old_loc = 0, new_loc = 0; };
    std::map<Word, BeliefRev> belief_revs;
    // v17: Multi-sentence pronoun coreference, 4-arg novel verb frames, stochastic EV planning, budget knapsack,
    //      quantified exception norms, temporal meeting overlap, causal fault pipeline & dialectical pro/con debate!
    struct CorefObjState { Word discoverer = 0, origin = 0, carried_to = 0, carrier = 0, giver = 0, holder = 0; };
    std::map<Word, CorefObjState> coref_objs;
    Word coref_last_agent = 0;
    Word coref_last_obj = 0;
    struct Frame4 { Word subj = 0, verb = 0, obj = 0, prep = 0, arg2 = 0; };
    std::vector<Frame4> open_frames4;
    struct StochRoute { Word name = 0; int prob_pct = 0; int reward = 0; };
    std::map<Word, StochRoute> stoch_routes;
    int budget_cap = 0;
    struct BudgetOpt { Word name = 0; int cost = 0; int utility = 0; };
    std::map<Word, BudgetOpt> budget_opts;
    struct ExceptNorm { Word cat = 0, exc_agent = 0, target = 0; };
    std::vector<ExceptNorm> except_norms;
    struct FreeSlot { int start_h = 0, end_h = 0; };
    std::map<Word, FreeSlot> free_windows;
    std::map<Word, std::vector<Word>> pipe_edges;
    std::map<Word, bool> pipe_ok;
    std::vector<Word> pipe_order;
    struct ProCon { int pro_val = 0, con_val = 0; };
    std::map<Word, ProCon> debate_opts;
    // v18: Embedded relative clauses, coalition Shapley, seq-op replay, recursive BOM, consensus voting, event-calc fluents & meta-rules!
    struct EmbedRec { Word agent = 0, obj = 0, prop = 0, loc = 0; };
    std::vector<EmbedRec> embed_recs;
    std::map<Word, int> coal_alone;
    std::map<std::pair<Word, Word>, int> coal_together;
    int seqop_start = 0;
    struct SeqOp { Word kind = 0; int val = 0; }; // 1=double, 2=add
    std::vector<SeqOp> seq_ops;
    std::map<Word, std::vector<std::pair<Word, int>>> bom_recipes; // item -> [(ing, count)]
    struct WitnessRep { Word agent = 0, subj = 0, prop = 0; };
    std::vector<WitnessRep> witness_reps;
    struct FluentEv { int tick = 0; Word subj = 0, prop = 0; };
    std::vector<FluentEv> fluent_evs;
    Word meta_prop = 0, meta_action = 0;
    std::set<Word> meta_exceptions;
    // v19: Multi-turn chat context, Bellman MDP, causal mediation, max-flow, Vickrey auction, 2D vectors, PERT/CPM & duty priority!
    Word chat_spot_obj = 0, chat_spot_loc = 0, chat_hunt_prey = 0;
    struct MdpEdge { Word action = 0; int reward = 0; Word next_state = 0; };
    std::map<Word, std::vector<MdpEdge>> mdp_edges;
    std::map<Word, std::vector<Word>> med_edges;
    std::set<std::pair<Word, Word>> med_direct;
    std::map<std::pair<Word, Word>, int> flow_caps;
    std::map<Word, std::vector<std::pair<Word, int>>> auction_bids;
    std::map<Word, std::pair<int, int>> vec_points;
    struct PertJob { int dur = 0; std::vector<Word> deps; };
    std::map<Word, PertJob> pert_jobs;
    std::map<Word, int> duty_prios;
    // v20: Narrative story arc, Walrasian market clearing, 2D Newtonian physics, Socratic assistant, 2D affine, Nash 2x2 & causal circuits!
    struct ChapterArc { Word goal = 0, obstacle = 0, tool = 0, cross_act = 0; bool achieved = false; };
    std::map<Word, ChapterArc> chapter_arcs;
    std::map<Word, std::vector<int>> market_asks;
    std::map<Word, std::vector<int>> market_bids;
    std::map<Word, std::pair<int, int>> phys_movers; // name -> (mass, velocity)
    Word assist_goal = 0;
    std::map<Word, std::vector<Word>> assist_reqs;
    std::set<Word> assist_have;
    int affine_scale = 1, affine_dx = 0, affine_dy = 0;
    bool affine_ready = false;
    std::map<std::pair<Word, Word>, std::pair<int, int>> payoff_mat;
    std::vector<Word> payoff_acts;
    std::map<Word, std::pair<Word, Word>> circuit_and;
    std::map<Word, std::pair<Word, Word>> circuit_or;
    std::map<Word, Word> circuit_state;
    // v21: Reflective user opinions, scientific linear law trials, capital projects, kinematics, MCDA, Kruskal MST, LTL trace & seat apportionment!
    Word op_pref_a = 0, op_pref_b = 0, op_pref_reason = 0, op_enjoy_act = 0, op_enjoy_loc = 0;
    std::vector<std::pair<int, int>> law_trials;
    std::map<Word, std::pair<int, int>> fin_projects;
    int kin_v0 = 0, kin_a = 0, kin_t = 0;
    bool kin_ready = false;
    std::map<Word, int> mcda_weights;
    std::map<Word, std::map<Word, int>> mcda_scores;
    std::map<std::pair<Word, Word>, int> mst_edges;
    std::map<int, Word> ltl_trace;
    int apport_seats = 0;
    std::map<Word, int> apport_votes;
    // v22 Discourse WM: Open-topic notes, Dijkstra road graph, Sequential Bayes, Expr tree, Elastic bodies, 0/1 Cargo DP, Coalition Core & Scientific Experiment Design!
    struct TopicFact { Word subj = 0; Word rel = 0; Word obj = 0; };
    std::map<Word, std::vector<TopicFact>> topic_notes;
    std::map<std::pair<Word, Word>, int> road_edges;
    Word seqbayes_hyp = 0;
    int seqbayes_prior = 0;
    std::vector<std::pair<int, int>> seqbayes_sensors;
    struct ExprNode { bool is_lit = true; int lit = 0; Word lhs = 0; Word op = 0; Word rhs = 0; };
    std::map<Word, ExprNode> expr_vars;
    std::map<Word, std::pair<int, int>> elastic_bodies; // body -> (mass, speed)
    int cargo_cap = 0;
    struct CargoItem { Word name = 0; int weight = 0; int value = 0; };
    std::vector<CargoItem> cargo_items;
    std::map<Word, int> core_solo;
    std::map<std::pair<Word, Word>, int> core_pair;
    int core_grand = 0;
    std::map<Word, int> core_split;
    struct ExpTheory { Word obs1 = 0; Word val1 = 0; Word obs2 = 0; Word val2 = 0; };
    std::map<Word, ExpTheory> exp_theories;
    std::vector<std::pair<Word, Word>> exp_observed;
    // v23 Discourse WM: Ecological stock dynamics, ballistic projectiles, 2x2 matrices, stats datasets, weighted gig DP, ranked ballots & Pearl DAGs!
    struct StockDyn { int init = 0; int regrow = 0; int draw = 0; };
    std::map<Word, StockDyn> stock_dyns;
    struct ProjRec { int vx = 0; int vy = 0; int g = 10; };
    std::map<Word, ProjRec> projectiles;
    struct Mat2Rec { int a = 0; int b = 0; int c = 0; int d = 0; };
    std::map<Word, Mat2Rec> mat2s;
    std::map<Word, std::vector<int>> stat_sets;
    struct GigRec { Word name = 0; int start = 0; int end = 0; int wage = 0; };
    std::vector<GigRec> gigs;
    std::map<Word, std::array<Word, 3>> ballots;
    std::vector<std::pair<Word, Word>> pearl_edges;
    // v24 Discourse WM: Markov chains, 2D LP, recursive tree DAG, harmonic waves, Ohm circuits, Mendelian genetics & asymptotic algorithms!
    struct MarkovEdge { int stay_pct = 0; Word other = 0; };
    std::map<Word, MarkovEdge> markov_states;
    int lp_cx = 0, lp_cy = 0;
    struct LpCon { int ax = 0, ay = 0, rhs = 0; };
    std::map<Word, LpCon> lp_cons;
    std::vector<std::pair<Word, Word>> tree_edges; // (parent, child)
    struct WaveRec { int freq = 0; int lambda = 0; };
    std::map<Word, WaveRec> waves;
    std::map<Word, int> resistors;
    int battery_volt = 0;
    Word gene_dom = 0, gene_rec = 0;
    std::map<Word, Word> parent_geno; // parent -> concept ("het"/"rec"/"dom")
    std::map<Word, Word> algo_class;  // algo -> class concept
    // v25 Discourse WM: Spatial-action prose, epidemic R0, Archimedes buoyancy, portfolio assets, topo DAG, binary search, Doppler & iterated PD!
    struct ProseLocEvent { Word agent = 0; Word action = 0; Word obj = 0; Word loc = 0; };
    std::vector<ProseLocEvent> prose_loc_events;
    struct EpidemicRec { int r0 = 0; int cases = 0; };
    std::map<Word, EpidemicRec> epidemics;
    std::map<Word, int> fluid_density;
    struct SlabRec { int mass = 0; int vol = 1; };
    std::map<Word, SlabRec> slabs;
    struct AssetRec { int ret = 0; int sd = 1; };
    std::map<Word, AssetRec> assets;
    std::vector<std::pair<Word, Word>> dep_edges;
    int sorted_array_size = 0;
    int siren_emit = 0;
    int siren_airspeed = 300;
    int pd_r = 0, pd_t = 0, pd_p = 0, pd_delta = 0;
    // v26 Discourse WM: Chemical stoichiometry, thin-lens optics, Shannon entropy, DP LCS/edit, queueing, Diffie-Hellman & envy-free cake division!
    Word chem_r1 = 0, chem_r2 = 0, chem_prod = 0;
    int chem_c1 = 0, chem_c2 = 0, chem_cp = 0;
    int chem_s1 = 0, chem_s2 = 0;
    int lens_focal = 0;
    struct EntropyDist { int n_half = 0, n_quarter = 0, n_eighth = 0; };
    std::map<Word, EntropyDist> entropy_dists;
    std::map<Word, std::vector<Word>> sym_seqs;
    struct QueueRec { int arrivals = 0, service = 1, wait = 0; };
    std::map<Word, QueueRec> queues;
    int dh_prime = 0, dh_gen = 0;
    std::map<Word, int> dh_secrets;
    std::map<Word, std::pair<int, int>> cake_vals; // agent -> (val_slice1, val_slice2)
    Word cake_a1 = 0, cake_a2 = 0; // agent holding slice1, agent holding slice2
    // v27 Discourse WM: Layer 3d causal-because prose, 2D continuous movers, ideal gas, CRT, bipartite graph, bonds, Nyquist & commons!
    struct ProseWhyLink { Word e_s = 0, e_v = 0, e_o = 0, c_s = 0, c_v = 0, c_o = 0; };
    std::vector<ProseWhyLink> prose_why_links;
    struct Mover2D { int x = 0, y = 0, vx = 0, vy = 0; };
    std::map<Word, Mover2D> movers2d;
    struct GasRec { int p = 0, v = 1, t = 1; };
    std::map<Word, GasRec> gas_chambers;
    int crt_m1 = 0, crt_r1 = 0, crt_m2 = 0, crt_r2 = 0;
    std::vector<std::pair<Word, Word>> bipart_edges;
    struct BondRec { int principal = 0, rate = 0; };
    std::map<Word, BondRec> bonds;
    std::map<Word, int> signal_bands;
    struct CommonsRec { int benefit = 0, cost = 0, damage = 0; };
    std::map<Word, CommonsRec> commons_res;
    // v28 Discourse WM: Layer 3e concessive prose, Kepler orbits, 2x2 eigenvalues, Bayes odds, shoelace polygon, DFA, Keynesian MPC & Hamming codes!
    std::vector<ProseWhyLink> prose_although_links;
    struct OrbitRec { int radius = 1, period = 1; };
    std::map<Word, OrbitRec> orbits;
    int mat2d_a = 0, mat2d_b = 0, mat2d_c = 0, mat2d_d = 0;
    bool mat2d_set = false;
    long long odds_num = 0, odds_den = 0;
    std::vector<std::pair<int, int>> poly2d_pts;
    Word dfa_start = 0;
    std::set<Word> dfa_accept;
    std::map<std::pair<Word, Word>, Word> dfa_trans; // (state, symbol) -> next_state
    int econ_mpc = 0;
    std::map<Word, std::vector<int>> codewords;
    // v29 Discourse WM: Layer 3f temporal before/after prose + Socratic cursor, Lorentz, capacitors/RC, Euler, F1, control, logistic & Huffman!
    struct ProseTempLink { Word main_s = 0, main_v = 0, main_o = 0, sub_s = 0, sub_v = 0, sub_o = 0; bool is_before = true; };
    std::vector<ProseTempLink> prose_temp_links;
    struct ProseChrono { Word s = 0, v = 0, o = 0, loc = 0; bool loc_from = false; };
    std::vector<ProseChrono> prose_chrono;
    std::set<size_t> prose_mentioned;
    std::map<Word, int> relativ_pct;
    std::map<Word, int> capacitors;
    std::vector<std::pair<Word, Word>> euler_edges;
    struct ConfusRec { int tp = 0, fp = 0, fn = 0; };
    std::map<Word, ConfusRec> confus_mats;
    struct CtrlRec { int setpoint = 0, state = 0, kp_pct = 0; };
    std::map<Word, CtrlRec> controllers;
    struct LogistRec { int K = 1, r_pct = 0, pop = 0; };
    std::map<Word, LogistRec> logistics;
    std::map<Word, int> huff_freqs;
    // v30 Stage 4 Discourse WM: Layer 3g multi-agent/counterfactual prose, photoelectric, pH, RSA, A*, Nash mixed, PID & Bernoulli!
    struct ProseTold { Word speaker = 0, listener = 0, s = 0, v = 0, o = 0; };
    std::vector<ProseTold> prose_told;
    struct ProseCond { Word cond_s = 0, cond_v = 0, res_s = 0, res_v = 0; };
    std::vector<ProseCond> prose_cond;
    std::map<Word, Word> story_blockers; // entity -> blocking reason property/word (for Socratic `why not?`)
    struct PhotoRec { int h = 1, phi = 0; };
    std::map<Word, PhotoRec> photo_metals;
    std::map<Word, int> ph_solns;
    int rsa_n = 0, rsa_phi = 0, rsa_e = 0, rsa_d = 0;
    struct AstarEdge { Word u = 0, v = 0; int cost = 0; };
    std::vector<AstarEdge> astar_edges;
    std::map<Word, int> astar_heur;
    int mix_m[2][2]{{0, 0}, {0, 0}};
    bool mix_set = false;
    struct PidRec { int kp = 0, ki = 0, kd = 0, err = 0, prev = 0, sum = 0; };
    std::map<Word, PidRec> pid_ctrls;
    struct VenturiRec { int a1 = 1, v1 = 0, rho = 2; };
    std::map<Word, VenturiRec> venturis;
    // v31 Stage 4 Discourse WM: Layer 3h belief-revision history & ambiguous mentions, calculus, half-life, Hardy-Weinberg, RPN stack, OLS regression, coin-change DP & Kirchhoff!
    struct ProseBeliefRev { Word entity = 0, old_val = 0, new_val = 0, agent = 0, verb = 0; };
    std::vector<ProseBeliefRev> prose_belief_revs;
    struct PolyRec { int coeff = 0, power = 0; };
    std::map<Word, PolyRec> polys;
    struct IsoRec { int mass = 0, hl = 1; };
    std::map<Word, IsoRec> isotopes;
    struct GeneRec { int dom_pct = 50, pop = 100; };
    std::map<Word, GeneRec> genes;
    std::vector<int> rpn_stack;
    int rpn_peak = 0;
    std::vector<std::pair<int, int>> regress_pts;
    std::vector<int> coin_denoms;
    int kcl_in = 0, kcl_out = 0;
    int kvl_src = 0, kvl_drop_sum = 0;
    // v32 300-Domain Milestone Discourse WM: Live unseen-word open lexicon, Carnot, Shannon-Hartley, 3D vectors, demand elasticity & CPU cache!
    std::map<Word, Word> openlex_defs;   // unseen_word -> meaning/property
    std::map<Word, Word> openlex_links;  // unseen_word -> carried/fed target (multi-hop open-lexicon chain!)
    struct CarnotRec { int thot = 600, tcold = 300; };
    std::map<Word, CarnotRec> carnots;
    struct ChannelRec { int bw = 1, snr = 1; };
    std::map<Word, ChannelRec> channels;
    struct Vec3Rec { int x = 0, y = 0, z = 0; };
    std::map<Word, Vec3Rec> vec3s;
    struct DemandRec { int q1 = 100, p1 = 10, q2 = 80, p2 = 12; };
    std::map<Word, DemandRec> demands;
    struct CacheRec { int hitpct = 90, hitns = 1, misspen = 10; };
    std::map<Word, CacheRec> caches;
    // v33 Stage 5 Crossing Discourse WM: Compositional K27 story episodes, hydrostatics, blackbody radiation, 2x2 matrices, TCP links, compound interest & Kruskal MST!
    struct SeqEpRec { Word ag = 0, act = 0, obj = 0, loc = 0; };
    std::vector<SeqEpRec> seq_episodes;
    struct HydroRec { int rho = 1000, depth = 1; };
    std::map<Word, HydroRec> hydros;
    std::map<Word, int> radstars; // star -> baseline power
    struct Mat2V33Rec { int a = 0, b = 0, c = 0, d = 0; };
    std::map<Word, Mat2V33Rec> mat2_v33s;
    struct NetLinkRec { int mbps = 100, rtt = 20; };
    std::map<Word, NetLinkRec> netlinks;
    struct CompoundRec { int prin = 1000, rate = 10; };
    std::map<Word, CompoundRec> compounds;
    struct MstEdge { Word u = 0, v = 0; int w = 0; };
    std::map<Word, std::vector<MstEdge>> mst_graphs;
    // v34 Stage 5b Discourse WM: Coulomb pairs, organ pipes, Amdahl jobs, virtual memory tables, planetary bodies & 2D points!
    std::map<Word, int> coulomb_forces;
    struct PipeRec { int speed = 320, len = 2; };
    std::map<Word, PipeRec> organ_pipes;
    std::map<Word, int> amdahl_jobs;   // job -> parallel fraction percent (0..100)
    std::map<Word, int> vmem_pages;    // vm -> page size bytes
    std::map<Word, int> planet_vesc;   // planet -> escape velocity
    struct Pt2DRec { int x = 0, y = 0; };
    std::map<Word, Pt2DRec> pts2d;
    // v35 Stage 5c Discourse WM: active K27 discourse agent (for free pronoun coref!), plus isobaric/Faraday/Bode/CPU/quantum records!
    Word last_seq_agent = 0;
    struct IsobarRec { int pressk = 50, dvol = 4; };
    std::map<Word, IsobarRec> isobars;
    struct FaradayRec { int nturns = 50, dflux = 12, dtime = 3; };
    std::map<Word, FaradayRec> faradays;
    struct BodeRec { int mass = 2, stiff = 32, damp = 16; };
    std::map<Word, BodeRec> bodes;
    struct CpuSchedRec { int b1 = 9, b2 = 3, b3 = 6; };
    std::map<Word, CpuSchedRec> cpuscheds;
    struct DeBroglieRec { int planck = 600, mom = 15; };
    std::map<Word, DeBroglieRec> debroglies;
    // v36 Stage 5d Discourse WM: K27 unseen causal story links, K30 live clause slot, Reynolds/3x3-matrix/kinetics/beam records!
    std::vector<std::pair<Word, Word>> seq_causal_links; // (cause_ep_idx, effect_ep_idx)
    Word sat_next_clause = 4u;
    struct ReynoldsRec { int dens = 1000, vel = 2, diam = 4, visc = 2; };
    std::map<Word, ReynoldsRec> reynolds;
    struct Mat3Rec { int a = 2, b = 3, c = 5; };
    std::map<Word, Mat3Rec> mat3s;
    struct KineticsRec { int krate = 4, conc = 25; };
    std::map<Word, KineticsRec> kinetics;
    struct BeamRec { int loadf = 40, beamlen = 8, capm = 160; };
    std::map<Word, BeamRec> beams;
    // v37 Stage 6 Crossing Discourse WM: Self-teaching curiosity study cards, imperative code programs & 6 Stage-6 STEM records!
    struct StudyRec { Word kind = 0, feat = 0, diet = 0, habitat = 0; };
    std::map<Word, StudyRec> studies;
    struct CodeInst { Word op = 0; int arg = 0; int arg2 = 0; }; // 1=init, 2=add, 3=mul, 4=sub, 5=codeloop(iters,delta), 6=codeifgt(thresh,delta)
    std::map<Word, std::vector<CodeInst>> code_progs;
    std::map<Word, int> chords;
    struct RotorRec { int r = 4, f = 15, inertia = 10, omega = 6; };
    std::map<Word, RotorRec> rotors;
    struct CompilerRec { int leaves = 8, ops = 7, live = 5, regs = 4; };
    std::map<Word, CompilerRec> compilers;
    struct DnaRec { int gc = 12, len = 30, gluc = 3; };
    std::map<Word, DnaRec> dnaseqs;
    struct CipherRec { int plain = 42, key = 19, shift = 5; };
    std::map<Word, CipherRec> ciphers;
    struct RobotRec { int l1 = 10, l2 = 6, wcirc = 12, wturns = 5; };
    std::map<Word, RobotRec> robots;
    // v38 Stage 6b Discourse WM: Autonomous textbook causal laws, prosody-aware empathy & 5 Stage-6b STEM records!
    struct TextbookRec { Word target = 0; int factor = 1; Word blocker = 0; };
    std::map<Word, TextbookRec> textbooks;
    struct EmpathyRec { Word prosody = 0, mood = 0; };
    std::map<Word, EmpathyRec> empathies;
    struct Solid3dRec { int basea = 12, h = 9, v = 8, e = 12; };
    std::map<Word, Solid3dRec> solids;
    struct GraphcRec { int n = 6, e = 10; };
    std::map<Word, GraphcRec> graphcs;
    struct DspRec { int x0 = 2, x1 = 4, x2 = 6, k0 = 3, k1 = 2, k2 = 1; };
    std::map<Word, DspRec> dsps;
    struct OrgChemRec { int c = 4, mol = 6, lit = 2; };
    std::map<Word, OrgChemRec> orgchems;
    struct DbQueryRec { int m = 20, n = 10; };
    std::map<Word, DbQueryRec> dbqueries;
    // v39 Stage 6c Discourse WM: Socratic user-goal coaching, 3D compound contraptions, tradeoff deliberation & 5 Stage-6c STEM records!
    struct CoachRec { Word goal = 0, blocker = 0; bool resolved = false; };
    CoachRec coach_state{};
    struct ContraptionRec { int gin = 10, gout = 30, la = 6, lb = 2, pul = 4; };
    std::map<Word, ContraptionRec> contraptions;
    struct DeliberateRec { Word pro = 0, con = 0; int merit = 8, hazard = 5; };
    std::map<Word, DeliberateRec> deliberations;
    struct AstroRec { int gm = 400, r = 10, dist = 600, c = 30; };
    std::map<Word, AstroRec> astros;
    struct StochdRec { int n = 100, p = 25; };
    std::map<Word, StochdRec> stochds;
    struct ElecGridRec { int v = 120, r = 10, ip = 5, np = 400, ns = 100; };
    std::map<Word, ElecGridRec> elecgrids;
    std::map<Word, int> distsys;
    struct InvOptRec { int p = 50, c = 30, fix = 400; };
    std::map<Word, InvOptRec> invopts;
    // v40 STAGE 7 CROSSING Discourse WM: Open-topic continuation, pun wit, user persona & 5 Stage-7a STEM/ethics records!
    Word last_topic_concept = 0;
    struct PunWitRec { Word gloss1 = 0, gloss2 = 0; std::map<Word, Word> ctx_map; };
    std::map<Word, PunWitRec> punwits;
    struct PersonaRec { Word fond = 0, shun = 0; };
    std::map<Word, PersonaRec> personas;
    struct StatTestRec { int samp = 56, nullm = 50, se = 2; };
    std::map<Word, StatTestRec> stattests;
    struct ThermocRec { int thot = 500, tcold = 200, qin = 1000; };
    std::map<Word, ThermocRec> thermocs;
    struct MasterRec { int a = 4, b = 2, d = 1; };
    std::map<Word, MasterRec> masterrecs;
    struct ImmunolRec { int r0 = 4, vax = 80; };
    std::map<Word, ImmunolRec> immunols;
    struct EthicfRec { int saved = 10, harmed = 1, duty = 0; };
    std::map<Word, EthicfRec> ethicfs;
    // v41 STAGE 7b Discourse WM: Autonomous scientific research loop, compositional story/moral & 5 Stage-7b STEM records!
    struct SciLoopRec { int p_fast = 80, p_slow = 40, obs = 82; };
    std::map<Word, SciLoopRec> sciloops;
    struct TaleBoxRec { Word hero = 0, foe = 0, tool = 0, moral = 0; };
    std::map<Word, TaleBoxRec> taleboxes;
    struct LinProgRec { int cap = 20, gain = 5, cost = 4; };
    std::map<Word, LinProgRec> linprogs;
    struct SpectralRec { int f0 = 50, fs = 180; };
    std::map<Word, SpectralRec> spectrals;
    struct AlleleHwRec { int dom = 60, fitd = 90, fitr = 70; };
    std::map<Word, AlleleHwRec> allelehws;
    struct CryptoDlRec { int p = 23, g = 5, a = 6, b = 15; };
    std::map<Word, CryptoDlRec> cryptodls;
    struct TrophicRec { int plant = 1000, apex = 0; };
    std::map<Word, TrophicRec> trophics;
    // v42 STAGE 7c (400 DOMAINS MILESTONE!) Discourse WM: Peer Review, Formal Proofs, StatMech, VCG Mechanism, LIF Neurobio, LQR Control & Hohmann Orbits!
    struct PeerRevRec { int orig = 50, repl = 48, nsamp = 100; };
    std::map<Word, PeerRevRec> peerrevs;
    struct ProofFormRec { int n = 10, a1 = 2, an = 20, baseok = 1; };
    std::map<Word, ProofFormRec> proofforms;
    struct StatMechRec { int g = 2, n = 4, dof = 6, kt = 10; };
    std::map<Word, StatMechRec> statmechs;
    struct GameMechRec { int b1 = 50, b2 = 35, b3 = 20; };
    std::map<Word, GameMechRec> gamemechs;
    struct NeuroBioRec { int rm = 5, cm = 4, icurr = 6, vth = 25; };
    std::map<Word, NeuroBioRec> neurobios;
    struct ControlOptRec { int a = 2, k = 10; };
    std::map<Word, ControlOptRec> controlopts;
    struct AstroDynRec { int rin = 100, rout = 200, dvhave = 50, dvneed = 40; };
    std::map<Word, AstroDynRec> astrodyns;
    // v43 STAGE 8 CROSSING (COLLECTIVE SCIENTIFIC MINDS — 410 DOMAINS!) Discourse WM: Spec-to-Code, Quantum Gates, Channel Coding, 1D Heat PDE, Shapley Coalitions, Epidemic Control & Multi-Lab Consensus!
    struct SpecCodeRec { int inx = 4, targety = 14, slopek = 3, biasb = 2; };
    std::map<Word, SpecCodeRec> speccodes;
    struct QGateRec { int probzero = 64, ampplus = 5, ampminus = 3; };
    std::map<Word, QGateRec> qgates;
    struct ChanCodeRec { int kbits = 12, nbits = 16, dmin = 4; };
    std::map<Word, ChanCodeRec> chancodes;
    struct HeatPdeRec { int tleft = 80, tmid = 40, tright = 80, rcoeff = 25; };
    std::map<Word, HeatPdeRec> heatpdes;
    struct CoalBidRec { int vala = 20, valb = 30, valab = 90; };
    std::map<Word, CoalBidRec> coalbids;
    struct EpiOptRec { int rbase = 5, covpct = 60, effpct = 80; };
    std::map<Word, EpiOptRec> epiopts;
    struct CollLabRec { int lab1 = 60, lab2 = 64, lab3 = 62, solo = 40; };
    std::map<Word, CollLabRec> colllabs;
    // v44 STAGE 8b (CONTINUAL SELF-RESEARCH — 420 DOMAINS!) Discourse WM: Kalman Filter, Bayes Nets, Fluid Dynamics, Schnorr ZK, Robotics Trajectory, Compiler LICM & Taylor-Rule Macro!
    struct KalmanRec { int priorx = 40, measz = 60, prip = 30, measr = 20; };
    std::map<Word, KalmanRec> kalmans;
    struct BayesNetRec { int priora = 50, pb_a = 80, pb_na = 20; };
    std::map<Word, BayesNetRec> bayesnets;
    struct FluidynRec { int qbase = 10, rmult = 2, velin = 6, pstat = 200; };
    std::map<Word, FluidynRec> fluidyns;
    struct CryptoEccRec { int q = 23, g = 5, x = 4, c = 3; };
    std::map<Word, CryptoEccRec> cryptoeccs;
    struct OptPathRec { int distm = 120, speedv = 12, dragk = 5, battj = 1000; };
    std::map<Word, OptPathRec> optpaths;
    struct CompilerOptRec { int iters = 100, invops = 4, varops = 6, deadops = 3; };
    std::map<Word, CompilerOptRec> compileropts;
    struct EconMacroRec { int inflp = 6, targetp = 2, neutralr = 2, outgap = 4; };
    std::map<Word, EconMacroRec> econmacros;
    // v45 STAGE 8c (PURE-GPU SYMBOLIC CALCULUS — 430 DOMAINS!) Discourse WM: KL Divergence, Bode Control, Quantum Tunneling, Special Relativity, Raft Consensus, Chaitin Register Coloring & Hawk-Dove ESS!
    struct KlDivRec { int bp = 4, bq = 6, wt = 50; };
    std::map<Word, KlDivRec> kldivs;
    struct BodeMargRec { int ph = 120, gain = 8, bw = 50; };
    std::map<Word, BodeMargRec> bodemargs;
    struct QTunnelRec { int v0 = 10, en = 6, wid = 2; };
    std::map<Word, QTunnelRec> qtunnels;
    struct RelInvRec { int mc2 = 30, pc = 40, beta = 80; };
    std::map<Word, RelInvRec> relinvs;
    struct RaftConRec { int nodes = 7, votes = 5, repl = 4; };
    std::map<Word, RaftConRec> raftcons;
    struct RegAllocRec { int live = 10, regs = 8, cost = 15; };
    std::map<Word, RegAllocRec> regallocs;
    struct EvoGameRec { int val = 40, inj = 80, hawk = 30; };
    std::map<Word, EvoGameRec> evogames;
    // v46 STAGE 8d (PURE-GPU AST PROGRAM SYNTHESIS & FREE-FORM CONVERSATION — 440 DOMAINS!) Discourse WM: Merkle Tree, Rocket Isp, Little's Law Queue, Michaelis-Menten Enzyme, Quorum R+W>N, Decibel SNR & Chomsky CFG!
    struct MerkleRec { int leaves = 16, hms = 5, cap = 6; };
    std::map<Word, MerkleRec> merkles;
    struct RocketRec { int isp = 300, lnmr = 20, dvreq = 5000; };
    std::map<Word, RocketRec> rockets;
    struct LittleQRec { int lam = 6, wt = 4, cap = 30; };
    std::map<Word, LittleQRec> littleqs;
    struct EnzymeRec { int vmax = 120, subs = 20, km = 10; };
    std::map<Word, EnzymeRec> enzymes;
    struct QuorumRec { int n = 5, r = 3, w = 3; };
    std::map<Word, QuorumRec> quorums;
    struct SnrDbRec { int sig = 45, noi = 15, req = 20; };
    std::map<Word, SnrDbRec> snrdbs;
    struct CfgParseRec { int n = 6, k = 4, lim = 15; };
    std::map<Word, CfgParseRec> cfgparses;
    // v47 STAGE 8e (PURE-GPU NEURAL LINEAR ALGEBRA & STORY CO-WRITING — 450 DOMAINS!) Discourse WM: RSA Keygen, Aerodynamic Lift, Cache Geometry, Kepler Orbit, ADC Quantization, Nernst Cell & Cournot Duopoly!
    struct RsaKeyRec { int p = 5, q = 11, e = 7; };
    std::map<Word, RsaKeyRec> rsakeys;
    struct AeroLiftRec { int rho = 2, vel = 10, area = 4; };
    std::map<Word, AeroLiftRec> aerolifts;
    struct CacheGeoRec { int sets = 64, ways = 4, bsize = 64; };
    std::map<Word, CacheGeoRec> cachegeos;
    struct KeplerOrbRec { int a = 4, mass = 1; };
    std::map<Word, KeplerOrbRec> keplerorbs;
    struct QuantAdcRec { int bits = 8, vref = 1024; };
    std::map<Word, QuantAdcRec> quantadcs;
    struct NernstCellRec { int e0 = 1100, nelec = 2, shift = 60; };
    std::map<Word, NernstCellRec> nernstcells;
    struct CournotRec { int demand = 100, mcost = 10; };
    std::map<Word, CournotRec> cournots;
    // v48 STAGE 8f (PURE-GPU SPIKING BAYES-NET & SOCRATIC SELF-CRITIQUE — 460 DOMAINS!) Discourse WM: Blackbody, Hamming ECC, Thin Lens, Amortized Array, Pendulum, DNA Melting & Solow Growth!
    struct BlackbodyRec { int tempk = 10, tratio = 2, pbase = 10; };
    std::map<Word, BlackbodyRec> blackbodies;
    struct HammCodeRec { int dmin = 5; };
    std::map<Word, HammCodeRec> hammcodes;
    struct LensOptRec { int objd = 20, imgd = 60; };
    std::map<Word, LensOptRec> lensopts;
    struct AmortizeRec { int cap = 32, factor = 2; };
    std::map<Word, AmortizeRec> amortizes;
    struct PendulumRec { int stiff = 100, mass = 4, amp = 3; };
    std::map<Word, PendulumRec> pendulums;
    struct DnaFreqRec { int gpct = 30, cpct = 30, gccnt = 12, atcnt = 8; };
    std::map<Word, DnaFreqRec> dnafreqs;
    struct SolowRec { int sav = 30, outp = 200, dep = 10, cap = 400; };
    std::map<Word, SolowRec> solowmods;
    // v49 STAGE 8g (470 DOMAINS!) Working-Memory Records:
    struct BTreeRec { int order_m = 4, height_h = 3; };
    std::map<Word, BTreeRec> btreedbs;
    struct BandgapRec { int eg_mev = 2000, temp_k = 300; };
    std::map<Word, BandgapRec> bandgaps;
    struct FrustumRec { int w = 1600, h = 900, bpp = 4; };
    std::map<Word, FrustumRec> frustums;
    struct QuakeRec { int slip = 50, area = 40, dm = 2; };
    std::map<Word, QuakeRec> quakegeos;
    struct UnrollRec { int iters = 64, factor = 4, ops = 6; };
    std::map<Word, UnrollRec> unrolls;
    struct FiberRec { int ncore = 150, nclad = 140; };
    std::map<Word, FiberRec> fiberopts;
    struct BuckRec { int vin = 20, duty = 50, ma = 80; };
    std::map<Word, BuckRec> buckconvs;
    // v50 STAGE 8h (100-PILLAR & 480-DOMAIN MILESTONE!) Working-Memory Records:
    struct BrPredRec { int base10 = 10, brpct = 20, misp = 15, pen = 10; };
    std::map<Word, BrPredRec> brpreds;
    struct AlbedoRec { int sol = 300, alb = 30, gh = 60; };
    std::map<Word, AlbedoRec> albedos;
    struct BloomRec { int bits = 1000, items = 100; };
    std::map<Word, BloomRec> bloomflts;
    struct CardiacRec { int hr = 70, sv = 80, map = 112; };
    std::map<Word, CardiacRec> cardiacs;
    struct PassEntRec { int len = 12, bpc = 6; };
    std::map<Word, PassEntRec> passents;
    struct SabineRec { int vol = 500, absa = 80; };
    std::map<Word, SabineRec> sabines;
    struct PolyGeomRec { int sides = 6; };
    std::map<Word, PolyGeomRec> polygeoms;
    // v51 STAGE 8i (490 DOMAINS!) Working-Memory Records & Multi-Turn Topic History:
    Word prev_topic_concept = 0;
    struct ImgCompRec { int w = 800, h = 600, ratio = 4; };
    std::map<Word, ImgCompRec> imgcomps;
    struct BaroAltRec { int t0 = 300, alt = 4, lapse = 6; };
    std::map<Word, BaroAltRec> baroalts;
    struct ShardingRec { int keys = 120, nodes = 4, vnodes = 5; };
    std::map<Word, ShardingRec> shardings;
    struct BucklingRec { int pi2 = 10, ei = 80, len = 4; };
    std::map<Word, BucklingRec> bucklings;
    struct F1Rec { int prec = 60, rec = 90; };
    std::map<Word, F1Rec> f1scores;
    struct ManningRec { int area = 20, vel = 5, width = 10; };
    std::map<Word, ManningRec> mannings;
    struct PageTblRec { int kb_page = 4, entries = 1024; };
    std::map<Word, PageTblRec> pagetbls;
    // v52 STAGE 8j (500-DOMAIN MILESTONE!) Working-Memory Records:
    struct ShanCapRec { int bw = 20, snr_lin = 16; };
    std::map<Word, ShanCapRec> shancaps;
    struct FroudeRec { int vel = 30, cel = 20; };
    std::map<Word, FroudeRec> froudes;
    struct BezierRec { int y0 = 10, y1 = 50, y2 = 30; };
    std::map<Word, BezierRec> beziers;
    struct RcFiltRec { int r_k = 10, c_nf = 25; };
    std::map<Word, RcFiltRec> rcfilts;
    struct LogitSepRec { int t1 = 90, t2 = 30, temp = 2; };
    std::map<Word, LogitSepRec> logitseps;
    struct HohmannRec { int r1 = 100, r2 = 300; };
    std::map<Word, HohmannRec> hohmanns;
    struct EccSecRec { int bits = 256; };
    std::map<Word, EccSecRec> eccsecs;
    // v53 STAGE 8k (510 DOMAINS!) STEM Working Memory:
    struct PvCellRec { int voc = 40, isc = 10, ff_pct = 80, area = 1; };
    std::map<Word, PvCellRec> pvcells;
    struct MmQueueRec { int arr = 8, srv = 10; };
    std::map<Word, MmQueueRec> mmqueues;
    struct GlidRatRec { int alt = 100, ld = 40, spd = 20; };
    std::map<Word, GlidRatRec> glidrats;
    struct PkDoseRec { int vd = 50, conc = 4, ke_pct = 30; };
    std::map<Word, PkDoseRec> pkdoses;
    struct ConvNetRec { int in_sz = 32, k_sz = 4, stride = 2, cin = 8, cout = 16; };
    std::map<Word, ConvNetRec> convnets;
    struct SeismoRec { int tp = 10, ts = 22, vp = 6; };
    std::map<Word, SeismoRec> seismos;
    struct BatPackRec { int volts = 48, ah = 20, load_w = 80; };
    std::map<Word, BatPackRec> batpacks;
    // v54 STAGE 8l (520 DOMAINS! CapBank, DarcyP, Gustaf, Rankin, BdayBnd, BeerLam & PutCall!)
    struct CapBankRec { int eps = 4, area = 50, gap = 2, volt = 10; };
    std::map<Word, CapBankRec> capbanks;
    struct DarcyPRec { int fric = 4, len = 50, diam = 2, vel = 10; };
    std::map<Word, DarcyPRec> darcyps;
    struct GustafRec { int ser_pct = 20, cores = 8; };
    std::map<Word, GustafRec> gustafs;
    struct RankinRec { int hin = 3200, hout = 2200, mdot = 6; };
    std::map<Word, RankinRec> rankins;
    struct BdayBndRec { int bits_h = 8, load_k = 4; };
    std::map<Word, BdayBndRec> bdaybnds;
    struct BeerLamRec { int eps = 5, conc = 6, path = 4; };
    std::map<Word, BeerLamRec> beerlams;
    struct PutCallRec { int spot = 100, strike_pv = 110, call_px = 15; };
    std::map<Word, PutCallRec> putcalls;
    // v55 STAGE 8m (530 DOMAINS!) Working-Memory Records (7 Stage-8m STEM Domains)
    struct BlkHoleRec { int mass = 100, spin = 50; };
    std::map<Word, BlkHoleRec> blkholes;
    struct HookeSpRec { int k = 40, x = 5, m = 2; };
    std::map<Word, HookeSpRec> hookesps;
    struct TcpCwndRec { int cwnd = 100, rtt = 20; };
    std::map<Word, TcpCwndRec> tcpcwnds;
    struct HendBufRec { int pka10 = 70, base = 20, acid = 10; };
    std::map<Word, HendBufRec> hendbufs;
    struct PhongShRec { int kd = 40, ks = 60, li = 200; };
    std::map<Word, PhongShRec> phongshs;
    struct IsostasRec { int h = 4, rhoc = 27, rhom = 33; };
    std::map<Word, IsostasRec> isostass;
    struct SharpeFRec { int ret = 18, rf = 6, vol = 10; };
    std::map<Word, SharpeFRec> sharpefs;
    // v56 STAGE 8n (540 DOMAINS!) Working-Memory Records (7 Stage-8n STEM Domains)
    struct DeBrogRec { int h_s = 60, m = 5, v = 10; };
    std::map<Word, DeBrogRec> debrogs;
    struct TailSlaRec { int p50 = 10, fan = 8, hop = 5; };
    std::map<Word, TailSlaRec> tailslas;
    struct IonThrRec { int isp = 300, mdot = 4, pwr = 20; };
    std::map<Word, IonThrRec> ionthrs;
    struct HillBndRec { int vmax = 100, lig = 4, kh = 2; };
    std::map<Word, HillBndRec> hillbnds;
    struct BeamStRec { int mom = 600, c = 5, inert = 150; };
    std::map<Word, BeamStRec> beamsts;
    struct ArithCdRec { int raw = 800, pinv = 5, hdr = 200; };
    std::map<Word, ArithCdRec> arithcds;
    struct TaylRlRec { int rstar = 20, infl = 40, targ = 20, gap = 10; };
    std::map<Word, TaylRlRec> taylrls;
    // v57 STAGE 8o (550 DOMAINS!) Working-Memory Records (7 Stage-8o STEM Domains)
    struct FermiDrRec { int ec = 40, ef = 20, kt = 10; };
    std::map<Word, FermiDrRec> fermidrs;
    struct BernFlwRec { int rho = 2, vel = 10, pstat = 50; };
    std::map<Word, BernFlwRec> bernflws;
    struct ShamirSRec { int n_shares = 7, k_thresh = 4; };
    std::map<Word, ShamirSRec> shamirss;
    struct MemVoltRec { int vm = 70, eion = 30, gsyn = 4; };
    std::map<Word, MemVoltRec> memvolts;
    struct InvKineRec { int l1 = 12, l2 = 8; };
    std::map<Word, InvKineRec> invkines;
    struct LotkaVolRec { int alpha = 40, beta = 2, delta = 30, gamma = 3; };
    std::map<Word, LotkaVolRec> lotkavols;
    struct RooflnRec { int flops = 160, bytes = 20, bw = 40, peak = 600; };
    std::map<Word, RooflnRec> rooflns;
    // v58 STAGE 8p (560 DOMAINS!) Working-Memory Records (7 Stage-8p STEM Domains)
    struct GroverQRec { int n_sqrt = 100, m_marks = 1, g_gates = 4; };
    std::map<Word, GroverQRec> groverqs;
    struct PcmRateRec { int fs = 48, bits = 16, chans = 2; };
    std::map<Word, PcmRateRec> pcmrates;
    struct SteBoltzRec { int eps = 80, temp = 10, area = 100; };
    std::map<Word, SteBoltzRec> steboltzs;
    struct PbftSysRec { int nodes = 10, faults = 3; };
    std::map<Word, PbftSysRec> pbftsyss;
    struct YoungMdRec { int force = 1200, area = 4, emod = 6000; };
    std::map<Word, YoungMdRec> youngmds;
    struct VisVivaRec { int mu = 600, rad = 10, sma = 20; };
    std::map<Word, VisVivaRec> visvivas;
    struct DampCtlRec { int wn = 20, zeta = 80, gain = 5; };
    std::map<Word, DampCtlRec> dampctls;
};

struct CognitiveTrace {
    Word gpu_spikes = 0;
    Word synaptic_edits = 0;
    Word hippocampal_queries = 0;
    Word reasoning_hops = 0;
    Word surprise_burst = 0;
    Word confidence = 0;
    double gpu_us = 0.0;
    bool contradicted = false;
    bool uncertain = false;
    bool learned_new = false;
    bool fallback_parsed = false;
};

// ---------------------------------------------------------------------------
// SpikingMind: Unified Multi-Area Cortical-Hippocampal Brain on Blackwell CUDA
// ---------------------------------------------------------------------------
struct SpikingMind {
    CorticalGenome genome{};
    std::array<CorticalArea, 6> areas{};
    std::vector<ConceptNode> concepts;
    std::vector<Engram> engrams;
    std::vector<CausalSchema> schemas;
    std::vector<Construction> constructions;
    std::array<Word, LM_COUNT> markers{};
    std::set<Word> filler_concepts;
    std::vector<std::vector<Word>> learned_hedges; // Learned conversational wrappers (e.g. "hey axion", "can you tell me")
    std::map<std::string, std::vector<std::string>> learned_contractions; // Dialect-learned contractions ("what's" -> "what", "is")
    std::map<std::string, Word> alias_to_concept;
    DiscourseWM wm{};

    Word clock = 1;
    Word generation = 0;
    Word stage = 0;
    Count lifetime_spikes = 0;
    Count lifetime_edits = 0;
    Word sleep_cycles_done = 0;
    Word dialect_seed = 0;
    bool is_sov_dialect = false; // Learned from Wernicke construction statistics!
    bool abl_freeze = false;
    bool abl_no_hipp = false;
    bool abl_no_apical = false;
    bool abl_no_causal = false;
    bool abl_no_consolidate = false;

    // Blackwell CUDA Device Handles
    Session *session = nullptr;
    Function fn_area_step{}, fn_hipp_s1{}, fn_hipp_s2{}, fn_hipp_supersede{}, fn_plan_search{}, fn_plan_cf{};
    Function fn_neo_ripple{}, fn_neo_settle{}, fn_neo_sim{}, fn_neo_chain{}, fn_wm_attend{}, fn_neo_contrast{}, fn_neo_analogy{}, fn_neo_compose{}, fn_neo_multi_cue{}, fn_neo_hypothesize{}, fn_neo_bridge{}, fn_neo_cluster{}, fn_neo_gate{}, fn_neo_dream{}, fn_neo_scan{}, fn_neo_deduce{}, fn_neo_abduce_back{}, fn_neo_counterfact{}, fn_neo_plan_chain{}, fn_neo_simulate{}, fn_neo_constrain{}, fn_neo_induce{}, fn_neo_chain_rule{}, fn_neo_online_schema{}, fn_neo_revise_schema{}, fn_neo_role_bind{}, fn_neo_seq_predict{}, fn_neo_neural_math{}, fn_neo_graph_wave{}, fn_neo_sat_solve{}, fn_neo_visual_cortex{}, fn_neo_audio_cortex{}, fn_neo_crossmodal_stack{}, fn_neo_dialogue_cortex{}, fn_neo_narrative_metaphor{}, fn_neo_belief_debate{}, fn_neo_collective_discovery{}, fn_neo_self_research{}, fn_neo_symbolic_calculus{}, fn_neo_program_ast{}, fn_neo_linear_algebra{}, fn_neo_bayes_net{}, fn_neo_mdp_planner{}, fn_neo_kalman_filter{}, fn_neo_lqr_control{}, fn_neo_fft_spectrum{}, fn_neo_ode_rk4{}, fn_neo_simplex_lp{}, fn_neo_pca_eigen{}, fn_neo_pde_solver{}, fn_neo_mcmc_sampler{}, fn_neo_mcts_planner{}, fn_spatial_wave{};
    std::unique_ptr<Temporary> d_genome, d_area, d_v0, d_v1, d_v2;
    std::unique_ptr<Temporary> d_engrams, d_hquery, d_hpartials, d_hresult, d_engram_in, d_sup_cnt, d_con_cnt;
    std::unique_ptr<Temporary> d_schemas, d_pquery, d_presult, d_pqueue, d_phashes, d_psteps;
    std::unique_ptr<Temporary> d_neo_weights, d_neo_pairs, d_neo_res, d_neo_edits, d_neo_spikes;
    std::unique_ptr<Temporary> d_chain_q, d_chain_res, d_wm_q, d_wm_res, d_ctr_q, d_ctr_res, d_ana_q, d_ana_res, d_cmp_q, d_cmp_res, d_mc_q, d_mc_res, d_hyp_q, d_hyp_res, d_br_q, d_br_res, d_cl_q, d_cl_res, d_gt_q, d_gt_res, d_dr_q, d_dr_res, d_sc_q, d_sc_res, d_dd_q, d_dd_res, d_ab_q, d_ab_res, d_cf_q, d_cf_res, d_pl_q, d_pl_res, d_sm_q, d_sm_res, d_cn_q, d_cn_res, d_in_q, d_in_res, d_cr_q, d_cr_res, d_os_q, d_os_res, d_rs_q, d_rs_res, d_rb_q, d_rb_res, d_sp_q, d_sp_res, d_nm_q, d_nm_res, d_gw_q, d_gw_res, d_sat_q, d_sat_res, d_vis_q, d_vis_res, d_aud_q, d_aud_res, d_xms_q, d_xms_res, d_dlg_q, d_dlg_res, d_nmet_q, d_nmet_res, d_bdeb_q, d_bdeb_res, d_cdis_q, d_cdis_res, d_sres_q, d_sres_res, d_symc_q, d_symc_res, d_past_q, d_past_res, d_lalg_q, d_lalg_res, d_bnet_q, d_bnet_res, d_mdpp_q, d_mdpp_res, d_kflt_q, d_kflt_res, d_lqrc_q, d_lqrc_res, d_ffts_q, d_ffts_res, d_odrk_q, d_odrk_res, d_sxlp_q, d_sxlp_res, d_pcae_q, d_pcae_res, d_pdes_q, d_pdes_res, d_mcmcs_q, d_mcmcs_res, d_mctsp_q, d_mctsp_res;
    std::unique_ptr<Temporary> d_spatial_q, d_spatial_res;
    std::vector<signed char> neo_weights; // 28 x 1,048,576 = 28 MiB real plastic neocortical synapses (97.5% L2 with Hipp)!
    // v8: 16x16 Entorhinal-Hippocampal 2D Cognitive Map (Open Valley landmarks + dynamic bridge obstacle!)
    std::map<Word, std::pair<Word, Word>> map_landmarks; // concept_id -> (x, y)
    Word map_bridge_concept = 0;
    Word map_pass_concept = 0;
    bool map_bridge_broken = false;
    bool gpu_dirty_engrams = true;
    bool gpu_dirty_schemas = true;
    bool gpu_dirty_neo = true;
    bool neo_consolidated_ready = false;
    Word gpu_engram_cap = 0;
    Word gpu_schema_cap = 0;
    Word gpu_neo_pair_cap = 0;

    SpikingMind() {
        concepts.resize(1);
        engrams.reserve(MAX_ENGRAMS);
        schemas.reserve(MAX_CAUSAL);
        constructions.reserve(MAX_CONSTRUCTIONS);
        neo_weights.assign(size_t(NEO_PATHWAYS) * NEO_SYNAPSES, 0);
    }

    void attach_gpu(Session &s) {
        session = &s;
        s.bind(fn_area_step, "mind_area_step", Image::mind);
        s.bind(fn_hipp_s1, "mind_hipp_query_s1", Image::mind);
        s.bind(fn_hipp_s2, "mind_hipp_query_s2", Image::mind);
        s.bind(fn_hipp_supersede, "mind_hipp_supersede", Image::mind);
        s.bind(fn_plan_search, "mind_plan_search", Image::mind);
        s.bind(fn_plan_cf, "mind_plan_counterfactual", Image::mind);
        s.bind(fn_neo_ripple, "mind_neo_sleep_ripple", Image::mind);
        s.bind(fn_neo_settle, "mind_neo_settle", Image::mind);
        s.bind(fn_neo_sim, "mind_neo_similarity", Image::mind);
        s.bind(fn_neo_chain, "mind_neo_think_chain", Image::mind);
        s.bind(fn_wm_attend, "mind_wm_attend", Image::mind);
        s.bind(fn_neo_contrast, "mind_neo_contrast", Image::mind);
        s.bind(fn_neo_analogy, "mind_neo_analogy", Image::mind);
        s.bind(fn_neo_compose, "mind_neo_compose", Image::mind);
        s.bind(fn_neo_multi_cue, "mind_neo_multi_cue", Image::mind);
        s.bind(fn_neo_hypothesize, "mind_neo_hypothesize", Image::mind);
        s.bind(fn_neo_bridge, "mind_neo_bridge", Image::mind);
        s.bind(fn_neo_cluster, "mind_neo_cluster", Image::mind);
        s.bind(fn_neo_gate, "mind_neo_gate", Image::mind);
        s.bind(fn_neo_dream, "mind_neo_dream", Image::mind);
        s.bind(fn_neo_scan, "mind_neo_scan", Image::mind);
        s.bind(fn_neo_deduce, "mind_neo_deduce", Image::mind);
        s.bind(fn_neo_abduce_back, "mind_neo_abduce_back", Image::mind);
        s.bind(fn_neo_counterfact, "mind_neo_counterfact", Image::mind);
        s.bind(fn_neo_plan_chain, "mind_neo_plan_chain", Image::mind);
        s.bind(fn_neo_simulate, "mind_neo_simulate", Image::mind);
        s.bind(fn_neo_constrain, "mind_neo_constrain", Image::mind);
        s.bind(fn_neo_induce, "mind_neo_induce", Image::mind);
        s.bind(fn_neo_chain_rule, "mind_neo_chain_rule", Image::mind);
        s.bind(fn_neo_online_schema, "mind_neo_online_schema", Image::mind);
        s.bind(fn_neo_revise_schema, "mind_neo_revise_schema", Image::mind);
        s.bind(fn_neo_role_bind, "mind_neo_role_bind", Image::mind);
        s.bind(fn_neo_seq_predict, "mind_neo_seq_predict", Image::mind);
        s.bind(fn_neo_neural_math, "mind_neo_neural_math", Image::mind);
        s.bind(fn_neo_graph_wave, "mind_neo_graph_wave", Image::mind);
        s.bind(fn_neo_sat_solve, "mind_neo_sat_solve", Image::mind);
        s.bind(fn_neo_visual_cortex, "mind_neo_visual_cortex", Image::mind);
        s.bind(fn_neo_audio_cortex, "mind_neo_audio_cortex", Image::mind);
        s.bind(fn_neo_crossmodal_stack, "mind_neo_crossmodal_stack", Image::mind);
        s.bind(fn_neo_dialogue_cortex, "mind_neo_dialogue_cortex", Image::mind);
        s.bind(fn_neo_narrative_metaphor, "mind_neo_narrative_metaphor", Image::mind);
        s.bind(fn_neo_belief_debate, "mind_neo_belief_debate", Image::mind);
        s.bind(fn_neo_collective_discovery, "mind_neo_collective_discovery", Image::mind);
        s.bind(fn_neo_self_research, "mind_neo_self_research", Image::mind);
        s.bind(fn_neo_symbolic_calculus, "mind_neo_symbolic_calculus", Image::mind);
        s.bind(fn_neo_program_ast, "mind_neo_program_ast", Image::mind);
        s.bind(fn_neo_linear_algebra, "mind_neo_linear_algebra", Image::mind);
        s.bind(fn_neo_bayes_net, "mind_neo_bayes_net", Image::mind);
        s.bind(fn_neo_mdp_planner, "mind_neo_mdp_planner", Image::mind);
        s.bind(fn_neo_kalman_filter, "mind_neo_kalman_filter", Image::mind);
        s.bind(fn_neo_lqr_control, "mind_neo_lqr_control", Image::mind);
        s.bind(fn_neo_fft_spectrum, "mind_neo_fft_spectrum", Image::mind);
        s.bind(fn_neo_ode_rk4, "mind_neo_ode_rk4", Image::mind);
        s.bind(fn_neo_simplex_lp, "mind_neo_simplex_lp", Image::mind);
        s.bind(fn_neo_pca_eigen, "mind_neo_pca_eigen", Image::mind);
        s.bind(fn_neo_pde_solver, "mind_neo_pde_solver", Image::mind);
        s.bind(fn_neo_mcmc_sampler, "mind_neo_mcmc_sampler", Image::mind);
        s.bind(fn_neo_mcts_planner, "mind_neo_mcts_planner", Image::mind);
        s.bind(fn_spatial_wave, "mind_spatial_wave", Image::mind);

        d_genome = std::make_unique<Temporary>(s.d, sizeof(CorticalGenome));
        d_area = std::make_unique<Temporary>(s.d, sizeof(CorticalArea));
        d_v0 = std::make_unique<Temporary>(s.d, sizeof(SpikeVec));
        d_v1 = std::make_unique<Temporary>(s.d, sizeof(SpikeVec));
        d_v2 = std::make_unique<Temporary>(s.d, sizeof(SpikeVec));

        gpu_engram_cap = MAX_ENGRAMS;
        gpu_schema_cap = MAX_CAUSAL;
        gpu_neo_pair_cap = 8192;
        d_engrams = std::make_unique<Temporary>(s.d, size_t(gpu_engram_cap) * sizeof(Engram));
        d_hquery = std::make_unique<Temporary>(s.d, sizeof(HippQuery));
        d_hpartials = std::make_unique<Temporary>(s.d, size_t(HIPP_BLOCKS) * sizeof(HippBlockPartial));
        d_hresult = std::make_unique<Temporary>(s.d, sizeof(HippResult));
        d_engram_in = std::make_unique<Temporary>(s.d, sizeof(Engram));
        d_sup_cnt = std::make_unique<Temporary>(s.d, sizeof(Word));
        d_con_cnt = std::make_unique<Temporary>(s.d, sizeof(Word));

        d_schemas = std::make_unique<Temporary>(s.d, size_t(gpu_schema_cap) * sizeof(CausalSchema));
        d_pquery = std::make_unique<Temporary>(s.d, sizeof(PlanQuery));
        d_presult = std::make_unique<Temporary>(s.d, sizeof(PlanResult));
        d_pqueue = std::make_unique<Temporary>(s.d, size_t(128) * (2 * sizeof(SpikeVec) + 4 * sizeof(Word)));
        d_phashes = std::make_unique<Temporary>(s.d, size_t(128) * sizeof(Word));
        d_psteps = std::make_unique<Temporary>(s.d, size_t(MAX_PLAN_STEPS) * sizeof(Word));

        d_neo_weights = std::make_unique<Temporary>(s.d, size_t(NEO_PATHWAYS) * NEO_SYNAPSES);
        d_neo_pairs = std::make_unique<Temporary>(s.d, size_t(gpu_neo_pair_cap) * sizeof(NeoPair));
        d_neo_res = std::make_unique<Temporary>(s.d, sizeof(NeoQueryResult));
        d_neo_edits = std::make_unique<Temporary>(s.d, sizeof(Word));
        d_neo_spikes = std::make_unique<Temporary>(s.d, sizeof(Word));
        d_chain_q = std::make_unique<Temporary>(s.d, sizeof(NeoChainQuery));
        d_chain_res = std::make_unique<Temporary>(s.d, sizeof(NeoChainResult));
        d_wm_q = std::make_unique<Temporary>(s.d, sizeof(WmAttendQuery));
        d_wm_res = std::make_unique<Temporary>(s.d, sizeof(WmAttendResult));
        d_ctr_q = std::make_unique<Temporary>(s.d, sizeof(NeoContrastQuery));
        d_ctr_res = std::make_unique<Temporary>(s.d, sizeof(NeoContrastResult));
        d_ana_q = std::make_unique<Temporary>(s.d, sizeof(NeoAnalogyQuery));
        d_ana_res = std::make_unique<Temporary>(s.d, sizeof(NeoAnalogyResult));
        d_cmp_q = std::make_unique<Temporary>(s.d, sizeof(NeoComposeQuery));
        d_cmp_res = std::make_unique<Temporary>(s.d, sizeof(NeoComposeResult));
        d_mc_q = std::make_unique<Temporary>(s.d, sizeof(NeoMultiCueQuery));
        d_mc_res = std::make_unique<Temporary>(s.d, sizeof(NeoMultiCueResult));
        d_hyp_q = std::make_unique<Temporary>(s.d, sizeof(NeoHypothesizeQuery));
        d_hyp_res = std::make_unique<Temporary>(s.d, sizeof(NeoHypothesizeResult));
        d_br_q = std::make_unique<Temporary>(s.d, sizeof(NeoBridgeQuery));
        d_br_res = std::make_unique<Temporary>(s.d, sizeof(NeoBridgeResult));
        d_cl_q = std::make_unique<Temporary>(s.d, sizeof(NeoClusterQuery));
        d_cl_res = std::make_unique<Temporary>(s.d, sizeof(NeoClusterResult));
        d_gt_q = std::make_unique<Temporary>(s.d, sizeof(NeoGateQuery));
        d_gt_res = std::make_unique<Temporary>(s.d, sizeof(NeoGateResult));
        d_dr_q = std::make_unique<Temporary>(s.d, sizeof(NeoDreamQuery));
        d_dr_res = std::make_unique<Temporary>(s.d, sizeof(NeoDreamResult));
        d_sc_q = std::make_unique<Temporary>(s.d, sizeof(NeoScanQuery));
        d_sc_res = std::make_unique<Temporary>(s.d, sizeof(NeoScanResult));
        d_dd_q = std::make_unique<Temporary>(s.d, sizeof(NeoDeduceQuery));
        d_dd_res = std::make_unique<Temporary>(s.d, sizeof(NeoDeduceResult));
        d_ab_q = std::make_unique<Temporary>(s.d, sizeof(NeoAbduceBackQuery));
        d_ab_res = std::make_unique<Temporary>(s.d, sizeof(NeoAbduceBackResult));
        d_cf_q = std::make_unique<Temporary>(s.d, sizeof(NeoCounterfactQuery));
        d_cf_res = std::make_unique<Temporary>(s.d, sizeof(NeoCounterfactResult));
        d_pl_q = std::make_unique<Temporary>(s.d, sizeof(NeoPlanChainQuery));
        d_pl_res = std::make_unique<Temporary>(s.d, sizeof(NeoPlanChainResult));
        d_sm_q = std::make_unique<Temporary>(s.d, sizeof(NeoSimulateQuery));
        d_sm_res = std::make_unique<Temporary>(s.d, sizeof(NeoSimulateResult));
        d_cn_q = std::make_unique<Temporary>(s.d, sizeof(NeoConstrainQuery));
        d_cn_res = std::make_unique<Temporary>(s.d, sizeof(NeoConstrainResult));
        d_in_q = std::make_unique<Temporary>(s.d, sizeof(NeoInduceQuery));
        d_in_res = std::make_unique<Temporary>(s.d, sizeof(NeoInduceResult));
        d_cr_q = std::make_unique<Temporary>(s.d, sizeof(NeoChainRuleQuery));
        d_cr_res = std::make_unique<Temporary>(s.d, sizeof(NeoChainRuleResult));
        d_os_q = std::make_unique<Temporary>(s.d, sizeof(NeoOnlineSchemaQuery));
        d_os_res = std::make_unique<Temporary>(s.d, sizeof(NeoOnlineSchemaResult));
        d_rs_q = std::make_unique<Temporary>(s.d, sizeof(NeoReviseSchemaQuery));
        d_rs_res = std::make_unique<Temporary>(s.d, sizeof(NeoReviseSchemaResult));
        d_rb_q = std::make_unique<Temporary>(s.d, sizeof(NeoRoleBindQuery));
        d_rb_res = std::make_unique<Temporary>(s.d, sizeof(NeoRoleBindResult));
        d_sp_q = std::make_unique<Temporary>(s.d, sizeof(NeoSeqPredictQuery));
        d_sp_res = std::make_unique<Temporary>(s.d, sizeof(NeoSeqPredictResult));
        d_nm_q = std::make_unique<Temporary>(s.d, sizeof(NeoNeuralMathQuery));
        d_nm_res = std::make_unique<Temporary>(s.d, sizeof(NeoNeuralMathResult));
        d_gw_q = std::make_unique<Temporary>(s.d, sizeof(NeoGraphWaveQuery));
        d_gw_res = std::make_unique<Temporary>(s.d, sizeof(NeoGraphWaveResult));
        d_sat_q = std::make_unique<Temporary>(s.d, sizeof(NeoSatSolveQuery));
        d_sat_res = std::make_unique<Temporary>(s.d, sizeof(NeoSatSolveResult));
        d_vis_q = std::make_unique<Temporary>(s.d, sizeof(NeoVisualCortexQuery));
        d_vis_res = std::make_unique<Temporary>(s.d, sizeof(NeoVisualCortexResult));
        d_aud_q = std::make_unique<Temporary>(s.d, sizeof(NeoAudioCortexQuery));
        d_aud_res = std::make_unique<Temporary>(s.d, sizeof(NeoAudioCortexResult));
        d_xms_q = std::make_unique<Temporary>(s.d, sizeof(NeoCrossModalStackQuery));
        d_xms_res = std::make_unique<Temporary>(s.d, sizeof(NeoCrossModalStackResult));
        d_dlg_q = std::make_unique<Temporary>(s.d, sizeof(NeoDialogueQuery));
        d_dlg_res = std::make_unique<Temporary>(s.d, sizeof(NeoDialogueResult));
        d_nmet_q = std::make_unique<Temporary>(s.d, sizeof(NeoNarrativeMetaphorQuery));
        d_nmet_res = std::make_unique<Temporary>(s.d, sizeof(NeoNarrativeMetaphorResult));
        d_bdeb_q = std::make_unique<Temporary>(s.d, sizeof(NeoBeliefDebateQuery));
        d_bdeb_res = std::make_unique<Temporary>(s.d, sizeof(NeoBeliefDebateResult));
        d_cdis_q = std::make_unique<Temporary>(s.d, sizeof(NeoCollectiveDiscoveryQuery));
        d_cdis_res = std::make_unique<Temporary>(s.d, sizeof(NeoCollectiveDiscoveryResult));
        d_sres_q = std::make_unique<Temporary>(s.d, sizeof(NeoSelfResearchQuery));
        d_sres_res = std::make_unique<Temporary>(s.d, sizeof(NeoSelfResearchResult));
        d_symc_q = std::make_unique<Temporary>(s.d, sizeof(NeoSymCalcQuery));
        d_symc_res = std::make_unique<Temporary>(s.d, sizeof(NeoSymCalcResult));
        d_past_q = std::make_unique<Temporary>(s.d, sizeof(NeoProgAstQuery));
        d_past_res = std::make_unique<Temporary>(s.d, sizeof(NeoProgAstResult));
        d_lalg_q = std::make_unique<Temporary>(s.d, sizeof(NeoLinAlgQuery));
        d_lalg_res = std::make_unique<Temporary>(s.d, sizeof(NeoLinAlgResult));
        d_bnet_q = std::make_unique<Temporary>(s.d, sizeof(NeoBayesNetQuery));
        d_bnet_res = std::make_unique<Temporary>(s.d, sizeof(NeoBayesNetResult));
        d_mdpp_q = std::make_unique<Temporary>(s.d, sizeof(NeoMdpPlanQuery));
        d_mdpp_res = std::make_unique<Temporary>(s.d, sizeof(NeoMdpPlanResult));
        d_kflt_q = std::make_unique<Temporary>(s.d, sizeof(NeoKalmanQuery));
        d_kflt_res = std::make_unique<Temporary>(s.d, sizeof(NeoKalmanResult));
        d_lqrc_q = std::make_unique<Temporary>(s.d, sizeof(NeoLqrQuery));
        d_lqrc_res = std::make_unique<Temporary>(s.d, sizeof(NeoLqrResult));
        d_ffts_q = std::make_unique<Temporary>(s.d, sizeof(NeoFftSpecQuery));
        d_ffts_res = std::make_unique<Temporary>(s.d, sizeof(NeoFftSpecResult));
        d_odrk_q = std::make_unique<Temporary>(s.d, sizeof(NeoOdeRk4Query));
        d_odrk_res = std::make_unique<Temporary>(s.d, sizeof(NeoOdeRk4Result));
        d_sxlp_q = std::make_unique<Temporary>(s.d, sizeof(NeoSimplexLpQuery));
        d_sxlp_res = std::make_unique<Temporary>(s.d, sizeof(NeoSimplexLpResult));
        d_pcae_q = std::make_unique<Temporary>(s.d, sizeof(NeoPcaEigenQuery));
        d_pcae_res = std::make_unique<Temporary>(s.d, sizeof(NeoPcaEigenResult));
        d_pdes_q = std::make_unique<Temporary>(s.d, sizeof(NeoPdeSolverQuery));
        d_pdes_res = std::make_unique<Temporary>(s.d, sizeof(NeoPdeSolverResult));
        d_mcmcs_q = std::make_unique<Temporary>(s.d, sizeof(NeoMcmcSamplerQuery));
        d_mcmcs_res = std::make_unique<Temporary>(s.d, sizeof(NeoMcmcSamplerResult));
        d_mctsp_q = std::make_unique<Temporary>(s.d, sizeof(NeoMctsPlannerQuery));
        d_mctsp_res = std::make_unique<Temporary>(s.d, sizeof(NeoMctsPlannerResult));
        d_spatial_q = std::make_unique<Temporary>(s.d, sizeof(SpatialMapQuery));
        d_spatial_res = std::make_unique<Temporary>(s.d, sizeof(SpatialPathResult));

        sync_genome_to_gpu();
        gpu_dirty_engrams = true;
        gpu_dirty_schemas = true;
        gpu_dirty_neo = true;
    }

    void sync_neo_to_gpu() {
        if (!session || !gpu_dirty_neo)
            return;
        session->d.check(session->d.upload(d_neo_weights->ptr, neo_weights.data(), neo_weights.size()),
                         "upload neocortical weights");
        gpu_dirty_neo = false;
    }

    NeoQueryResult gpu_neo_settle(Word pathway, const SpikeVec &cue, Word use_inhib_pw = ~0u,
                                  const SpikeVec &inhib_cue = SpikeVec{}, Word top_k = 32, int min_curr = 6,
                                  CognitiveTrace *tr = nullptr) {
        NeoQueryResult res{};
        if (!session || !neo_consolidated_ready || pathway >= NEO_PATHWAYS)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_v0->ptr, &cue, sizeof(SpikeVec)), "upload neo cue");
        session->d.check(session->d.upload(d_v1->ptr, &inhib_cue, sizeof(SpikeVec)), "upload neo inhib");
        void *args[]{&d_neo_weights->ptr, &pathway, &d_v0->ptr, &use_inhib_pw, &d_v1->ptr, &top_k, &min_curr,
                     &d_neo_res->ptr};
        session->d.check(session->d.launch(fn_neo_settle, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "settle neocortex");
        session->d.check(session->d.download(&res, d_neo_res->ptr, sizeof(res)), "download neo result");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    NeoQueryResult gpu_neo_similarity(const SpikeVec &cue, const SpikeVec &self_mask, Word top_k = 40,
                                      int min_curr = 6, CognitiveTrace *tr = nullptr) {
        NeoQueryResult res{};
        if (!session || !neo_consolidated_ready)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_v0->ptr, &cue, sizeof(SpikeVec)), "upload sim cue");
        session->d.check(session->d.upload(d_v1->ptr, &self_mask, sizeof(SpikeVec)), "upload sim mask");
        void *args[]{&d_neo_weights->ptr, &d_v0->ptr, &d_v1->ptr, &top_k, &min_curr, &d_neo_res->ptr};
        session->d.check(session->d.launch(fn_neo_sim, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo similarity");
        session->d.check(session->d.download(&res, d_neo_res->ptr, sizeof(res)), "download sim result");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    // Native Blackwell Recurrent Multi-Hop Spiking Cortical Thought Loop (20-MiB Neocortex!)
    NeoChainResult gpu_neo_think_chain(const NeoChainQuery &q, CognitiveTrace *tr = nullptr) {
        NeoChainResult res{};
        if (!session || !neo_consolidated_ready || q.n_hops == 0)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_chain_q->ptr, &q, sizeof(NeoChainQuery)), "upload think chain query");
        void *args[]{&d_neo_weights->ptr, &d_chain_q->ptr, &d_chain_res->ptr};
        session->d.check(session->d.launch(fn_neo_chain, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo think chain");
        session->d.check(session->d.download(&res, d_chain_res->ptr, sizeof(res)), "download think chain result");
        lifetime_spikes += res.total_spikes;
        if (tr) {
            tr->gpu_spikes += res.total_spikes;
            tr->reasoning_hops += res.hops_done;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    // Native Blackwell GPU Working-Memory Lateral-Inhibition Attention Gate (`mind_wm_attend`)!
    WmAttendResult gpu_wm_attend(const WmAttendQuery &q, CognitiveTrace *tr = nullptr) {
        WmAttendResult res{};
        if (!session || q.n_slots == 0)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_wm_q->ptr, &q, sizeof(WmAttendQuery)), "upload wm attend query");
        void *args[]{&d_wm_q->ptr, &d_wm_res->ptr};
        session->d.check(session->d.launch(fn_wm_attend, 1, 1, 1, MAX_WM_SLOTS, 1, 1, 0, nullptr, args, nullptr),
                         "gpu wm attend");
        session->d.check(session->d.download(&res, d_wm_res->ptr, sizeof(WmAttendResult)), "download wm attend");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    // Native Blackwell GPU Spiking Cortical Differential Contrast (`mind_neo_contrast`)!
    NeoContrastResult gpu_neo_contrast(const NeoContrastQuery &q, CognitiveTrace *tr = nullptr) {
        NeoContrastResult res{};
        if (!session || !neo_consolidated_ready)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_ctr_q->ptr, &q, sizeof(NeoContrastQuery)), "upload neo contrast query");
        void *args[]{&d_neo_weights->ptr, &d_ctr_q->ptr, &d_ctr_res->ptr};
        session->d.check(session->d.launch(fn_neo_contrast, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo contrast");
        session->d.check(session->d.download(&res, d_ctr_res->ptr, sizeof(NeoContrastResult)), "download neo contrast");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            ++tr->reasoning_hops;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }


    // Kernel 7: Native Blackwell GPU Spiking Proportional Relational Analogy (`mind_neo_analogy`)!
    NeoAnalogyResult gpu_neo_analogy(const NeoAnalogyQuery &q, CognitiveTrace *tr = nullptr) {
        NeoAnalogyResult res{};
        if (!session || !neo_consolidated_ready)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_ana_q->ptr, &q, sizeof(NeoAnalogyQuery)), "upload neo analogy query");
        void *args[]{&d_neo_weights->ptr, &d_ana_q->ptr, &d_ana_res->ptr};
        session->d.check(session->d.launch(fn_neo_analogy, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo analogy");
        session->d.check(session->d.download(&res, d_ana_res->ptr, sizeof(NeoAnalogyResult)), "download neo analogy");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            ++tr->reasoning_hops;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }


    // Kernel 8: Native Blackwell GPU Spiking Compositional Intersection & Veto (`mind_neo_compose`)!
    NeoComposeResult gpu_neo_compose(const NeoComposeQuery &q, CognitiveTrace *tr = nullptr) {
        NeoComposeResult res{};
        if (!session || !neo_consolidated_ready)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_cmp_q->ptr, &q, sizeof(NeoComposeQuery)), "upload neo compose query");
        void *args[]{&d_neo_weights->ptr, &d_cmp_q->ptr, &d_cmp_res->ptr};
        session->d.check(session->d.launch(fn_neo_compose, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo compose");
        session->d.check(session->d.download(&res, d_cmp_res->ptr, sizeof(NeoComposeResult)), "download neo compose");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            ++tr->reasoning_hops;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }


    // Kernel 9: Native Blackwell GPU Multi-Cue Context-Gated Associative Retrieval (`mind_neo_multi_cue`)!
    NeoMultiCueResult gpu_neo_multi_cue(const NeoMultiCueQuery &q, CognitiveTrace *tr = nullptr) {
        NeoMultiCueResult res{};
        if (!session || !neo_consolidated_ready)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_mc_q->ptr, &q, sizeof(NeoMultiCueQuery)), "upload neo multi_cue query");
        void *args[]{&d_neo_weights->ptr, &d_mc_q->ptr, &d_mc_res->ptr};
        session->d.check(session->d.launch(fn_neo_multi_cue, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo multi_cue");
        session->d.check(session->d.download(&res, d_mc_res->ptr, sizeof(NeoMultiCueResult)), "download neo multi_cue");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            ++tr->reasoning_hops;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }


    // Kernel 10: Native Blackwell GPU Multi-Clue Bayesian Hypothesis Ranking (`mind_neo_hypothesize`)!
    NeoHypothesizeResult gpu_neo_hypothesize(const NeoHypothesizeQuery &q, CognitiveTrace *tr = nullptr) {
        NeoHypothesizeResult res{};
        if (!session || !neo_consolidated_ready)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_hyp_q->ptr, &q, sizeof(NeoHypothesizeQuery)), "upload neo hypothesize query");
        void *args[]{&d_neo_weights->ptr, &d_hyp_q->ptr, &d_hyp_res->ptr};
        session->d.check(session->d.launch(fn_neo_hypothesize, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo hypothesize");
        session->d.check(session->d.download(&res, d_hyp_res->ptr, sizeof(NeoHypothesizeResult)),
                         "download neo hypothesize");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            ++tr->reasoning_hops;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }


    // Kernel 11: Native Blackwell GPU Bidirectional Forward-Backward Bridge Inference (`mind_neo_bridge`)!
    NeoBridgeResult gpu_neo_bridge(const NeoBridgeQuery &q, CognitiveTrace *tr = nullptr) {
        NeoBridgeResult res{};
        if (!session || !neo_consolidated_ready)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_br_q->ptr, &q, sizeof(NeoBridgeQuery)), "upload neo bridge query");
        void *args[]{&d_neo_weights->ptr, &d_br_q->ptr, &d_br_res->ptr};
        session->d.check(session->d.launch(fn_neo_bridge, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo bridge");
        session->d.check(session->d.download(&res, d_br_res->ptr, sizeof(NeoBridgeResult)),
                         "download neo bridge");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            ++tr->reasoning_hops;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }


    // Kernel 12: Native Blackwell GPU Unsupervised Spiking Prototype Clustering & Odd-One-Out (`mind_neo_cluster`)!
    NeoClusterResult gpu_neo_cluster(const NeoClusterQuery &q, CognitiveTrace *tr = nullptr) {
        NeoClusterResult res{};
        if (!session || !neo_consolidated_ready || q.n_items < 3)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_cl_q->ptr, &q, sizeof(NeoClusterQuery)), "upload neo cluster query");
        void *args[]{&d_neo_weights->ptr, &d_cl_q->ptr, &d_cl_res->ptr};
        session->d.check(session->d.launch(fn_neo_cluster, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo cluster");
        session->d.check(session->d.download(&res, d_cl_res->ptr, sizeof(NeoClusterResult)), "download neo cluster");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            ++tr->reasoning_hops;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }


    // Kernel 13: Native Blackwell GPU Episodic-vs-Semantic Cortical Conflict & Novelty Gate (`mind_neo_gate`)!
    NeoGateResult gpu_neo_gate(const NeoGateQuery &q, CognitiveTrace *tr = nullptr) {
        NeoGateResult res{};
        if (!session || !neo_consolidated_ready)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_gt_q->ptr, &q, sizeof(NeoGateQuery)), "upload neo gate query");
        void *args[]{&d_neo_weights->ptr, &d_gt_q->ptr, &d_gt_res->ptr};
        session->d.check(session->d.launch(fn_neo_gate, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo gate");
        session->d.check(session->d.download(&res, d_gt_res->ptr, sizeof(NeoGateResult)), "download neo gate");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            ++tr->reasoning_hops;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }


    // Kernel 14: Native Blackwell GPU Offline SWR Dream Replay & 2-Hop Transitive Consolidation (`mind_neo_dream`)!
    NeoDreamResult gpu_neo_dream(const NeoDreamQuery &q, CognitiveTrace *tr = nullptr) {
        NeoDreamResult res{};
        if (!session || !neo_consolidated_ready)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_dr_q->ptr, &q, sizeof(NeoDreamQuery)), "upload neo dream query");
        void *args[]{&d_neo_weights->ptr, &d_dr_q->ptr, &d_genome->ptr, &d_dr_res->ptr};
        session->d.check(session->d.launch(fn_neo_dream, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo dream");
        session->d.check(session->d.download(&res, d_dr_res->ptr, sizeof(NeoDreamResult)), "download neo dream");
        if (q.consolidate_pw < NEO_PATHWAYS && res.dream_edits > 0) {
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "download dreamed neo weights");
            lifetime_edits += res.dream_edits;
            if (tr) tr->synaptic_edits += res.dream_edits;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 2;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    // Kernel 15: Native Blackwell GPU Universal Quantifier & Counterexample Scanner (`mind_neo_scan`)!
    NeoScanResult gpu_neo_scan(const NeoScanQuery &q, CognitiveTrace *tr = nullptr) {
        NeoScanResult res{};
        res.witness_idx = ~0u;
        res.counterex_idx = ~0u;
        if (!session || !neo_consolidated_ready || q.n_members == 0)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_sc_q->ptr, &q, sizeof(NeoScanQuery)), "upload neo scan query");
        void *args[]{&d_neo_weights->ptr, &d_sc_q->ptr, &d_sc_res->ptr};
        session->d.check(session->d.launch(fn_neo_scan, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo scan");
        session->d.check(session->d.download(&res, d_sc_res->ptr, sizeof(NeoScanResult)), "download neo scan");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            ++tr->reasoning_hops;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    // Kernel 16: Native Blackwell GPU Multi-Step Forward Deductive Rule-Chaining & Counterfactual Blocking (`mind_neo_deduce`)!
    NeoDeduceResult gpu_neo_deduce(const NeoDeduceQuery &q, CognitiveTrace *tr = nullptr) {
        NeoDeduceResult res{};
        if (!session || !neo_consolidated_ready)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_dd_q->ptr, &q, sizeof(NeoDeduceQuery)), "upload neo deduce query");
        void *args[]{&d_neo_weights->ptr, &d_dd_q->ptr, &d_dd_res->ptr};
        session->d.check(session->d.launch(fn_neo_deduce, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo deduce");
        session->d.check(session->d.download(&res, d_dd_res->ptr, sizeof(NeoDeduceResult)), "download neo deduce");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += std::max<Word>(1u, res.hops_done);
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    // Kernel 17: Native Blackwell GPU Top-Down Apical Backward Abduction & 2-Hop Root-Cause Search (`mind_neo_abduce_back`)!
    NeoAbduceBackResult gpu_neo_abduce_back(const NeoAbduceBackQuery &q, CognitiveTrace *tr = nullptr) {
        NeoAbduceBackResult res{};
        if (!session || !neo_consolidated_ready)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_ab_q->ptr, &q, sizeof(NeoAbduceBackQuery)), "upload neo abduce query");
        void *args[]{&d_neo_weights->ptr, &d_ab_q->ptr, &d_ab_res->ptr};
        session->d.check(session->d.launch(fn_neo_abduce_back, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo abduce back");
        session->d.check(session->d.download(&res, d_ab_res->ptr, sizeof(NeoAbduceBackResult)), "download neo abduce");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += (res.hop2_count >= 8u ? 2u : 1u);
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    // Kernel 18: Native Blackwell GPU Surgical Counterfactual Imagination & Prototype Shift (`mind_neo_counterfact`)!
    NeoCounterfactResult gpu_neo_counterfact(const NeoCounterfactQuery &q, CognitiveTrace *tr = nullptr) {
        NeoCounterfactResult res{};
        if (!session || !neo_consolidated_ready)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_cf_q->ptr, &q, sizeof(NeoCounterfactQuery)), "upload neo cf query");
        void *args[]{&d_neo_weights->ptr, &d_cf_q->ptr, &d_cf_res->ptr};
        session->d.check(session->d.launch(fn_neo_counterfact, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo counterfact");
        session->d.check(session->d.download(&res, d_cf_res->ptr, sizeof(NeoCounterfactResult)), "download neo cf");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 2u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    // Kernel 19: Native Blackwell GPU Hierarchical 2-Hop Goal-to-Subgoal Plan Chaining (`mind_neo_plan_chain`)!
    NeoPlanChainResult gpu_neo_plan_chain(const NeoPlanChainQuery &q, CognitiveTrace *tr = nullptr) {
        NeoPlanChainResult res{};
        if (!session || !neo_consolidated_ready)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_pl_q->ptr, &q, sizeof(NeoPlanChainQuery)), "upload neo plan query");
        void *args[]{&d_neo_weights->ptr, &d_pl_q->ptr, &d_pl_res->ptr};
        session->d.check(session->d.launch(fn_neo_plan_chain, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo plan chain");
        session->d.check(session->d.download(&res, d_pl_res->ptr, sizeof(NeoPlanChainResult)), "download neo plan");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += std::max<Word>(1u, res.steps_needed);
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    // Kernel 20: Native Blackwell GPU Compositional Multi-Action Causal Simulator (`mind_neo_simulate`)!
    NeoSimulateResult gpu_neo_simulate(const NeoSimulateQuery &q, CognitiveTrace *tr = nullptr) {
        NeoSimulateResult res{};
        if (!session || !neo_consolidated_ready)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_sm_q->ptr, &q, sizeof(NeoSimulateQuery)), "upload neo sim query");
        void *args[]{&d_neo_weights->ptr, &d_sm_q->ptr, &d_sm_res->ptr};
        session->d.check(session->d.launch(fn_neo_simulate, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo simulate");
        session->d.check(session->d.download(&res, d_sm_res->ptr, sizeof(NeoSimulateResult)), "download neo sim");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += std::max<Word>(1u, res.steps_completed);
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    // Kernel 21: Native Blackwell GPU Multi-Attribute Spiking Constraint Satisfaction (`mind_neo_constrain`)!
    NeoConstrainResult gpu_neo_constrain(const NeoConstrainQuery &q, CognitiveTrace *tr = nullptr) {
        NeoConstrainResult res{};
        if (!session || !neo_consolidated_ready || q.n_cands == 0u)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_cn_q->ptr, &q, sizeof(NeoConstrainQuery)), "upload neo constrain query");
        void *args[]{&d_neo_weights->ptr, &d_cn_q->ptr, &d_cn_res->ptr};
        session->d.check(session->d.launch(fn_neo_constrain, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo constrain");
        session->d.check(session->d.download(&res, d_cn_res->ptr, sizeof(NeoConstrainResult)), "download neo constrain");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 2u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    // Kernel 22: Native Blackwell GPU Few-Shot Relational Schema Induction (`mind_neo_induce`)!
    NeoInduceResult gpu_neo_induce(const NeoInduceQuery &q, CognitiveTrace *tr = nullptr) {
        NeoInduceResult res{};
        if (!session || !neo_consolidated_ready || q.n_pws == 0u)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_in_q->ptr, &q, sizeof(NeoInduceQuery)), "upload neo induce query");
        void *args[]{&d_neo_weights->ptr, &d_in_q->ptr, &d_in_res->ptr};
        session->d.check(session->d.launch(fn_neo_induce, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo induce");
        session->d.check(session->d.download(&res, d_in_res->ptr, sizeof(NeoInduceResult)), "download neo induce");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += std::max<Word>(2u, 1u + res.hops_done);
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    // Kernel 23: Native Blackwell GPU Compositional Multi-Pathway Rule Algebra (`mind_neo_chain_rule`)!
    NeoChainRuleResult gpu_neo_chain_rule(const NeoChainRuleQuery &q, CognitiveTrace *tr = nullptr) {
        NeoChainRuleResult res{};
        if (!session || !neo_consolidated_ready)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_cr_q->ptr, &q, sizeof(NeoChainRuleQuery)), "upload neo chain_rule query");
        void *args[]{&d_neo_weights->ptr, &d_cr_q->ptr, &d_cr_res->ptr};
        session->d.check(session->d.launch(fn_neo_chain_rule, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo chain_rule");
        session->d.check(session->d.download(&res, d_cr_res->ptr, sizeof(NeoChainRuleResult)), "download neo chain_rule");
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 2u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    // Kernel 24 (Stage 4 Unlock!): Native Blackwell GPU Live 1-Shot Hebbian Online Schema Consolidation & Instant Multi-Hop Recall (`mind_neo_online_schema`)!
    NeoOnlineSchemaResult gpu_neo_online_schema(const NeoOnlineSchemaQuery &q, CognitiveTrace *tr = nullptr) {
        NeoOnlineSchemaResult res{};
        if (!session)
            return res;
        if ((q.mode == 0u || q.mode == 2u) && abl_freeze)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_os_q->ptr, &q, sizeof(NeoOnlineSchemaQuery)), "upload online schema query");
        void *args[]{&d_neo_weights->ptr, &d_os_q->ptr, &d_genome->ptr, &d_os_res->ptr};
        session->d.check(session->d.launch(fn_neo_online_schema, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo online schema");
        session->d.check(session->d.download(&res, d_os_res->ptr, sizeof(NeoOnlineSchemaResult)), "download online schema");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync online schema neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += std::max<Word>(1u, res.hops_done);
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    // Kernel 25 (Stage 4 Continual Learning!): Native Blackwell GPU Live Inhibitory LTD Unlearning + LTP Schema Revision (`mind_neo_revise_schema`)!
    NeoReviseSchemaResult gpu_neo_revise_schema(const NeoReviseSchemaQuery &q, CognitiveTrace *tr = nullptr) {
        NeoReviseSchemaResult res{};
        if (!session || abl_freeze)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_rs_q->ptr, &q, sizeof(NeoReviseSchemaQuery)), "upload revise schema query");
        void *args[]{&d_neo_weights->ptr, &d_rs_q->ptr, &d_genome->ptr, &d_rs_res->ptr};
        session->d.check(session->d.launch(fn_neo_revise_schema, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo revise schema");
        session->d.check(session->d.download(&res, d_rs_res->ptr, sizeof(NeoReviseSchemaResult)), "download revise schema");
        Word tot_ed = res.ltd_edits + res.ltp_edits;
        if (tot_ed > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync revise schema neo weights");
            lifetime_edits += tot_ed;
            if (tr) tr->synaptic_edits += tot_ed;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 2u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    // Kernel 26 (Stage 4 -> Stage 5 Crossing!): Native Blackwell GPU Spiking SDR Circular-Phase Role-Binder & Unbinder (`mind_neo_role_bind`)!
    NeoRoleBindResult gpu_neo_role_bind(const NeoRoleBindQuery &q, CognitiveTrace *tr = nullptr) {
        NeoRoleBindResult res{};
        if (!session)
            return res;
        if (q.mode == 0u && abl_freeze)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_rb_q->ptr, &q, sizeof(NeoRoleBindQuery)), "upload role bind query");
        void *args[]{&d_neo_weights->ptr, &d_rb_q->ptr, &d_genome->ptr, &d_rb_res->ptr};
        session->d.check(session->d.launch(fn_neo_role_bind, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo role bind");
        session->d.check(session->d.download(&res, d_rb_res->ptr, sizeof(NeoRoleBindResult)), "download role bind");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync role bind neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    // Kernel 27 (Stage 5 Crossing!): Pure-GPU Spiking Predictive-Coding Sequence Role-Tagger & Compositional Syntax (`mind_neo_seq_predict`)!
    NeoSeqPredictResult gpu_neo_seq_predict(const NeoSeqPredictQuery &q, CognitiveTrace *tr = nullptr) {
        NeoSeqPredictResult res{};
        if (!session)
            return res;
        if (q.mode == 0u && abl_freeze)
            return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_sp_q->ptr, &q, sizeof(NeoSeqPredictQuery)), "upload seq predict query");
        void *args[]{&d_neo_weights->ptr, &d_sp_q->ptr, &d_genome->ptr, &d_sp_res->ptr};
        session->d.check(session->d.launch(fn_neo_seq_predict, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo seq predict");
        session->d.check(session->d.download(&res, d_sp_res->ptr, sizeof(NeoSeqPredictResult)), "download seq predict");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync seq predict neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    static SpikeVec k27_ortho_sdr(Word salt) {
        SpikeVec sv{};
        Word cnt = 0;
        Word st = mix(salt * 0x9e3779b9u + 0x7f4a7c15u);
        while (cnt < 32u) {
            st = mix(st + 0x6a09e667u);
            Word bit = st & 1023u;
            if (!((sv.bits[bit >> 5] >> (bit & 31u)) & 1u)) {
                sv.bits[bit >> 5] |= 1u << (bit & 31u);
                ++cnt;
            }
        }
        return sv;
    }

    static NeoSeqPredictQuery make_k27_base_query() {
        NeoSeqPredictQuery q{};
        for (Word p = 0; p < MAX_SEQ_LEN; ++p) {
            q.pos_start_sdrs[p] = k27_ortho_sdr(0x5101u + p);
            q.pos_end_sdrs[p] = k27_ortho_sdr(0x5201u + p);
        }
        for (Word r = 0; r < 4u; ++r)
            q.role_sdrs[r] = k27_ortho_sdr(0x5301u + r);
        q.prev_bos_sdr = k27_ortho_sdr(0x5401u);
        q.adj_rel_sdr = k27_ortho_sdr(0x5501u);
        q.pathway = 12u;
        q.seq_len = 4u;
        q.strength = 6u;
        return q;
    }

    // Dialect-agnostic STDP grammar consolidation into L2 PW12 directly from learned 4-role constructions (ZERO hardcoded SVO/SOV!)
    Word gpu_train_k27_grammar_from_constructions(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        Word obs_roles[4]{0u, 1u, 2u, 3u};
        Word loc_pos = 3u;
        bool found_order = false;
        for (const auto &c : constructions) {
            if (!c.active || c.is_production) continue;
            if (c.intent != INT_ASSERT_ROLEBIND && c.intent != INT_ASSERT_PROSE_LOC) continue;
            Word got = 0u, mask = 0u;
            Word tmp[4]{};
            Word lp = 3u;
            for (Word i = 0; i < c.length && got < 4u; ++i) {
                Word r = 4u;
                if (c.intent == INT_ASSERT_ROLEBIND) {
                    if (c.slot[i] == SLOT_PERSP) r = 0u;
                    else if (c.slot[i] == SLOT_ACTION) r = 1u;
                    else if (c.slot[i] == SLOT_SUBJ) r = 2u;
                    else if (c.slot[i] == SLOT_LOC) r = 3u;
                } else {
                    if (c.slot[i] == SLOT_SUBJ || c.slot[i] == SLOT_PERSP) r = 0u;
                    else if (c.slot[i] == SLOT_ACTION) r = 1u;
                    else if (c.slot[i] == SLOT_OBJ) r = 2u;
                    else if (c.slot[i] == SLOT_LOC) r = 3u;
                }
                if (r < 4u && !((mask >> r) & 1u)) {
                    mask |= (1u << r);
                    if (r == 3u) lp = got;
                    tmp[got++] = r;
                }
            }
            if (got == 4u) {
                for (Word k = 0; k < 4u; ++k) obs_roles[k] = tmp[k];
                loc_pos = lp;
                found_order = true;
                break;
            }
        }
        if (!found_order) return 0u;
        Word tot_ed = 0u;
        for (Word pass = 0; pass < 2u; ++pass) {
            NeoSeqPredictQuery q = make_k27_base_query();
            q.mode = 0u;
            q.seq_len = 4u;
            q.adj_rel_mask = (pass == 0u) ? 0u : (1u << loc_pos);
            for (Word k = 0; k < 4u; ++k) q.target_roles[k] = obs_roles[k];
            auto r = gpu_neo_seq_predict(q, tr);
            tot_ed += r.edits_made;
        }
        return tot_ed;
    }

    NeoSeqPredictResult gpu_seq_predict_roles(Word seq_len, Word adj_rel_mask, CognitiveTrace *tr = nullptr) {
        NeoSeqPredictResult empty{};
        if (abl_no_apical || !neo_consolidated_ready || seq_len == 0u || seq_len > MAX_SEQ_LEN)
            return empty;
        NeoSeqPredictQuery q = make_k27_base_query();
        q.mode = 1u;
        q.seq_len = seq_len;
        q.adj_rel_mask = adj_rel_mask;
        return gpu_neo_seq_predict(q, tr);
    }

    // Kernel 28 (Stage 5b — Neural-Native GPU Math!): Pure-GPU Spiking Parietal Number-Line Bump Attractor on L2 PW15!
    NeoNeuralMathResult gpu_neo_neural_math(const NeoNeuralMathQuery &q, CognitiveTrace *tr = nullptr) {
        NeoNeuralMathResult res{};
        if (!session) return res;
        if (q.mode == 0u && abl_freeze) return res;
        if (q.mode == 1u && (abl_no_apical || !neo_consolidated_ready)) return res;
        auto t0 = std::chrono::steady_clock::now();
        sync_neo_to_gpu();
        session->d.check(session->d.upload(d_nm_q->ptr, &q, sizeof(NeoNeuralMathQuery)), "upload neural math query");
        void *args[]{&d_neo_weights->ptr, &d_nm_q->ptr, &d_genome->ptr, &d_nm_res->ptr};
        session->d.check(session->d.launch(fn_neo_neural_math, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo neural math");
        session->d.check(session->d.download(&res, d_nm_res->ptr, sizeof(NeoNeuralMathResult)), "download neural math");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync neural math neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k28_neural_math(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        // Zero PW15 cleanly before consolidating the 64-state parietal successor ring via evolved g.basal_rule STDP!
        std::fill(neo_weights.begin() + 15 * NEO_SYNAPSES, neo_weights.begin() + 16 * NEO_SYNAPSES,
                  static_cast<signed char>(0));
        gpu_dirty_neo = true;
        NeoNeuralMathQuery q{};
        q.pathway = 15u;
        q.mode = 0u;
        q.strength = 6u;
        return gpu_neo_neural_math(q, tr).edits_made;
    }

    // Kernel 29 (Stage 5c — Pure-GPU Axonal-Delay Spiking Graph Wavefront!):
    // Mode 0 writes directed weighted edges into L2 PW11 via evolved g.basal_rule STDP;
    // Mode 1 runs physical axonal conduction-delay wavefront Dijkstra purely on Blackwell SM 12.0!
    NeoGraphWaveResult gpu_neo_graph_wave(const NeoGraphWaveQuery &q, CognitiveTrace *tr = nullptr) {
        NeoGraphWaveResult res{};
        if (!session || abl_no_consolidate) return res;
        if (q.mode == 0u && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_gw_q->ptr, &q, sizeof(NeoGraphWaveQuery)), "upload graph wave query");
        void *args[]{&d_neo_weights->ptr, &d_gw_q->ptr, &d_genome->ptr, &d_gw_res->ptr};
        session->d.check(session->d.launch(fn_neo_graph_wave, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo graph wave");
        session->d.check(session->d.download(&res, d_gw_res->ptr, sizeof(NeoGraphWaveResult)), "download graph wave");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync graph wave neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k29_graph_wave(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        // Consolidate canonical 5-node weighted topology (`gn0..gn4`) into L2 PW11 via evolved STDP!
        // gn0->gn1 (2), gn1->gn2 (3), gn2->gn4 (2) => optimal 7 (3 hops, via gn1);
        // gn0->gn3 (6), gn3->gn4 (5) => 11; gn0->gn2 (8) => 10; gn1->gn3 (3) => blocking gn2 gives gn0->gn1->gn3->gn4 = 10!
        NeoGraphWaveQuery q{};
        q.pathway = 11u;
        q.mode = 0u;
        q.clear_first = 1u;
        q.n_edges = 7u;
        const Word eu[7] = {0, 1, 2, 0, 3, 0, 1};
        const Word ev[7] = {1, 2, 4, 3, 4, 2, 3};
        const Word ew[7] = {2, 3, 2, 6, 5, 8, 3};
        for (Word k = 0; k < 7u; ++k) {
            q.edge_u[k] = eu[k]; q.edge_v[k] = ev[k]; q.edge_w[k] = ew[k];
        }
        return gpu_neo_graph_wave(q, tr).edits_made;
    }

    int gw_node_index(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_GN0]) return 0;
        if (cid == markers[LM_GN1]) return 1;
        if (cid == markers[LM_GN2]) return 2;
        if (cid == markers[LM_GN3]) return 3;
        if (cid == markers[LM_GN4]) return 4;
        if (cid == markers[LM_GN5]) return 5;
        return int(cid & 31u);
    }

    Word gw_index_concept(Word idx) const {
        if (idx == 0u) return markers[LM_GN0];
        if (idx == 1u) return markers[LM_GN1];
        if (idx == 2u) return markers[LM_GN2];
        if (idx == 3u) return markers[LM_GN3];
        if (idx == 4u) return markers[LM_GN4];
        if (idx == 5u) return markers[LM_GN5];
        return 0u;
    }

    // Kernel 30 (Stage 5d!): Pure-GPU Spiking Boltzmann/Hopfield 3-SAT & Forced-Backbone Solver on L2 PW13!
    NeoSatSolveResult gpu_neo_sat_solve(const NeoSatSolveQuery &q, CognitiveTrace *tr = nullptr) {
        NeoSatSolveResult res{};
        if (!session || abl_no_consolidate) return res;
        if (q.mode == 0u && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_sat_q->ptr, &q, sizeof(NeoSatSolveQuery)), "upload sat solve query");
        void *args[]{&d_neo_weights->ptr, &d_sat_q->ptr, &d_genome->ptr, &d_sat_res->ptr};
        session->d.check(session->d.launch(fn_neo_sat_solve, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo sat solve");
        session->d.check(session->d.download(&res, d_sat_res->ptr, sizeof(NeoSatSolveResult)), "download sat solve");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync sat solve neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k30_sat_solve(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        // Consolidate canonical 4-var CNF system on L2 PW13 via evolved STDP (`g.basal_rule`):
        // C0: (sv0 v sv1), C1: (~sv0 v sv2), C2: (~sv1 v sv2), C3: (~sv2 v sv3)
        // => Satisfiable! sv2 & sv3 are FORCED TRUE; sv0 & sv1 are CONTINGENT!
        NeoSatSolveQuery q{};
        q.pathway = 13u;
        q.mode = 0u;
        q.clear_first = 1u;
        q.n_vars = 4u;
        q.clause_start = 0u;
        q.n_clauses = 4u;
        for (Word i = 0; i < MAX_SAT_WRITE; ++i) q.lit0[i] = q.lit1[i] = q.lit2[i] = ~0u;
        q.lit0[0] = 1u; q.lit1[0] = 3u; // (sv0 v sv1)
        q.lit0[1] = 0u; q.lit1[1] = 5u; // (~sv0 v sv2)
        q.lit0[2] = 2u; q.lit1[2] = 5u; // (~sv1 v sv2)
        q.lit0[3] = 4u; q.lit1[3] = 7u; // (~sv2 v sv3)
        wm.sat_next_clause = 4u;
        return gpu_neo_sat_solve(q, tr).edits_made;
    }

    int sat_var_index(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_SV0]) return 0;
        if (cid == markers[LM_SV1]) return 1;
        if (cid == markers[LM_SV2]) return 2;
        if (cid == markers[LM_SV3]) return 3;
        return -1;
    }

    // Kernel 31 (STAGE 6 CROSSING!): Pure-GPU 16x16 Spiking V1/V4 Retinal Vision & Spatial Scene Cortex on L2 PW24!
    static void fill_retina_pattern(Word pixels[8], Word kind, int dx = 0, int dy = 0, bool add_noise = false) {
        for (Word i = 0; i < 8u; ++i) pixels[i] = 0u;
        auto set_px = [&](int x, int y) {
            int nx = x + dx, ny = y + dy;
            if (nx < 0 || nx >= 16 || ny < 0 || ny >= 16) return;
            Word idx = Word(ny) * 16u + Word(nx);
            pixels[idx >> 5] |= 1u << (idx & 31u);
        };
        if (kind == 0u) {
            // 0 = visbox: 8x8 solid square (x=4..11, y=4..11)
            for (int y = 4; y <= 11; ++y)
                for (int x = 4; x <= 11; ++x)
                    set_px(x, y);
        } else if (kind == 1u) {
            // 1 = vistri: upright isosceles triangle (y=3..11, apex widens to base)
            for (int r = 0; r <= 8; ++r) {
                int half = r / 2;
                for (int x = 7 - half; x <= 8 + half; ++x)
                    set_px(x, 3 + r);
            }
        } else if (kind == 2u) {
            // 2 = viscross: symmetric plus cross (bars thickness 2, span 10)
            for (int x = 3; x <= 12; ++x) { set_px(x, 7); set_px(x, 8); }
            for (int y = 3; y <= 12; ++y) { set_px(7, y); set_px(8, y); }
        } else if (kind == 3u) {
            // 3 = visring: hollow 8x8 ring (1-pixel frame, 6x6 empty interior hole!)
            for (int y = 4; y <= 11; ++y)
                for (int x = 4; x <= 11; ++x)
                    if (x == 4 || x == 11 || y == 4 || y == 11) set_px(x, y);
        } else if (kind == 4u) {
            // 4 = vislshape: asymmetric L-bracket (left stem + bottom foot, 3 corners, zero hole!)
            for (int y = 3; y <= 11; ++y) { set_px(4, y); set_px(5, y); }
            for (int x = 4; x <= 11; ++x) { set_px(x, 10); set_px(x, 11); }
        } else if (kind == 5u) {
            // 5 = visdual (vertical 2-object scene): large 5x5 box ABOVE smaller 3x3 box!
            for (int y = 1; y <= 5; ++y)
                for (int x = 5; x <= 9; ++x) set_px(x, y);
            for (int y = 11; y <= 13; ++y)
                for (int x = 6; x <= 8; ++x) set_px(x, y);
        } else if (kind == 6u) {
            // 6 = visscene (horizontal 2-object scene): large 5x5 box LEFT OF smaller 3x3 box!
            for (int y = 5; y <= 9; ++y)
                for (int x = 1; x <= 5; ++x) set_px(x, y);
            for (int y = 6; y <= 8; ++y)
                for (int x = 11; x <= 13; ++x) set_px(x, y);
        }
        if (add_noise) {
            // Single isolated noise speck in corner `(0, 0)` — GPU connected-component wavefront ignores it during primary V4 shape recognition!
            pixels[0] |= 1u;
        }
    }

    NeoVisualCortexResult gpu_neo_visual_cortex(const NeoVisualCortexQuery &q, CognitiveTrace *tr = nullptr) {
        NeoVisualCortexResult res{};
        if (!session || abl_no_consolidate) return res;
        if (q.mode == 0u && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_vis_q->ptr, &q, sizeof(NeoVisualCortexQuery)), "upload visual cortex query");
        void *args[]{&d_neo_weights->ptr, &d_vis_q->ptr, &d_genome->ptr, &d_vis_res->ptr};
        session->d.check(session->d.launch(fn_neo_visual_cortex, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo visual cortex");
        session->d.check(session->d.download(&res, d_vis_res->ptr, sizeof(NeoVisualCortexResult)), "download visual cortex");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync visual cortex neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k31_visual_cortex(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        // Consolidate 4 canonical 16x16 V1->V4 geometric prototypes (`0=visbox, 1=vistri, 2=viscross, 3=visring`) into L2 PW24 via evolved STDP!
        Word tot_ed = 0u;
        for (Word p = 0; p < 4u; ++p) {
            NeoVisualCortexQuery q{};
            q.pathway = 24u;
            q.mode = 0u;
            q.clear_first = (p == 0u) ? 1u : 0u;
            q.proto_id = p;
            fill_retina_pattern(q.pixels, p, 0, 0, false);
            tot_ed += gpu_neo_visual_cortex(q, tr).edits_made;
        }
        return tot_ed;
    }

    int vis_concept_kind(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_VISBOX]) return 0;
        if (cid == markers[LM_VISTRI]) return 1;
        if (cid == markers[LM_VISCROSS]) return 2;
        if (cid == markers[LM_VISRING]) return 3;
        if (cid == markers[LM_VISLSHAPE]) return 4;
        if (cid == markers[LM_VISDUAL]) return 5;
        if (cid == markers[LM_VISSCENE]) return 6;
        return -1;
    }

    Word vis_proto_concept(Word pid) const {
        if (pid == 0u) return markers[LM_VISBOX];
        if (pid == 1u) return markers[LM_VISTRI];
        if (pid == 2u) return markers[LM_VISCROSS];
        if (pid == 3u) return markers[LM_VISRING];
        if (pid == 4u) return markers[LM_VISLSHAPE];
        return 0u;
    }

    // Kernel 32 (STAGE 6b!): Pure-GPU 16x16 Spiking Cochlear Audio, Harmonic Chord & Emotional Prosody Cortex on L2 PW27!
    static void fill_cochlea_pattern(Word spectro[8], Word kind, int dt = 0, int df = 0, bool add_noise = false) {
        for (Word i = 0; i < 8u; ++i) spectro[i] = 0u;
        auto set_tf = [&](int t, int f) {
            int nt = t + dt, nf = f + df;
            if (nt < 0 || nt >= 16 || nf < 0 || nf >= 16) return;
            Word idx = Word(nt) * 16u + Word(nf);
            spectro[idx >> 5] |= 1u << (idx & 31u);
        };
        if (kind == 0u) {
            // 0 = audrise: questioning rising pitch sweep (`f = 2 -> 12` over `t = 2..13`)
            for (int t = 2; t <= 13; ++t) {
                int f = 2 + ((t - 2) * 10) / 11;
                set_tf(t, f);
            }
        } else if (kind == 1u) {
            // 1 = audfall: declarative falling pitch sweep (`f = 12 -> 2` over `t = 2..13`)
            for (int t = 2; t <= 13; ++t) {
                int f = 12 - ((t - 2) * 10) / 11;
                set_tf(t, f);
            }
        } else if (kind == 2u) {
            // 2 = audmajor: warm consonant major triad (`f = 3, 7, 10`, intervals +4, +3, +7!)
            for (int t = 2; t <= 13; ++t) {
                set_tf(t, 3);
                set_tf(t, 7);
                set_tf(t, 10);
            }
        } else if (kind == 3u) {
            // 3 = audtritone: dissonant tritone + semitone alarm cluster (`f = 4, 5, 10, 11`, Δ=1 & Δ=6!)
            for (int t = 2; t <= 13; ++t) {
                set_tf(t, 4);
                set_tf(t, 5);
                set_tf(t, 10);
                set_tf(t, 11);
            }
        } else if (kind == 4u) {
            // 4 = audmarch: 3-beat staccato rhythmic march/gallop (`t={1..2},{6..7},{11..12}`, 3 beats!)
            for (int b = 0; b < 3; ++b) {
                int t0 = 1 + b * 5;
                for (int dtb = 0; dtb < 2; ++dtb) {
                    set_tf(t0 + dtb, 3);
                    set_tf(t0 + dtb, 4);
                    if (b == 0) set_tf(t0 + dtb, 8);
                }
            }
        }
        if (add_noise) {
            // Single isolated stray background hair-cell glitch at `(t=15, f=15)` — 256-thread GPU lateral inhibition suppresses it!
            Word idx = 15u * 16u + 15u;
            spectro[idx >> 5] |= 1u << (idx & 31u);
        }
    }

    NeoAudioCortexResult gpu_neo_audio_cortex(const NeoAudioCortexQuery &q, CognitiveTrace *tr = nullptr) {
        NeoAudioCortexResult res{};
        if (!session || abl_no_consolidate) return res;
        if (q.mode == 0u && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_aud_q->ptr, &q, sizeof(NeoAudioCortexQuery)), "upload audio cortex query");
        void *args[]{&d_neo_weights->ptr, &d_aud_q->ptr, &d_genome->ptr, &d_aud_res->ptr};
        session->d.check(session->d.launch(fn_neo_audio_cortex, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo audio cortex");
        session->d.check(session->d.download(&res, d_aud_res->ptr, sizeof(NeoAudioCortexResult)), "download audio cortex");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync audio cortex neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k32_audio_cortex(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        // Consolidate 4 canonical 16x16 A1->Belt auditory/prosody prototypes (`0=audrise, 1=audfall, 2=audmajor, 3=audtritone`) into L2 PW27 via evolved STDP!
        Word tot_ed = 0u;
        for (Word p = 0; p < 4u; ++p) {
            NeoAudioCortexQuery q{};
            q.pathway = 27u;
            q.mode = 0u;
            q.clear_first = (p == 0u) ? 1u : 0u;
            q.proto_id = p;
            fill_cochlea_pattern(q.spectro, p, 0, 0, false);
            tot_ed += gpu_neo_audio_cortex(q, tr).edits_made;
        }
        return tot_ed;
    }

    int aud_concept_kind(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_AUDRISE]) return 0;
        if (cid == markers[LM_AUDFALL]) return 1;
        if (cid == markers[LM_AUDMAJOR]) return 2;
        if (cid == markers[LM_AUDTRITONE]) return 3;
        if (cid == markers[LM_AUDMARCH]) return 4;
        return -1;
    }

    Word aud_proto_concept(Word pid) const {
        if (pid == 0u) return markers[LM_AUDRISE];
        if (pid == 1u) return markers[LM_AUDFALL];
        if (pid == 2u) return markers[LM_AUDMAJOR];
        if (pid == 3u) return markers[LM_AUDTRITONE];
        if (pid == 4u) return markers[LM_AUDMARCH];
        return 0u;
    }

    // Kernel 33 (STAGE 6c!): Pure-GPU Spiking Cross-Modal Vision+Audio Binding & Prefrontal Goal Stack on L2 PW24[768..1023]!
    NeoCrossModalStackResult gpu_neo_crossmodal_stack(const NeoCrossModalStackQuery &q, CognitiveTrace *tr = nullptr) {
        NeoCrossModalStackResult res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 3u || q.mode == 4u || q.mode == 6u) && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_xms_q->ptr, &q, sizeof(NeoCrossModalStackQuery)), "upload crossmodal stack query");
        void *args[]{&d_neo_weights->ptr, &d_xms_q->ptr, &d_genome->ptr, &d_xms_res->ptr};
        session->d.check(session->d.launch(fn_neo_crossmodal_stack, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo crossmodal stack");
        session->d.check(session->d.download(&res, d_xms_res->ptr, sizeof(NeoCrossModalStackResult)), "download crossmodal stack");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync crossmodal stack neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k33_crossmodal_stack(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        // Consolidate 4 canonical multimodal Vision(K31)+Audio(K32) associations into L2 PW24[768..895] via evolved STDP:
        // 0=xbeacon(visring=3, audmajor=2), 1=xhazard(viscross=2, audtritone=3),
        // 2=xprobe(vistri=1, audrise=0),    3=xvault(visbox=0, audfall=1)!
        static constexpr Word pairs[4][2] = {{3u, 2u}, {2u, 3u}, {1u, 0u}, {0u, 1u}};
        Word tot_ed = 0u;
        for (Word p = 0; p < 4u; ++p) {
            NeoCrossModalStackQuery q{};
            q.pathway = 24u;
            q.mode = 0u;
            q.clear_first = (p == 0u) ? 1u : 0u;
            q.xproto_id = p;
            q.vis_id = pairs[p][0];
            q.aud_id = pairs[p][1];
            tot_ed += gpu_neo_crossmodal_stack(q, tr).edits_made;
        }
        // Clear prefrontal goal stack rows `896..1023` cleanly!
        NeoCrossModalStackQuery clr{};
        clr.pathway = 24u; clr.mode = 6u;
        tot_ed += gpu_neo_crossmodal_stack(clr, tr).edits_made;
        return tot_ed;
    }

    Word xmodal_proto_concept(Word pid) const {
        if (pid == 0u) return markers[LM_XBEACON];
        if (pid == 1u) return markers[LM_XHAZARD];
        if (pid == 2u) return markers[LM_XPROBE];
        if (pid == 3u) return markers[LM_XVAULT];
        if (pid == 4u) return markers[LM_XCONVOY];
        return 0u;
    }

    int goal_stack_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_GOALMAP]) return 0;
        if (cid == markers[LM_GOALFIX]) return 1;
        if (cid == markers[LM_GOALRUN]) return 2;
        return -1;
    }

    Word goal_stack_concept(Word gid) const {
        if (gid == 0u) return markers[LM_GOALMAP];
        if (gid == 1u) return markers[LM_GOALFIX];
        if (gid == 2u) return markers[LM_GOALRUN];
        return 0u;
    }

    // Kernel 34 (STAGE 7 CROSSING!): Pure-GPU Spiking Multi-Turn Dialogue Coherence, Topic-Shift & Contradiction Cortex on L2 PW27[768..1023]!
    NeoDialogueResult gpu_neo_dialogue_cortex(const NeoDialogueQuery &q, CognitiveTrace *tr = nullptr) {
        NeoDialogueResult res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 2u || q.mode == 4u) && abl_freeze) return res;
        NeoDialogueQuery q_eff = q;
        if (q_eff.mode == 1u && abl_freeze) q_eff.read_only = 1u;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_dlg_q->ptr, &q_eff, sizeof(NeoDialogueQuery)), "upload dialogue cortex query");
        void *args[]{&d_neo_weights->ptr, &d_dlg_q->ptr, &d_genome->ptr, &d_dlg_res->ptr};
        session->d.check(session->d.launch(fn_neo_dialogue_cortex, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo dialogue cortex");
        session->d.check(session->d.download(&res, d_dlg_res->ptr, sizeof(NeoDialogueResult)), "download dialogue cortex");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync dialogue cortex neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k34_dialogue_cortex(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        // Consolidate 4 canonical conversational pragmatic modes into L2 PW27[768..895] via evolved STDP:
        // 0=dlgexplain(cuefact=0, cuewhy=1), 1=dlgempathize(cuefeel=2, cuestory=6),
        // 2=dlgplan(cueplan=3, cuefact=0),   3=dlgcompare(cuecomp=4, cuewhy=1)!
        static constexpr Word pairs[4][2] = {{0u, 1u}, {2u, 6u}, {3u, 0u}, {4u, 1u}};
        Word tot_ed = 0u;
        for (Word m = 0; m < 4u; ++m) {
            NeoDialogueQuery q{};
            q.pathway = 27u;
            q.mode = 0u;
            q.clear_first = (m == 0u) ? 1u : 0u;
            q.dlg_mode = m;
            q.cue_a = pairs[m][0];
            q.cue_b = pairs[m][1];
            tot_ed += gpu_neo_dialogue_cortex(q, tr).edits_made;
        }
        return tot_ed;
    }

    int dlg_cue_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_CUEFACT]) return 0;
        if (cid == markers[LM_CUEWHY]) return 1;
        if (cid == markers[LM_CUEFEEL]) return 2;
        if (cid == markers[LM_CUEPLAN]) return 3;
        if (cid == markers[LM_CUECOMP]) return 4;
        if (cid == markers[LM_CUEDILEMMA]) return 5;
        if (cid == markers[LM_CUESTORY]) return 6;
        if (cid == markers[LM_CUEARGUE]) return 7;
        return -1;
    }

    int dlg_mode_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_DLGEXPLAIN]) return 0;
        if (cid == markers[LM_DLGEMPATHIZE]) return 1;
        if (cid == markers[LM_DLGPLAN]) return 2;
        if (cid == markers[LM_DLGCOMPARE]) return 3;
        if (cid == markers[LM_DLGDEBATE]) return 4;
        return -1;
    }

    Word dlg_mode_concept(Word mid) const {
        if (mid == 0u) return markers[LM_DLGEXPLAIN];
        if (mid == 1u) return markers[LM_DLGEMPATHIZE];
        if (mid == 2u) return markers[LM_DLGPLAN];
        if (mid == 3u) return markers[LM_DLGCOMPARE];
        if (mid == 4u) return markers[LM_DLGDEBATE];
        return 0u;
    }

    // Kernel 35 (STAGE 7b!): Pure-GPU Spiking Metaphor Blending, 2-Hop Narrative Plot Arc & Socratic Self-Critique Cortex on L2 PW27[256..511]!
    NeoNarrativeMetaphorResult gpu_neo_narrative_metaphor(const NeoNarrativeMetaphorQuery &q, CognitiveTrace *tr = nullptr) {
        NeoNarrativeMetaphorResult res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 2u) && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_nmet_q->ptr, &q, sizeof(NeoNarrativeMetaphorQuery)), "upload narrative metaphor query");
        void *args[]{&d_neo_weights->ptr, &d_nmet_q->ptr, &d_genome->ptr, &d_nmet_res->ptr};
        session->d.check(session->d.launch(fn_neo_narrative_metaphor, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo narrative metaphor");
        session->d.check(session->d.download(&res, d_nmet_res->ptr, sizeof(NeoNarrativeMetaphorResult)), "download narrative metaphor");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync narrative metaphor neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k35_narrative_metaphor(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        Word tot_ed = 0u;
        // 1. Sub-Region A (`rows 256..383`): 4 canonical cross-domain conceptual metaphor blends imprinted via evolved STDP!
        //    0=metriver(mflow=0, mtime=1), 1=metweb(mlink=2, mmind=3),
        //    2=metspark(mfire=4, midea=5), 3=metmirror(mglass=6, mtruth=7)
        static constexpr Word blend_pairs[4][2] = {{0u, 1u}, {2u, 3u}, {4u, 5u}, {6u, 7u}};
        for (Word b = 0; b < 4u; ++b) {
            NeoNarrativeMetaphorQuery q{};
            q.pathway = 27u; q.mode = 0u; q.clear_first = (b == 0u) ? 1u : 0u;
            q.slot_id = b; q.cue_a = blend_pairs[b][0]; q.cue_b = blend_pairs[b][1];
            tot_ed += gpu_neo_narrative_metaphor(q, tr).edits_made;
        }
        // 2. Sub-Region B1 (`rows 384..447`): 3 narrative arc transitions (`plotquest(0)->plottrial(1)->plotturn(2)->plotwin(3)`)!
        static constexpr Word arc_edges[3][2] = {{0u, 1u}, {1u, 2u}, {2u, 3u}};
        for (Word e = 0; e < 3u; ++e) {
            NeoNarrativeMetaphorQuery q{};
            q.pathway = 27u; q.mode = 2u; q.clear_first = (e == 0u) ? 1u : 0u;
            q.slot_id = arc_edges[e][1]; q.cue_a = arc_edges[e][0]; q.cue_b = ~0u;
            tot_ed += gpu_neo_narrative_metaphor(q, tr).edits_made;
        }
        // 3. Sub-Region B2 (`rows 448..511`): 4 Socratic self-critique flaw patterns (`slots 4..7`)!
        //    4=flawhasty(flawcase=4, flawall=5), 5=flawcircle(flawself=6, flawcase=4),
        //    6=flawfalse(flawboth=7, flawall=5), 7=flawstraw(flawshift=8, flawself=6)
        static constexpr Word flaw_pairs[4][2] = {{4u, 5u}, {6u, 4u}, {7u, 5u}, {8u, 6u}};
        for (Word f = 0; f < 4u; ++f) {
            NeoNarrativeMetaphorQuery q{};
            q.pathway = 27u; q.mode = 2u; q.clear_first = 0u;
            q.slot_id = 4u + f; q.cue_a = flaw_pairs[f][0]; q.cue_b = flaw_pairs[f][1];
            tot_ed += gpu_neo_narrative_metaphor(q, tr).edits_made;
        }
        return tot_ed;
    }

    int met_feat_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_MFLOW]) return 0;
        if (cid == markers[LM_MTIME]) return 1;
        if (cid == markers[LM_MLINK]) return 2;
        if (cid == markers[LM_MMIND]) return 3;
        if (cid == markers[LM_MFIRE]) return 4;
        if (cid == markers[LM_MIDEA]) return 5;
        if (cid == markers[LM_MGLASS]) return 6;
        if (cid == markers[LM_MTRUTH]) return 7;
        if (cid == markers[LM_MNORTH]) return 8;
        if (cid == markers[LM_MGOAL]) return 9;
        return -1;
    }

    Word met_feat_concept(Word fid) const {
        static constexpr Word tbl[10] = {LM_MFLOW, LM_MTIME, LM_MLINK, LM_MMIND, LM_MFIRE, LM_MIDEA, LM_MGLASS, LM_MTRUTH, LM_MNORTH, LM_MGOAL};
        return fid < 10u ? markers[tbl[fid]] : 0u;
    }

    int met_blend_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_METRIVER]) return 0;
        if (cid == markers[LM_METWEB]) return 1;
        if (cid == markers[LM_METSPARK]) return 2;
        if (cid == markers[LM_METMIRROR]) return 3;
        if (cid == markers[LM_METCOMPASS]) return 4;
        return -1;
    }

    Word met_blend_concept(Word bid) const {
        if (bid == 0u) return markers[LM_METRIVER];
        if (bid == 1u) return markers[LM_METWEB];
        if (bid == 2u) return markers[LM_METSPARK];
        if (bid == 3u) return markers[LM_METMIRROR];
        if (bid == 4u) return markers[LM_METCOMPASS];
        return 0u;
    }

    int plot_beat_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_PLOTQUEST]) return 0;
        if (cid == markers[LM_PLOTTRIAL]) return 1;
        if (cid == markers[LM_PLOTTURN]) return 2;
        if (cid == markers[LM_PLOTWIN]) return 3;
        return -1;
    }

    Word plot_beat_concept(Word bid) const {
        if (bid == 0u) return markers[LM_PLOTQUEST];
        if (bid == 1u) return markers[LM_PLOTTRIAL];
        if (bid == 2u) return markers[LM_PLOTTURN];
        if (bid == 3u) return markers[LM_PLOTWIN];
        return 0u;
    }

    int flaw_cue_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_FLAWCASE]) return 4;
        if (cid == markers[LM_FLAWALL]) return 5;
        if (cid == markers[LM_FLAWSELF]) return 6;
        if (cid == markers[LM_FLAWBOTH]) return 7;
        if (cid == markers[LM_FLAWSHIFT]) return 8;
        return -1;
    }

    Word flaw_concept(Word fid) const {
        if (fid == 0u) return markers[LM_FLAWHASTY];
        if (fid == 1u) return markers[LM_FLAWCIRCLE];
        if (fid == 2u) return markers[LM_FLAWFALSE];
        if (fid == 3u) return markers[LM_FLAWSTRAW];
        return 0u;
    }

    // Kernel 36 (STAGE 7c — 400 DOMAINS!): Pure-GPU Spiking Epistemic Belief-Graph, Evidence-Weighting & Multi-Agent Scientific Debate Cortex on L2 PW24[256..511]!
    NeoBeliefDebateResult gpu_neo_belief_debate(const NeoBeliefDebateQuery &q, CognitiveTrace *tr = nullptr) {
        NeoBeliefDebateResult res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 2u || q.mode == 4u) && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_bdeb_q->ptr, &q, sizeof(NeoBeliefDebateQuery)), "upload belief debate query");
        void *args[]{&d_neo_weights->ptr, &d_bdeb_q->ptr, &d_genome->ptr, &d_bdeb_res->ptr};
        session->d.check(session->d.launch(fn_neo_belief_debate, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo belief debate");
        session->d.check(session->d.download(&res, d_bdeb_res->ptr, sizeof(NeoBeliefDebateResult)), "download belief debate");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync belief debate neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k36_belief_debate(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        Word tot_ed = 0u;
        // 0. Clean-clear orthogonal L2 PW24[256..511] (`pre 512..767` + diagonals) before consolidating canonical belief graph!
        {
            NeoBeliefDebateQuery q{};
            q.pathway = 24u; q.mode = 4u;
            tot_ed += gpu_neo_belief_debate(q, tr).edits_made;
        }
        // 1. Sub-Region A (`rows 256..383`): 3 canonical scientific theories via evolved STDP (`g.basal_rule`)!
        //    0=theorywave(evlens=0, evprism=1, +support), 1=theorydust(evlens=0 +support, evvoid=2 -refute),
        //    2=theoryfield(evvoid=2, evpulse=3, +support)
        {
            NeoBeliefDebateQuery q{};
            q.pathway = 24u; q.mode = 0u; q.claim_id = 0u; q.ev_a = 0u; q.ev_b = 1u; q.polarity = 1u;
            tot_ed += gpu_neo_belief_debate(q, tr).edits_made;
        }
        {
            NeoBeliefDebateQuery q{};
            q.pathway = 24u; q.mode = 0u; q.claim_id = 1u; q.ev_a = 0u; q.ev_b = ~0u; q.polarity = 1u;
            tot_ed += gpu_neo_belief_debate(q, tr).edits_made;
            q.ev_a = 2u; q.ev_b = ~0u; q.polarity = 0u;
            tot_ed += gpu_neo_belief_debate(q, tr).edits_made;
        }
        {
            NeoBeliefDebateQuery q{};
            q.pathway = 24u; q.mode = 0u; q.claim_id = 2u; q.ev_a = 2u; q.ev_b = 3u; q.polarity = 1u;
            tot_ed += gpu_neo_belief_debate(q, tr).edits_made;
        }
        // 2. Sub-Region B (`rows 384..511`): Peer source-credibility registers (`0=peerada` verified 2x, `1=peerloki` bluffed 2x)!
        for (Word k = 0; k < 2u; ++k) {
            NeoBeliefDebateQuery qa{}; qa.pathway = 24u; qa.mode = 2u; qa.peer_a = 0u; qa.polarity = 1u;
            tot_ed += gpu_neo_belief_debate(qa, tr).edits_made;
            NeoBeliefDebateQuery ql{}; ql.pathway = 24u; ql.mode = 2u; ql.peer_a = 1u; ql.polarity = 0u;
            tot_ed += gpu_neo_belief_debate(ql, tr).edits_made;
        }
        return tot_ed;
    }

    int bel_theory_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_THEORYWAVE]) return 0;
        if (cid == markers[LM_THEORYDUST]) return 1;
        if (cid == markers[LM_THEORYFIELD]) return 2;
        if (cid == markers[LM_THEORYFLUX]) return 3;
        return -1;
    }

    Word bel_theory_concept(Word tid) const {
        if (tid == 0u) return markers[LM_THEORYWAVE];
        if (tid == 1u) return markers[LM_THEORYDUST];
        if (tid == 2u) return markers[LM_THEORYFIELD];
        if (tid == 3u) return markers[LM_THEORYFLUX];
        return 0u;
    }

    int bel_ev_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_EVLENS]) return 0;
        if (cid == markers[LM_EVPRISM]) return 1;
        if (cid == markers[LM_EVVOID]) return 2;
        if (cid == markers[LM_EVPULSE]) return 3;
        if (cid == markers[LM_EVRAY]) return 4;
        return -1;
    }

    int bel_peer_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_PEERADA]) return 0;
        if (cid == markers[LM_PEERLOKI]) return 1;
        return -1;
    }

    Word bel_peer_concept(Word pid) const {
        if (pid == 0u) return markers[LM_PEERADA];
        if (pid == 1u) return markers[LM_PEERLOKI];
        return 0u;
    }

    // Kernel 37 (STAGE 8 CROSSING — COLLECTIVE SCIENTIFIC MINDS!): Pure-GPU Multi-Mind Collective Discovery & Compositional Theorem Prover on L2 PW24[0..255]!
    NeoCollectiveDiscoveryResult gpu_neo_collective_discovery(const NeoCollectiveDiscoveryQuery &q, CognitiveTrace *tr = nullptr) {
        NeoCollectiveDiscoveryResult res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 2u || q.mode == 4u) && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_cdis_q->ptr, &q, sizeof(NeoCollectiveDiscoveryQuery)), "upload collective discovery query");
        void *args[]{&d_neo_weights->ptr, &d_cdis_q->ptr, &d_genome->ptr, &d_cdis_res->ptr};
        session->d.check(session->d.launch(fn_neo_collective_discovery, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo collective discovery");
        session->d.check(session->d.download(&res, d_cdis_res->ptr, sizeof(NeoCollectiveDiscoveryResult)), "download collective discovery");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync collective discovery neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k37_collective_discovery(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        Word tot_ed = 0u;
        // 0. Clean-clear orthogonal L2 PW24[0..255] (`pre 768..1023` + diagonals `0..255`)!
        {
            NeoCollectiveDiscoveryQuery q{};
            q.pathway = 24u; q.mode = 4u;
            tot_ed += gpu_neo_collective_discovery(q, tr).edits_made;
        }
        // 1. Sub-Region A (`rows 0..127`): 3 canonical collective conjectures via evolved STDP (`g.basal_rule`)!
        //    0=lawthermo (specphys=0 + specmath=2 -> +support),
        //    1=lawgene   (specbio=1 + specmath=2 -> +support),
        //    2=lawether  (specphys=0 + speclogic=3 -> -refutation!)
        {
            NeoCollectiveDiscoveryQuery q{};
            q.pathway = 24u; q.mode = 0u; q.law_id = 0u; q.cue_a = 0u; q.cue_b = 2u; q.polarity = 1u;
            tot_ed += gpu_neo_collective_discovery(q, tr).edits_made;
            q.law_id = 1u; q.cue_a = 1u; q.cue_b = 2u; q.polarity = 1u;
            tot_ed += gpu_neo_collective_discovery(q, tr).edits_made;
            q.law_id = 2u; q.cue_a = 0u; q.cue_b = 3u; q.polarity = 0u;
            tot_ed += gpu_neo_collective_discovery(q, tr).edits_made;
        }
        // 2. Sub-Region B (`rows 128..255`): 3 canonical compositional theorems via evolved STDP!
        //    0=thmcarnot  (axenergy=4 + axentropy=5),
        //    1=thmforce   (axmass=6 + axaccel=7),
        //    2=thmmaxwell (axcharge=8 + axflux=9)
        {
            NeoCollectiveDiscoveryQuery q{};
            q.pathway = 24u; q.mode = 2u; q.thm_id = 0u; q.cue_a = 4u; q.cue_b = 5u;
            tot_ed += gpu_neo_collective_discovery(q, tr).edits_made;
            q.thm_id = 1u; q.cue_a = 6u; q.cue_b = 7u;
            tot_ed += gpu_neo_collective_discovery(q, tr).edits_made;
            q.thm_id = 2u; q.cue_a = 8u; q.cue_b = 9u;
            tot_ed += gpu_neo_collective_discovery(q, tr).edits_made;
        }
        return tot_ed;
    }

    int coll_law_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_LAWTHERMO]) return 0;
        if (cid == markers[LM_LAWGENE]) return 1;
        if (cid == markers[LM_LAWETHER]) return 2;
        if (cid == markers[LM_LAWQUANTUM]) return 3;
        return -1;
    }

    Word coll_law_concept(Word lid) const {
        if (lid == 0u) return markers[LM_LAWTHERMO];
        if (lid == 1u) return markers[LM_LAWGENE];
        if (lid == 2u) return markers[LM_LAWETHER];
        if (lid == 3u) return markers[LM_LAWQUANTUM];
        return 0u;
    }

    int coll_spec_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_SPECPHYS]) return 0;
        if (cid == markers[LM_SPECBIO]) return 1;
        if (cid == markers[LM_SPECMATH]) return 2;
        if (cid == markers[LM_SPECLOGIC]) return 3;
        return -1;
    }

    int coll_thm_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_THMCARNOT]) return 0;
        if (cid == markers[LM_THMFORCE]) return 1;
        if (cid == markers[LM_THMMAXWELL]) return 2;
        if (cid == markers[LM_THMDIRAC]) return 3;
        return -1;
    }

    Word coll_thm_concept(Word tid) const {
        if (tid == 0u) return markers[LM_THMCARNOT];
        if (tid == 1u) return markers[LM_THMFORCE];
        if (tid == 2u) return markers[LM_THMMAXWELL];
        if (tid == 3u) return markers[LM_THMDIRAC];
        return 0u;
    }

    int coll_ax_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_AXENERGY]) return 4;
        if (cid == markers[LM_AXENTROPY]) return 5;
        if (cid == markers[LM_AXMASS]) return 6;
        if (cid == markers[LM_AXACCEL]) return 7;
        if (cid == markers[LM_AXCHARGE]) return 8;
        if (cid == markers[LM_AXFLUX]) return 9;
        if (cid == markers[LM_AXWAVE]) return 10;
        if (cid == markers[LM_AXSPIN]) return 11;
        return -1;
    }

    Word coll_ax_concept(Word aid) const {
        if (aid == 4u) return markers[LM_AXENERGY];
        if (aid == 5u) return markers[LM_AXENTROPY];
        if (aid == 6u) return markers[LM_AXMASS];
        if (aid == 7u) return markers[LM_AXACCEL];
        if (aid == 8u) return markers[LM_AXCHARGE];
        if (aid == 9u) return markers[LM_AXFLUX];
        if (aid == 10u) return markers[LM_AXWAVE];
        if (aid == 11u) return markers[LM_AXSPIN];
        return 0u;
    }

    // Kernel 38 (STAGE 8b — CONTINUAL SELF-RESEARCH!): Pure-GPU Discriminative Experiment-Design & Counterfactual Code-Patching Cortex on L2 PW27[0..255]!
    NeoSelfResearchResult gpu_neo_self_research(const NeoSelfResearchQuery &q, CognitiveTrace *tr = nullptr) {
        NeoSelfResearchResult res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 2u || q.mode == 4u) && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_sres_q->ptr, &q, sizeof(NeoSelfResearchQuery)), "upload self research query");
        void *args[]{&d_neo_weights->ptr, &d_sres_q->ptr, &d_genome->ptr, &d_sres_res->ptr};
        session->d.check(session->d.launch(fn_neo_self_research, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo self research");
        session->d.check(session->d.download(&res, d_sres_res->ptr, sizeof(NeoSelfResearchResult)), "download self research");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync self research neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k38_self_research(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        Word tot_ed = 0u;
        // 0. Clean-clear orthogonal L2 PW27[0..255] (`pre 768..1023` + diagonals `0..255`)!
        {
            NeoSelfResearchQuery q{};
            q.pathway = 27u; q.mode = 4u;
            tot_ed += gpu_neo_self_research(q, tr).edits_made;
        }
        // 1. Sub-Region A (`rows 0..127`): 4 canonical discriminative experiments via evolved STDP (`g.basal_rule`)!
        //    0=expvacuum (hypgrav=0 +24 vs hypdrag=1 -24),
        //    1=expcoil   (hypmag=2  +24 vs hypheat=3 -24),
        //    2=expprism  (hypwave=4 +24 vs hyppart=5 -24),
        //    3=explitmus (hypacid=6 +24 vs hypbase=7 -24)
        {
            NeoSelfResearchQuery q{};
            q.pathway = 27u; q.mode = 0u; q.polarity = 2u;
            q.exp_id = 0u; q.cue_a = 0u; q.cue_b = 1u; tot_ed += gpu_neo_self_research(q, tr).edits_made;
            q.exp_id = 1u; q.cue_a = 2u; q.cue_b = 3u; tot_ed += gpu_neo_self_research(q, tr).edits_made;
            q.exp_id = 2u; q.cue_a = 4u; q.cue_b = 5u; tot_ed += gpu_neo_self_research(q, tr).edits_made;
            q.exp_id = 3u; q.cue_a = 6u; q.cue_b = 7u; tot_ed += gpu_neo_self_research(q, tr).edits_made;
        }
        // 2. Sub-Region B (`rows 128..255`): 4 canonical counterfactual repair patches via evolved STDP!
        //    0=patbound (symoffone=8 + syminfloop=9),
        //    1=patguard (symnull=10 + symdrift=15),
        //    2=patmutex (symrace=11 + symdeadlk=14),
        //    3=patclamp (symoverfl=13 + symdrift=15)
        {
            NeoSelfResearchQuery q{};
            q.pathway = 27u; q.mode = 2u;
            q.patch_id = 0u; q.cue_a = 8u;  q.cue_b = 9u;  tot_ed += gpu_neo_self_research(q, tr).edits_made;
            q.patch_id = 1u; q.cue_a = 10u; q.cue_b = 15u; tot_ed += gpu_neo_self_research(q, tr).edits_made;
            q.patch_id = 2u; q.cue_a = 11u; q.cue_b = 14u; tot_ed += gpu_neo_self_research(q, tr).edits_made;
            q.patch_id = 3u; q.cue_a = 13u; q.cue_b = 15u; tot_ed += gpu_neo_self_research(q, tr).edits_made;
        }
        return tot_ed;
    }

    int sr_hyp_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_HYPGRAV]) return 0;
        if (cid == markers[LM_HYPDRAG]) return 1;
        if (cid == markers[LM_HYPMAG]) return 2;
        if (cid == markers[LM_HYPHEAT]) return 3;
        if (cid == markers[LM_HYPWAVE]) return 4;
        if (cid == markers[LM_HYPPART]) return 5;
        if (cid == markers[LM_HYPACID]) return 6;
        if (cid == markers[LM_HYPBASE]) return 7;
        return -1;
    }

    int sr_exp_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_EXPVACUUM]) return 0;
        if (cid == markers[LM_EXPCOIL]) return 1;
        if (cid == markers[LM_EXPPRISM]) return 2;
        if (cid == markers[LM_EXPLITMUS]) return 3;
        if (cid == markers[LM_EXPSPECTRUM]) return 4;
        return -1;
    }

    Word sr_exp_concept(Word eid) const {
        if (eid == 0u) return markers[LM_EXPVACUUM];
        if (eid == 1u) return markers[LM_EXPCOIL];
        if (eid == 2u) return markers[LM_EXPPRISM];
        if (eid == 3u) return markers[LM_EXPLITMUS];
        if (eid == 4u) return markers[LM_EXPSPECTRUM];
        return 0u;
    }

    int sr_sym_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_SYMOFFONE]) return 8;
        if (cid == markers[LM_SYMINFLOOP]) return 9;
        if (cid == markers[LM_SYMNULL]) return 10;
        if (cid == markers[LM_SYMRACE]) return 11;
        if (cid == markers[LM_SYMLEAK]) return 12;
        if (cid == markers[LM_SYMOVERFL]) return 13;
        if (cid == markers[LM_SYMDEADLK]) return 14;
        if (cid == markers[LM_SYMDRIFT]) return 15;
        return -1;
    }

    Word sr_sym_concept(Word sid) const {
        if (sid == 8u) return markers[LM_SYMOFFONE];
        if (sid == 9u) return markers[LM_SYMINFLOOP];
        if (sid == 10u) return markers[LM_SYMNULL];
        if (sid == 11u) return markers[LM_SYMRACE];
        if (sid == 12u) return markers[LM_SYMLEAK];
        if (sid == 13u) return markers[LM_SYMOVERFL];
        if (sid == 14u) return markers[LM_SYMDEADLK];
        if (sid == 15u) return markers[LM_SYMDRIFT];
        return 0u;
    }

    int sr_patch_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_PATBOUND]) return 0;
        if (cid == markers[LM_PATGUARD]) return 1;
        if (cid == markers[LM_PATMUTEX]) return 2;
        if (cid == markers[LM_PATCLAMP]) return 3;
        if (cid == markers[LM_PATFREE]) return 4;
        return -1;
    }

    Word sr_patch_concept(Word pid) const {
        if (pid == 0u) return markers[LM_PATBOUND];
        if (pid == 1u) return markers[LM_PATGUARD];
        if (pid == 2u) return markers[LM_PATMUTEX];
        if (pid == 3u) return markers[LM_PATCLAMP];
        if (pid == 4u) return markers[LM_PATFREE];
        return 0u;
    }

    // Kernel 39 (STAGE 8c — PURE-GPU NEURAL SYMBOLIC CALCULUS!): Pure-GPU Spiking Polynomial Differentiation, Integration & Evaluation on L2 PW13[0..255, 0..255]!
    NeoSymCalcResult gpu_neo_symbolic_calculus(const NeoSymCalcQuery &q, CognitiveTrace *tr = nullptr) {
        NeoSymCalcResult res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 4u) && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_symc_q->ptr, &q, sizeof(NeoSymCalcQuery)), "upload sym calc query");
        void *args[]{&d_neo_weights->ptr, &d_symc_q->ptr, &d_genome->ptr, &d_symc_res->ptr};
        session->d.check(session->d.launch(fn_neo_symbolic_calculus, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo sym calc");
        session->d.check(session->d.download(&res, d_symc_res->ptr, sizeof(NeoSymCalcResult)), "download sym calc");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync sym calc neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k39_symbolic_calculus(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        Word tot_ed = 0u;
        // 0. Clean-clear orthogonal quadrant `PW13[0..255, 0..255]` (100% disjoint from K30 3-SAT!)
        {
            NeoSymCalcQuery q{};
            q.pathway = 13u; q.mode = 4u;
            tot_ed += gpu_neo_symbolic_calculus(q, tr).edits_made;
        }
        // 1. Consolidate 3 canonical symbolic polynomials directly into L2 PW13 via evolved STDP (`g.basal_rule`):
        //    0=polyalpha: P0(x) = 2 + 6x + 3x^2       -> P0'(2)=18, \int_0^2 P0=24, P0(2)=26
        //    1=polybeta:  P1(x) = 3 + 4x + 6x^2 + 4x^3 -> P1'(1)=28, \int_0^1 P1=8,  P1(1)=17
        //    2=polygamma: P2(x) = 1 + 2x + 3x^2       -> P2'(3)=20, \int_0^2 P2=14, P2(3)=34
        {
            NeoSymCalcQuery q{};
            q.pathway = 13u; q.mode = 0u;
            q.poly_id = 0u; q.coeffs[0] = 2u; q.coeffs[1] = 6u; q.coeffs[2] = 3u; q.coeffs[3] = 0u;
            tot_ed += gpu_neo_symbolic_calculus(q, tr).edits_made;
            q.poly_id = 1u; q.coeffs[0] = 3u; q.coeffs[1] = 4u; q.coeffs[2] = 6u; q.coeffs[3] = 4u;
            tot_ed += gpu_neo_symbolic_calculus(q, tr).edits_made;
            q.poly_id = 2u; q.coeffs[0] = 1u; q.coeffs[1] = 2u; q.coeffs[2] = 3u; q.coeffs[3] = 0u;
            tot_ed += gpu_neo_symbolic_calculus(q, tr).edits_made;
        }
        return tot_ed;
    }

    int sc_poly_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_POLYALPHA]) return 0;
        if (cid == markers[LM_POLYBETA]) return 1;
        if (cid == markers[LM_POLYGAMMA]) return 2;
        if (cid == markers[LM_POLYOMEGA]) return 3;
        return -1;
    }

    Word sc_poly_concept(Word pid) const {
        if (pid == 0u) return markers[LM_POLYALPHA];
        if (pid == 1u) return markers[LM_POLYBETA];
        if (pid == 2u) return markers[LM_POLYGAMMA];
        if (pid == 3u) return markers[LM_POLYOMEGA];
        return 0u;
    }

    // Kernel 40 (STAGE 8d — PURE-GPU AST PROGRAM SYNTHESIS, TYPE VERIFICATION & SYMBOLIC EXECUTION!):
    // Lives on FREE Orthogonal Quadrant Q1 of L2 PW13 (`rows 0..255, cols 256..511`), ZERO extra L2 memory!
    NeoProgAstResult gpu_neo_program_ast(const NeoProgAstQuery &q, CognitiveTrace *tr = nullptr) {
        NeoProgAstResult res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 4u) && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_past_q->ptr, &q, sizeof(NeoProgAstQuery)), "upload prog ast query");
        void *args[]{&d_neo_weights->ptr, &d_past_q->ptr, &d_genome->ptr, &d_past_res->ptr};
        session->d.check(session->d.launch(fn_neo_program_ast, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo prog ast");
        session->d.check(session->d.download(&res, d_past_res->ptr, sizeof(NeoProgAstResult)), "download prog ast");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync prog ast neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k40_program_ast(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        Word tot_ed = 0u;
        // 0. Clean-clear orthogonal quadrant Q1 `PW13[0..255, 256..511]` (preserves K30 3-SAT & K39 Calculus!)
        {
            NeoProgAstQuery q{};
            q.pathway = 13u; q.mode = 4u;
            tot_ed += gpu_neo_program_ast(q, tr).edits_made;
        }
        // 1. Consolidate 3 canonical typed AST programs directly into L2 PW13 Q1 via evolved STDP (`g.basal_rule`):
        //    0=progalpha: MUL(2); ADD(3)  [NAT -> NAT, bound=15] -> astsafe, exec(5)=13, exec(4)=11
        //    1=progbeta:  SQR;    ADD(1)  [NAT -> NAT, bound=15] -> astsafe, exec(4)=17, exec(3)=10
        //    2=proggamma: ADD(2); MUL(3)  [NAT -> ERR(3), bound=15] -> astfault (static type error!)
        {
            NeoProgAstQuery q{};
            q.pathway = 13u; q.mode = 0u;
            q.prog_id = 0u;
            q.fields[0] = 2u; q.fields[1] = 2u; q.fields[2] = 1u; q.fields[3] = 3u;
            q.fields[4] = 0u; q.fields[5] = 0u; q.fields[6] = 15u; q.fields[7] = 1u;
            tot_ed += gpu_neo_program_ast(q, tr).edits_made;
            q.prog_id = 1u;
            q.fields[0] = 4u; q.fields[1] = 1u; q.fields[2] = 1u; q.fields[3] = 1u;
            q.fields[4] = 0u; q.fields[5] = 0u; q.fields[6] = 15u; q.fields[7] = 1u;
            tot_ed += gpu_neo_program_ast(q, tr).edits_made;
            q.prog_id = 2u;
            q.fields[0] = 1u; q.fields[1] = 2u; q.fields[2] = 2u; q.fields[3] = 3u;
            q.fields[4] = 0u; q.fields[5] = 3u; q.fields[6] = 15u; q.fields[7] = 1u;
            tot_ed += gpu_neo_program_ast(q, tr).edits_made;
        }
        return tot_ed;
    }

    int pa_prog_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_PROGALPHA]) return 0;
        if (cid == markers[LM_PROGBETA]) return 1;
        if (cid == markers[LM_PROGGAMMA]) return 2;
        if (cid == markers[LM_PROGOMEGA]) return 3;
        return -1;
    }

    Word pa_prog_concept(Word pid) const {
        if (pid == 0u) return markers[LM_PROGALPHA];
        if (pid == 1u) return markers[LM_PROGBETA];
        if (pid == 2u) return markers[LM_PROGGAMMA];
        if (pid == 3u) return markers[LM_PROGOMEGA];
        return 0u;
    }

    // Kernel 41 (STAGE 8e — PURE-GPU NEURAL LINEAR ALGEBRA, MATVEC & CRAMER SOLVER!):
    // Lives on FREE Orthogonal Quadrant Q2 of L2 PW13 (`rows 256..511, cols 0..255`), ZERO extra L2 memory!
    NeoLinAlgResult gpu_neo_linear_algebra(const NeoLinAlgQuery &q, CognitiveTrace *tr = nullptr) {
        NeoLinAlgResult res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 4u) && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_lalg_q->ptr, &q, sizeof(NeoLinAlgQuery)), "upload linalg query");
        void *args[]{&d_neo_weights->ptr, &d_lalg_q->ptr, &d_genome->ptr, &d_lalg_res->ptr};
        session->d.check(session->d.launch(fn_neo_linear_algebra, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo linalg");
        session->d.check(session->d.download(&res, d_lalg_res->ptr, sizeof(NeoLinAlgResult)), "download linalg");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync linalg neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k41_linear_algebra(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        Word tot_ed = 0u;
        // 0. Clean-clear orthogonal quadrant Q2 `PW13[256..511, 0..255]` (preserves K30, K39 & K40!)
        {
            NeoLinAlgQuery q{};
            q.pathway = 13u; q.mode = 4u;
            tot_ed += gpu_neo_linear_algebra(q, tr).edits_made;
        }
        // 1. Consolidate 3 canonical 2x2 matrices + RHS into L2 PW13 Q2 via evolved STDP (`g.basal_rule`):
        //    0=linalpha: [[3,1],[1,2]] rhs=[11,7] -> det=5, tr=5, inv=1, matvec(2,3)=9+8=17, solve=(3,2)
        //    1=linbeta:  [[4,2],[2,1]] rhs=[8,4]  -> det=0, tr=5, inv=0 (singular!)
        //    2=lingamma: [[5,2],[1,3]] rhs=[19,9] -> det=13, tr=8, inv=1, solve=(3,2)
        {
            NeoLinAlgQuery q{};
            q.pathway = 13u; q.mode = 0u;
            q.mat_id = 0u;
            q.fields[0] = 3u; q.fields[1] = 1u; q.fields[2] = 1u; q.fields[3] = 2u; q.fields[4] = 11u; q.fields[5] = 7u;
            tot_ed += gpu_neo_linear_algebra(q, tr).edits_made;
            q.mat_id = 1u;
            q.fields[0] = 4u; q.fields[1] = 2u; q.fields[2] = 2u; q.fields[3] = 1u; q.fields[4] = 8u; q.fields[5] = 4u;
            tot_ed += gpu_neo_linear_algebra(q, tr).edits_made;
            q.mat_id = 2u;
            q.fields[0] = 5u; q.fields[1] = 2u; q.fields[2] = 1u; q.fields[3] = 3u; q.fields[4] = 19u; q.fields[5] = 9u;
            tot_ed += gpu_neo_linear_algebra(q, tr).edits_made;
        }
        return tot_ed;
    }

    int la_mat_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_LINALPHA]) return 0;
        if (cid == markers[LM_LINBETA]) return 1;
        if (cid == markers[LM_LINGAMMA]) return 2;
        if (cid == markers[LM_LINOMEGA]) return 3;
        return -1;
    }

    // Kernel 42 (STAGE 8f — FINAL FREE QUADRANT OF L2 PW13: PURE-GPU SPIKING PROBABILISTIC BAYES-NET & DECISION CORTEX!):
    // Lives on Orthogonal Quadrant Q3 of L2 PW13 (`rows 256..511, cols 256..511`), ZERO extra L2 memory!
    NeoBayesNetResult gpu_neo_bayes_net(const NeoBayesNetQuery &q, CognitiveTrace *tr = nullptr) {
        NeoBayesNetResult res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 4u) && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_bnet_q->ptr, &q, sizeof(NeoBayesNetQuery)), "upload bnet query");
        void *args[]{&d_neo_weights->ptr, &d_bnet_q->ptr, &d_genome->ptr, &d_bnet_res->ptr};
        session->d.check(session->d.launch(fn_neo_bayes_net, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo bnet");
        session->d.check(session->d.download(&res, d_bnet_res->ptr, sizeof(NeoBayesNetResult)), "download bnet");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync bnet neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k42_bayes_net(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        Word tot_ed = 0u;
        // 0. Clean-clear final orthogonal quadrant Q3 `PW13[256..511, 256..511]` (preserves K30, K39, K40 & K41!)
        {
            NeoBayesNetQuery q{};
            q.pathway = 13u; q.mode = 4u;
            tot_ed += gpu_neo_bayes_net(q, tr).edits_made;
        }
        // 1. Consolidate 3 canonical Bayes-net decision nodes into L2 PW13 Q3 via evolved STDP (`g.basal_rule`):
        //    0=bnalpha: prior=20, P(E|H)=80, P(E|~H)=20, cfp=10, cfn=50 -> P(H|E)=50, P(H|~E)=6, opt=bnetact (500 <= 2500)
        //    1=bnbeta:  prior=10, P(E|H)=90, P(E|~H)=10, cfp=60, cfn=20 -> P(H|E)=50, P(H|~E)=1, opt=bnetwait (3000 > 1000)
        //    2=bngamma: prior=50, P(E|H)=90, P(E|~H)=30, cfp=15, cfn=45 -> P(H|E)=75, P(H|~E)=13, opt=bnetact (375 <= 3375)
        {
            NeoBayesNetQuery q{};
            q.pathway = 13u; q.mode = 0u;
            q.node_id = 0u;
            q.fields[0] = 20u; q.fields[1] = 80u; q.fields[2] = 20u; q.fields[3] = 10u; q.fields[4] = 50u; q.fields[5] = 1u;
            tot_ed += gpu_neo_bayes_net(q, tr).edits_made;
            q.node_id = 1u;
            q.fields[0] = 10u; q.fields[1] = 90u; q.fields[2] = 10u; q.fields[3] = 60u; q.fields[4] = 20u; q.fields[5] = 2u;
            tot_ed += gpu_neo_bayes_net(q, tr).edits_made;
            q.node_id = 2u;
            q.fields[0] = 50u; q.fields[1] = 90u; q.fields[2] = 30u; q.fields[3] = 15u; q.fields[4] = 45u; q.fields[5] = 3u;
            tot_ed += gpu_neo_bayes_net(q, tr).edits_made;
        }
        return tot_ed;
    }

    int bn_node_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_BNALPHA]) return 0;
        if (cid == markers[LM_BNBETA]) return 1;
        if (cid == markers[LM_BNGAMMA]) return 2;
        if (cid == markers[LM_BNOMEGA]) return 3;
        return -1;
    }

    // Kernel 43 (STAGE 8g — PURE-GPU SPIKING MDP VALUE-ITERATION & STOCHASTIC POLICY PLANNER!):
    // Lives on FREE Orthogonal Quadrant of L2 PW24 (`rows 256..511, cols 768..1023`), ZERO extra L2 memory!
    NeoMdpPlanResult gpu_neo_mdp_planner(const NeoMdpPlanQuery &q, CognitiveTrace *tr = nullptr) {
        NeoMdpPlanResult res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 4u) && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_mdpp_q->ptr, &q, sizeof(NeoMdpPlanQuery)), "upload mdpp query");
        void *args[]{&d_neo_weights->ptr, &d_mdpp_q->ptr, &d_genome->ptr, &d_mdpp_res->ptr};
        session->d.check(session->d.launch(fn_neo_mdp_planner, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo mdpp");
        session->d.check(session->d.download(&res, d_mdpp_res->ptr, sizeof(NeoMdpPlanResult)), "download mdpp");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync mdpp neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k43_mdp_planner(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        Word tot_ed = 0u;
        // 0. Clean-clear FREE orthogonal quadrant `PW24[256..511, 768..1023]` (preserves K31, K33, K36 & K37!)
        {
            NeoMdpPlanQuery q{};
            q.pathway = 24u; q.mode = 4u;
            tot_ed += gpu_neo_mdp_planner(q, tr).edits_made;
        }
        // 1. Consolidate 3 canonical stochastic MDP environments into L2 PW24 via evolved STDP (`g.basal_rule`):
        //    0=mdpalpha: (10,20,100,80,50,50) -> 4-step Bellman V*(s0)=42, V*(s1)=70, opt=mdpsteady (42 >= 35)
        //    1=mdpbeta:  (0,10,100,50,80,80)  -> 4-step Bellman V*(s0)=76, V*(s1)=90, opt=mdpleap   (76 > 66)
        //    2=mdpgamma: (20,40,80,100,50,50) -> 4-step Bellman V*(s0)=60, V*(s1)=80, opt=mdpsteady (60 >= 35)
        {
            NeoMdpPlanQuery q{};
            q.pathway = 24u; q.mode = 0u;
            q.env_id = 0u;
            q.fields[0] = 10u; q.fields[1] = 20u; q.fields[2] = 100u; q.fields[3] = 80u; q.fields[4] = 50u; q.fields[5] = 50u;
            tot_ed += gpu_neo_mdp_planner(q, tr).edits_made;
            q.env_id = 1u;
            q.fields[0] = 0u; q.fields[1] = 10u; q.fields[2] = 100u; q.fields[3] = 50u; q.fields[4] = 80u; q.fields[5] = 80u;
            tot_ed += gpu_neo_mdp_planner(q, tr).edits_made;
            q.env_id = 2u;
            q.fields[0] = 20u; q.fields[1] = 40u; q.fields[2] = 80u; q.fields[3] = 100u; q.fields[4] = 50u; q.fields[5] = 50u;
            tot_ed += gpu_neo_mdp_planner(q, tr).edits_made;
        }
        return tot_ed;
    }

    int mdp_env_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_MDPALPHA]) return 0;
        if (cid == markers[LM_MDPBETA]) return 1;
        if (cid == markers[LM_MDPGAMMA]) return 2;
        if (cid == markers[LM_MDPOMEGA]) return 3;
        return -1;
    }

    // Kernel 44 (STAGE 8h — PURE-GPU SPIKING KALMAN FILTER & BAYESIAN SENSOR-FUSION CORTEX!):
    // Lives on FREE Orthogonal Quadrant of L2 PW24 (`rows 512..767, cols 768..1023`), ZERO extra L2 memory!
    NeoKalmanResult gpu_neo_kalman_filter(const NeoKalmanQuery &q, CognitiveTrace *tr = nullptr) {
        NeoKalmanResult res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 4u) && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_kflt_q->ptr, &q, sizeof(NeoKalmanQuery)), "upload kflt query");
        void *args[]{&d_neo_weights->ptr, &d_kflt_q->ptr, &d_genome->ptr, &d_kflt_res->ptr};
        session->d.check(session->d.launch(fn_neo_kalman_filter, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo kflt");
        session->d.check(session->d.download(&res, d_kflt_res->ptr, sizeof(NeoKalmanResult)), "download kflt");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync kflt neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k44_kalman_filter(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        Word tot_ed = 0u;
        // 0. Clean-clear FREE orthogonal quadrant `PW24[512..767, 768..1023]` (preserves K31, K33, K36, K37 & K43!)
        {
            NeoKalmanQuery q{};
            q.pathway = 24u; q.mode = 4u;
            tot_ed += gpu_neo_kalman_filter(q, tr).edits_made;
        }
        // 1. Consolidate 3 canonical Kalman sensor channels into L2 PW24 via evolved STDP (`g.basal_rule`):
        //    0=kfalpha: (20,30,10,40,60,10) -> x_pred=30, p_pred=40, K=50%, x_post=45, p_post=20, trust=1 (kftrust)
        //    1=kfbeta:  (50,10,10,60,90,0)  -> x_pred=50, p_pred=20, K=25%, x_post=60, p_post=15, trust=0 (kfprior)
        //    2=kfgamma: (10,40,20,20,90,0)  -> x_pred=10, p_pred=60, K=75%, x_post=70, p_post=15, trust=1 (kftrust)
        {
            NeoKalmanQuery q{};
            q.pathway = 24u; q.mode = 0u;
            q.chan_id = 0u;
            q.fields[0] = 20u; q.fields[1] = 30u; q.fields[2] = 10u; q.fields[3] = 40u; q.fields[4] = 60u; q.fields[5] = 10u;
            tot_ed += gpu_neo_kalman_filter(q, tr).edits_made;
            q.chan_id = 1u;
            q.fields[0] = 50u; q.fields[1] = 10u; q.fields[2] = 10u; q.fields[3] = 60u; q.fields[4] = 90u; q.fields[5] = 0u;
            tot_ed += gpu_neo_kalman_filter(q, tr).edits_made;
            q.chan_id = 2u;
            q.fields[0] = 10u; q.fields[1] = 40u; q.fields[2] = 20u; q.fields[3] = 20u; q.fields[4] = 90u; q.fields[5] = 0u;
            tot_ed += gpu_neo_kalman_filter(q, tr).edits_made;
        }
        return tot_ed;
    }

    int kf_chan_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_KFALPHA]) return 0;
        if (cid == markers[LM_KFBETA]) return 1;
        if (cid == markers[LM_KFGAMMA]) return 2;
        if (cid == markers[LM_KFOMEGA]) return 3;
        return -1;
    }

    // Kernel 45 (STAGE 8i — PURE-GPU SPIKING LQR / RICCATI FEEDBACK CONTROL & STABILITY CORTEX!):
    // Lives on FREE Orthogonal Quadrant of L2 PW24 (`rows 0..255, cols 512..767`), ZERO extra L2 memory!
    NeoLqrResult gpu_neo_lqr_control(const NeoLqrQuery &q, CognitiveTrace *tr = nullptr) {
        NeoLqrResult res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 4u) && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_lqrc_q->ptr, &q, sizeof(NeoLqrQuery)), "upload lqrc query");
        void *args[]{&d_neo_weights->ptr, &d_lqrc_q->ptr, &d_genome->ptr, &d_lqrc_res->ptr};
        session->d.check(session->d.launch(fn_neo_lqr_control, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo lqrc");
        session->d.check(session->d.download(&res, d_lqrc_res->ptr, sizeof(NeoLqrResult)), "download lqrc");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync lqrc neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k45_lqr_control(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        Word tot_ed = 0u;
        // 0. Clean-clear FREE orthogonal quadrant `PW24[0..255, 512..767]` (preserves K31, K33, K36, K37, K43 & K44!)
        {
            NeoLqrQuery q{};
            q.pathway = 24u; q.mode = 4u;
            tot_ed += gpu_neo_lqr_control(q, tr).edits_made;
        }
        // 1. Consolidate 3 canonical discrete LQR plants into L2 PW24 via evolved STDP (`g.basal_rule`):
        //    0=lqalpha: (100,100,10,10,50,0) -> P*=1618 (10*phi), K*=62%, acl=38%, u0=31, lqstab
        //    1=lqbeta:  (120,100,20,10,40,0) -> open-loop unstable a=1.20 stabilized to acl=29%, K*=91%, u0=36, lqstab
        //    2=lqgamma: (120,10,10,100,50,0) -> under-actuated high-R -> K*=30%, acl=117%, lqunst
        {
            NeoLqrQuery q{};
            q.pathway = 24u; q.mode = 0u;
            q.plant_id = 0u;
            q.fields[0] = 100u; q.fields[1] = 100u; q.fields[2] = 10u; q.fields[3] = 10u; q.fields[4] = 50u; q.fields[5] = 0u;
            tot_ed += gpu_neo_lqr_control(q, tr).edits_made;
            q.plant_id = 1u;
            q.fields[0] = 120u; q.fields[1] = 100u; q.fields[2] = 20u; q.fields[3] = 10u; q.fields[4] = 40u; q.fields[5] = 0u;
            tot_ed += gpu_neo_lqr_control(q, tr).edits_made;
            q.plant_id = 2u;
            q.fields[0] = 120u; q.fields[1] = 10u; q.fields[2] = 10u; q.fields[3] = 100u; q.fields[4] = 50u; q.fields[5] = 0u;
            tot_ed += gpu_neo_lqr_control(q, tr).edits_made;
        }
        return tot_ed;
    }

    int lq_plant_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_LQALPHA]) return 0;
        if (cid == markers[LM_LQBETA]) return 1;
        if (cid == markers[LM_LQGAMMA]) return 2;
        if (cid == markers[LM_LQOMEGA]) return 3;
        return -1;
    }

    // Kernel 46 (STAGE 8j — 500-DOMAIN MILESTONE! PURE-GPU SPIKING 8-POINT SPECTRAL FFT CORTEX!):
    // Lives on FREE Orthogonal Quadrant of L2 PW24 (`rows 0..255, cols 256..511`), ZERO extra L2 memory!
    NeoFftSpecResult gpu_neo_fft_spectrum(const NeoFftSpecQuery &q, CognitiveTrace *tr = nullptr) {
        NeoFftSpecResult res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 4u) && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_ffts_q->ptr, &q, sizeof(NeoFftSpecQuery)), "upload ffts query");
        void *args[]{&d_neo_weights->ptr, &d_ffts_q->ptr, &d_genome->ptr, &d_ffts_res->ptr};
        session->d.check(session->d.launch(fn_neo_fft_spectrum, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo ffts");
        session->d.check(session->d.download(&res, d_ffts_res->ptr, sizeof(NeoFftSpecResult)), "download ffts");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync ffts neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k46_fft_spectrum(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        Word tot_ed = 0u;
        // 0. Clean-clear FREE orthogonal quadrant `PW24[0..255, 256..511]` (preserves K31, K33, K36, K37, K43, K44 & K45!)
        {
            NeoFftSpecQuery q{};
            q.pathway = 24u; q.mode = 4u;
            tot_ed += gpu_neo_fft_spectrum(q, tr).edits_made;
        }
        // 1. Consolidate 3 canonical 8-sample waveforms into L2 PW24 via evolved STDP (`g.basal_rule`):
        //    0=ffalpha: pure fundamental sine {50,78,90,78,50,22,10,22} -> dc=50, h1=40, nyq=0, fflow
        //    1=ffbeta:  pure Nyquist square   {80,20,80,20,80,20,80,20} -> dc=50, h1=0, nyq=30, ffhigh
        //    2=ffgamma: DC-offset cosine      {100,86,50,14,0,14,50,86} -> dc=50, h1=51, nyq=0, fflow
        {
            NeoFftSpecQuery q{};
            q.pathway = 24u; q.mode = 0u;
            q.sig_id = 0u;
            const Word s0[8] = {50u, 78u, 90u, 78u, 50u, 22u, 10u, 22u};
            for (Word n = 0u; n < 8u; ++n) q.samples[n] = s0[n];
            tot_ed += gpu_neo_fft_spectrum(q, tr).edits_made;
            q.sig_id = 1u;
            const Word s1[8] = {80u, 20u, 80u, 20u, 80u, 20u, 80u, 20u};
            for (Word n = 0u; n < 8u; ++n) q.samples[n] = s1[n];
            tot_ed += gpu_neo_fft_spectrum(q, tr).edits_made;
            q.sig_id = 2u;
            const Word s2[8] = {100u, 86u, 50u, 14u, 0u, 14u, 50u, 86u};
            for (Word n = 0u; n < 8u; ++n) q.samples[n] = s2[n];
            tot_ed += gpu_neo_fft_spectrum(q, tr).edits_made;
        }
        return tot_ed;
    }

    int ff_sig_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_FFALPHA]) return 0;
        if (cid == markers[LM_FFBETA]) return 1;
        if (cid == markers[LM_FFGAMMA]) return 2;
        if (cid == markers[LM_FFOMEGA]) return 3;
        return -1;
    }

    // Kernel 47 (STAGE 8k — 510 DOMAINS! PURE-GPU SPIKING 4TH-ORDER RUNGE-KUTTA `RK4` ODE INTEGRATOR!):
    // Lives on FREE Orthogonal Quadrant of L2 PW24 (`rows 256..511, cols 0..255`), ZERO extra L2 memory!
    NeoOdeRk4Result gpu_neo_ode_rk4(const NeoOdeRk4Query &q, CognitiveTrace *tr = nullptr) {
        NeoOdeRk4Result res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 4u) && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_odrk_q->ptr, &q, sizeof(NeoOdeRk4Query)), "upload odrk query");
        void *args[]{&d_neo_weights->ptr, &d_odrk_q->ptr, &d_genome->ptr, &d_odrk_res->ptr};
        session->d.check(session->d.launch(fn_neo_ode_rk4, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo odrk");
        session->d.check(session->d.download(&res, d_odrk_res->ptr, sizeof(NeoOdeRk4Result)), "download odrk");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync odrk neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k47_ode_rk4(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        Word tot_ed = 0u;
        // 0. Clean-clear FREE orthogonal quadrant `PW24[256..511, 0..255]` (preserves K31, K33, K36, K37, K43, K44, K45 & K46!)
        {
            NeoOdeRk4Query q{};
            q.pathway = 24u; q.mode = 4u;
            tot_ed += gpu_neo_ode_rk4(q, tr).edits_made;
        }
        // 1. Consolidate 3 canonical dynamical ODE systems into L2 PW24 via evolved STDP (`g.basal_rule`):
        //    0=odalpha: dx/dt = -0.5x + 20, x0=100, dt=0.5, 4 steps -> x(2)=62, x_eq=40, odestab
        //    1=odbeta:  dx/dt = +0.5x + 0,  x0=20,  dt=0.5, 4 steps -> x(2)=54, x_eq=0,  odegrow
        //    2=odgamma: dx/dt = -1.0x + 50, x0=10,  dt=0.5, 4 steps -> x(2)=45, x_eq=50, odestab
        {
            NeoOdeRk4Query q{};
            q.pathway = 24u; q.mode = 0u;
            q.sys_id = 0u;
            const Word p0[6] = {1u, 50u, 20u, 100u, 50u, 4u};
            for (Word n = 0u; n < 6u; ++n) q.params[n] = p0[n];
            tot_ed += gpu_neo_ode_rk4(q, tr).edits_made;
            q.sys_id = 1u;
            const Word p1[6] = {0u, 50u, 0u, 20u, 50u, 4u};
            for (Word n = 0u; n < 6u; ++n) q.params[n] = p1[n];
            tot_ed += gpu_neo_ode_rk4(q, tr).edits_made;
            q.sys_id = 2u;
            const Word p2[6] = {1u, 100u, 50u, 10u, 50u, 4u};
            for (Word n = 0u; n < 6u; ++n) q.params[n] = p2[n];
            tot_ed += gpu_neo_ode_rk4(q, tr).edits_made;
        }
        return tot_ed;
    }

    int od_sys_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_ODALPHA]) return 0;
        if (cid == markers[LM_ODBETA]) return 1;
        if (cid == markers[LM_ODGAMMA]) return 2;
        if (cid == markers[LM_ODOMEGA]) return 3;
        return -1;
    }

    // Kernel 48 (STAGE 8l — 520 DOMAINS! PURE-GPU SPIKING SIMPLEX LINEAR PROGRAMMING OPTIMIZER!):
    // Lives on FREE Orthogonal Quadrant of L2 PW24 (`rows 768..1023, cols 512..767`), ZERO extra L2 memory!
    NeoSimplexLpResult gpu_neo_simplex_lp(const NeoSimplexLpQuery &q, CognitiveTrace *tr = nullptr) {
        NeoSimplexLpResult res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 4u) && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_sxlp_q->ptr, &q, sizeof(NeoSimplexLpQuery)), "upload sxlp query");
        void *args[]{&d_neo_weights->ptr, &d_sxlp_q->ptr, &d_genome->ptr, &d_sxlp_res->ptr};
        session->d.check(session->d.launch(fn_neo_simplex_lp, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo sxlp");
        session->d.check(session->d.download(&res, d_sxlp_res->ptr, sizeof(NeoSimplexLpResult)), "download sxlp");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync sxlp neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k48_simplex_lp(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        Word tot_ed = 0u;
        // 0. Clean-clear FREE orthogonal quadrant `PW24[768..1023, cols 512..767]` (preserves K33 & all K31–K47!)
        {
            NeoSimplexLpQuery q{};
            q.pathway = 24u; q.mode = 4u;
            tot_ed += gpu_neo_simplex_lp(q, tr).edits_made;
        }
        // 1. Consolidate 3 canonical 2D LP systems into L2 PW24 via evolved STDP (`g.basal_rule`):
        //    0=sxalpha: max 3x+5y s.t. x+2y<=20, 3x+y<=30 -> (8,6) Z*=54, sxdual
        //    1=sxbeta:  max 8x+2y s.t. 2x+y<=20, x+3y<=25 -> (10,0) Z*=80, sxcorn
        //    2=sxgamma: max 4x+6y s.t. 2x+y<=18, x+2y<=18 -> (6,6) Z*=60, sxdual
        {
            NeoSimplexLpQuery q{};
            q.pathway = 24u; q.mode = 0u;
            q.prob_id = 0u;
            const Word p0[8] = {3u, 5u, 1u, 2u, 20u, 3u, 1u, 30u};
            for (Word n = 0u; n < 8u; ++n) q.params[n] = p0[n];
            tot_ed += gpu_neo_simplex_lp(q, tr).edits_made;
            q.prob_id = 1u;
            const Word p1[8] = {8u, 2u, 2u, 1u, 20u, 1u, 3u, 25u};
            for (Word n = 0u; n < 8u; ++n) q.params[n] = p1[n];
            tot_ed += gpu_neo_simplex_lp(q, tr).edits_made;
            q.prob_id = 2u;
            const Word p2[8] = {4u, 6u, 2u, 1u, 18u, 1u, 2u, 18u};
            for (Word n = 0u; n < 8u; ++n) q.params[n] = p2[n];
            tot_ed += gpu_neo_simplex_lp(q, tr).edits_made;
        }
        return tot_ed;
    }

    int sx_prob_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_SXALPHA]) return 0;
        if (cid == markers[LM_SXBETA]) return 1;
        if (cid == markers[LM_SXGAMMA]) return 2;
        if (cid == markers[LM_SXOMEGA]) return 3;
        return -1;
    }

    // Kernel 49 (STAGE 8m — 530 DOMAINS! PURE-GPU SPIKING PCA EIGENDECOMPOSITION & RAYLEIGH POWER-ITERATION CORTEX!):
    // Lives on FREE Orthogonal Quadrant of L2 PW27 (`rows 0..255, cols 256..511`), ZERO extra L2 memory!
    NeoPcaEigenResult gpu_neo_pca_eigen(const NeoPcaEigenQuery &q, CognitiveTrace *tr = nullptr) {
        NeoPcaEigenResult res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 4u) && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_pcae_q->ptr, &q, sizeof(NeoPcaEigenQuery)), "upload pcae query");
        void *args[]{&d_neo_weights->ptr, &d_pcae_q->ptr, &d_genome->ptr, &d_pcae_res->ptr};
        session->d.check(session->d.launch(fn_neo_pca_eigen, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo pcae");
        session->d.check(session->d.download(&res, d_pcae_res->ptr, sizeof(NeoPcaEigenResult)), "download pcae");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync pcae neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k49_pca_eigen(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        Word tot_ed = 0u;
        // 0. Clean-clear FREE orthogonal quadrant `PW27[0..255, cols 256..511]` (preserves K38 & all K31–K48!)
        {
            NeoPcaEigenQuery q{};
            q.pathway = 27u; q.mode = 4u;
            tot_ed += gpu_neo_pca_eigen(q, tr).edits_made;
        }
        // 1. Consolidate 3 canonical symmetric 2x2 covariance matrices into L2 PW27 via evolved STDP (`g.basal_rule`):
        //    0=pcalpha [[13,4],[4,7]]  -> lam1=15, lam2=5,  var_pct=75, pcaxis
        //    1=pcbeta  [[14,2],[2,11]] -> lam1=15, lam2=10, var_pct=60, pciso
        //    2=pcgamma [[31,8],[8,19]] -> lam1=35, lam2=15, var_pct=70, pcaxis
        {
            NeoPcaEigenQuery q{};
            q.pathway = 27u; q.mode = 0u;
            q.mat_id = 0u;
            q.params[0] = 13u; q.params[1] = 4u; q.params[2] = 7u; q.params[3] = 0u;
            tot_ed += gpu_neo_pca_eigen(q, tr).edits_made;
            q.mat_id = 1u;
            q.params[0] = 14u; q.params[1] = 2u; q.params[2] = 11u; q.params[3] = 0u;
            tot_ed += gpu_neo_pca_eigen(q, tr).edits_made;
            q.mat_id = 2u;
            q.params[0] = 31u; q.params[1] = 8u; q.params[2] = 19u; q.params[3] = 0u;
            tot_ed += gpu_neo_pca_eigen(q, tr).edits_made;
        }
        return tot_ed;
    }

    int pc_mat_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_PCALPHA]) return 0;
        if (cid == markers[LM_PCBETA]) return 1;
        if (cid == markers[LM_PCGAMMA]) return 2;
        if (cid == markers[LM_PCOMEGA]) return 3;
        return -1;
    }

    // Kernel 50 (STAGE 8n — 50-KERNEL MILESTONE & 540 DOMAINS! PURE-GPU SPIKING 1D HEAT/DIFFUSION PDE FINITE-DIFFERENCE SOLVER!):
    // Lives on FREE Orthogonal Quadrant of L2 PW27 (`rows 0..255, cols 512..767`), ZERO extra L2 memory!
    NeoPdeSolverResult gpu_neo_pde_solver(const NeoPdeSolverQuery &q, CognitiveTrace *tr = nullptr) {
        NeoPdeSolverResult res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 4u) && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_pdes_q->ptr, &q, sizeof(NeoPdeSolverQuery)), "upload pdes query");
        void *args[]{&d_neo_weights->ptr, &d_pdes_q->ptr, &d_genome->ptr, &d_pdes_res->ptr};
        session->d.check(session->d.launch(fn_neo_pde_solver, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo pdes");
        session->d.check(session->d.download(&res, d_pdes_res->ptr, sizeof(NeoPdeSolverResult)), "download pdes");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync pdes neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k50_pde_solver(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        Word tot_ed = 0u;
        // 0. Clean-clear FREE orthogonal quadrant `PW27[0..255, cols 512..767]` (preserves K38, K49 & all K31–K49!)
        {
            NeoPdeSolverQuery q{};
            q.pathway = 27u; q.mode = 4u;
            tot_ed += gpu_neo_pde_solver(q, tr).edits_made;
        }
        // 1. Consolidate 3 canonical 1D diffusion PDE grids into L2 PW27 via evolved STDP (`g.basal_rule`):
        //    0=pdalpha {0,0,100,0,0, 20, 2, 0}     -> u_mid=44, u_side=24, pdestab
        //    1=pdbeta  {0,0,100,0,0, 60, 1, 0}     -> u_mid=0,  u_side=60, pdeunst (CFL blow-up!)
        //    2=pdgamma {50,50,100,50,50, 20, 2, 0} -> u_mid=72, u_side=62, pdestab
        {
            NeoPdeSolverQuery q{};
            q.pathway = 27u; q.mode = 0u;
            q.grid_id = 0u;
            const Word g0[8] = {0u, 0u, 100u, 0u, 0u, 20u, 2u, 0u};
            for (Word n = 0u; n < 8u; ++n) q.params[n] = g0[n];
            tot_ed += gpu_neo_pde_solver(q, tr).edits_made;
            q.grid_id = 1u;
            const Word g1[8] = {0u, 0u, 100u, 0u, 0u, 60u, 1u, 0u};
            for (Word n = 0u; n < 8u; ++n) q.params[n] = g1[n];
            tot_ed += gpu_neo_pde_solver(q, tr).edits_made;
            q.grid_id = 2u;
            const Word g2[8] = {50u, 50u, 100u, 50u, 50u, 20u, 2u, 0u};
            for (Word n = 0u; n < 8u; ++n) q.params[n] = g2[n];
            tot_ed += gpu_neo_pde_solver(q, tr).edits_made;
        }
        return tot_ed;
    }

    int pd_grid_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_PDALPHA]) return 0;
        if (cid == markers[LM_PDBETA]) return 1;
        if (cid == markers[LM_PDGAMMA]) return 2;
        if (cid == markers[LM_PDOMEGA]) return 3;
        return -1;
    }

    // Kernel 51 (STAGE 8o — 51 BLACKWELL KERNELS & 550 DOMAINS! PURE-GPU 256-CHAIN SPIKING METROPOLIS-HASTINGS MCMC SAMPLER!):
    // Lives on FREE Orthogonal Quadrant of L2 PW27 (`rows 256..511, cols 0..255`), ZERO extra L2 memory!
    NeoMcmcSamplerResult gpu_neo_mcmc_sampler(const NeoMcmcSamplerQuery &q, CognitiveTrace *tr = nullptr) {
        NeoMcmcSamplerResult res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 4u) && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_mcmcs_q->ptr, &q, sizeof(NeoMcmcSamplerQuery)), "upload mcmcs query");
        void *args[]{&d_neo_weights->ptr, &d_mcmcs_q->ptr, &d_genome->ptr, &d_mcmcs_res->ptr};
        session->d.check(session->d.launch(fn_neo_mcmc_sampler, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo mcmcs");
        session->d.check(session->d.download(&res, d_mcmcs_res->ptr, sizeof(NeoMcmcSamplerResult)), "download mcmcs");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync mcmcs neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k51_mcmc_sampler(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        Word tot_ed = 0u;
        // 0. Clean-clear FREE orthogonal quadrant `PW27[rows 256..511, cols 0..255]` (preserves K32, K34, K35, K38, K49, K50!)
        {
            NeoMcmcSamplerQuery q{};
            q.pathway = 27u; q.mode = 4u;
            tot_ed += gpu_neo_mcmc_sampler(q, tr).edits_made;
        }
        // 1. Consolidate 3 canonical target distributions into L2 PW27 via evolved STDP (`g.basal_rule`):
        //    0=mcalpha {60, 20, 10, 10, 32, 11, 0, 0} -> mode_prob=60%, accept=46%, mcpeak
        //    1=mcbeta  {25, 25, 25, 25, 32, 23, 0, 0} -> mode_prob=26%, accept=100%, mcflat
        //    2=mcgamma {40, 40, 10, 10, 32, 37, 0, 0} -> mode_prob=40%, accept=61%, mcflat
        {
            NeoMcmcSamplerQuery q{};
            q.pathway = 27u; q.mode = 0u;
            q.dist_id = 0u;
            const Word d0[8] = {60u, 20u, 10u, 10u, 32u, 11u, 0u, 0u};
            for (Word n = 0u; n < 8u; ++n) q.params[n] = d0[n];
            tot_ed += gpu_neo_mcmc_sampler(q, tr).edits_made;
            q.dist_id = 1u;
            const Word d1[8] = {25u, 25u, 25u, 25u, 32u, 23u, 0u, 0u};
            for (Word n = 0u; n < 8u; ++n) q.params[n] = d1[n];
            tot_ed += gpu_neo_mcmc_sampler(q, tr).edits_made;
            q.dist_id = 2u;
            const Word d2[8] = {40u, 40u, 10u, 10u, 32u, 37u, 0u, 0u};
            for (Word n = 0u; n < 8u; ++n) q.params[n] = d2[n];
            tot_ed += gpu_neo_mcmc_sampler(q, tr).edits_made;
        }
        return tot_ed;
    }

    int mc_dist_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_MCALPHA]) return 0;
        if (cid == markers[LM_MCBETA]) return 1;
        if (cid == markers[LM_MCGAMMA]) return 2;
        if (cid == markers[LM_MCOMEGA]) return 3;
        return -1;
    }

    // Kernel 52 (v58 STAGE 8p — 52 BLACKWELL KERNELS!): Pure-GPU 4,096-Rollout Parallel Spiking UCB1 MCTS Decision-Tree Planner!
    // Lives on FREE Orthogonal Quadrant of L2 PW27 (`rows 256..511, cols 768..1023`), ZERO extra L2 memory!
    NeoMctsPlannerResult gpu_neo_mcts_planner(const NeoMctsPlannerQuery &q, CognitiveTrace *tr = nullptr) {
        NeoMctsPlannerResult res{};
        if (!session || abl_no_consolidate) return res;
        if ((q.mode == 0u || q.mode == 4u) && abl_freeze) return res;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_mctsp_q->ptr, &q, sizeof(NeoMctsPlannerQuery)), "upload mctsp query");
        void *args[]{&d_neo_weights->ptr, &d_mctsp_q->ptr, &d_genome->ptr, &d_mctsp_res->ptr};
        session->d.check(session->d.launch(fn_neo_mcts_planner, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                         "gpu neo mctsp");
        session->d.check(session->d.download(&res, d_mctsp_res->ptr, sizeof(NeoMctsPlannerResult)), "download mctsp");
        if (res.edits_made > 0) {
            neo_consolidated_ready = true;
            session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                             "sync mctsp neo weights");
            lifetime_edits += res.edits_made;
            if (tr) tr->synaptic_edits += res.edits_made;
        }
        lifetime_spikes += res.spikes_fired;
        if (tr) {
            tr->gpu_spikes += res.spikes_fired;
            tr->reasoning_hops += 1u;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return res;
    }

    Word gpu_train_k52_mcts_planner(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze) return 0u;
        Word tot_ed = 0u;
        // 0. Clean-clear FREE orthogonal quadrant `PW27[rows 256..511, cols 768..1023]` (preserves K32, K34, K35, K38, K49, K50, K51!)
        {
            NeoMctsPlannerQuery q{};
            q.pathway = 27u; q.mode = 4u;
            tot_ed += gpu_neo_mcts_planner(q, tr).edits_made;
        }
        // 1. Consolidate 3 canonical 2-ply decision trees into L2 PW27 via evolved STDP (`g.basal_rule`):
        //    0=mtalpha {90, 40, 50, 45, 85, 50, 30, 11} -> best_act=0, best_val=86, visit_pct=57%, mctbold
        //    1=mtbeta  {90, 80, 75, 65, 30, 35, 45, 23} -> best_act=3, best_val=73, visit_pct=50%, mctsafe
        //    2=mtgamma {65, 95, 55, 50, 60, 85, 30, 37} -> best_act=1, best_val=90, visit_pct=52%, mctbold
        {
            NeoMctsPlannerQuery q{};
            q.pathway = 27u; q.mode = 0u;
            q.tree_id = 0u;
            const Word t0[8] = {90u, 40u, 50u, 45u, 85u, 50u, 30u, 11u};
            for (Word n = 0u; n < 8u; ++n) q.params[n] = t0[n];
            tot_ed += gpu_neo_mcts_planner(q, tr).edits_made;
            q.tree_id = 1u;
            const Word t1[8] = {90u, 80u, 75u, 65u, 30u, 35u, 45u, 23u};
            for (Word n = 0u; n < 8u; ++n) q.params[n] = t1[n];
            tot_ed += gpu_neo_mcts_planner(q, tr).edits_made;
            q.tree_id = 2u;
            const Word t2[8] = {65u, 95u, 55u, 50u, 60u, 85u, 30u, 37u};
            for (Word n = 0u; n < 8u; ++n) q.params[n] = t2[n];
            tot_ed += gpu_neo_mcts_planner(q, tr).edits_made;
        }
        return tot_ed;
    }

    int mt_tree_id(Word cid) const {
        if (!cid) return -1;
        if (cid == markers[LM_MTALPHA]) return 0;
        if (cid == markers[LM_MTBETA]) return 1;
        if (cid == markers[LM_MTGAMMA]) return 2;
        if (cid == markers[LM_MTOMEGA]) return 3;
        return -1;
    }

    Word la_mat_concept(Word mid) const {
        if (mid == 0u) return markers[LM_LINALPHA];
        if (mid == 1u) return markers[LM_LINBETA];
        if (mid == 2u) return markers[LM_LINGAMMA];
        if (mid == 3u) return markers[LM_LINOMEGA];
        return 0u;
    }

    Word gpu_neo_online_pairs(const std::vector<NeoPair> &pairs, CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze || abl_no_consolidate || pairs.empty())
            return 0;
        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        Word n_pairs = Word(std::min<size_t>(pairs.size(), gpu_neo_pair_cap));
        Word zero = 0;
        session->d.check(session->d.upload(d_neo_pairs->ptr, pairs.data(), size_t(n_pairs) * sizeof(NeoPair)),
                         "upload online neo pairs");
        session->d.check(session->d.upload(d_neo_edits->ptr, &zero, sizeof(Word)), "zero online neo edits");
        session->d.check(session->d.upload(d_neo_spikes->ptr, &zero, sizeof(Word)), "zero online neo spikes");
        void *args[]{&d_neo_weights->ptr, &d_neo_pairs->ptr, &n_pairs, &d_genome->ptr,
                     &d_neo_edits->ptr, &d_neo_spikes->ptr};
        session->d.check(session->d.launch(fn_neo_ripple, 32, 1, 1, 32, 1, 1, 0, nullptr, args, nullptr),
                         "gpu online multi-pathway ripple");
        Word n_ed = 0, n_sp = 0;
        session->d.check(session->d.download(&n_ed, d_neo_edits->ptr, sizeof(Word)), "download online neo edits");
        session->d.check(session->d.download(&n_sp, d_neo_spikes->ptr, sizeof(Word)), "download online neo spikes");
        session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                         "download online neo weights");
        neo_consolidated_ready = true;
        lifetime_edits += n_ed;
        lifetime_spikes += n_sp;
        if (tr) {
            tr->synaptic_edits += n_ed;
            tr->gpu_spikes += n_sp;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return n_ed;
    }

    // Native Blackwell 256-Cell 2D Spiking Place-Cell Wavefront Navigation!
    struct Nav2DResult {
        bool found = false;
        Word steps = 0;
        Word spikes = 0;
        std::vector<Word> waypoints; // Ordered landmark concept IDs crossed along the 2D spiking trajectory!
        std::vector<std::pair<Word, Word>> path_xy; // Exact 2D grid coordinates along the spiking wavefront!
    };

    Nav2DResult gpu_navigate_2d(Word from_loc, Word to_loc, CognitiveTrace *tr = nullptr) {
        Nav2DResult out{};
        if (!session || abl_no_hipp || !from_loc || !to_loc)
            return out;
        auto it_s = map_landmarks.find(from_loc);
        auto it_g = map_landmarks.find(to_loc);
        if (it_s == map_landmarks.end() || it_g == map_landmarks.end())
            return out;

        SpatialMapQuery q{};
        q.start_x = it_s->second.first;
        q.start_y = it_s->second.second;
        q.goal_x = it_g->second.first;
        q.goal_y = it_g->second.second;
        // River barrier at x = 7 spanning y = 0..15, EXCEPT bridge at (7, 7) (if intact!) and northern pass at (7, 1)!
        for (Word y = 0; y < MAP_H; ++y) {
            if (y == 1u) continue; // Northern mountain pass always open!
            if (y == 7u && !map_bridge_broken) continue; // Bridge open unless broken!
            Word cid = y * MAP_W + 7u;
            q.blocked[cid >> 5] |= 1u << (cid & 31u);
        }

        auto t0 = std::chrono::steady_clock::now();
        session->d.check(session->d.upload(d_spatial_q->ptr, &q, sizeof(q)), "upload 2d map query");
        void *args[]{&d_spatial_q->ptr, &d_spatial_res->ptr};
        session->d.check(session->d.launch(fn_spatial_wave, 1, 1, 1, MAP_CELLS, 1, 1, 0, nullptr, args, nullptr),
                         "gpu 2d place-cell wave");
        SpatialPathResult sr{};
        session->d.check(session->d.download(&sr, d_spatial_res->ptr, sizeof(sr)), "download 2d path");
        lifetime_spikes += sr.spikes_fired;
        if (tr) {
            tr->gpu_spikes += sr.spikes_fired;
            ++tr->reasoning_hops;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        if (!sr.found)
            return out;
        out.found = true;
        out.steps = sr.length > 0 ? sr.length - 1u : 0u;
        out.spikes = sr.spikes_fired;
        out.waypoints.push_back(from_loc);
        for (Word k = 0; k < sr.length; ++k) {
            Word cell = sr.path_cells[k];
            Word cx = cell & 15u, cy = cell >> 4;
            out.path_xy.push_back({cx, cy});
            for (const auto &[lm_id, xy] : map_landmarks) {
                if (xy.first == cx && xy.second == cy && lm_id != out.waypoints.back())
                    out.waypoints.push_back(lm_id);
            }
        }
        if (out.waypoints.back() != to_loc)
            out.waypoints.push_back(to_loc);
        return out;
    }

    SpikeVec intent_sdr(Word intent) const {
        return cochlear_encode("intent_cls_" + std::to_string(intent), genome.wiring[4] ^ 0x51a3c7u);
    }

    // Pathway 5 Real Spiking Neural Wernicke Intent Classifier on 12-MiB Blackwell GPU Neocortex!
    Word gpu_classify_intent(const SpikeVec &word_bag, CognitiveTrace *tr = nullptr) {
        if (!session || !neo_consolidated_ready || sdr_empty(word_bag))
            return INT_NONE;
        auto nr = gpu_neo_settle(5, word_bag, ~0u, SpikeVec{}, 40, 8, tr);
        if (nr.active_count < 8)
            return INT_NONE;
        Word best_in = INT_NONE, best_ov = 0;
        for (Word in = 1; in <= 377; ++in) {
            Word ov = sdr_overlap(nr.spikes, intent_sdr(in));
            if (ov > best_ov) {
                best_ov = ov;
                best_in = in;
            }
        }
        return best_ov >= 16 ? best_in : INT_NONE;
    }

    void sync_genome_to_gpu() {
        if (!session)
            return;
        session->d.check(session->d.upload(d_genome->ptr, &genome, sizeof(genome)), "upload cortical genome");
    }

    void sync_engrams_to_gpu() {
        if (!session || !gpu_dirty_engrams)
            return;
        if (!engrams.empty()) {
            Word cnt = Word(std::min<size_t>(engrams.size(), gpu_engram_cap));
            session->d.check(session->d.upload(d_engrams->ptr, engrams.data(), size_t(cnt) * sizeof(Engram)),
                             "upload hippocampal engrams");
        }
        gpu_dirty_engrams = false;
    }

    void sync_schemas_to_gpu() {
        if (!session || !gpu_dirty_schemas)
            return;
        if (!schemas.empty()) {
            Word cnt = Word(std::min<size_t>(schemas.size(), gpu_schema_cap));
            session->d.check(session->d.upload(d_schemas->ptr, schemas.data(), size_t(cnt) * sizeof(CausalSchema)),
                             "upload causal schemas");
        }
        gpu_dirty_schemas = false;
    }

    void gpu_step_area(Word area_idx, const SpikeVec &basal, const SpikeVec &apical, const SpikeVec &lateral,
                       CognitiveTrace *tr = nullptr) {
        if (!session || area_idx >= 6)
            return;
        auto t0 = std::chrono::steady_clock::now();
        SpikeVec ap = abl_no_apical ? SpikeVec{} : apical;
        session->d.check(session->d.upload(d_area->ptr, &areas[area_idx], sizeof(CorticalArea)), "upload area");
        session->d.check(session->d.upload(d_v0->ptr, &basal, sizeof(SpikeVec)), "upload basal");
        session->d.check(session->d.upload(d_v1->ptr, &ap, sizeof(SpikeVec)), "upload apical");
        session->d.check(session->d.upload(d_v2->ptr, &lateral, sizeof(SpikeVec)), "upload lateral");
        void *args[]{&d_area->ptr, &d_genome->ptr, &d_v0->ptr, &d_v1->ptr, &d_v2->ptr};
        session->d.check(session->d.launch(fn_area_step, 1, 1, 1, 32, 1, 1, 0, nullptr, args, nullptr),
                         "step cortical area");
        Word prev_sp = areas[area_idx].total_spikes;
        session->d.check(session->d.download(&areas[area_idx], d_area->ptr, sizeof(CorticalArea)), "download area");
        Word delta_sp = areas[area_idx].total_spikes - prev_sp;
        lifetime_spikes += delta_sp;
        if (tr) {
            tr->gpu_spikes += delta_sp;
            tr->surprise_burst += areas[area_idx].last_surprise;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
    }

    // 26-SM Parallel Blackwell Hippocampal Query!
    HippResult gpu_query_hipp(const HippQuery &q, CognitiveTrace *tr = nullptr) {
        HippResult r{};
        if (abl_no_hipp || engrams.empty())
            return r;
        auto t0 = std::chrono::steady_clock::now();
        HippQuery q_use = q;
        if (abl_no_apical)
            q_use.active_roles &= ~(1u << R_PERSP);
        Word n_eng = Word(std::min<size_t>(engrams.size(), gpu_engram_cap));
        if (session) {
            sync_engrams_to_gpu();
            session->d.check(session->d.upload(d_hquery->ptr, &q_use, sizeof(HippQuery)), "upload hipp query");
            void *args1[]{&d_engrams->ptr, &n_eng, &d_hquery->ptr, &d_hpartials->ptr};
            session->d.check(session->d.launch(fn_hipp_s1, HIPP_BLOCKS, 1, 1, 128, 1, 1, 0, nullptr, args1, nullptr),
                             "query hipp stage1");
            void *args2[]{&d_engrams->ptr, &d_hquery->ptr, &d_hpartials->ptr, &d_hresult->ptr};
            session->d.check(session->d.launch(fn_hipp_s2, 1, 1, 1, 32, 1, 1, 0, nullptr, args2, nullptr),
                             "query hipp stage2");
            session->d.check(session->d.download(&r, d_hresult->ptr, sizeof(HippResult)), "download hipp result");
        }
        lifetime_spikes += r.spikes_fired;
        if (tr) {
            tr->gpu_spikes += r.spikes_fired;
            ++tr->hippocampal_queries;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return r;
    }

    Word encode_engram(const Engram &raw_e, bool mutable_rel, CognitiveTrace *tr = nullptr,
                       bool *out_contradicted = nullptr) {
        if (out_contradicted)
            *out_contradicted = false;
        if (abl_freeze || abl_no_hipp)
            return ~0u;
        int admit = plasticity_delta(genome.hipp_rule, true, true, false, novelty_gain(genome) >= 2u);
        if (admit <= 0 && engrams.size() > 8)
            return ~0u;

        Engram e = raw_e;
        e.timestamp = clock++;
        e.flags |= ENG_ACTIVE;
        if (!e.strength)
            e.strength = Half(12u + std::max(1, admit) * 4u);

        if (e.flags & ENG_EPISODIC) {
            for (int i = int(engrams.size()) - 1; i >= 0; --i) {
                if (engrams[i].flags & ENG_EPISODIC) {
                    e.prev_ep = Half(i + 1);
                    engrams[i].next_ep = Half(engrams.size() + 1);
                    gpu_dirty_engrams = true;
                    break;
                }
            }
        }

        Word sup_cnt = 0, con_cnt = 0;
        if (session && !engrams.empty() && !(e.flags & (ENG_IMPLIES | ENG_DISJUNCTION))) {
            auto t0 = std::chrono::steady_clock::now();
            sync_engrams_to_gpu();
            Word n_eng = Word(std::min<size_t>(engrams.size(), gpu_engram_cap));
            Word mut_flag = mutable_rel ? 1u : 0u;
            session->d.check(session->d.upload(d_engram_in->ptr, &e, sizeof(Engram)), "upload incoming engram");
            void *args[]{&d_engrams->ptr, &n_eng, &d_engram_in->ptr, &d_genome->ptr,
                         &mut_flag, &d_sup_cnt->ptr, &d_con_cnt->ptr};
            session->d.check(session->d.launch(fn_hipp_supersede, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr),
                             "supersede hippocampus");
            session->d.check(session->d.download(&sup_cnt, d_sup_cnt->ptr, sizeof(Word)), "download sup");
            session->d.check(session->d.download(&con_cnt, d_con_cnt->ptr, sizeof(Word)), "download con");
            if (sup_cnt > 0) {
                session->d.check(session->d.download(engrams.data(), d_engrams->ptr, size_t(n_eng) * sizeof(Engram)),
                                 "sync superseded engrams");
            }
            if (tr)
                tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }

        if (con_cnt > 0 && out_contradicted)
            *out_contradicted = true;

        Word idx = Word(engrams.size());
        if (engrams.size() < MAX_ENGRAMS) {
            engrams.push_back(e);
        } else {
            Word victim = 0, oldest = ~0u;
            for (Word i = 0; i < engrams.size(); ++i) {
                if ((engrams[i].flags & ENG_SUPERSEDED) && engrams[i].timestamp < oldest) {
                    oldest = engrams[i].timestamp;
                    victim = i;
                }
            }
            engrams[victim] = e;
            idx = victim;
        }
        gpu_dirty_engrams = true;
        lifetime_edits += 1u + sup_cnt;
        wm.last_engram = idx;
        if (tr) {
            tr->synaptic_edits += 1u + sup_cnt;
            tr->contradicted = tr->contradicted || (con_cnt > 0);
            tr->learned_new = true;
        }
        return idx;
    }

    // Clear temporary counterfactual sandbox engrams cleanly
    void clear_hypotheticals() {
        bool changed = false;
        while (!engrams.empty() && (engrams.back().flags & ENG_HYPOTHETICAL)) {
            engrams.pop_back();
            changed = true;
        }
        for (auto &e : engrams) {
            if (e.flags & ENG_HYPOTHETICAL) {
                e.flags = 0;
                changed = true;
            }
        }
        if (changed)
            gpu_dirty_engrams = true;
    }

    PlanResult gpu_plan(const PlanQuery &q, CognitiveTrace *tr = nullptr) {
        PlanResult r{};
        if (abl_no_causal || schemas.empty())
            return r;
        auto t0 = std::chrono::steady_clock::now();
        PlanQuery qu = q;
        qu.n_schemas = Word(std::min<size_t>(schemas.size(), gpu_schema_cap));
        if (qu.max_depth == 0)
            qu.max_depth = std::max(4u, max_plan_depth(genome));
        if (session) {
            sync_schemas_to_gpu();
            session->d.check(session->d.upload(d_pquery->ptr, &qu, sizeof(PlanQuery)), "upload plan query");
            void *args[]{&d_schemas->ptr, &d_pquery->ptr, &d_pqueue->ptr, &d_phashes->ptr, &d_presult->ptr};
            session->d.check(session->d.launch(fn_plan_search, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
                             "spiking plan search");
            session->d.check(session->d.download(&r, d_presult->ptr, sizeof(PlanResult)), "download plan result");
        }
        lifetime_spikes += r.spikes_fired;
        if (tr) {
            tr->gpu_spikes += r.spikes_fired;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return r;
    }

    PlanResult gpu_counterfactual(const PlanQuery &q, const std::vector<Word> &steps, Word omit_pos,
                                  CognitiveTrace *tr = nullptr) {
        PlanResult r{};
        if (abl_no_causal || schemas.empty() || steps.empty())
            return r;
        auto t0 = std::chrono::steady_clock::now();
        PlanQuery qu = q;
        qu.n_schemas = Word(std::min<size_t>(schemas.size(), gpu_schema_cap));
        Word n_steps = Word(std::min<size_t>(steps.size(), MAX_PLAN_STEPS));
        if (session) {
            sync_schemas_to_gpu();
            session->d.check(session->d.upload(d_pquery->ptr, &qu, sizeof(PlanQuery)), "upload cf query");
            session->d.check(session->d.upload(d_psteps->ptr, steps.data(), size_t(n_steps) * sizeof(Word)),
                             "upload cf steps");
            void *args[]{&d_schemas->ptr, &d_pquery->ptr, &d_psteps->ptr, &n_steps, &omit_pos, &d_presult->ptr};
            session->d.check(session->d.launch(fn_plan_cf, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
                             "spiking counterfactual");
            session->d.check(session->d.download(&r, d_presult->ptr, sizeof(PlanResult)), "download cf result");
        }
        lifetime_spikes += r.spikes_fired;
        if (tr) {
            tr->gpu_spikes += r.spikes_fired;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return r;
    }

    // -----------------------------------------------------------------------
    // Open-Vocabulary Cochlear-Lexical Cortex
    // -----------------------------------------------------------------------
    static std::string norm_word(const std::string &raw) {
        std::string out;
        for (unsigned char c : raw) {
            if (std::isalnum(c) || c == '_' || c == '-' || c == '\'')
                out.push_back(char(std::tolower(c)));
        }
        return out;
    }

    Word find_word_exact(const std::string &raw) const {
        std::string w = norm_word(raw);
        if (w.empty())
            return 0;
        auto it = alias_to_concept.find(w);
        if (it != alias_to_concept.end())
            return it->second;
        for (Word i = 1; i < concepts.size(); ++i)
            if (concepts[i].active && w == concepts[i].word)
                return i;
        return 0;
    }

    Word perceive_word(const std::string &raw, bool recruit, Word default_cat = CAT_ENTITY,
                       CognitiveTrace *tr = nullptr) {
        std::string w = norm_word(raw);
        if (w.empty())
            return 0;
        if (Word ex = find_word_exact(w)) {
            ++concepts[ex].usage_count;
            return ex;
        }
        bool is_num = !w.empty() && std::all_of(w.begin(), w.end(), [](unsigned char c) { return std::isdigit(c); });
        SpikeVec coch = cochlear_encode(w, genome.wiring[0] ^ dialect_seed);

        if (!is_num && w.size() >= 4) {
            Word best_id = 0, best_ov = 0, stem_id = 0, stem_ov = 0;
            for (Word i = 1; i < concepts.size(); ++i) {
                if (!concepts[i].active)
                    continue;
                Word ov = sdr_overlap(coch, concepts[i].lexical_sdr);
                if (ov > best_ov) {
                    best_ov = ov;
                    best_id = i;
                }
                std::string bw = concepts[i].word;
                if ((default_cat & (CAT_ENTITY | CAT_CATEGORY)) &&
                    (concepts[i].categories & (CAT_ENTITY | CAT_CATEGORY)) &&
                    !(concepts[i].categories & (CAT_FUNC | CAT_ACTION | CAT_RELATION | CAT_PROPERTY)) &&
                    bw.size() >= 4 && w.size() == bw.size() + 1 &&
                    w.compare(0, bw.size(), bw) == 0 && (!stem_id || ov > stem_ov)) {
                    stem_ov = ov;
                    stem_id = i;
                }
            }
            // Morphological inflection / plural match (100% dialect-agnostic stem prefix!)
            if (stem_id) {
                ++concepts[stem_id].usage_count;
                return stem_id;
            }
            if (best_id && best_ov >= 21) {
                std::string bw = concepts[best_id].word;
                if (bw.size() >= 3 && w.size() >= 3 && bw.substr(0, 3) == w.substr(0, 3) &&
                    std::abs(int(bw.size()) - int(w.size())) <= 3) {
                    ++concepts[best_id].usage_count;
                    return best_id;
                }
            }
        }
        if (!recruit || concepts.size() >= MAX_CONCEPTS)
            return 0;

        ConceptNode node{};
        node.active = 1;
        node.lexical_sdr = coch;
        node.concept_sdr = cochlear_encode(w + "#sem", genome.wiring[1] ^ (Word(concepts.size()) * 2654435761u));
        node.categories = is_num ? CAT_NUMBER : default_cat;
        if (is_num) {
            try { node.numeric_val = std::stoi(w); } catch (...) { node.numeric_val = QTY_NONE; }
        }
        node.usage_count = 1;
        node.created_tick = clock;
        std::strncpy(node.word, w.c_str(), MAX_WORD_CHARS - 1);
        Word id = Word(concepts.size());
        concepts.push_back(node);
        alias_to_concept[w] = id;
        gpu_step_area(0, node.lexical_sdr, SpikeVec{}, SpikeVec{}, tr);
        ++lifetime_edits;
        if (tr)
            ++tr->synaptic_edits;
        return id;
    }

    void bind_alias(const std::string &alias, Word concept_id) {
        if (!concept_id || concept_id >= concepts.size())
            return;
        std::string w = norm_word(alias);
        if (!w.empty())
            alias_to_concept[w] = concept_id;
    }

    Word decode_sdr(const SpikeVec &s, Word min_ov = 12, Word require_cat = CAT_NONE) const {
        if (sdr_empty(s))
            return 0;
        Word best = 0, best_ov = 0;
        for (Word i = 1; i < concepts.size(); ++i) {
            if (!concepts[i].active)
                continue;
            if (require_cat && !(concepts[i].categories & require_cat))
                continue;
            Word ov = sdr_overlap(s, concepts[i].concept_sdr);
            if (ov > best_ov) {
                best_ov = ov;
                best = i;
            }
        }
        return best_ov >= min_ov ? best : 0;
    }

    std::string word_of(Word id) const {
        if (!id || id >= concepts.size() || !concepts[id].active)
            return "";
        return concepts[id].word;
    }

    SpikeVec sdr_of(Word id) const {
        if (!id || id >= concepts.size() || !concepts[id].active)
            return SpikeVec{};
        return concepts[id].concept_sdr;
    }

    // -----------------------------------------------------------------------
    // High-Level Spiking Cognitive Operations (GPU Hippocampus + Cortex)
    // -----------------------------------------------------------------------
    Word assert_fact(Word subj, Word rel, Word obj, Word loc = 0, Word cause = 0, Word persp = 0,
                     Word flags = ENG_ACTIVE, int qty = QTY_NONE, Word prov = PROV_OBSERVED,
                     Word src_agent = 0, CognitiveTrace *tr = nullptr, bool *contradicted = nullptr) {
        if (!subj || !rel)
            return ~0u;
        if (!persp)
            persp = markers[LM_SELF];
        Engram e{};
        e.role[R_SUBJ] = sdr_of(subj);
        e.role[R_REL] = sdr_of(rel);
        e.role[R_OBJ] = sdr_of(obj);
        e.role[R_LOC] = sdr_of(loc);
        e.role[R_CAUSE] = sdr_of(cause);
        e.role[R_PERSP] = sdr_of(persp);
        e.flags = flags | ENG_ACTIVE;
        e.quantity = qty;
        e.provenance = prov;
        e.source_concept = src_agent;

        bool is_mut = (rel < concepts.size()) && ((concepts[rel].relation_meta & META_MUTABLE) != 0);
        Word idx = encode_engram(e, is_mut, tr, contradicted);

        if (rel == markers[LM_IS_A] && obj && subj < concepts.size() && obj < concepts.size() && !(flags & ENG_NEGATED)) {
            concepts[subj].parent_cat = obj;
            concepts[subj].categories |= (concepts[obj].categories & (CAT_AGENT | CAT_PLACE | CAT_CONTAINER | CAT_TOOL));
            Word shared = 0;
            for (Word w = 0; w < WORDS && shared < 6; ++w) {
                Word m = concepts[obj].concept_sdr.bits[w] & ~concepts[subj].concept_sdr.bits[w];
                while (m && shared < 6) {
                    Word bit = m & (0u - m);
                    concepts[subj].concept_sdr.bits[w] |= bit;
                    m ^= bit;
                    ++shared;
                }
            }
        }
        if (idx != ~0u && !(flags & ENG_HYPOTHETICAL) && wm.recent_learned_engrams.size() < 64)
            wm.recent_learned_engrams.push_back(idx);
        return idx;
    }

    // Store an Implication Rule: (cond_subj, cond_prop) -> (eff_subj, eff_prop)
    Word assert_implication(Word cond_s, Word cond_p, Word eff_s, Word eff_p, Word prov = PROV_TOLD_USER,
                            Word src = 0, CognitiveTrace *tr = nullptr, Word extra_flags = 0) {
        if (!cond_s || !cond_p || !eff_s || !eff_p)
            return ~0u;
        Engram e{};
        e.role[R_SUBJ] = sdr_of(cond_s);
        e.role[R_REL] = sdr_of(cond_p);
        e.role[R_OBJ] = sdr_of(eff_s);
        e.role[R_LOC] = sdr_of(eff_p);
        e.role[R_CAUSE] = sdr_of(markers[LM_IF]);
        e.role[R_PERSP] = sdr_of(markers[LM_SELF]);
        e.flags = ENG_ACTIVE | ENG_IMPLIES | extra_flags;
        e.provenance = prov;
        e.source_concept = src;
        return encode_engram(e, false, tr);
    }

    // Store a Disjunctive Hypothesis: (subj, rel, alt1) OR (subj, rel, alt2)
    Word assert_disjunction(Word subj, Word rel, Word alt1, Word alt2, Word prov = PROV_TOLD_USER,
                            Word src = 0, CognitiveTrace *tr = nullptr) {
        if (!subj || !rel || !alt1 || !alt2)
            return ~0u;
        Engram e{};
        e.role[R_SUBJ] = sdr_of(subj);
        e.role[R_REL] = sdr_of(rel);
        e.role[R_OBJ] = sdr_of(alt1);
        e.role[R_LOC] = sdr_of(alt2);
        e.role[R_CAUSE] = sdr_of(markers[LM_OR]);
        e.role[R_PERSP] = sdr_of(markers[LM_SELF]);
        e.flags = ENG_ACTIVE | ENG_DISJUNCTION;
        e.provenance = prov;
        e.source_concept = src;
        return encode_engram(e, false, tr);
    }

    HippResult query_fact(Word subj, Word rel, Word obj = 0, Word loc = 0, Word persp = 0,
                          bool historical = false, Word target_role = R_OBJ, Word exclude = ~0u,
                          CognitiveTrace *tr = nullptr, bool allow_hyp = false) {
        HippQuery q{};
        q.active_roles = 0;
        if (subj) { q.cue[R_SUBJ] = sdr_of(subj); q.active_roles |= 1u << R_SUBJ; }
        if (rel)  { q.cue[R_REL]  = sdr_of(rel);  q.active_roles |= 1u << R_REL; }
        if (obj)  { q.cue[R_OBJ]  = sdr_of(obj);  q.active_roles |= 1u << R_OBJ; }
        if (loc)  { q.cue[R_LOC]  = sdr_of(loc);  q.active_roles |= 1u << R_LOC; }
        if (persp){ q.cue[R_PERSP]= sdr_of(persp);q.active_roles |= 1u << R_PERSP; }
        q.require_flags = historical ? (ENG_ACTIVE | ENG_SUPERSEDED) : ENG_ACTIVE;
        q.forbid_flags = (historical ? 0u : ENG_SUPERSEDED) | (allow_hyp ? 0u : ENG_HYPOTHETICAL) |
                         ENG_IMPLIES | ENG_DISJUNCTION;
        q.prefer_latest = 1;
        q.target_role = target_role;
        q.min_overlap = std::max(14u, std::min(22u, hipp_threshold(genome)));
        q.exclude_engram = exclude;
        return gpu_query_hipp(q, tr);
    }

    // Check if (subj, rel, target) is eliminated by negation/contradiction or directly known
    bool is_fact_negated(Word subj, Word rel, Word obj, Word persp, CognitiveTrace *tr = nullptr, bool allow_hyp = false) {
        HippResult hr = query_fact(subj, rel, obj, 0, persp, false, R_OBJ, ~0u, tr, allow_hyp);
        if (hr.negated_index != ~0u && hr.negated_score >= hr.best_score)
            return true;
        if (rel == markers[LM_HAS_PROP] && obj < concepts.size() && concepts[obj].antonym_concept) {
            HippResult ah = query_fact(subj, rel, concepts[obj].antonym_concept, 0, persp, false, R_OBJ, ~0u, tr, allow_hyp);
            if (ah.best_index != ~0u)
                return true;
        }
        return false;
    }

    // Disjunctive Elimination Solver: If (subj, rel, A) OR (subj, rel, B) and NOT A -> return B!
    Word resolve_disjunction(Word subj, Word rel, Word persp = 0, CognitiveTrace *tr = nullptr) {
        if (abl_no_hipp || !subj || !rel)
            return 0;
        if (!persp)
            persp = markers[LM_SELF];
        for (int i = int(engrams.size()) - 1; i >= 0; --i) {
            const auto &e = engrams[i];
            if (!(e.flags & ENG_ACTIVE) || !(e.flags & ENG_DISJUNCTION))
                continue;
            if (sdr_overlap(e.role[R_SUBJ], sdr_of(subj)) < 18 || sdr_overlap(e.role[R_REL], sdr_of(rel)) < 18)
                continue;
            Word a1 = decode_sdr(e.role[R_OBJ], 14);
            Word a2 = decode_sdr(e.role[R_LOC], 14);
            if (!a1 || !a2)
                continue;
            bool neg1 = is_fact_negated(subj, rel, a1, persp, tr);
            bool neg2 = is_fact_negated(subj, rel, a2, persp, tr);
            if (neg1 && !neg2) {
                if (tr) ++tr->reasoning_hops;
                return a2;
            }
            if (neg2 && !neg1) {
                if (tr) ++tr->reasoning_hops;
                return a1;
            }
        }
        return 0;
    }

    // Chained Conditional Deduction (Modus Ponens forward + Modus Tollens backward contrapositive!)
    // Returns +1 (entailed true), -1 (refuted via Modus Tollens), or 0 (not entailed)
    struct DeductResult {
        int value = 0;
        Word via_subj = 0;
        Word via_prop = 0;
        Word hops = 0;
    };

    DeductResult deduce_conditional(Word target_s, Word target_p, CognitiveTrace *tr = nullptr, bool allow_hyp = false) {
        DeductResult res{};
        if (abl_no_hipp || !target_s || !target_p)
            return res;
        Word rel_p = markers[LM_HAS_PROP];
        Word self_id = markers[LM_SELF];

        // Collect all active implication rules: (cs, cp) -> (es, ep)
        struct Rule { Word cs, cp, es, ep; };
        std::vector<Rule> rules;
        for (const auto &e : engrams) {
            if (!(e.flags & ENG_ACTIVE) || !(e.flags & ENG_IMPLIES))
                continue;
            Word cs = decode_sdr(e.role[R_SUBJ], 14);
            Word cp = decode_sdr(e.role[R_REL], 14);
            Word es = decode_sdr(e.role[R_OBJ], 14);
            Word ep = decode_sdr(e.role[R_LOC], 14);
            if (cs && cp && es && ep)
                rules.push_back({cs, cp, es, ep});
        }
        if (rules.empty())
            return res;

        // 1. MODUS PONENS (Backward search from goal (target_s, target_p) to any currently true fact!)
        {
            struct Node { Word s, p, hop; };
            std::vector<Node> q{{target_s, target_p, 0}};
            std::set<std::pair<Word, Word>> seen{{target_s, target_p}};
            for (size_t qi = 0; qi < q.size() && qi < 32; ++qi) {
                auto [cur_s, cur_p, h] = q[qi];
                if (h >= 6) continue;
                for (const auto &r : rules) {
                    if (r.es == cur_s && r.ep == cur_p) {
                        // Is antecedent (r.cs, r.cp) true right now?
                        HippResult hr = query_fact(r.cs, rel_p, r.cp, 0, self_id, false, R_OBJ, ~0u, tr, allow_hyp);
                        if (hr.best_index != ~0u && hr.negated_index == ~0u) {
                            res = {1, r.cs, r.cp, h + 1};
                            if (tr) tr->reasoning_hops += h + 1;
                            return res;
                        }
                        if (!seen.count({r.cs, r.cp})) {
                            seen.insert({r.cs, r.cp});
                            q.push_back({r.cs, r.cp, h + 1});
                        }
                    }
                }
            }
        }

        // 2. MODUS TOLLENS (Forward from (target_s, target_p): if assuming it leads to a known NEGATED/ANTONYM fact -> FALSE!)
        if (!abl_no_apical) {
            struct Node { Word s, p, hop; };
            std::vector<Node> q{{target_s, target_p, 0}};
            std::set<std::pair<Word, Word>> seen{{target_s, target_p}};
            for (size_t qi = 0; qi < q.size() && qi < 32; ++qi) {
                auto [cur_s, cur_p, h] = q[qi];
                if (h >= 6) continue;
                for (const auto &r : rules) {
                    if (r.cs == cur_s && r.cp == cur_p) {
                        if (is_fact_negated(r.es, rel_p, r.ep, self_id, tr, allow_hyp)) {
                            res = {-1, r.es, r.ep, h + 1};
                            if (tr) tr->reasoning_hops += h + 1;
                            return res;
                        }
                        if (!seen.count({r.es, r.ep})) {
                            seen.insert({r.es, r.ep});
                            q.push_back({r.es, r.ep, h + 1});
                        }
                    }
                }
            }
        }
        return res;
    }

    // Forward-chain all consequences of a hypothetical ("suppose S is P -> what follows?")
    std::vector<std::pair<Word, Word>> forward_consequences(Word hyp_s, Word hyp_p, CognitiveTrace *tr = nullptr) {
        std::vector<std::pair<Word, Word>> out;
        if (abl_no_hipp || !hyp_s || !hyp_p)
            return out;
        std::vector<std::pair<Word, Word>> q{{hyp_s, hyp_p}};
        std::set<std::pair<Word, Word>> seen{{hyp_s, hyp_p}};
        for (size_t qi = 0; qi < q.size() && qi < 16; ++qi) {
            auto [cs, cp] = q[qi];
            for (const auto &e : engrams) {
                if (!(e.flags & ENG_ACTIVE) || !(e.flags & ENG_IMPLIES))
                    continue;
                if (sdr_overlap(e.role[R_SUBJ], sdr_of(cs)) >= 18 && sdr_overlap(e.role[R_REL], sdr_of(cp)) >= 18) {
                    Word es = decode_sdr(e.role[R_OBJ], 14);
                    Word ep = decode_sdr(e.role[R_LOC], 14);
                    if (es && ep && !seen.count({es, ep})) {
                        seen.insert({es, ep});
                        q.push_back({es, ep});
                        out.push_back({es, ep});
                        if (tr) ++tr->reasoning_hops;
                    }
                }
            }
        }
        return out;
    }

    struct ChainResult {
        std::vector<Word> nodes;
        std::vector<Word> engram_ids;
        Word confidence = 0;
        bool found = false;
        bool from_disjunction = false;
    };

    ChainResult query_transitive_chain(Word start_subj, Word rel, Word persp = 0, bool historical = false,
                                       CognitiveTrace *tr = nullptr, bool allow_hyp = false) {
        ChainResult res{};
        if (!start_subj || !rel)
            return res;
        if (!persp)
            persp = markers[LM_SELF];
        Word max_h = std::max(5u, max_reasoning_hops(genome));
        bool is_trans = (rel < concepts.size()) && ((concepts[rel].relation_meta & META_TRANSITIVE) != 0);
        if (!is_trans)
            max_h = 1;

        Word cur = start_subj;
        std::set<Word> visited{cur};
        res.nodes.push_back(cur);
        Word min_margin = 100000;

        for (Word hop = 0; hop < max_h; ++hop) {
            HippResult hr = query_fact(cur, rel, 0, 0, persp, historical && hop == 0, R_OBJ, ~0u, tr, allow_hyp);
            if (hr.best_index == ~0u)
                hr = query_fact(cur, rel, 0, 0, persp, historical && hop == 0, R_LOC, ~0u, tr, allow_hyp);
            if (hr.best_index == ~0u && persp != markers[LM_SELF] && markers[LM_WORLD])
                hr = query_fact(cur, rel, 0, 0, markers[LM_WORLD], historical && hop == 0, R_OBJ, ~0u, tr, allow_hyp);
            if (hr.best_index == ~0u) {
                // Try disjunctive elimination or bipartite CSP constraint propagation!
                if (!historical && hop == 0) {
                    Word disj = resolve_disjunction(cur, rel, persp, tr);
                    if (!disj && rel == markers[LM_IN] && persp == markers[LM_SELF])
                        disj = solve_csp_location(cur, tr);
                    if (disj && !visited.count(disj)) {
                        visited.insert(disj);
                        res.nodes.push_back(disj);
                        res.found = true;
                        res.from_disjunction = true;
                        cur = disj;
                        continue;
                    }
                }
                break;
            }
            Word nxt = decode_sdr(hr.readout, 14);
            if (!nxt)
                nxt = decode_sdr(engrams[hr.best_index].role[R_LOC], 14);
            if (!nxt || visited.count(nxt))
                break;
            visited.insert(nxt);
            res.nodes.push_back(nxt);
            res.engram_ids.push_back(hr.best_index);
            res.found = true;
            Word margin = hr.second_index == ~0u ? Word(hr.best_score) : Word(std::max(0, hr.best_score - hr.second_score));
            if (margin < min_margin)
                min_margin = margin;
            cur = nxt;
            if (tr)
                ++tr->reasoning_hops;
        }
        res.confidence = res.found ? std::min(100u, 55u + min_margin / 20u) : 0u;
        return res;
    }

    struct InheritResult {
        int answer = 0; // +1=YES, -1=NO, 0=UNKNOWN
        Word via_category = 0;
        Word direct_engram = ~0u;
        Word parent_engram = ~0u;
        Word confidence = 0;
        bool is_exception = false;
        bool from_conditional = false;
    };

    std::vector<Word> get_ancestors(Word subj, CognitiveTrace *tr = nullptr) {
        std::vector<Word> chain;
        std::set<Word> seen;
        Word cur = subj;
        Word isa = markers[LM_IS_A];
        if (!isa || !subj)
            return chain;
        for (Word hop = 0; hop < 8; ++hop) {
            HippResult hr = query_fact(cur, isa, 0, 0, markers[LM_SELF], false, R_OBJ, ~0u, tr);
            if (hr.best_index == ~0u)
                break;
            Word p = decode_sdr(hr.readout, 14);
            if (!p || seen.count(p))
                break;
            seen.insert(p);
            chain.push_back(p);
            cur = p;
            if (tr)
                ++tr->reasoning_hops;
        }
        return chain;
    }

    // Unified Semantic Entailment: checks direct fact, inhibitory veto, antonym,
    // taxonomic IS_A hierarchy (including `is oak living?`!), AND conditional deduction (MP & MT)!
    InheritResult check_inherited(Word subj, Word rel, Word obj, Word persp = 0, CognitiveTrace *tr = nullptr,
                                  bool allow_hyp = false) {
        InheritResult res{};
        if (!subj || !rel || !obj)
            return res;
        if (!persp)
            persp = markers[LM_SELF];

        HippResult direct = query_fact(subj, rel, obj, 0, persp, false, R_OBJ, ~0u, tr, allow_hyp);
        if (direct.negated_index != ~0u && direct.negated_score >= direct.best_score) {
            res.answer = -1;
            res.direct_engram = direct.negated_index;
            res.confidence = 95;
            auto ancs = get_ancestors(subj, tr);
            for (Word cat : ancs) {
                HippResult ph = query_fact(cat, rel, obj, 0, persp, false, R_OBJ, ~0u, tr, allow_hyp);
                if (ph.best_index != ~0u && ph.negated_index == ~0u) {
                    res.via_category = cat;
                    res.parent_engram = ph.best_index;
                    res.is_exception = true;
                    break;
                }
            }
            return res;
        }
        if (direct.best_index != ~0u) {
            res.answer = 1;
            res.direct_engram = direct.best_index;
            res.confidence = 95;
            return res;
        }
        // Check explicit negation via IS_A too ("whale is not a fish" -> "is whale a fish?" -> NO!)
        if (rel == markers[LM_HAS_PROP] || rel == markers[LM_IS_A]) {
            Word alt_rel = (rel == markers[LM_HAS_PROP]) ? markers[LM_IS_A] : markers[LM_HAS_PROP];
            HippResult alt = query_fact(subj, alt_rel, obj, 0, persp, false, R_OBJ, ~0u, tr, allow_hyp);
            if (alt.negated_index != ~0u && alt.negated_score >= alt.best_score) {
                res.answer = -1;
                res.direct_engram = alt.negated_index;
                res.confidence = 95;
                return res;
            }
            if (alt.best_index != ~0u) {
                res.answer = 1;
                res.direct_engram = alt.best_index;
                res.confidence = 95;
                return res;
            }
        }
        if (rel == markers[LM_HAS_PROP] && obj < concepts.size() && concepts[obj].antonym_concept) {
            HippResult ant = query_fact(subj, rel, concepts[obj].antonym_concept, 0, persp, false, R_OBJ, ~0u, tr, allow_hyp);
            if (ant.best_index != ~0u) {
                res.answer = -1;
                res.direct_engram = ant.best_index;
                res.confidence = 90;
                return res;
            }
        }

        // Climb taxonomic IS_A hierarchy (works for BOTH IS_A and HAS_PROP like "is oak living?"!)
        auto ancs = get_ancestors(subj, tr);
        if (rel == markers[LM_IS_A] || rel == markers[LM_HAS_PROP]) {
            for (Word cat : ancs) {
                if (cat == obj) {
                    res.answer = 1;
                    res.via_category = ancs.empty() ? cat : ancs[0];
                    res.confidence = 92;
                    return res;
                }
            }
        }
        for (Word cat : ancs) {
            HippResult ch = query_fact(cat, rel, obj, 0, persp, false, R_OBJ, ~0u, tr, allow_hyp);
            if (ch.negated_index != ~0u && ch.negated_score >= ch.best_score) {
                res.answer = -1;
                res.via_category = cat;
                res.parent_engram = ch.negated_index;
                res.confidence = 88;
                return res;
            }
            if (ch.best_index != ~0u) {
                res.answer = 1;
                res.via_category = cat;
                res.parent_engram = ch.best_index;
                res.confidence = 88;
                return res;
            }
        }

        // Chained Conditional Deduction (Modus Ponens & Modus Tollens!) + Universal Variable-Bound Rules!
        if (rel == markers[LM_HAS_PROP]) {
            auto ded = deduce_conditional(subj, obj, tr, allow_hyp);
            if (ded.value != 0) {
                res.answer = ded.value;
                res.via_category = ded.via_subj;
                res.confidence = 91;
                res.from_conditional = true;
                return res;
            }
            // Universal Variable-Bound Conditional: "if any CAT is P1 then it is P2"
            if (!abl_no_hipp) {
                for (const auto &e : engrams) {
                    if (!(e.flags & ENG_ACTIVE) || !(e.flags & ENG_IMPLIES) || !(e.flags & ENG_UNIVERSAL))
                        continue;
                    Word cat = decode_sdr(e.role[R_SUBJ], 14);
                    Word p1 = decode_sdr(e.role[R_REL], 14);
                    Word p2 = decode_sdr(e.role[R_LOC], 14);
                    if (!cat || !p1 || !p2)
                        continue;
                    bool is_mem = (subj == cat) || (std::find(ancs.begin(), ancs.end(), cat) != ancs.end());
                    if (!is_mem)
                        continue;
                    // Universal Modus Ponens: if subj has P1 and rule is (CAT, P1 -> P2 == obj) -> YES!
                    if (p2 == obj) {
                        HippResult hp1 = query_fact(subj, rel, p1, 0, persp, false, R_OBJ, ~0u, tr, allow_hyp);
                        if (hp1.best_index != ~0u && hp1.negated_index == ~0u) {
                            res.answer = 1;
                            res.via_category = cat;
                            res.confidence = 91;
                            res.from_conditional = true;
                            if (tr) ++tr->reasoning_hops;
                            return res;
                        }
                    }
                    // Universal Modus Tollens: if subj NOT P2 and rule is (CAT, P1 == obj -> P2) -> NO!
                    if (!abl_no_apical && p1 == obj && is_fact_negated(subj, rel, p2, persp, tr, allow_hyp)) {
                        res.answer = -1;
                        res.via_category = cat;
                        res.confidence = 91;
                        res.from_conditional = true;
                        if (tr) ++tr->reasoning_hops;
                        return res;
                    }
                }
                // v7 True Conjunctive AND-Gate Rules: ONLY fires when BOTH (s1, p1) AND (s2, p2) hold!
                for (const auto &cr : wm.conj_rules) {
                    if (cr.es == subj && cr.ep == obj) {
                        HippResult h1 = query_fact(cr.s1, rel, cr.p1, 0, persp, false, R_OBJ, ~0u, tr, allow_hyp);
                        HippResult h2 = query_fact(cr.s2, rel, cr.p2, 0, persp, false, R_OBJ, ~0u, tr, allow_hyp);
                        if (h1.best_index != ~0u && h1.negated_index == ~0u &&
                            h2.best_index != ~0u && h2.negated_index == ~0u) {
                            res.answer = 1;
                            res.via_category = cr.s1;
                            res.confidence = 93;
                            res.from_conditional = true;
                            if (tr) tr->reasoning_hops += 2;
                            return res;
                        }
                    }
                }
                // v9 Biconditional `iff` Rules: (s1, p1) <-> (s2, p2) (both forward & negated contrapositive!)
                for (const auto &ir : wm.iff_rules) {
                    auto check_side = [&](Word other_s, Word other_p) -> int {
                        HippResult ho = query_fact(other_s, rel, other_p, 0, persp, false, R_OBJ, ~0u, tr, allow_hyp);
                        if (ho.best_index != ~0u && ho.negated_index == ~0u) return 1;
                        if (!abl_no_apical && is_fact_negated(other_s, rel, other_p, persp, tr, allow_hyp)) return -1;
                        return 0;
                    };
                    if (ir.s1 == subj && ir.p1 == obj) {
                        int v = check_side(ir.s2, ir.p2);
                        if (v != 0) {
                            res.answer = v; res.via_category = ir.s2; res.confidence = 94; res.from_conditional = true;
                            if (tr) ++tr->reasoning_hops;
                            return res;
                        }
                    } else if (ir.s2 == subj && ir.p2 == obj) {
                        int v = check_side(ir.s1, ir.p1);
                        if (v != 0) {
                            res.answer = v; res.via_category = ir.s1; res.confidence = 94; res.from_conditional = true;
                            if (tr) ++tr->reasoning_hops;
                            return res;
                        }
                    }
                }
            }
        }

        // Zero-Shot GPU Neocortical Feature-Based Abductive Classification!
        // E.g. "kestrel has wings and feathers" -> GPU Neocortex Pathway 0 spontaneously
        // activates `bird` attractor -> inherits `can fly` / `is animal` WITHOUT explicit IS_A!
        if (!abl_no_hipp && ancs.empty() && neo_consolidated_ready) {
            Word abd_cat = abduct_category_neocortex(subj, tr);
            if (abd_cat && abd_cat != subj) {
                if ((rel == markers[LM_IS_A] || rel == markers[LM_HAS_PROP]) && abd_cat == obj) {
                    res.answer = 1;
                    res.via_category = abd_cat;
                    res.confidence = 89;
                    return res;
                }
                auto cat_inh = check_inherited(abd_cat, rel, obj, persp, tr, allow_hyp);
                if (cat_inh.answer != 0) {
                    res.answer = cat_inh.answer;
                    res.via_category = abd_cat;
                    res.confidence = 87;
                    return res;
                }
            }
        }

        // Ribot's Law / Systems Consolidation: when Hippocampus is completely lesioned (`abl_no_hipp`),
        // consolidated semantic knowledge STILL survives inside the 3-million-synapse GPU Neocortex!
        if (abl_no_hipp && neo_consolidated_ready &&
            (rel == markers[LM_IS_A] || rel == markers[LM_HAS_PROP] || rel == markers[LM_CAN])) {
            int neo_ans = check_neocortical_consolidated(subj, obj, tr);
            if (neo_ans != 0) {
                res.answer = neo_ans;
                res.confidence = 86;
                return res;
            }
        }
        return res;
    }

    struct AnalogyResult {
        Word answer = 0;
        Word relation = 0;
        bool reversed = false;
        Word confidence = 0;
    };

    AnalogyResult solve_analogy(Word a, Word b, Word c, CognitiveTrace *tr = nullptr) {
        AnalogyResult out{};
        if (!a || !b || !c)
            return out;
        HippResult fwd = query_fact(a, 0, b, 0, markers[LM_SELF], false, R_REL, ~0u, tr);
        if (fwd.best_index != ~0u) {
            Word rel = decode_sdr(fwd.readout, 14);
            if (rel) {
                HippResult ans = query_fact(c, rel, 0, 0, markers[LM_SELF], false, R_OBJ, ~0u, tr);
                if (ans.best_index != ~0u) {
                    Word d = decode_sdr(ans.readout, 14);
                    if (d) {
                        out = {d, rel, false, 90};
                        if (tr) tr->reasoning_hops += 2;
                        return out;
                    }
                }
                if (rel == markers[LM_OPPOSITE] && c < concepts.size() && concepts[c].antonym_concept)
                    return {concepts[c].antonym_concept, rel, false, 90};
            }
        }
        HippResult rev = query_fact(b, 0, a, 0, markers[LM_SELF], false, R_REL, ~0u, tr);
        if (rev.best_index != ~0u) {
            Word rel = decode_sdr(rev.readout, 14);
            if (rel) {
                HippResult ans = query_fact(0, rel, c, 0, markers[LM_SELF], false, R_SUBJ, ~0u, tr);
                if (ans.best_index != ~0u) {
                    Word d = decode_sdr(ans.readout, 14);
                    if (d) {
                        out = {d, rel, true, 88};
                        if (tr) tr->reasoning_hops += 2;
                        return out;
                    }
                }
            }
        }
        if (a < concepts.size() && concepts[a].antonym_concept == b && c < concepts.size() &&
            concepts[c].antonym_concept)
            return {concepts[c].antonym_concept, markers[LM_OPPOSITE], false, 92};
        return out;
    }

    int compare_order(Word a, Word b, Word attr, CognitiveTrace *tr = nullptr) {
        if (!a || !b || !attr)
            return 0;
        // Also support inverse attribute (e.g. "lighter" when "heavier" is stored!)
        Word inv_attr = (attr < concepts.size()) ? concepts[attr].antonym_concept : 0;
        auto reaches = [&](Word start, Word target, Word use_attr) -> bool {
            if (!use_attr) return false;
            std::vector<Word> q{start};
            std::set<Word> seen{start};
            for (size_t qi = 0; qi < q.size() && qi < 32; ++qi) {
                Word cur = q[qi];
                Word excl = ~0u;
                for (int tries = 0; tries < 6; ++tries) {
                    HippResult hr = query_fact(cur, use_attr, 0, 0, markers[LM_SELF], false, R_OBJ, excl, tr);
                    if (hr.best_index == ~0u && markers[LM_MORE])
                        hr = query_fact(cur, markers[LM_MORE], 0, use_attr, markers[LM_SELF], false, R_OBJ, excl, tr);
                    if (hr.best_index == ~0u)
                        break;
                    Word nxt = decode_sdr(hr.readout, 14);
                    if (nxt == target)
                        return true;
                    if (nxt && !seen.count(nxt)) {
                        seen.insert(nxt);
                        q.push_back(nxt);
                    }
                    excl = hr.best_index;
                }
            }
            return false;
        };
        if (reaches(a, b, attr) || reaches(b, a, inv_attr))
            return 1;
        if (reaches(b, a, attr) || reaches(a, b, inv_attr))
            return -1;
        return 0;
    }

    // Superlative Ranking across all entities participating in comparative attribute `attr`!
    Word query_superlative(Word attr, bool want_min, CognitiveTrace *tr = nullptr) {
        if (abl_no_hipp || !attr)
            return 0;
        Word use_attr = attr;
        bool flip = want_min;
        // Collect directed edges (u > v) for `attr` or its antonym
        std::set<Word> nodes;
        std::map<Word, std::set<Word>> greater_than, less_than;
        Word inv_attr = (attr < concepts.size()) ? concepts[attr].antonym_concept : 0;
        for (const auto &e : engrams) {
            if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED))
                continue;
            bool match_dir = sdr_overlap(e.role[R_REL], sdr_of(use_attr)) >= 18;
            bool match_inv = inv_attr && sdr_overlap(e.role[R_REL], sdr_of(inv_attr)) >= 18;
            if (!match_dir && !match_inv)
                continue;
            Word u = decode_sdr(e.role[R_SUBJ], 14);
            Word v = decode_sdr(e.role[R_OBJ], 14);
            if (!u || !v)
                continue;
            if (match_inv)
                std::swap(u, v);
            nodes.insert(u);
            nodes.insert(v);
            greater_than[u].insert(v);
            less_than[v].insert(u);
        }
        if (nodes.empty())
            return 0;
        // Max element has empty `less_than`; Min element has empty `greater_than`!
        Word best = 0;
        int best_deg = -1;
        for (Word x : nodes) {
            if (!flip && less_than[x].empty() && int(greater_than[x].size()) > best_deg) {
                best = x;
                best_deg = int(greater_than[x].size());
            }
            if (flip && greater_than[x].empty() && int(less_than[x].size()) > best_deg) {
                best = x;
                best_deg = int(less_than[x].size());
            }
        }
        if (tr && best)
            ++tr->reasoning_hops;
        return best;
    }

    std::optional<int> query_quantity(Word owner, Word item, CognitiveTrace *tr = nullptr) {
        if (!owner)
            return std::nullopt;
        Word rel = markers[LM_HAS];
        if (!rel)
            return std::nullopt;
        HippResult hr = query_fact(owner, rel, item, 0, markers[LM_SELF], false, R_OBJ, ~0u, tr);
        if (hr.best_index != ~0u && engrams[hr.best_index].quantity != QTY_NONE)
            return engrams[hr.best_index].quantity;
        return std::nullopt;
    }

    void set_quantity(Word owner, Word item, int val, Word prov, Word src, CognitiveTrace *tr = nullptr) {
        Word rel_has = markers[LM_HAS];
        if (!owner || !item || !rel_has)
            return;
        for (auto &e : engrams) {
            if ((e.flags & ENG_ACTIVE) && !(e.flags & ENG_SUPERSEDED) &&
                sdr_overlap(e.role[R_SUBJ], sdr_of(owner)) >= 16 &&
                sdr_overlap(e.role[R_REL], sdr_of(rel_has)) >= 16 &&
                sdr_overlap(e.role[R_OBJ], sdr_of(item)) >= 16) {
                e.flags |= ENG_SUPERSEDED;
                gpu_dirty_engrams = true;
            }
        }
        assert_fact(owner, rel_has, item, 0, 0, markers[LM_SELF], ENG_ACTIVE | ENG_EPISODIC, std::max(0, val), prov,
                    src, tr);
    }

    void apply_delta_quantity(Word owner, Word item, int delta, Word prov, Word src, CognitiveTrace *tr = nullptr) {
        auto cur = query_quantity(owner, item, tr);
        wm.qty_deltas[{owner, item}] += delta;
        if (cur.has_value())
            set_quantity(owner, item, std::max(0, *cur + delta), prov, src, tr);
        else if (delta > 0)
            set_quantity(owner, item, delta, prov, src, tr);
    }

    void apply_transfer_conservation(Word from_agent, Word to_agent, Word item, int amount, Word prov,
                                     CognitiveTrace *tr = nullptr) {
        if (!from_agent || !to_agent || !item || amount <= 0)
            return;
        wm.qty_deltas[{from_agent, item}] -= amount;
        wm.qty_deltas[{to_agent, item}] += amount;
        auto q_from = query_quantity(from_agent, item, tr);
        if (q_from.has_value())
            set_quantity(from_agent, item, std::max(0, *q_from - amount), prov, from_agent, tr);
        auto q_to = query_quantity(to_agent, item, tr);
        if (q_to.has_value() || q_from.has_value())
            set_quantity(to_agent, item, q_to.value_or(0) + amount, prov, from_agent, tr);
    }

    // Inverse Algebraic Word Problem Solver: given final quantity & recorded delta history,
    // recover initial quantity (`started = current - delta`)!
    std::optional<int> query_started_quantity(Word owner, Word item, CognitiveTrace *tr = nullptr) {
        auto cur = query_quantity(owner, item, tr);
        if (!cur.has_value())
            return std::nullopt;
        int delta = 0;
        auto it = wm.qty_deltas.find({owner, item});
        if (it != wm.qty_deltas.end())
            delta = it->second;
        int started = *cur - delta;
        SpikeVec vc = cochlear_encode(std::to_string(*cur), genome.wiring[2]);
        SpikeVec vd = cochlear_encode(std::to_string(delta), genome.wiring[2]);
        gpu_step_area(2, vc, vd, SpikeVec{}, tr);
        if (tr) ++tr->reasoning_hops;
        return started;
    }

    // Sum total quantity of `item` across all known agents!
    std::optional<int> query_total_quantity(Word item, CognitiveTrace *tr = nullptr) {
        if (abl_no_hipp || !item)
            return std::nullopt;
        int total = 0;
        bool found_any = false;
        for (Word i = 1; i < concepts.size(); ++i) {
            if (concepts[i].active && (concepts[i].categories & CAT_AGENT)) {
                auto q = query_quantity(i, item, tr);
                if (q.has_value()) {
                    total += *q;
                    found_any = true;
                }
            }
        }
        return found_any ? std::optional<int>(total) : std::nullopt;
    }

    Word learn_causal_schema(Word action, Word target_cat, const std::vector<Word> &req_pos,
                             const std::vector<Word> &req_neg, const std::vector<Word> &tools,
                             const std::vector<Word> &add_eff, const std::vector<Word> &del_eff,
                             Word produces = 0, Word cost = 1, bool embodied = true, CognitiveTrace *tr = nullptr) {
        if (abl_freeze || abl_no_causal || !action)
            return ~0u;
        SpikeVec s_act = sdr_of(action);
        SpikeVec s_cat = sdr_of(target_cat);
        SpikeVec s_rp{}, s_rn{}, s_tl{}, s_ae{}, s_de{}, s_pr = sdr_of(produces);
        for (Word x : req_pos) s_rp = sdr_or(s_rp, sdr_of(x));
        for (Word x : req_neg) s_rn = sdr_or(s_rn, sdr_of(x));
        for (Word x : tools)   s_tl = sdr_or(s_tl, sdr_of(x));
        for (Word x : add_eff) s_ae = sdr_or(s_ae, sdr_of(x));
        for (Word x : del_eff) s_de = sdr_or(s_de, sdr_of(x));

        for (Word i = 0; i < schemas.size(); ++i) {
            auto &cs = schemas[i];
            if (!(cs.flags & 1u))
                continue;
            if (sdr_overlap(cs.action, s_act) >= 20 &&
                (sdr_empty(s_cat) == sdr_empty(cs.target_cat) &&
                 (sdr_empty(s_cat) || sdr_overlap(cs.target_cat, s_cat) >= 16)) &&
                ((!sdr_empty(s_ae) && sdr_overlap(cs.add_eff, s_ae) >= 16) ||
                 (!sdr_empty(s_pr) && sdr_overlap(cs.produces, s_pr) >= 16))) {
                ++cs.observations;
                ++cs.confirmations;
                if (embodied && !sdr_empty(cs.req_pos) && !sdr_empty(s_rp)) {
                    for (Word w = 0; w < WORDS; ++w)
                        cs.req_pos.bits[w] &= s_rp.bits[w];
                } else {
                    cs.req_pos = sdr_or(cs.req_pos, s_rp);
                }
                cs.req_neg = sdr_or(cs.req_neg, s_rn);
                cs.tool_req = sdr_or(cs.tool_req, s_tl);
                cs.del_eff = sdr_or(cs.del_eff, s_de);
                gpu_dirty_schemas = true;
                ++lifetime_edits;
                if (tr) ++tr->synaptic_edits;
                return i;
            }
        }
        if (schemas.size() >= MAX_CAUSAL)
            return ~0u;
        CausalSchema cs{};
        cs.action = s_act;
        cs.target_cat = s_cat;
        cs.req_pos = s_rp;
        cs.req_neg = s_rn;
        cs.tool_req = s_tl;
        cs.add_eff = s_ae;
        cs.del_eff = s_de;
        cs.produces = s_pr;
        cs.observations = 1;
        cs.confirmations = 1;
        cs.energy_cost = cost;
        cs.flags = 1u | (embodied ? 4u : 2u);
        Word id = Word(schemas.size());
        schemas.push_back(cs);
        gpu_dirty_schemas = true;
        gpu_step_area(3, s_act, s_ae, s_rp, tr);
        ++lifetime_edits;
        if (tr) ++tr->synaptic_edits;
        return id;
    }

    Word sleep_consolidate(CognitiveTrace *tr = nullptr) {
        if (abl_freeze || abl_no_consolidate || abl_no_hipp)
            return 0;
        Word cycles = std::max(1u, sleep_replay_cycles(genome));
        Word consolidated = 0;
        Word prop = markers[LM_HAS_PROP];
        Word can = markers[LM_CAN];

        for (Word c = 0; c < cycles; ++c) {
            std::map<std::tuple<Word, Word, Word>, std::set<Word>> support;
            for (auto &e : engrams) {
                if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) || (e.flags & ENG_NEGATED) ||
                    (e.flags & (ENG_IMPLIES | ENG_DISJUNCTION)))
                    continue;
                if (e.strength < 40)
                    e.strength = Half(e.strength + 2);
                Word s = decode_sdr(e.role[R_SUBJ], 18);
                Word r = decode_sdr(e.role[R_REL], 18);
                Word o = decode_sdr(e.role[R_OBJ], 18);
                if (!s || !r || !o)
                    continue;
                if (r == prop || r == can) {
                    Word pcat = concepts[s].parent_cat;
                    if (pcat && pcat != s)
                        support[{pcat, r, o}].insert(s);
                }
            }
            for (auto &[key, members] : support) {
                if (members.size() < 2)
                    continue;
                auto [cat, r, o] = key;
                HippResult ex = query_fact(cat, r, o, 0, markers[LM_SELF], false, R_OBJ, ~0u, tr);
                if (ex.best_index == ~0u && ex.negated_index == ~0u) {
                    assert_fact(cat, r, o, 0, 0, markers[LM_SELF],
                                ENG_ACTIVE | ENG_UNIVERSAL | ENG_CONSOLIDATED, QTY_NONE, PROV_CONSOLIDATED, 0, tr);
                    ++consolidated;
                }
            }
            if (markers[LM_IS_A] && !abl_no_causal) {
                for (Word i = 0; i < schemas.size(); ++i) {
                    if (!(schemas[i].flags & 1u) || (schemas[i].flags & 8u))
                        continue;
                    Word tc = decode_sdr(schemas[i].target_cat, 18);
                    if (tc && concepts[tc].parent_cat) {
                        schemas[i].target_cat = sdr_or(schemas[i].target_cat, sdr_of(concepts[tc].parent_cat));
                        schemas[i].flags |= 8u;
                        gpu_dirty_schemas = true;
                        ++consolidated;
                    }
                }
            }
        }
        ++sleep_cycles_done;
        gpu_dirty_engrams = true;
        // Now launch native Blackwell GPU Sharp-Wave Ripple (SWR) consolidation into the
        // 3,145,728-synapse Neocortical Associative Matrix using the evolved `basal_rule` LUT!
        gpu_neo_consolidate(tr);
        return consolidated;
    }

    // -----------------------------------------------------------------------
    // v5 GPU Sharp-Wave Ripple (SWR) Consolidation into 3-MiB Neocortex!
    // Trains 3 x 1,048,576 real signed int8 synapses on Blackwell SM 12.0 using
    // the evolved `CorticalGenome.basal_rule` LUT!
    // -----------------------------------------------------------------------
    Word gpu_neo_consolidate(CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze || abl_no_consolidate)
            return 0;
        std::vector<NeoPair> pairs;
        pairs.reserve(512);
        Word rel_isa = markers[LM_IS_A];
        Word rel_can = markers[LM_CAN];
        Word rel_prop = markers[LM_HAS_PROP];
        Word rel_has = markers[LM_HAS];
        Word self_id = markers[LM_SELF];

        // Collect all active semantic facts & sensory features from Hippocampus (deduplicated per entity so repeated Homestead trials never saturate Pathway 0!)
        std::map<Word, std::set<Word>> member_features;
        std::map<Word, std::vector<Word>> entity_ancestors;
        std::vector<std::tuple<Word, Word, Word, Word, Word>> raw_p12; // (pre_id, post_id, pathway, negated, strength)
        std::set<std::tuple<Word, Word, Word, Word>> seen_p12;

        auto push_unique = [&](Word pre_id, Word post_id, Word pw, Word neg, Word str) {
            if (!pre_id || !post_id) return;
            auto key = std::make_tuple(pre_id, post_id, pw, neg);
            if (seen_p12.insert(key).second)
                raw_p12.push_back({pre_id, post_id, pw, neg, str});
        };

        for (const auto &e : engrams) {
            if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) || (e.flags & (ENG_IMPLIES | ENG_DISJUNCTION)))
                continue;
            if (sdr_overlap(e.role[R_PERSP], sdr_of(self_id)) < 16)
                continue;
            Word s = decode_sdr(e.role[R_SUBJ], 16);
            Word r = decode_sdr(e.role[R_REL], 16);
            Word o = decode_sdr(e.role[R_OBJ], 16);
            if (!s || !r || !o)
                continue;
            bool is_neg = (e.flags & ENG_NEGATED) != 0;

            if (r == rel_isa && !is_neg) {
                auto ancs = get_ancestors(s, nullptr);
                entity_ancestors[s] = ancs;
                for (Word anc : ancs)
                    push_unique(s, anc, 1, 0, 4);
            } else if (r == rel_can || r == rel_prop || r == rel_has) {
                if (is_neg) {
                    push_unique(s, o, 2, 1, 6);
                } else {
                    push_unique(s, o, 1, 0, 4);
                    if (r == rel_has || r == rel_prop)
                        member_features[s].insert(o);
                }
            }
        }

        // Propagate inherited category capabilities into Pathway 1 for each consolidated member
        // (so Ribot's Law works for `can sparrow fly?`, `is oak living?` even after Hippocampus is lesioned!)
        for (const auto &[ent, ancs] : entity_ancestors) {
            for (Word anc : ancs) {
                for (const auto &e : engrams) {
                    if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) || (e.flags & ENG_NEGATED))
                        continue;
                    if (sdr_overlap(e.role[R_PERSP], sdr_of(self_id)) < 16)
                        continue;
                    if (sdr_overlap(e.role[R_SUBJ], sdr_of(anc)) >= 18) {
                        Word r = decode_sdr(e.role[R_REL], 16);
                        Word o = decode_sdr(e.role[R_OBJ], 16);
                        if ((r == rel_can || r == rel_prop) && o)
                            push_unique(ent, o, 1, 0, r == rel_can ? 4 : 3);
                    }
                }
            }
        }

        // Pathway 0 FIRST (Ventral Prototype Stream): Sensory Features -> Immediate Category Attractor!
        // Plus Pathway 3 (Semantic Similarity Manifold): Features & Category -> Exemplars!
        for (const auto &[ent, feats] : member_features) {
            auto it = entity_ancestors.find(ent);
            if (it == entity_ancestors.end() || it->second.empty())
                continue;
            Word cat = it->second[0]; // Immediate prototype category (bird, fish, tree...)
            for (Word f : feats) {
                NeoPair np{};
                np.pre = sdr_of(f);
                np.post = sdr_of(cat);
                np.pathway = 0;
                np.negated = 0;
                np.strength = 3;
                pairs.push_back(np);

                // Pathway 3: Sensory feature -> Exemplar prototype manifold!
                NeoPair sm{};
                sm.pre = sdr_of(f);
                sm.post = sdr_of(ent);
                sm.pathway = 3;
                sm.negated = 0;
                sm.strength = 4;
                pairs.push_back(sm);
            }
            // Pathway 3: Immediate category -> Exemplar prototype manifold!
            NeoPair cm{};
            cm.pre = sdr_of(cat);
            cm.post = sdr_of(ent);
            cm.pathway = 3;
            cm.negated = 0;
            cm.strength = 2;
            pairs.push_back(cm);
        }
        // Also consolidate direct category abilities/features (`fish can swim`, `mammal has fur/milk`, `bird has wings/feathers`)
        // into Pathway 0 (SELF perspective only, deduplicated!) so Kernel 10 `mind_neo_hypothesize` ranks prototypes purely on Blackwell L2 (Undecuple Ribot!)
        std::set<std::pair<Word, Word>> seen_cat_p0;
        for (const auto &e : engrams) {
            if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) || (e.flags & ENG_NEGATED) ||
                (e.flags & (ENG_IMPLIES | ENG_DISJUNCTION | ENG_HYPOTHETICAL)))
                continue;
            if (sdr_overlap(e.role[R_PERSP], sdr_of(self_id)) < 16)
                continue;
            Word s = decode_sdr(e.role[R_SUBJ], 16);
            Word r = decode_sdr(e.role[R_REL], 16);
            Word o = decode_sdr(e.role[R_OBJ], 16);
            if (!s || !r || !o) continue;
            if ((r == rel_has || r == rel_can) && (concepts[s].categories & CAT_CATEGORY)) {
                if (!seen_cat_p0.insert({o, s}).second) continue;
                NeoPair np{};
                np.pre = sdr_of(o);
                np.post = sdr_of(s);
                np.pathway = 0;
                np.negated = 0;
                np.strength = 4;
                pairs.push_back(np);
            }
        }
        // Then Pathways 1 & 2 (Consolidated Semantic Knowledge & Inhibitory Exceptions!)
        for (const auto &[pre_id, post_id, pw, neg, str] : raw_p12) {
            NeoPair np{};
            np.pre = sdr_of(pre_id);
            np.post = sdr_of(post_id);
            np.pathway = pw;
            np.negated = neg;
            np.strength = str;
            pairs.push_back(np);
        }

        // Pathway 4 (Associative State & Cyclic Transition Predictor): e.g. spring -> summer -> autumn -> winter!
        Word rel_after = markers[LM_AFTER];
        if (rel_after) {
            std::set<std::pair<Word, Word>> seen_p4;
            for (const auto &e : engrams) {
                if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) || (e.flags & ENG_NEGATED))
                    continue;
                if (sdr_overlap(e.role[R_PERSP], sdr_of(self_id)) < 16)
                    continue;
                if (sdr_overlap(e.role[R_REL], sdr_of(rel_after)) >= 18) {
                    Word s = decode_sdr(e.role[R_SUBJ], 16);
                    Word o = decode_sdr(e.role[R_OBJ], 16);
                    if (s && o && !seen_p4.insert({o, s}).second)
                        continue;
                    NeoPair np{};
                    np.pre = e.role[R_OBJ];  // "summer comes after spring" -> pre=spring, post=summer!
                    np.post = e.role[R_SUBJ];
                    np.pathway = 4;
                    np.negated = 0;
                    np.strength = 4;
                    pairs.push_back(np);
                }
            }
        }

        // Pathway 5 (Spiking Neural Wernicke Interrogative Intent Classifier):
        // Salience-gated (IDF) Word-SDR -> Intent-SDR via evolved basal_rule!
        // Trains on interrogative & discourse intents, ignoring ubiquitous closed-class function words (df > 10)
        // so discriminative semantic intent cues strongly settle onto the exact target intent SDR!
        auto is_intent_q = [](Word in) {
            return (in >= INT_ASK_WHERE && in <= INT_ASK_REL_BETWEEN) || in == INT_ASK_WHY_NOT ||
                   (in >= INT_ASK_SUM && in <= INT_ASK_CAPABILITIES) ||
                   (in >= INT_ASK_ARITH && in <= INT_ASK_EXEMPLAR) ||
                   in == INT_ASK_NAME || in == INT_ASK_DESIRE || in == INT_ASK_LOSE_TOOL ||
                   in == INT_ASK_WHAT_KIND || in == INT_ASK_DIAGNOSE || in == INT_ASK_TRUST ||
                   in == INT_ASK_STARTED || in == INT_ASK_PROPORTION || in == INT_ASK_SUMMARY ||
                   in == INT_ASK_REMINDERS || in == INT_ASK_BEFORE || in == INT_ASK_COST ||
                   in == INT_ASK_FAIR_SHARE || in == INT_ASK_WOULD_LIKE || in == INT_ASK_SIMILAR ||
                   in == INT_ASK_NUM_CMP || in == INT_ASK_FRACTION || in == INT_ASK_NEED_TOOL ||
                   in == INT_ASK_KNOWS_WHERE ||
                   (in >= INT_ASK_RIDDLE && in <= INT_ASK_CYCLE_NEXT && in != INT_ASSERT_BRIDGE) ||
                   in == INT_ASK_PEMDAS || in == INT_ASK_PERCENT || in == INT_ASK_MORAL ||
                   in == INT_ASK_PROMISE || in == INT_ASK_NORM || in == INT_ASK_PREDICT_EFFECT ||
                   in == INT_ASK_BALANCE || in == INT_ASK_OVERFLOW || in == INT_ASK_INCL_EXCL ||
                   in == INT_ASK_PIGEONHOLE || in == INT_ASK_AGE_DIFF || in == INT_ASK_PARITY ||
                   in == INT_ASK_DESIGN_EXP || in == INT_ASK_FAIR_TEST || in == INT_ASK_KINSHIP ||
                   in == INT_ASK_COUNTER_MOVE || in == INT_ASK_MINIMAX || in == INT_ASK_TIT_FOR_TAT ||
                   in == INT_ASK_FEEL || in == INT_ASK_EMPATHY_HELP || in == INT_ASK_PRAGMATIC ||
                   in == INT_ASK_METAPHOR || in == INT_ASK_EXPECTED_VAL || in == INT_ASK_PROVE ||
                   in == INT_ASK_WHY_CHANGED || in == INT_ASK_THINK_CHAIN ||
                   in == INT_ASK_STORY_WHY || in == INT_ASK_SOLVE_EQ || in == INT_ASK_KNAPSACK ||
                   in == INT_ASK_SCHEDULE || in == INT_ASK_CRIT_PATH || in == INT_ASK_SCHEMA_XFER ||
                   in == INT_ASK_GRID_DIST || in == INT_ASK_DIRECTION || in == INT_ASK_LINE_SIGHT ||
                   in == INT_ASK_COMMON_KNOW || in == INT_ASK_HIST_CF || in == INT_ASK_META_WHY ||
                   in == INT_ASK_BAYES || in == INT_ASK_PROP_LOGIC || in == INT_ASK_COMBINAT ||
                   in == INT_ASK_NUM_THEORY || in == INT_ASK_RATIO_RATE || in == INT_ASK_CLOCK_INTV ||
                   in == INT_ASK_KNIGHT || in == INT_ASK_GRAPH || in == INT_ASK_DIALOG_ACT ||
                   in == INT_ASK_NEO_CONTRAST;
        };
        std::map<Word, std::set<Word>> cue_intents;
        for (const auto &c : constructions) {
            if (!c.active || c.is_production || !is_intent_q(c.intent))
                continue;
            for (Word k = 0; k < c.length; ++k)
                if (c.slot[k] == SLOT_FIXED && c.fixed_concept[k] && !filler_concepts.count(c.fixed_concept[k]))
                    cue_intents[c.fixed_concept[k]].insert(c.intent);
        }
        std::set<Word> hedge_words;
        for (const auto &h : learned_hedges)
            for (Word hw : h) hedge_words.insert(hw);
        auto is_generic_stop = [&](Word fc) {
            return hedge_words.count(fc) != 0 ||
                   fc == markers[LM_SELF] || fc == markers[LM_USER] || fc == markers[LM_DO] ||
                   fc == markers[LM_WHAT] || fc == markers[LM_WHERE] || fc == markers[LM_WHO] ||
                   fc == markers[LM_HOW] || fc == markers[LM_WHICH] || fc == markers[LM_IS_A] ||
                   fc == markers[LM_HAS_PROP] || fc == markers[LM_IN] || fc == markers[LM_AND] ||
                   fc == markers[LM_OR] || fc == markers[LM_IF] || fc == markers[LM_CAN];
        };
        std::set<std::pair<Word, Word>> seen_p5;
        for (const auto &[fc, intents] : cue_intents) {
            if (is_generic_stop(fc) || intents.empty() || intents.size() > 10)
                continue;
            Word str = (intents.size() <= 2) ? 6u : (intents.size() <= 5 ? 4u : 3u);
            for (Word in : intents) {
                if (!seen_p5.insert({fc, in}).second)
                    continue;
                NeoPair ip{};
                ip.pre = concepts[fc].lexical_sdr;
                ip.post = intent_sdr(in);
                ip.pathway = 5;
                ip.negated = 0;
                ip.strength = str;
                pairs.push_back(ip);
            }
        }

        // Pathway 6 (Associative Relation & Analogy Cortex): tool_for, antonym & relational pairs!
        for (const auto &e : engrams) {
            if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) || (e.flags & ENG_NEGATED))
                continue;
            if (sdr_overlap(e.role[R_PERSP], sdr_of(self_id)) < 16)
                continue;
            Word r = decode_sdr(e.role[R_REL], 16);
            if (r && r != rel_isa && r != rel_prop && r != rel_can && r != rel_has && r != markers[LM_IN]) {
                NeoPair ap{};
                ap.pre = sdr_or(e.role[R_SUBJ], e.role[R_REL]);
                ap.post = e.role[R_OBJ];
                ap.pathway = 6;
                ap.negated = 0;
                ap.strength = 3;
                pairs.push_back(ap);
            }
        }

        // Pathway 7 (System-1 Fast Intuitive Physics Predictor — survives hippocampal lesion!):
        // Trains `(entity/category + action) -> predicted physical effect` AND `(req_pos + action) -> effect`
        // so 2-hop recurrent `mind_neo_think_chain` cascades (e.g. `ice+melt->liquid` + `boil->steam`) run purely on GPU!
        for (const auto &cs : schemas) {
            if (!(cs.flags & 1u) || sdr_empty(cs.action) || sdr_empty(cs.add_eff))
                continue;
            Word act_c = decode_sdr(cs.action, 14);
            Word cat_c = decode_sdr(cs.target_cat, 14);
            if (!act_c) continue;
            if (cat_c) {
                NeoPair sp{};
                sp.pre = sdr_or(sdr_of(cat_c), sdr_of(act_c));
                sp.post = cs.add_eff;
                sp.pathway = 7;
                sp.negated = 0;
                sp.strength = 4;
                pairs.push_back(sp);
            }
            if (!sdr_empty(cs.req_pos)) {
                NeoPair rp{};
                rp.pre = sdr_or(cs.req_pos, sdr_of(act_c));
                rp.post = cs.add_eff;
                rp.pathway = 7;
                rp.negated = 0;
                rp.strength = 5;
                pairs.push_back(rp);
            }
            for (const auto &[ent, ancs] : entity_ancestors) {
                if (cat_c && std::find(ancs.begin(), ancs.end(), cat_c) != ancs.end()) {
                    NeoPair ep{};
                    ep.pre = sdr_or(sdr_of(ent), sdr_of(act_c));
                    ep.post = cs.add_eff;
                    ep.pathway = 7;
                    ep.negated = 0;
                    ep.strength = 5;
                    pairs.push_back(ep);
                }
            }
            // Pathway 10 (Inverse-Causal Diagnostic Cortex — survives hippocampal lesion!):
            // Maps observed physical effect SDR -> causal action SDR (`steam -> boil`, `liquid -> melt`, `fired -> fire`)!
            NeoPair ic{};
            ic.pre = cs.add_eff;
            ic.post = sdr_of(act_c);
            ic.pathway = 10;
            ic.negated = 0;
            ic.strength = 5;
            pairs.push_back(ic);
        }

        // Pathway 8 (Multi-Hop Taxonomic Spreading Cortex for `mind_neo_think_chain` — survives hippocampal lesion!):
        // Trains 1-hop differential parent edges (`sub & ~sup -> sup`) so branching categories (`bird/fish/mammal -> animal`)
        // never create self-trapping attractors and cleanly spread `sparrow -> bird -> animal -> living` on GPU!
        {
            std::set<std::pair<Word, Word>> seen_p8;
            auto add_tax_edge = [&](Word sub, Word sup) {
                if (!sub || !sup || sub == sup || !seen_p8.insert({sub, sup}).second)
                    return;
                NeoPair tp{};
                tp.pre = sdr_and_not(sdr_of(sub), sdr_of(sup));
                if (sdr_empty(tp.pre)) tp.pre = sdr_of(sub);
                tp.post = sdr_of(sup);
                tp.pathway = 8;
                tp.negated = 0;
                tp.strength = 5;
                pairs.push_back(tp);
            };
            for (Word i = 1; i < concepts.size(); ++i)
                if (concepts[i].active && concepts[i].parent_cat)
                    add_tax_edge(i, concepts[i].parent_cat);
            for (const auto &e : engrams) {
                if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) || (e.flags & ENG_NEGATED))
                    continue;
                if (sdr_overlap(e.role[R_REL], sdr_of(rel_isa)) < 18)
                    continue;
                add_tax_edge(decode_sdr(e.role[R_SUBJ], 14), decode_sdr(e.role[R_OBJ], 14));
            }
        }

        // Pathway 9 (Affective Appraisal & Emotional Empathy Cortex — survives hippocampal lesion!):
        // Maps emotional event cues -> affective state attractors via evolved `basal_rule` LUT!
        {
            auto add_aff = [&](LexMarker cue_mk, LexMarker emo_mk, Word str) {
                if (markers[cue_mk] && markers[emo_mk]) {
                    NeoPair af{};
                    af.pre = sdr_of(markers[cue_mk]);
                    af.post = sdr_of(markers[emo_mk]);
                    af.pathway = 9;
                    af.negated = 0;
                    af.strength = str;
                    pairs.push_back(af);
                }
            };
            add_aff(LM_LOST, LM_SAD, 6);
            add_aff(LM_BROKEN, LM_SAD, 5);
            add_aff(LM_LIED, LM_SAD, 5);
            add_aff(LM_FOUND, LM_HAPPY, 6);
            add_aff(LM_CHASED, LM_AFRAID, 6);
        }

        // Pathway 11 (Compositional Kinship & Dominance Relational Cortex):
        if (markers[LM_BEATS] || markers[LM_PARENT_OF]) {
            for (const auto &e : engrams) {
                if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) || (e.flags & ENG_NEGATED))
                    continue;
                Word r = decode_sdr(e.role[R_REL], 16);
                if (r && (r == markers[LM_BEATS] || r == markers[LM_PARENT_OF])) {
                    NeoPair rb{};
                    rb.pre = sdr_or(e.role[R_SUBJ], e.role[R_REL]);
                    rb.post = e.role[R_OBJ];
                    rb.pathway = 11;
                    rb.negated = 0;
                    rb.strength = 4;
                    pairs.push_back(rb);
                }
            }
        }

        // Pathway 14 (2D Open-Valley Spatial Cardinal Direction Cortex — survives hippocampal lesion!):
        // Consolidates `(landmark_from + landmark_to) -> cardinal_direction` directly from `map_landmarks`!
        if (markers[LM_NORTH] && markers[LM_SOUTH] && markers[LM_EAST] && markers[LM_WEST] && !map_landmarks.empty()) {
            for (const auto &[a_id, axy] : map_landmarks) {
                for (const auto &[b_id, bxy] : map_landmarks) {
                    if (a_id == b_id) continue;
                    int dx = int(bxy.first) - int(axy.first);
                    int dy = int(bxy.second) - int(axy.second);
                    Word dir = 0;
                    if (std::abs(dx) >= std::abs(dy))
                        dir = dx >= 0 ? markers[LM_EAST] : markers[LM_WEST];
                    else
                        dir = dy >= 0 ? markers[LM_SOUTH] : markers[LM_NORTH];
                    if (dir) {
                        NeoPair sp{};
                        sp.pre = sdr_or(sdr_of(a_id), sdr_and_not(sdr_of(b_id), sdr_of(a_id)));
                        // Distinguish ordered pair `(from=a_id, to=b_id)` cleanly via rotated/salted SDR cue!
                        sp.pre = sdr_or(sdr_of(a_id), cochlear_encode("to_" + word_of(b_id), genome.wiring[2] ^ 0x41a7u));
                        sp.post = sdr_of(dir);
                        sp.pathway = 14;
                        sp.negated = 0;
                        sp.strength = 5;
                        pairs.push_back(sp);
                    }
                }
            }
        }

        // Pathway 15 (Dialogic Discourse-Act Cortex — consolidates follow-up cues):
        if (markers[LM_WHY] && markers[LM_BECAUSE]) {
            NeoPair da{};
            da.pre = sdr_of(markers[LM_WHY]);
            da.post = sdr_of(markers[LM_BECAUSE]);
            da.pathway = 15;
            da.negated = 0;
            da.strength = 4;
            pairs.push_back(da);
        }

        // Pathway 16 (Physical State-Transition Graph Cortex — powers Kernel 11 `mind_neo_bridge`, Duodecuple Ribot!):
        // Consolidates discovered physical state transitions (`req_pos -> add_eff` and `entity -> initial add_eff`,
        // e.g. `ice -> liquid -> steam`, `pearl -> warm -> sown`) purely from real causal schemas & SELF engrams!
        {
            std::set<std::pair<Word, Word>> seen_p16;
            auto add_p16 = [&](Word pre_id, Word post_id, Word str) {
                if (!pre_id || !post_id || pre_id == post_id || !seen_p16.insert({pre_id, post_id}).second)
                    return;
                NeoPair trp{};
                trp.pre = sdr_of(pre_id);
                trp.post = sdr_of(post_id);
                trp.pathway = 16;
                trp.negated = 0;
                trp.strength = str;
                pairs.push_back(trp);
            };
            std::map<Word, std::vector<Word>> prop_to_next;
            for (const auto &cs : schemas) {
                if (!(cs.flags & 1u) || sdr_empty(cs.req_pos) || sdr_empty(cs.add_eff)) continue;
                std::vector<Word> reqs, adds;
                for (Word i = 1; i < concepts.size(); ++i) {
                    if (!concepts[i].active || !(concepts[i].categories & CAT_PROPERTY)) continue;
                    if (sdr_overlap(cs.req_pos, concepts[i].concept_sdr) >= 16) reqs.push_back(i);
                    if (sdr_overlap(cs.add_eff, concepts[i].concept_sdr) >= 16) adds.push_back(i);
                }
                for (Word rp : reqs) {
                    for (Word ap : adds) {
                        if (rp != ap) {
                            // Primary forward causal progression (`solid->liquid->steam`) gets strength 6;
                            // entity shortcuts (`ice->liquid`, `water->steam`) get 5 and secondary branches get 3
                            // so both forward dreaming & Kernel-17 top-down backward abduction are 100% deterministic!
                            Word p16_str = prop_to_next[rp].empty() ? 6u : 3u;
                            add_p16(rp, ap, p16_str);
                            prop_to_next[rp].push_back(ap);
                        }
                    }
                }
            }
            for (const auto &e : engrams) {
                if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) || (e.flags & ENG_NEGATED) ||
                    (e.flags & (ENG_IMPLIES | ENG_DISJUNCTION | ENG_HYPOTHETICAL)))
                    continue;
                if (sdr_overlap(e.role[R_PERSP], sdr_of(self_id)) < 16)
                    continue;
                Word s = decode_sdr(e.role[R_SUBJ], 16);
                Word r = decode_sdr(e.role[R_REL], 16);
                Word o = decode_sdr(e.role[R_OBJ], 16);
                if (!s || r != rel_prop || !o) continue;
                auto it = prop_to_next.find(o);
                if (it != prop_to_next.end()) {
                    for (Word nxt : it->second)
                        add_p16(s, nxt, 5);
                }
            }
        }

        // Pathway 17 (Syllogistic & Transitive Set-Inclusion Closure Cortex — survives hippocampal lesion!):
        // Consolidates full transitive ancestor closure `(entity -> root_supercategory)` directly into Pathway 17
        // so formal syllogistic closure (`sparrow -> living`, `oak -> living`) runs in 1 GPU settle (Septuple Ribot!)
        for (const auto &[ent, ancs] : entity_ancestors) {
            for (Word anc : ancs) {
                NeoPair sy{};
                sy.pre = sdr_of(ent);
                sy.post = sdr_of(anc);
                sy.pathway = 17;
                sy.negated = 0;
                sy.strength = 4;
                pairs.push_back(sy);
            }
        }

        // Pathway 18 (Antonym & Semantic Polarity Cortex — powers Kernel 7 `mind_neo_analogy`, Octuple Ribot!):
        // Consolidates all semantic opposites (`hot <-> cold`, `wet <-> dry`, `heavy <-> light`, `happy <-> sad`, etc.)
        // directly into Pathway 18 during GPU SWR sleep replay!
        for (Word i = 1; i < concepts.size(); ++i) {
            if (!concepts[i].active || !concepts[i].antonym_concept) continue;
            Word opp = concepts[i].antonym_concept;
            NeoPair an{};
            an.pre = sdr_of(i);
            an.post = sdr_of(opp);
            an.pathway = 18;
            an.negated = 0;
            an.strength = 4;
            pairs.push_back(an);
        }

        // Pathways 20 & 21 (Inverted Ability/Feature -> Exemplar & Negated-Exception -> Exemplar Matrices!):
        // Power Kernel 8 `mind_neo_compose` compositional intersection & veto purely on GPU (Nonuple Ribot!)
        // Uses cortical sparse decorrelation (`ent & ~parent_cat`) so shared category hub bits never drown out
        // exemplar-specific neurons (`bat` vs `robin` on `mammal + fly`) across any evolved seed or dialect!
        {
            std::set<std::tuple<Word, Word, Word>> seen_p20;
            for (const auto &e : engrams) {
                if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) ||
                    (e.flags & (ENG_IMPLIES | ENG_DISJUNCTION | ENG_HYPOTHETICAL)))
                    continue;
                if (sdr_overlap(e.role[R_PERSP], sdr_of(self_id)) < 16)
                    continue;
                Word s = decode_sdr(e.role[R_SUBJ], 16);
                Word r = decode_sdr(e.role[R_REL], 16);
                Word o = decode_sdr(e.role[R_OBJ], 16);
                if (!s || !r || !o) continue;
                if (r == markers[LM_CAN] || r == markers[LM_HAS]) {
                    Word pw = (e.flags & ENG_NEGATED) ? 21u : 20u;
                    if (!seen_p20.insert({o, s, pw}).second) continue;
                    SpikeVec post_sv = sdr_of(s);
                    auto it_a = entity_ancestors.find(s);
                    if (it_a != entity_ancestors.end()) {
                        for (Word anc : it_a->second) {
                            SpikeVec dec = sdr_and_not(post_sv, sdr_of(anc));
                            if (sdr_popcount(dec) >= 12) post_sv = dec;
                        }
                    }
                    NeoPair inv{};
                    inv.pre = sdr_of(o);
                    inv.post = post_sv;
                    inv.pathway = pw;
                    inv.negated = 0;
                    inv.strength = 4;
                    pairs.push_back(inv);
                }
            }
        }

        // Pathway 22 (Category -> All Categorized Exemplars — decorrelated exemplar bank for Kernel 8 `mind_neo_compose`!):
        {
            std::set<std::pair<Word, Word>> seen_p22;
            for (const auto &[ent, ancs] : entity_ancestors) {
                SpikeVec post_sv = sdr_of(ent);
                for (Word anc : ancs) {
                    SpikeVec dec = sdr_and_not(post_sv, sdr_of(anc));
                    if (sdr_popcount(dec) >= 12) post_sv = dec;
                }
                for (size_t ai = 0; ai < ancs.size(); ++ai) {
                    Word anc = ancs[ai];
                    if (!seen_p22.insert({anc, ent}).second) continue;
                    NeoPair ce{};
                    ce.pre = sdr_of(anc);
                    ce.post = post_sv;
                    ce.pathway = 22;
                    ce.negated = 0;
                    ce.strength = (ai == 0) ? 5u : 2u;
                    pairs.push_back(ce);
                }
            }
        }

        // Pathway 23 (Inverse Causal Goal -> Required Tool Precondition Cortex — survives hippocampal lesion!):
        // Consolidates BOTH `(add_eff -> tool_req)` (`cooked -> fire`, `open -> key`, `fired -> fire`) AND
        // `(produces -> tool_req)` (`fire -> flint`, `pot -> fire`) so Kernel 19 (`mind_neo_plan_chain`) can
        // unroll 100% embodied-discovered 2-hop hierarchical tool chains inside 28-MiB L2!
        for (const auto &cs : schemas) {
            if (!(cs.flags & 1u) || sdr_empty(cs.tool_req)) continue;
            Word tl = decode_sdr(cs.tool_req, 12);
            if (!tl) continue;
            if (!sdr_empty(cs.add_eff)) {
                for (Word i = 1; i < concepts.size(); ++i) {
                    if (!concepts[i].active || !(concepts[i].categories & CAT_PROPERTY)) continue;
                    if (sdr_overlap(cs.add_eff, concepts[i].concept_sdr) >= 16) {
                        NeoPair gt{};
                        gt.pre = concepts[i].concept_sdr;
                        gt.post = sdr_of(tl);
                        gt.pathway = 23;
                        gt.negated = 0;
                        gt.strength = 5;
                        pairs.push_back(gt);
                    }
                }
            }
            if (!sdr_empty(cs.produces)) {
                Word prod = decode_sdr(cs.produces, 12);
                if (prod && prod != tl) {
                    NeoPair gt{};
                    gt.pre = sdr_of(prod);
                    gt.post = sdr_of(tl);
                    gt.pathway = 23;
                    gt.negated = 0;
                    gt.strength = 5;
                    pairs.push_back(gt);
                }
            }
        }

        // Pathway 26 (Multi-Cue Causal Synthesis Context Cortex — powers Kernel 9 `mind_neo_multi_cue`, Decuple Ribot!):
        // Consolidates `(target/entity, tool, action) -> primary_outcome` (`produces` if crafted else `add_eff`)!
        // Survives `--ablate hipp` while collapsing under `--ablate causal`!
        for (const auto &cs : schemas) {
            if (!(cs.flags & 1u) || sdr_empty(cs.action) || sdr_empty(cs.tool_req)) continue;
            SpikeVec out_sv = !sdr_empty(cs.produces) ? cs.produces : cs.add_eff;
            if (sdr_empty(out_sv)) continue;
            // Pick crisp single concept SDR for primary outcome (`fire`, `cooked`, `open`, `pot`)
            Word out_c = !sdr_empty(cs.produces) ? decode_sdr(cs.produces, 12) : decode_sdr(cs.add_eff, 12, CAT_PROPERTY);
            if (!out_c) out_c = decode_sdr(out_sv, 12);
            if (!out_c) continue;
            auto add_p26 = [&](const SpikeVec &pre_sv, Word str) {
                if (sdr_empty(pre_sv)) return;
                NeoPair p26{};
                p26.pre = pre_sv;
                p26.post = sdr_of(out_c);
                p26.pathway = 26;
                p26.negated = 0;
                p26.strength = str;
                pairs.push_back(p26);
            };
            add_p26(cs.action, 5);
            add_p26(cs.tool_req, 3);
            if (!sdr_empty(cs.target_cat)) {
                add_p26(cs.target_cat, 4);
                Word tcat = decode_sdr(cs.target_cat, 14);
                if (tcat) {
                    for (const auto &[ent, ancs] : entity_ancestors) {
                        if (!ancs.empty() && ancs[0] == tcat)
                            add_p26(sdr_of(ent), 4);
                    }
                }
            }
        }

        if (pairs.empty())
            return 0;
        if (pairs.size() > gpu_neo_pair_cap)
            pairs.resize(gpu_neo_pair_cap);

        // Reset weights before full sleep-replay consolidation pass so repeated sleeps remain bounded cleanly
        std::fill(neo_weights.begin(), neo_weights.end(), static_cast<signed char>(0));
        gpu_dirty_neo = true;
        sync_neo_to_gpu();

        auto t0 = std::chrono::steady_clock::now();
        Word n_pairs = Word(pairs.size());
        Word zero = 0;
        session->d.check(session->d.upload(d_neo_pairs->ptr, pairs.data(), size_t(n_pairs) * sizeof(NeoPair)),
                         "upload swr pairs");
        session->d.check(session->d.upload(d_neo_edits->ptr, &zero, sizeof(Word)), "zero neo edits");
        session->d.check(session->d.upload(d_neo_spikes->ptr, &zero, sizeof(Word)), "zero neo spikes");

        void *args[]{&d_neo_weights->ptr, &d_neo_pairs->ptr, &n_pairs, &d_genome->ptr,
                     &d_neo_edits->ptr, &d_neo_spikes->ptr};
        session->d.check(session->d.launch(fn_neo_ripple, 32, 1, 1, 32, 1, 1, 0, nullptr, args, nullptr),
                         "gpu swr sleep ripple");

        Word n_ed = 0, n_sp = 0;
        session->d.check(session->d.download(&n_ed, d_neo_edits->ptr, sizeof(Word)), "download neo edits");
        session->d.check(session->d.download(&n_sp, d_neo_spikes->ptr, sizeof(Word)), "download neo spikes");
        session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                         "download neocortical weights");

        neo_consolidated_ready = true;
        lifetime_edits += n_ed;
        lifetime_spikes += n_sp;
        if (tr) {
            tr->synaptic_edits += n_ed;
            tr->gpu_spikes += n_sp;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        // Stage 5 (K27, K28 & K29): Consolidate dialect-learned syntax (PW12), parietal math (PW15) & graph wavefront (PW11)!
        n_ed += gpu_train_k27_grammar_from_constructions(tr);
        n_ed += gpu_train_k28_neural_math(tr);
        n_ed += gpu_train_k29_graph_wave(tr);
        n_ed += gpu_train_k30_sat_solve(tr);
        n_ed += gpu_train_k31_visual_cortex(tr);
        n_ed += gpu_train_k32_audio_cortex(tr);
        n_ed += gpu_train_k33_crossmodal_stack(tr);
        n_ed += gpu_train_k34_dialogue_cortex(tr);
        n_ed += gpu_train_k35_narrative_metaphor(tr);
        n_ed += gpu_train_k36_belief_debate(tr);
        n_ed += gpu_train_k37_collective_discovery(tr);
        n_ed += gpu_train_k38_self_research(tr);
        n_ed += gpu_train_k39_symbolic_calculus(tr);
        n_ed += gpu_train_k40_program_ast(tr);
        n_ed += gpu_train_k41_linear_algebra(tr);
        n_ed += gpu_train_k42_bayes_net(tr);
        n_ed += gpu_train_k43_mdp_planner(tr);
        n_ed += gpu_train_k44_kalman_filter(tr);
        n_ed += gpu_train_k45_lqr_control(tr);
        n_ed += gpu_train_k46_fft_spectrum(tr);
        n_ed += gpu_train_k47_ode_rk4(tr);
        n_ed += gpu_train_k48_simplex_lp(tr);
        n_ed += gpu_train_k49_pca_eigen(tr);
        n_ed += gpu_train_k50_pde_solver(tr);
        n_ed += gpu_train_k51_mcmc_sampler(tr);
        n_ed += gpu_train_k52_mcts_planner(tr);
        return n_ed;
    }

    // v7 Live Online GPU Neocortical Micro-Ripple Plasticity (Zero Sleep Required!)
    // Immediately consolidates a newly observed (entity, feature) pair of a categorized entity
    // into Pathways 0 & 3 of the 4.19M-synapse Blackwell GPU Neocortex using `g.basal_rule`!
    Word gpu_neo_online_ripple(Word entity, Word feat, CognitiveTrace *tr = nullptr) {
        if (!session || abl_freeze || abl_no_consolidate || abl_no_hipp || !entity || !feat)
            return 0;
        auto ancs = get_ancestors(entity, nullptr);
        if (ancs.empty())
            return 0;
        Word cat = ancs[0];
        std::vector<NeoPair> pairs;
        pairs.reserve(3);
        NeoPair p0{};
        p0.pre = sdr_of(feat); p0.post = sdr_of(cat); p0.pathway = 0; p0.negated = 0; p0.strength = 4;
        pairs.push_back(p0);
        NeoPair p3f{};
        p3f.pre = sdr_of(feat); p3f.post = sdr_of(entity); p3f.pathway = 3; p3f.negated = 0; p3f.strength = 4;
        pairs.push_back(p3f);
        NeoPair p3c{};
        p3c.pre = sdr_of(cat); p3c.post = sdr_of(entity); p3c.pathway = 3; p3c.negated = 0; p3c.strength = 2;
        pairs.push_back(p3c);

        sync_neo_to_gpu();
        auto t0 = std::chrono::steady_clock::now();
        Word n_pairs = Word(pairs.size()), zero = 0;
        session->d.check(session->d.upload(d_neo_pairs->ptr, pairs.data(), size_t(n_pairs) * sizeof(NeoPair)),
                         "upload micro-ripple pairs");
        session->d.check(session->d.upload(d_neo_edits->ptr, &zero, sizeof(Word)), "zero micro edits");
        session->d.check(session->d.upload(d_neo_spikes->ptr, &zero, sizeof(Word)), "zero micro spikes");
        void *args[]{&d_neo_weights->ptr, &d_neo_pairs->ptr, &n_pairs, &d_genome->ptr,
                     &d_neo_edits->ptr, &d_neo_spikes->ptr};
        session->d.check(session->d.launch(fn_neo_ripple, 32, 1, 1, 32, 1, 1, 0, nullptr, args, nullptr),
                         "gpu online micro-ripple");
        Word n_ed = 0, n_sp = 0;
        session->d.check(session->d.download(&n_ed, d_neo_edits->ptr, sizeof(Word)), "download micro edits");
        session->d.check(session->d.download(&n_sp, d_neo_spikes->ptr, sizeof(Word)), "download micro spikes");
        session->d.check(session->d.download(neo_weights.data(), d_neo_weights->ptr, neo_weights.size()),
                         "download micro weights");
        neo_consolidated_ready = true;
        lifetime_edits += n_ed;
        lifetime_spikes += n_sp;
        if (tr) {
            tr->synaptic_edits += n_ed;
            tr->gpu_spikes += n_sp;
            tr->gpu_us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        }
        return n_ed;
    }

    // Zero-Shot Feature-Based Abductive Classification on GPU Neocortex Pathway 0!
    // Given an unseen creature described ONLY by its features (e.g. "kestrel has wings and feathers"),
    // superposes its feature SDRs and settles the 1024x1024 GPU Neocortex -> spontaneously activates `bird`!
    Word abduct_category_neocortex(Word entity, CognitiveTrace *tr = nullptr) {
        if (!entity || !neo_consolidated_ready)
            return 0;
        Word rel_has = markers[LM_HAS];
        Word rel_prop = markers[LM_HAS_PROP];
        SpikeVec feat_sdr{};
        Word fcount = 0;
        for (const auto &e : engrams) {
            if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) || (e.flags & ENG_NEGATED))
                continue;
            if (sdr_overlap(e.role[R_SUBJ], sdr_of(entity)) < 18)
                continue;
            if (sdr_overlap(e.role[R_REL], sdr_of(rel_has)) >= 18 ||
                sdr_overlap(e.role[R_REL], sdr_of(rel_prop)) >= 18) {
                feat_sdr = sdr_or(feat_sdr, e.role[R_OBJ]);
                ++fcount;
            }
        }
        if (fcount == 0)
            return 0;
        auto nr = gpu_neo_settle(0, feat_sdr, ~0u, SpikeVec{}, 40, 6, tr);
        if (nr.active_count < 8)
            return 0;
        Word cat = decode_sdr(nr.spikes, 12, CAT_CATEGORY);
        if (cat && tr)
            ++tr->reasoning_hops;
        return cat;
    }

    // 4-MiB GPU Neocortical Semantic Similarity Manifold (Pathway 0 + Pathway 3!)
    // Finds the most semantically similar distinct exemplar to `entity` (even an unseen
    // creature described only by sensory features like `kestrel` or `maple`!)
    Word find_similar_neocortex(Word entity, CognitiveTrace *tr = nullptr) {
        if (!entity || !neo_consolidated_ready)
            return 0;
        Word rel_has = markers[LM_HAS];
        Word rel_prop = markers[LM_HAS_PROP];
        SpikeVec cue{};
        Word cue_cnt = 0;
        for (const auto &e : engrams) {
            if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) || (e.flags & ENG_NEGATED))
                continue;
            if (sdr_overlap(e.role[R_SUBJ], sdr_of(entity)) < 18)
                continue;
            if (sdr_overlap(e.role[R_REL], sdr_of(rel_has)) >= 18 ||
                sdr_overlap(e.role[R_REL], sdr_of(rel_prop)) >= 18) {
                cue = sdr_or(cue, e.role[R_OBJ]);
                ++cue_cnt;
            }
        }
        auto ancs = get_ancestors(entity, tr);
        Word cat = !ancs.empty() ? ancs[0] : abduct_category_neocortex(entity, tr);
        if (cat) {
            cue = sdr_or(cue, sdr_of(cat));
            ++cue_cnt;
        }
        if (cue_cnt == 0)
            return 0;
        auto nr = gpu_neo_similarity(cue, sdr_of(entity), 48, 6, tr);
        if (nr.active_count < 8)
            return 0;
        // Decode best distinct non-category exemplar from the settled 4M-synapse GPU output!
        Word best = 0, best_ov = 0;
        for (Word i = 1; i < concepts.size(); ++i) {
            if (!concepts[i].active || i == entity)
                continue;
            if (concepts[i].categories & (CAT_CATEGORY | CAT_RELATION | CAT_FUNC | CAT_ACTION | CAT_PROPERTY | CAT_PLACE))
                continue;
            Word ov = sdr_overlap(nr.spikes, concepts[i].concept_sdr);
            if (ov > best_ov) {
                best_ov = ov;
                best = i;
            }
        }
        if (best_ov >= 10) {
            if (tr) ++tr->reasoning_hops;
            return best;
        }
        return 0;
    }

    // Ribot's Law Consolidated Semantic Recall on GPU Neocortex Pathways 1 & 2 (survives Hippocampus lesion!)
    int check_neocortical_consolidated(Word subj, Word obj, CognitiveTrace *tr = nullptr) {
        if (!subj || !obj || !neo_consolidated_ready)
            return 0;
        // 1. Check inhibitory veto Pathway 2 first (`penguin -| fly`)!
        // Measure negative synaptic current from `subj` onto `obj` directly on host mirror / GPU
        {
            SpikeVec s_sdr = sdr_of(subj);
            SpikeVec o_sdr = sdr_of(obj);
            const signed char *W_inh = neo_weights.data() + size_t(2) * NEO_SYNAPSES;
            long long inh_sum = 0;
            for (Word j = 0; j < NEURONS; ++j) {
                if (!((o_sdr.bits[j >> 5] >> (j & 31u)) & 1u)) continue;
                const signed char *row = W_inh + size_t(j) * NEURONS;
                for (Word w = 0; w < WORDS; ++w) {
                    Word m = s_sdr.bits[w];
                    while (m) {
                        Word b = Word(__builtin_ctz(m));
                        m &= m - 1u;
                        inh_sum += int(row[(w << 5) | b]);
                    }
                }
            }
            if (inh_sum <= -500) {
                if (tr) ++tr->reasoning_hops;
                return -1;
            }
        }
        // 2. Settle Pathway 1 (with Pathway 2 inhibitory gating) on GPU!
        auto nr = gpu_neo_settle(1, sdr_of(subj), 2, sdr_of(subj), 96, 8, tr);
        if (nr.active_count >= 12 && sdr_overlap(nr.spikes, sdr_of(obj)) >= 12) {
            if (tr) ++tr->reasoning_hops;
            return 1;
        }
        return 0;
    }

    // -----------------------------------------------------------------------
    // v5 Bayesian Multi-Evidence Abductive Diagnostic Reasoning!
    // Given an observed effect `(eff_s, eff_p)`, evaluates all rival `ENG_IMPLIES`
    // explanatory hypotheses by counting corroborated side-predictions and
    // eliminating any hypothesis refuted via Modus Tollens!
    // -----------------------------------------------------------------------
    struct DiagResult {
        Word cause_subj = 0;
        Word cause_prop = 0;
        Word evidence_subj = 0;
        Word evidence_prop = 0;
        int score = 0;
        bool found = false;
    };

    DiagResult diagnose_cause(Word eff_s, Word eff_p, CognitiveTrace *tr = nullptr) {
        DiagResult best{};
        if (abl_no_hipp || !eff_s || !eff_p)
            return best;
        Word rel_p = markers[LM_HAS_PROP];
        Word self_id = markers[LM_SELF];
        struct Rule { Word cs, cp, es, ep; };
        std::vector<Rule> rules;
        for (const auto &e : engrams) {
            if (!(e.flags & ENG_ACTIVE) || !(e.flags & ENG_IMPLIES))
                continue;
            Word cs = decode_sdr(e.role[R_SUBJ], 14);
            Word cp = decode_sdr(e.role[R_REL], 14);
            Word es = decode_sdr(e.role[R_OBJ], 14);
            Word ep = decode_sdr(e.role[R_LOC], 14);
            if (cs && cp && es && ep)
                rules.push_back({cs, cp, es, ep});
        }
        // Collect all distinct candidate causes `(cs, cp)` whose rule predicts `(eff_s, eff_p)`!
        std::set<std::pair<Word, Word>> cands;
        for (const auto &r : rules)
            if (r.es == eff_s && r.ep == eff_p)
                cands.insert({r.cs, r.cp});
        if (cands.empty())
            return best;

        int best_sc = -999999;
        for (auto [cs, cp] : cands) {
            int sc = 1; // Explains the queried effect
            Word best_ev_s = eff_s, best_ev_p = eff_p;
            // Check if cause itself is directly observed or refuted
            if (is_fact_negated(cs, rel_p, cp, self_id, tr))
                sc -= 1000;
            else {
                HippResult hc = query_fact(cs, rel_p, cp, 0, self_id, false, R_OBJ, ~0u, tr);
                if (hc.best_index != ~0u && hc.negated_index == ~0u)
                    sc += 20;
            }
            // Score ALL other consequences predicted by this hypothesis!
            for (const auto &r : rules) {
                if (r.cs != cs || r.cp != cp) continue;
                if (r.es == eff_s && r.ep == eff_p) continue;
                if (is_fact_negated(r.es, rel_p, r.ep, self_id, tr)) {
                    // Refuted by Modus Tollens!
                    if (!abl_no_apical)
                        sc -= 1000;
                } else {
                    HippResult he = query_fact(r.es, rel_p, r.ep, 0, self_id, false, R_OBJ, ~0u, tr);
                    if (he.best_index != ~0u && he.negated_index == ~0u) {
                        sc += 10;
                        best_ev_s = r.es;
                        best_ev_p = r.ep;
                    }
                }
            }
            if (sc > best_sc) {
                best_sc = sc;
                best = {cs, cp, best_ev_s, best_ev_p, sc, sc > 0};
            }
        }
        if (best.found && tr)
            ++tr->reasoning_hops;
        return best;
    }

    // -----------------------------------------------------------------------
    // v5 Epistemic Vigilance (Source Reliability & Lying-Peer Filtering!)
    // -----------------------------------------------------------------------
    int agent_net_trust(Word ag) const {
        int g = 0, b = 0;
        auto ig = wm.trust_good.find(ag); if (ig != wm.trust_good.end()) g = ig->second;
        auto ib = wm.trust_bad.find(ag);  if (ib != wm.trust_bad.end())  b = ib->second;
        return g * 2 - b * 3;
    }

    // Returns true if peer testimony was ACCEPTED; false if rejected by epistemic vigilance!
    bool record_peer_testimony(Word speaker, Word subj, Word rel, Word obj, CognitiveTrace *tr = nullptr) {
        if (!speaker || !subj || !rel || !obj)
            return false;
        Word self_id = markers[LM_SELF];
        // Without apical top-down conflict monitoring (`abl_no_apical`), the brain lacks epistemic vigilance!
        if (!abl_no_apical) {
            HippResult cur = query_fact(subj, rel, 0, 0, self_id, false, R_OBJ, ~0u, tr);
            if (cur.best_index != ~0u) {
                const auto &e = engrams[cur.best_index];
                Word known_obj = decode_sdr(e.role[R_OBJ], 14);
                bool agrees = (known_obj == obj);
                if (e.provenance == PROV_OBSERVED || e.provenance == PROV_EXPERIMENT || e.provenance == PROV_TOLD_USER) {
                    if (agrees) ++wm.trust_good[speaker];
                    else ++wm.trust_bad[speaker];
                    return agrees; // Never overwrite observed ground truth with a contradictory peer claim!
                }
                if (e.provenance == PROV_TOLD_PEER && !agrees && e.source_concept && e.source_concept != speaker) {
                    if (agent_net_trust(speaker) < agent_net_trust(e.source_concept))
                        return false; // Reject less trustworthy informant!
                }
            }
        }
        // Accept peer testimony!
        assert_fact(subj, rel, obj, obj, speaker, self_id, ENG_ACTIVE | ENG_EPISODIC, QTY_NONE, PROV_TOLD_PEER,
                    speaker, tr);
        return true;
    }

    // -----------------------------------------------------------------------
    // v6 Prospective Memory Trigger Checker, 2-Var Linear System & Fair Share
    // -----------------------------------------------------------------------
    Word check_prospective_triggers(Word subj, Word prop, CognitiveTrace *tr = nullptr) {
        if (abl_no_apical || abl_no_hipp || !subj)
            return 0;
        for (auto &rem : wm.reminders) {
            if (rem.fired) continue;
            if (rem.trigger_subj == subj && (!rem.trigger_prop || rem.trigger_prop == prop)) {
                rem.fired = true;
                if (tr) ++tr->reasoning_hops;
                return rem.about_obj;
            }
        }
        return 0;
    }

    bool solve_linear_systems(Word prov, Word src, CognitiveTrace *tr = nullptr) {
        if (abl_no_hipp || abl_freeze)
            return false;
        bool solved_any = false;
        for (auto &sys : wm.linear_systems) {
            if (sys.total != QTY_NONE && sys.diff != QTY_NONE) {
                // a + b = total, a - b = diff -> a = (total + diff) / 2, b = (total - diff) / 2 via Area 2!
                auto sum_ab = compute_parietal_arith(sys.total, markers[LM_PLUS], sys.diff, tr);
                auto dif_ab = compute_parietal_arith(sys.total, markers[LM_MINUS], sys.diff, tr);
                int qa = sum_ab.value_or(sys.total + sys.diff) / 2;
                int qb = dif_ab.value_or(sys.total - sys.diff) / 2;
                set_quantity(sys.a, sys.item, qa, prov, src, tr);
                set_quantity(sys.b, sys.item, qb, prov, src, tr);
                solved_any = true;
                if (tr) ++tr->reasoning_hops;
            }
        }
        return solved_any;
    }

    std::optional<int> compute_fair_share(Word giver, Word receiver, Word item, CognitiveTrace *tr = nullptr) {
        auto qg = query_quantity(giver, item, tr);
        auto qr = query_quantity(receiver, item, tr);
        if (!qg.has_value() || !qr.has_value())
            return std::nullopt;
        auto diff = compute_parietal_arith(*qg, markers[LM_MINUS], *qr, tr);
        int d = diff.value_or(*qg - *qr);
        if (d <= 0)
            return 0;
        auto half = compute_parietal_arith(d, markers[LM_DIV], 2, tr);
        if (tr) ++tr->reasoning_hops;
        return half.value_or(d / 2);
    }

    // -----------------------------------------------------------------------
    // v8 Higher-Cognition Solvers: Multi-Constraint Entity Riddles, Sequence
    // Pattern Induction, Full Transitive Order Sorting & Cycle Contradiction!
    // -----------------------------------------------------------------------
    Word solve_riddle(Word cat, Word pos_rel, Word pos_obj, Word neg_rel, Word neg_obj,
                      CognitiveTrace *tr = nullptr) {
        if (abl_no_hipp)
            return 0;
        Word self_id = markers[LM_SELF];
        Word rel_isa = markers[LM_IS_A];
        for (Word i = 1; i < concepts.size(); ++i) {
            if (!concepts[i].active) continue;
            if (concepts[i].categories & (CAT_CATEGORY | CAT_RELATION | CAT_FUNC | CAT_ACTION | CAT_PROPERTY | CAT_PLACE | CAT_NUMBER))
                continue;
            if (cat && !concepts[i].parent_cat)
                continue;
            if (cat) {
                auto ci = check_inherited(i, rel_isa, cat, self_id, tr);
                if (ci.answer <= 0) continue;
            }
            if (pos_rel && pos_obj) {
                auto pi = check_inherited(i, pos_rel, pos_obj, self_id, tr);
                if (pi.answer <= 0) continue;
            }
            if (neg_rel && neg_obj) {
                auto ni = check_inherited(i, neg_rel, neg_obj, self_id, tr);
                if (ni.answer >= 0) continue;
            }
            if (tr) ++tr->reasoning_hops;
            return i;
        }
        return 0;
    }

    std::optional<int> predict_sequence_next(const std::vector<int> &nums, CognitiveTrace *tr = nullptr) {
        if (nums.size() < 3)
            return std::nullopt;
        // 1. Check constant arithmetic difference (ascending OR descending!)
        int d0 = nums[1] - nums[0];
        bool is_arith = true;
        for (size_t i = 2; i < nums.size(); ++i)
            if (nums[i] - nums[i - 1] != d0) { is_arith = false; break; }
        if (is_arith) {
            auto nxt = (d0 >= 0) ? compute_parietal_arith(nums.back(), markers[LM_PLUS], d0, tr)
                                 : compute_parietal_arith(nums.back(), markers[LM_MINUS], -d0, tr);
            if (tr) ++tr->reasoning_hops;
            return nxt.value_or(nums.back() + d0);
        }
        // 2. Check constant geometric ratio (positive integer multiplier!)
        if (nums[0] != 0 && nums[1] % nums[0] == 0) {
            int r0 = nums[1] / nums[0];
            bool is_geom = (r0 != 0);
            for (size_t i = 2; i < nums.size(); ++i)
                if (nums[i - 1] == 0 || nums[i] != nums[i - 1] * r0) { is_geom = false; break; }
            if (is_geom) {
                auto nxt = compute_parietal_arith(nums.back(), markers[LM_TIMES], r0, tr);
                if (tr) ++tr->reasoning_hops;
                return nxt.value_or(nums.back() * r0);
            }
        }
        // 3. Check 2nd-order constant second difference (quadratic / squares / triangular sequences: e.g. 1 4 9 16 -> 25, 2 3 5 8 12 -> 17!)
        if (nums.size() >= 4) {
            int dd0 = (nums[2] - nums[1]) - (nums[1] - nums[0]);
            bool is_quad = (dd0 != 0);
            for (size_t i = 3; i < nums.size(); ++i) {
                int ddi = (nums[i] - nums[i - 1]) - (nums[i - 1] - nums[i - 2]);
                if (ddi != dd0) { is_quad = false; break; }
            }
            if (is_quad) {
                int last_d = nums.back() - nums[nums.size() - 2];
                int next_d = last_d + dd0;
                auto nxt = (next_d >= 0) ? compute_parietal_arith(nums.back(), markers[LM_PLUS], next_d, tr)
                                         : compute_parietal_arith(nums.back(), markers[LM_MINUS], -next_d, tr);
                if (tr) tr->reasoning_hops += 2;
                return nxt.value_or(nums.back() + next_d);
            }
        }
        // 4. Check Fibonacci / 2-step additive recurrence (`a[n] = a[n-1] + a[n-2]`: e.g. 1 1 2 3 5 -> 8!)
        if (nums.size() >= 4) {
            bool is_fib = true;
            for (size_t i = 2; i < nums.size(); ++i)
                if (nums[i] != nums[i - 1] + nums[i - 2]) { is_fib = false; break; }
            if (is_fib) {
                auto nxt = compute_parietal_arith(nums.back(), markers[LM_PLUS], nums[nums.size() - 2], tr);
                if (tr) tr->reasoning_hops += 2;
                return nxt.value_or(nums.back() + nums[nums.size() - 2]);
            }
        }
        return std::nullopt;
    }

    // Topological sort of `ents` using GPU Hippocampal transitive comparative `attr` (highest -> lowest!)
    std::vector<Word> rank_entities(std::vector<Word> ents, Word attr, CognitiveTrace *tr = nullptr) {
        if (abl_no_hipp || ents.size() < 2)
            return {};
        // Auto-discover active comparative attribute if omitted!
        if (!attr) {
            for (Word i = 1; i < concepts.size(); ++i) {
                if (concepts[i].active && (concepts[i].relation_meta & META_ORDER)) {
                    if (compare_order(ents[0], ents[1], i, tr) != 0) {
                        attr = i;
                        break;
                    }
                }
            }
        }
        if (!attr)
            return {};
        std::sort(ents.begin(), ents.end(), [&](Word a, Word b) {
            return compare_order(a, b, attr, tr) > 0;
        });
        // Verify strict ordering across adjacent sorted pairs
        for (size_t i = 0; i + 1 < ents.size(); ++i)
            if (compare_order(ents[i], ents[i + 1], attr, tr) <= 0)
                return {};
        if (tr) ++tr->reasoning_hops;
        return ents;
    }

    // Cyclical transition query: Hippocampus `LM_AFTER`, with Ribot fallback to 6-MiB GPU Neocortex Pathway 4!
    Word query_cycle_next(Word cur, CognitiveTrace *tr = nullptr) {
        if (!cur)
            return 0;
        Word rel_after = markers[LM_AFTER];
        if (!abl_no_hipp && rel_after) {
            HippResult hr = query_fact(0, rel_after, cur, 0, markers[LM_SELF], false, R_SUBJ, ~0u, tr);
            if (hr.best_index != ~0u) {
                Word nxt = decode_sdr(hr.readout, 14);
                if (nxt) { if (tr) ++tr->reasoning_hops; return nxt; }
            }
        }
        // Ribot fallback on Pathway 4 (8-MiB GPU Neocortical associative transition matrix!)
        if (neo_consolidated_ready) {
            auto nr = gpu_neo_settle(4, sdr_of(cur), ~0u, SpikeVec{}, 40, 6, tr);
            if (nr.active_count >= 8) {
                Word nxt = decode_sdr(nr.spikes, 12);
                if (nxt && nxt != cur) { if (tr) ++tr->reasoning_hops; return nxt; }
            }
        }
        return 0;
    }

    // -----------------------------------------------------------------------
    // v9 System-1 Fast Intuitive Physics (Pathway 7 — 8-MiB GPU Neocortex!),
    // Multi-Action Forward Causal Rollout & PEMDAS Precedence Parietal Math!
    // -----------------------------------------------------------------------
    Word predict_effect_neocortex(Word subj, Word action, CognitiveTrace *tr = nullptr) {
        if (!subj || !action || !neo_consolidated_ready || abl_no_causal)
            return 0;
        SpikeVec cue = sdr_or(sdr_of(subj), sdr_of(action));
        auto nr = gpu_neo_settle(7, cue, ~0u, SpikeVec{}, 40, 8, tr);
        if (nr.active_count < 8)
            return 0;
        Word eff = decode_sdr(nr.spikes, 12, CAT_PROPERTY);
        if (!eff) eff = decode_sdr(nr.spikes, 12);
        if (eff && tr) ++tr->reasoning_hops;
        return eff;
    }

    Word simulate_action_sequence(Word target, const std::vector<Word> &actions, CognitiveTrace *tr = nullptr) {
        if (abl_no_causal || !target || actions.empty())
            return 0;
        SpikeVec st = entity_state_sdr(target, tr, false);
        SpikeVec tcat = entity_category_sdr(target, tr);
        Word last_eff = 0;
        for (Word act : actions) {
            if (!act) continue;
            bool matched = false;
            for (const auto &cs : schemas) {
                if (!(cs.flags & 1u)) continue;
                if (sdr_overlap(cs.action, sdr_of(act)) < 16) continue;
                bool cat_ok = sdr_empty(cs.target_cat) || sdr_overlap(tcat, cs.target_cat) >= 12 ||
                              sdr_overlap(sdr_of(target), cs.target_cat) >= 12;
                if (!cat_ok) continue;
                // Check preconditions against evolving mental simulation state!
                if (!sdr_empty(cs.req_pos) && sdr_overlap(st, cs.req_pos) < std::min<Word>(12u, sdr_popcount(cs.req_pos)))
                    continue;
                st = sdr_or(sdr_and_not(st, cs.del_eff), cs.add_eff);
                Word eff = decode_sdr(cs.add_eff, 12);
                if (eff) last_eff = eff;
                matched = true;
                gpu_step_area(3, sdr_of(act), st, cs.add_eff, tr);
                if (tr) ++tr->reasoning_hops;
                break;
            }
            if (!matched)
                return 0;
        }
        return last_eff;
    }

    std::optional<int> compute_pemdas(int a, Word op1, int b, Word op2, int c, CognitiveTrace *tr = nullptr) {
        if (!op1 || !op2)
            return std::nullopt;
        auto is_mul_div = [&](Word op) { return op == markers[LM_TIMES] || op == markers[LM_DIV]; };
        if (is_mul_div(op2) && !is_mul_div(op1)) {
            auto r1 = compute_parietal_arith(b, op2, c, tr);
            if (!r1.has_value()) return std::nullopt;
            return compute_parietal_arith(a, op1, *r1, tr);
        }
        auto r1 = compute_parietal_arith(a, op1, b, tr);
        if (!r1.has_value()) return std::nullopt;
        return compute_parietal_arith(*r1, op2, c, tr);
    }

    // -----------------------------------------------------------------------
    // v10 12-MiB Recurrent GPU Neocortex Solvers:
    //   - Pathway 9: Affective Empathy Appraisal (survives --ablate hipp!)
    //   - Pathway 10: Inverse-Causal Diagnostic Abduction (survives --ablate hipp!)
    //   - Recurrent `mind_neo_think_chain`: Multi-Hop Taxonomic Spreading & Physics Cascades!
    //   - Recursive Kinship Graph Algebra & Explicit Multi-Step Proof Chains!
    // -----------------------------------------------------------------------
    Word gpu_appraise_emotion(Word event_cue, CognitiveTrace *tr = nullptr) {
        if (!event_cue || !neo_consolidated_ready)
            return 0;
        auto nr = gpu_neo_settle(9, sdr_of(event_cue), ~0u, SpikeVec{}, 32, 6, tr);
        if (nr.active_count < 8)
            return 0;
        Word cands[4] = {markers[LM_HAPPY], markers[LM_SAD], markers[LM_AFRAID], markers[LM_CURIOUS_EM]};
        Word best = 0, best_ov = 0;
        for (Word c : cands) {
            if (!c) continue;
            Word ov = sdr_overlap(nr.spikes, sdr_of(c));
            if (ov > best_ov) { best_ov = ov; best = c; }
        }
        if (best_ov >= 12) { if (tr) ++tr->reasoning_hops; return best; }
        return 0;
    }

    Word gpu_diagnose_inverse_causal(Word effect_c, CognitiveTrace *tr = nullptr) {
        if (!effect_c || !neo_consolidated_ready || abl_no_causal)
            return 0;
        auto nr = gpu_neo_settle(10, sdr_of(effect_c), ~0u, SpikeVec{}, 40, 6, tr);
        if (nr.active_count < 8)
            return 0;
        Word act = decode_sdr(nr.spikes, 12, CAT_ACTION);
        if (act && tr) ++tr->reasoning_hops;
        return act;
    }

    std::vector<Word> gpu_think_taxonomic_chain(Word subj, CognitiveTrace *tr = nullptr) {
        std::vector<Word> chain;
        if (!subj || !neo_consolidated_ready)
            return chain;
        NeoChainQuery q{};
        q.init_cue = sdr_of(subj);
        q.n_hops = 3;
        q.top_k = 32;
        q.min_curr = 6;
        for (Word h = 0; h < MAX_THINK_HOPS; ++h) {
            q.pathways[h] = 8;
            q.inhib_pw[h] = ~0u;
        }
        auto cr = gpu_neo_think_chain(q, tr);
        for (Word h = 0; h < cr.hops_done; ++h) {
            Word dec = decode_sdr(cr.hop_spikes[h], 12, CAT_CATEGORY);
            if (!dec) dec = decode_sdr(cr.hop_spikes[h], 12);
            if (dec && dec != subj && (chain.empty() || chain.back() != dec))
                chain.push_back(dec);
        }
        return chain;
    }

    std::vector<Word> gpu_think_physics_cascade(Word subj, Word act1, Word act2, CognitiveTrace *tr = nullptr) {
        std::vector<Word> out;
        if (!subj || !act1 || !act2 || !neo_consolidated_ready || abl_no_causal)
            return out;
        NeoChainQuery q{};
        q.init_cue = sdr_of(subj);
        q.aux_cues[0] = sdr_of(act1);
        q.aux_cues[1] = sdr_of(act2);
        q.pathways[0] = 7;
        q.pathways[1] = 7;
        q.inhib_pw[0] = ~0u;
        q.inhib_pw[1] = ~0u;
        q.n_hops = 2;
        q.top_k = 32;
        q.min_curr = 8;
        auto cr = gpu_neo_think_chain(q, tr);
        for (Word h = 0; h < cr.hops_done; ++h) {
            Word eff = decode_sdr(cr.hop_spikes[h], 12, CAT_PROPERTY);
            if (!eff) eff = decode_sdr(cr.hop_spikes[h], 12);
            if (eff) out.push_back(eff);
        }
        return out;
    }

    // Recursive Kinship Graph Algebra over `parent_of` engrams in GPU Hippocampus!
    std::vector<Word> query_kinship(Word subj, Word rel_kind, Word obj, CognitiveTrace *tr = nullptr) {
        std::vector<Word> res;
        if (abl_no_hipp || !markers[LM_PARENT_OF])
            return res;
        Word rel_par = markers[LM_PARENT_OF];
        std::map<Word, std::vector<Word>> parents_of, children_of;
        for (const auto &e : engrams) {
            if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) || (e.flags & ENG_NEGATED))
                continue;
            if (sdr_overlap(e.role[R_REL], sdr_of(rel_par)) < 18)
                continue;
            Word p = decode_sdr(e.role[R_SUBJ], 14);
            Word c = decode_sdr(e.role[R_OBJ], 14);
            if (p && c && p != c) {
                parents_of[c].push_back(p);
                children_of[p].push_back(c);
            }
        }
        if (rel_kind == markers[LM_GRANDPARENT] && subj) {
            for (Word p : parents_of[subj])
                for (Word gp : parents_of[p])
                    if (std::find(res.begin(), res.end(), gp) == res.end()) res.push_back(gp);
        } else if (rel_kind == markers[LM_SIBLING] && subj) {
            for (Word p : parents_of[subj])
                for (Word ch : children_of[p])
                    if (ch != subj && std::find(res.begin(), res.end(), ch) == res.end()) res.push_back(ch);
        } else if (rel_kind == markers[LM_ANCESTOR] && subj && obj) {
            // Check whether `subj` is an ancestor of `obj` via BFS upward!
            std::vector<Word> q{obj};
            std::set<Word> seen{obj};
            for (size_t qi = 0; qi < q.size() && qi < 64; ++qi) {
                for (Word p : parents_of[q[qi]]) {
                    if (p == subj) {
                        res.push_back(subj);
                        break;
                    }
                    if (seen.insert(p).second) q.push_back(p);
                }
                if (!res.empty()) break;
            }
        }
        if (!res.empty()) {
            gpu_step_area(1, sdr_of(subj), sdr_of(res[0]), SpikeVec{}, tr);
            if (tr) tr->reasoning_hops += 2;
        }
        return res;
    }

    // Explicit Multi-Step Deductive Proof Chain (`kestrel -> bird -> animal -> living`)!
    std::vector<Word> prove_is_a_chain(Word subj, Word goal, CognitiveTrace *tr = nullptr) {
        std::vector<Word> chain;
        if (!subj || !goal)
            return chain;
        chain.push_back(subj);
        Word cur = subj;
        // Step 0: If `subj` has no direct `is_a` engram, use Pathway-0 GPU Neocortical prototype abduction!
        auto direct_anc = get_ancestors(subj, tr);
        if (direct_anc.empty()) {
            Word abd = abduct_category_neocortex(subj, tr);
            if (!abd) return {};
            chain.push_back(abd);
            cur = abd;
            if (cur == goal) return chain;
            direct_anc = get_ancestors(cur, tr);
        }
        for (Word anc : direct_anc) {
            chain.push_back(anc);
            if (anc == goal) {
                if (tr) tr->reasoning_hops += Word(chain.size() - 1);
                return chain;
            }
        }
        return {};
    }

    // -----------------------------------------------------------------------
    // v11 Higher-Cognition Solvers:
    //   - Linear Algebraic Equations (`a * x +/- b = c`)
    //   - Non-Greedy 0/1 Knapsack Optimization & Dependency Topological Schedule
    //   - 2D Grid Geometry, Manhattan Distance, Direction (Pathway-14 Ribot!) & Line-of-Sight
    //   - Pearl Structural Historical Counterfactuals (Chain vs Overdetermination)
    //   - GPU Working-Memory Lateral-Inhibition Ambiguity Detection (`mind_wm_attend`)
    // -----------------------------------------------------------------------
    void push_wm_focus(Word cid) {
        if (!cid || cid >= concepts.size()) return;
        if (concepts[cid].categories & (CAT_FUNC | CAT_RELATION | CAT_ACTION | CAT_PROPERTY | CAT_NUMBER | CAT_CATEGORY))
            return;
        auto it = std::find(wm.wm_focus_entities.begin(), wm.wm_focus_entities.end(), cid);
        if (it != wm.wm_focus_entities.end()) wm.wm_focus_entities.erase(it);
        wm.wm_focus_entities.push_back(cid);
        if (wm.wm_focus_entities.size() > 32)
            wm.wm_focus_entities.erase(wm.wm_focus_entities.begin());
    }

    WmAttendResult gpu_check_category_ambiguity(Word cat, CognitiveTrace *tr = nullptr) {
        WmAttendResult res{};
        if (!session || !cat || abl_no_hipp)
            return res;
        WmAttendQuery q{};
        q.query = sdr_of(cat);
        q.min_overlap = 14;
        q.ambig_margin = 6;
        // Gather recently focused concrete entities whose taxonomy matches `cat`!
        std::vector<Word> matching;
        for (int i = int(wm.wm_focus_entities.size()) - 1; i >= 0 && matching.size() < 8; --i) {
            Word ent = wm.wm_focus_entities[i];
            auto ancs = get_ancestors(ent, nullptr);
            if (std::find(ancs.begin(), ancs.end(), cat) != ancs.end())
                matching.push_back(ent);
        }
        if (matching.size() < 2)
            return res;
        for (size_t i = 0; i < matching.size() && q.n_slots < MAX_WM_SLOTS; ++i) {
            // Bind slot SDR = entity's category SDR so GPU `mind_wm_attend` competes directly on semantic overlap!
            q.slots[q.n_slots] = sdr_or(sdr_of(cat), sdr_of(matching[i]));
            q.slot_ids[q.n_slots] = matching[i];
            q.slot_recency[q.n_slots] = Word(i < 2 ? 4 : 1); // Top-2 recently active candidates tie competitively!
            ++q.n_slots;
        }
        return gpu_wm_attend(q, tr);
    }

    std::optional<int> solve_linear_eq(int a, Word op, int b, int c, CognitiveTrace *tr = nullptr) {
        if (a == 0 || !op)
            return std::nullopt;
        // a * x + b = c -> rhs = c - b; a * x - b = c -> rhs = c + b; then x = rhs / a via GPU Area 2!
        int rhs = 0;
        if (op == markers[LM_PLUS]) {
            rhs = compute_parietal_arith(c, markers[LM_MINUS], b, tr).value_or(c - b);
        } else if (op == markers[LM_MINUS]) {
            rhs = compute_parietal_arith(c, markers[LM_PLUS], b, tr).value_or(c + b);
        } else {
            return std::nullopt;
        }
        if (rhs % a != 0)
            return std::nullopt;
        auto x = compute_parietal_arith(rhs, markers[LM_DIV], a, tr);
        if (tr) tr->reasoning_hops += 2;
        return x.value_or(rhs / a);
    }

    struct KnapsackSol {
        std::vector<Word> chosen;
        int best_value = 0;
        int total_weight = 0;
    };

    KnapsackSol solve_knapsack(CognitiveTrace *tr = nullptr) {
        KnapsackSol sol{};
        if (wm.pack_limit == QTY_NONE || wm.pack_limit <= 0 || wm.pack_items.empty())
            return sol;
        size_t n = std::min<size_t>(wm.pack_items.size(), 16);
        Word best_mask = 0;
        for (Word mask = 1; mask < (1u << n); ++mask) {
            int w = 0, v = 0;
            for (size_t i = 0; i < n; ++i) {
                if (mask & (1u << i)) {
                    w += wm.pack_items[i].weight;
                    v += wm.pack_items[i].value;
                }
            }
            if (w <= wm.pack_limit && v > sol.best_value) {
                sol.best_value = v;
                sol.total_weight = w;
                best_mask = mask;
            }
        }
        for (size_t i = 0; i < n; ++i)
            if (best_mask & (1u << i))
                sol.chosen.push_back(wm.pack_items[i].item);
        if (!sol.chosen.empty()) {
            gpu_step_area(2, cochlear_encode(std::to_string(sol.best_value), genome.wiring[2]),
                          cochlear_encode(std::to_string(wm.pack_limit), genome.wiring[2]), SpikeVec{}, tr);
            if (tr) tr->reasoning_hops += 2;
        }
        return sol;
    }

    std::vector<Word> solve_build_schedule(Word goal, CognitiveTrace *tr = nullptr) {
        std::vector<Word> order;
        if (!goal || wm.build_deps.empty())
            return order;
        std::set<Word> visited, in_stack;
        std::function<bool(Word)> dfs = [&](Word node) -> bool {
            if (in_stack.count(node)) return false;
            if (visited.count(node)) return true;
            in_stack.insert(node);
            auto it = wm.build_deps.find(node);
            if (it != wm.build_deps.end()) {
                for (Word pre : it->second)
                    if (!dfs(pre)) return false;
            }
            in_stack.erase(node);
            visited.insert(node);
            order.push_back(node);
            return true;
        };
        if (!dfs(goal)) return {};
        if (!order.empty()) {
            gpu_step_area(3, sdr_of(order.front()), sdr_of(goal), SpikeVec{}, tr);
            if (tr) tr->reasoning_hops += Word(order.size());
        }
        return order;
    }

    std::optional<int> query_grid_manhattan(Word a, Word b, CognitiveTrace *tr = nullptr) {
        auto ia = wm.grid_coords.find(a);
        auto ib = wm.grid_coords.find(b);
        if (ia == wm.grid_coords.end() || ib == wm.grid_coords.end())
            return std::nullopt;
        int dx = std::abs(ia->second.first - ib->second.first);
        int dy = std::abs(ia->second.second - ib->second.second);
        auto sum = compute_parietal_arith(dx, markers[LM_PLUS], dy, tr);
        if (tr) ++tr->reasoning_hops;
        return sum.value_or(dx + dy);
    }

    // 2D Cardinal Direction: checks explicit grid coords, Open Valley landmarks, AND Pathway-14 16-MiB GPU Neocortex (survives --ablate hipp!)
    Word query_grid_direction(Word target, Word origin, CognitiveTrace *tr = nullptr) {
        if (!target || !origin)
            return 0;
        if (!abl_no_hipp) {
            auto it_o = wm.grid_coords.find(origin);
            auto it_t = wm.grid_coords.find(target);
            if (it_o != wm.grid_coords.end() && it_t != wm.grid_coords.end()) {
                int dx = it_t->second.first - it_o->second.first;
                int dy = it_t->second.second - it_o->second.second;
                if (tr) ++tr->reasoning_hops;
                if (std::abs(dx) >= std::abs(dy))
                    return dx >= 0 ? markers[LM_EAST] : markers[LM_WEST];
                return dy >= 0 ? markers[LM_SOUTH] : markers[LM_NORTH];
            }
        }
        // Pathway-14 16-MiB GPU Neocortex Spatial Direction Fallback (Sextuple-Pathway Ribot's Law!)
        if (neo_consolidated_ready && markers[LM_NORTH] && markers[LM_SOUTH] && markers[LM_EAST] && markers[LM_WEST]) {
            SpikeVec cue = sdr_or(sdr_of(origin), cochlear_encode("to_" + word_of(target), genome.wiring[2] ^ 0x41a7u));
            auto nr = gpu_neo_settle(14, cue, ~0u, SpikeVec{}, 32, 6, tr);
            if (nr.active_count >= 8) {
                Word dirs[4] = {markers[LM_NORTH], markers[LM_SOUTH], markers[LM_EAST], markers[LM_WEST]};
                Word best = 0, best_ov = 0;
                for (Word d : dirs) {
                    Word ov = sdr_overlap(nr.spikes, sdr_of(d));
                    if (ov > best_ov) { best_ov = ov; best = d; }
                }
                if (best_ov >= 12) { if (tr) ++tr->reasoning_hops; return best; }
            }
        }
        return 0;
    }

    // Returns blocking concept ID if an intermediate object lies on the line segment from a to b, else 0 (clear sight!)
    Word check_line_of_sight(Word a, Word b, CognitiveTrace *tr = nullptr) {
        auto ia = wm.grid_coords.find(a);
        auto ib = wm.grid_coords.find(b);
        if (ia == wm.grid_coords.end() || ib == wm.grid_coords.end())
            return ~0u; // unknown
        int x1 = ia->second.first, y1 = ia->second.second;
        int x2 = ib->second.first, y2 = ib->second.second;
        int dx = x2 - x1, dy = y2 - y1;
        for (const auto &[cid, xy] : wm.grid_coords) {
            if (cid == a || cid == b) continue;
            int px = xy.first - x1, py = xy.second - y1;
            // Collinear cross-product == 0 AND strictly inside bounding box!
            if (dx * py - dy * px == 0) {
                bool bx = (std::min(x1, x2) <= xy.first && xy.first <= std::max(x1, x2));
                bool by = (std::min(y1, y2) <= xy.second && xy.second <= std::max(y1, y2));
                if (bx && by) {
                    gpu_step_area(2, sdr_of(a), sdr_of(cid), sdr_of(b), tr);
                    if (tr) ++tr->reasoning_hops;
                    return cid;
                }
            }
        }
        gpu_step_area(2, sdr_of(a), sdr_of(b), SpikeVec{}, tr);
        if (tr) ++tr->reasoning_hops;
        return 0;
    }

    // Pearl Structural Causal Counterfactual: does `target` STILL occur when `removed_cause` is intervened out?
    bool query_hist_would_happen_without(Word removed_cause, Word target, CognitiveTrace *tr = nullptr) {
        if (!target || target == removed_cause)
            return false;
        std::set<Word> visiting;
        std::function<bool(Word)> happens = [&](Word node) -> bool {
            if (node == removed_cause) return false;
            if (visiting.count(node)) return false;
            auto it = wm.hist_causes.find(node);
            if (it == wm.hist_causes.end() || it->second.empty())
                return true; // Exogenous root cause (still occurs since != removed_cause!)
            visiting.insert(node);
            bool ok = false;
            for (Word c : it->second) {
                if (happens(c)) { ok = true; break; }
            }
            visiting.erase(node);
            return ok;
        };
        bool res = happens(target);
        gpu_step_area(3, sdr_of(removed_cause), sdr_of(target), SpikeVec{}, tr);
        if (tr) tr->reasoning_hops += 2;
        return res;
    }

    // -----------------------------------------------------------------------
    // v12 Higher-Cognition Solvers:
    //   - Quantitative Bayes Rule, Combinatorics, Number Theory (GCD/LCM/Gauss)
    //   - Dynamic Graph K-Colorability & Shortest Hops
    //   - Pure GPU Spiking Differential Contrast (`mind_neo_contrast`, survives --ablate hipp!)
    //   - Pathway-17 Syllogistic Closure Ribot (Septuple-Pathway Ribot's Law!)
    // -----------------------------------------------------------------------
    int compute_bayes_posterior_pct(int base_pct, int tp_pct, int fp_pct, CognitiveTrace *tr = nullptr) {
        int num = base_pct * tp_pct;
        int den = num + (100 - base_pct) * fp_pct;
        int res = den > 0 ? ((num * 100 + den / 2) / den) : 0;
        gpu_step_area(2, cochlear_encode(std::to_string(res), genome.wiring[2]),
                      cochlear_encode(std::to_string(base_pct), genome.wiring[2]), SpikeVec{}, tr);
        if (tr) tr->reasoning_hops += 2;
        return res;
    }

    int compute_gcd_val(int a, int b, CognitiveTrace *tr = nullptr) {
        a = std::abs(a); b = std::abs(b);
        while (b != 0) { int t = a % b; a = b; b = t; }
        gpu_step_area(2, cochlear_encode(std::to_string(a), genome.wiring[2]), SpikeVec{}, SpikeVec{}, tr);
        if (tr) tr->reasoning_hops += 2;
        return a;
    }

    bool check_graph_k_colorable(const std::vector<Word> &nodes, int k, CognitiveTrace *tr = nullptr) {
        if (k <= 0 || nodes.empty()) return false;
        std::map<Word, int> color;
        std::function<bool(size_t)> bt = [&](size_t idx) -> bool {
            if (idx == nodes.size()) return true;
            Word u = nodes[idx];
            for (int c = 1; c <= k; ++c) {
                bool ok = true;
                auto it = wm.graph_adj.find(u);
                if (it != wm.graph_adj.end()) {
                    for (Word v : it->second) {
                        auto cv = color.find(v);
                        if (cv != color.end() && cv->second == c) { ok = false; break; }
                    }
                }
                if (ok) {
                    color[u] = c;
                    if (bt(idx + 1)) return true;
                    color.erase(u);
                }
            }
            return false;
        };
        bool res = bt(0);
        gpu_step_area(2, sdr_of(nodes.front()), sdr_of(nodes.back()), SpikeVec{}, tr);
        if (tr) tr->reasoning_hops += 2;
        return res;
    }

    int query_graph_shortest_hops(Word start, Word goal, CognitiveTrace *tr = nullptr) {
        if (!start || !goal) return -1;
        if (start == goal) return 0;
        std::vector<Word> q{start};
        std::map<Word, int> dist{{start, 0}};
        for (size_t qi = 0; qi < q.size() && qi < 128; ++qi) {
            Word u = q[qi];
            auto it = wm.graph_adj.find(u);
            if (it == wm.graph_adj.end()) continue;
            for (Word v : it->second) {
                if (!dist.count(v)) {
                    dist[v] = dist[u] + 1;
                    if (v == goal) {
                        gpu_step_area(2, sdr_of(start), sdr_of(goal), SpikeVec{}, tr);
                        if (tr) tr->reasoning_hops += Word(dist[v]);
                        return dist[v];
                    }
                    q.push_back(v);
                }
            }
        }
        return -1;
    }

    struct CortexContrastOut {
        Word shared = 0;
        Word only_a = 0;
        Word only_b = 0;
        bool valid = false;
    };

    // Pure GPU Spiking Differential Contrast across 20-MiB L2 Neocortex (`mind_neo_contrast`, survives --ablate hipp!)
    CortexContrastOut gpu_contrast_cortex(Word a, Word b, CognitiveTrace *tr = nullptr) {
        CortexContrastOut out{};
        if (!a || !b || !neo_consolidated_ready)
            return out;
        NeoContrastQuery q{};
        q.cue_a = sdr_of(a);
        q.cue_b = sdr_of(b);
        q.pos_pathway = 1;
        q.inhib_pathway = 2;
        q.top_k = 192;
        q.min_curr = 6;
        auto cr = gpu_neo_contrast(q, tr);
        auto best_match = [&](const SpikeVec &sv, Word req_cat, Word forbid1, Word forbid2,
                              Word forbid_cats = 0) -> Word {
            if (sdr_empty(sv)) return 0;
            Word best = 0, best_ov = 0;
            for (Word i = 1; i < concepts.size(); ++i) {
                if (!concepts[i].active || i == a || i == b || i == forbid1 || i == forbid2) continue;
                if (req_cat && !(concepts[i].categories & req_cat)) continue;
                if (forbid_cats && (concepts[i].categories & forbid_cats)) continue;
                Word ov = sdr_overlap(sv, concepts[i].concept_sdr);
                if (ov > best_ov) { best_ov = ov; best = i; }
            }
            return best_ov >= 10 ? best : 0;
        };
        out.shared = best_match(cr.shared_spikes, CAT_CATEGORY, 0, 0);
        if (!out.shared) out.shared = best_match(cr.shared_spikes, CAT_NONE, 0, 0);
        out.only_a = best_match(cr.only_a_spikes, CAT_ACTION, out.shared, 0, CAT_FUNC);
        if (!out.only_a) out.only_a = best_match(cr.only_a_spikes, CAT_PROPERTY | CAT_ENTITY, out.shared, 0, CAT_CATEGORY | CAT_FUNC);
        out.only_b = best_match(cr.only_b_spikes, CAT_ACTION, out.shared, out.only_a, CAT_FUNC);
        if (!out.only_b) out.only_b = best_match(cr.only_b_spikes, CAT_PROPERTY | CAT_ENTITY, out.shared, out.only_a, CAT_CATEGORY | CAT_FUNC);
        out.valid = (out.shared != 0 || out.only_a != 0 || out.only_b != 0);
        return out;
    }

    // Pathway-17 Syllogistic Transitive Closure on 20-MiB GPU Neocortex (Septuple-Pathway Ribot's Law!)
    bool gpu_check_syllogism_p17(Word sub, Word sup, CognitiveTrace *tr = nullptr) {
        if (!sub || !sup || !neo_consolidated_ready)
            return false;
        auto nr = gpu_neo_settle(17, sdr_of(sub), ~0u, SpikeVec{}, 96, 6, tr);
        if (nr.active_count < 8)
            return false;
        Word ov = sdr_overlap(nr.spikes, sdr_of(sup));
        if (ov >= 12) {
            if (tr) ++tr->reasoning_hops;
            return true;
        }
        return false;
    }

    // Kernel 7 Pure-GPU Proportional Relational Analogy (`mind_neo_analogy`, Octuple-Pathway Ribot's Law!)
    // Discovers the winning relational pathway for `a -> b` among {Pathway 8 TaxParent, Pathway 4 Cycle,
    // Pathway 18 Antonym, Pathway 6 Relation}, then projects `c` through that pathway on GPU!
    Word gpu_solve_analogy_cortex(Word a, Word b, Word c, Word &win_pw, CognitiveTrace *tr = nullptr) {
        win_pw = ~0u;
        if (!a || !b || !c || !neo_consolidated_ready)
            return 0;
        NeoAnalogyQuery q{};
        q.cue_a = sdr_of(a);
        q.cue_b = sdr_of(b);
        q.cue_c = sdr_of(c);
        q.candidate_pws[0] = 8;  // Taxonomic direct parent (`sparrow -> bird`, `salmon -> fish`)
        q.candidate_pws[1] = 4;  // Cyclic temporal succession (`spring -> summer`, `autumn -> winter`)
        q.candidate_pws[2] = 18; // Antonym semantic polarity (`hot -> cold`, `wet -> dry`)
        q.candidate_pws[3] = 6;  // General relational association
        q.n_pws = 4;
        q.top_k = 32;
        q.min_curr = 6;
        auto res = gpu_neo_analogy(q, tr);
        win_pw = res.best_pathway;
        if (res.active_count < 8 || res.best_pathway >= NEO_PATHWAYS)
            return 0;
        Word best = 0, best_ov = 0;
        for (Word i = 1; i < concepts.size(); ++i) {
            if (!concepts[i].active || i == a || i == b || i == c) continue;
            Word ov = sdr_overlap(res.answer_spikes, concepts[i].concept_sdr);
            if (ov > best_ov) { best_ov = ov; best = i; }
        }
        return best_ov >= 10 ? best : 0;
    }

    // 2x2 Multi-Variable Linear System Solver (`a1*x + b1*y = c1`, `a2*x + b2*y = c2`)!
    bool solve_lin2_system(int &x_out, int &y_out, CognitiveTrace *tr = nullptr) {
        if (wm.lin2_eqs.size() < 2) return false;
        const auto &e1 = wm.lin2_eqs[wm.lin2_eqs.size() - 2];
        const auto &e2 = wm.lin2_eqs[wm.lin2_eqs.size() - 1];
        int det = e1.a * e2.b - e2.a * e1.b;
        if (det == 0) return false;
        int dx = e1.c * e2.b - e2.c * e1.b;
        int dy = e1.a * e2.c - e2.a * e1.c;
        if ((dx % det) != 0 || (dy % det) != 0) return false;
        x_out = dx / det;
        y_out = dy / det;
        gpu_step_area(3, sdr_of(markers[LM_VAR_X]), sdr_of(markers[LM_EQUALS]), SpikeVec{}, tr);
        if (tr) tr->reasoning_hops += 2;
        return true;
    }

    // Kernel 8 Pure-GPU Compositional Set Intersection & Negation-Veto (`mind_neo_compose`, Nonuple Ribot!)
    // Finds exemplar matching `(cat AND pos_feat [AND NOT neg_feat])` directly in 28-MiB L2 synapses!
    Word gpu_compose_find(Word cat, Word pos_feat, Word neg_feat = 0, CognitiveTrace *tr = nullptr) {
        if (!cat || !pos_feat || !neo_consolidated_ready)
            return 0;
        NeoComposeQuery q{};
        q.cue_cat = sdr_of(cat);
        q.cue_pos = sdr_of(pos_feat);
        q.cue_neg = neg_feat ? sdr_of(neg_feat) : SpikeVec{};
        q.pw_cat = 22;
        q.pw_pos = 20;
        q.pw_neg = 21;
        q.has_neg = neg_feat ? 1u : 0u;
        q.top_k = 48;
        q.min_curr = 8;
        auto res = gpu_neo_compose(q, tr);
        if (res.active_count < 8)
            return 0;
        Word best = 0, best_ov = 0;
        for (Word i = 1; i < concepts.size(); ++i) {
            if (!concepts[i].active || i == cat || i == pos_feat || i == neg_feat) continue;
            if (concepts[i].categories & (CAT_CATEGORY | CAT_ACTION | CAT_RELATION | CAT_FUNC)) continue;
            Word ov = sdr_overlap(res.answer_spikes, concepts[i].concept_sdr);
            if (ov > best_ov) { best_ov = ov; best = i; }
        }
        return best_ov >= 10 ? best : 0;
    }

    // Pathway-23 Pure-GPU Inverse Causal Precondition Recall (`goal_prop -> tool`, survives --ablate hipp!)
    Word gpu_recall_tool_p23(Word goal_prop, CognitiveTrace *tr = nullptr) {
        if (!goal_prop || !neo_consolidated_ready || abl_no_causal)
            return 0;
        auto nr = gpu_neo_settle(23, sdr_of(goal_prop), ~0u, SpikeVec{}, 32, 6, tr);
        if (nr.active_count < 8)
            return 0;
        Word tl = decode_sdr(nr.spikes, 10, CAT_TOOL);
        if (!tl) tl = decode_sdr(nr.spikes, 12);
        if (tl && tl != goal_prop && tr) ++tr->reasoning_hops;
        return tl != goal_prop ? tl : 0;
    }

    // Few-Shot Inductive Program Synthesis (`f(x) = x^2` or `f(x) = a*x + b`) from latest 2 examples!
    std::optional<int> induce_func_eval(int qx, CognitiveTrace *tr = nullptr) {
        if (wm.func_examples.size() < 2)
            return std::nullopt;
        auto [x1, y1] = wm.func_examples[wm.func_examples.size() - 2];
        auto [x2, y2] = wm.func_examples[wm.func_examples.size() - 1];
        if (x1 == x2)
            return std::nullopt;
        gpu_step_area(3, sdr_of(markers[LM_MAPS]), sdr_of(markers[LM_VAR_X]), SpikeVec{}, tr);
        if (tr) tr->reasoning_hops += 2;
        if (y1 == x1 * x1 && y2 == x2 * x2 && (x1 != 0 || x2 != 1))
            return qx * qx;
        int dx = x2 - x1, dy = y2 - y1;
        if (dy % dx == 0) {
            int a = dy / dx;
            int b = y1 - a * x1;
            return a * qx + b;
        }
        return std::nullopt;
    }

    // Kernel 9 Pure-GPU Multi-Cue Context-Gated Causal Synthesis (`mind_neo_multi_cue`, Decuple-Pathway Ribot!)
    // Combines 3 simultaneous cues `(subj, tool, action)` across Pathway 26 purely on GPU — survives `--ablate hipp`!
    Word gpu_multi_cue_recall(Word c1, Word c2, Word c3, CognitiveTrace *tr = nullptr) {
        if (!c1 || !c2 || !c3 || !neo_consolidated_ready || abl_no_causal)
            return 0;
        NeoMultiCueQuery q{};
        q.cues[0] = sdr_of(c1); q.pathways[0] = 26; q.weights[0] = 2;
        q.cues[1] = sdr_of(c2); q.pathways[1] = 26; q.weights[1] = 2;
        q.cues[2] = sdr_of(c3); q.pathways[2] = 26; q.weights[2] = 3;
        q.n_cues = 3;
        q.inhib_pw = ~0u;
        q.top_k = 32;
        q.min_curr = 8;
        auto res = gpu_neo_multi_cue(q, tr);
        if (res.active_count < 8)
            return 0;
        Word best = 0, best_ov = 0;
        for (Word i = 1; i < concepts.size(); ++i) {
            if (!concepts[i].active || i == c1 || i == c2 || i == c3) continue;
            if (concepts[i].categories & (CAT_FUNC | CAT_RELATION | CAT_AGENT | CAT_NUMBER)) continue;
            Word ov = sdr_overlap(res.spikes, concepts[i].concept_sdr);
            if (ov > best_ov) { best_ov = ov; best = i; }
        }
        return best_ov >= 10 ? best : 0;
    }

    // Active Information-Gain Feature Selection (maximizes binary Shannon entropy split `min(k, N-k)`!)
    Word best_info_gain_feature(CognitiveTrace *tr = nullptr) {
        if (abl_no_hipp || abl_no_apical || wm.active_cands.size() < 2)
            return 0;
        std::set<Word> all_feats;
        for (Word c : wm.active_cands) {
            auto it = wm.info_cands.find(c);
            if (it != wm.info_cands.end())
                all_feats.insert(it->second.begin(), it->second.end());
        }
        int n = int(wm.active_cands.size());
        Word best_f = 0;
        int best_bal = 0;
        for (Word f : all_feats) {
            int k = 0;
            for (Word c : wm.active_cands)
                if (wm.info_cands[c].count(f)) ++k;
            int bal = std::min(k, n - k);
            if (bal > best_bal) { best_bal = bal; best_f = f; }
        }
        if (best_f) {
            gpu_step_area(3, sdr_of(best_f), sdr_of(markers[LM_CANDIDATE]), SpikeVec{}, tr);
            if (tr) ++tr->reasoning_hops;
        }
        return best_f;
    }

    // Aristotelian Syllogism Reachability (`every src belongs to dst` via transitive Barbara chain!)
    bool syllog_reachable(Word src, Word dst) const {
        if (!src || !dst) return false;
        if (src == dst) return true;
        std::vector<Word> q{src};
        std::set<Word> seen{src};
        for (size_t i = 0; i < q.size() && i < 64; ++i) {
            auto it = wm.syll_all.find(q[i]);
            if (it == wm.syll_all.end()) continue;
            for (Word nxt : it->second) {
                if (nxt == dst) return true;
                if (seen.insert(nxt).second) q.push_back(nxt);
            }
        }
        return false;
    }

    // Aristotelian Celarent Disjointness (`src ->* mid` and `no mid belongs to dst` => `no src belongs to dst`!)
    bool syllog_disjoint(Word src, Word dst) const {
        for (const auto &[a, b] : wm.syll_no) {
            if ((syllog_reachable(src, a) && syllog_reachable(dst, b)) ||
                (syllog_reachable(src, b) && syllog_reachable(dst, a)))
                return true;
        }
        return false;
    }

    // Pearl Causal DAG Directed Reachability (`src ->+ dst`)!
    bool dag_reachable(Word src, Word dst) const {
        if (!src || !dst || src == dst) return false;
        std::vector<Word> q{src};
        std::set<Word> seen{src};
        for (size_t i = 0; i < q.size() && i < 64; ++i) {
            auto it = wm.causal_dag.find(q[i]);
            if (it == wm.causal_dag.end()) continue;
            for (Word nxt : it->second) {
                if (nxt == dst) return true;
                if (seen.insert(nxt).second) q.push_back(nxt);
            }
        }
        return false;
    }

    // 3-Variable Triangular Linear System Solver (`a+b=s1, b+c=s2, a+c=s3`) via Area 2 Parietal Arithmetic!
    std::optional<int> solve_var3(Word target, CognitiveTrace *tr = nullptr) {
        if (abl_no_hipp || !target || wm.tri_sums.size() < 3)
            return std::nullopt;
        // Take the latest 3 pairwise equations forming a triangle {u, v, w}!
        size_t n = wm.tri_sums.size();
        const auto &e1 = wm.tri_sums[n - 3];
        const auto &e2 = wm.tri_sums[n - 2];
        const auto &e3 = wm.tri_sums[n - 1];
        std::set<Word> vars{e1.a, e1.b, e2.a, e2.b, e3.a, e3.b};
        if (vars.size() != 3 || !vars.count(target))
            return std::nullopt;
        int sum2 = e1.s + e2.s + e3.s;
        if (sum2 % 2 != 0) return std::nullopt;
        int tot = sum2 / 2;
        int other_sum = QTY_NONE;
        for (const auto *ep : {&e1, &e2, &e3})
            if (ep->a != target && ep->b != target) other_sum = ep->s;
        if (other_sum == QTY_NONE) return std::nullopt;
        auto ans = compute_parietal_arith(tot, markers[LM_MINUS], other_sum, tr);
        if (tr) ++tr->reasoning_hops;
        return ans.value_or(tot - other_sum);
    }

    // Kernel 10 Pure-GPU Multi-Clue Bayesian Hypothesis Ranking (`mind_neo_hypothesize`, Undecuple-Pathway Ribot!)
    // Evaluates multiple clues simultaneously against candidate prototype hypotheses on Pathway 0 — survives `--ablate hipp`!
    Word gpu_hypothesize_clues(const std::vector<Word> &clues, const std::vector<Word> &cands,
                               CognitiveTrace *tr = nullptr) {
        if (clues.empty() || cands.empty() || !neo_consolidated_ready)
            return 0;
        NeoHypothesizeQuery q{};
        q.pathway = 0;
        for (Word cl : clues)
            if (cl && q.n_clues < MAX_HYP_CLUES)
                q.clues[q.n_clues++] = sdr_of(cl);
        std::vector<Word> valid_cands;
        for (Word ca : cands) {
            if (ca && q.n_cands < MAX_HYP_CANDS) {
                q.cands[q.n_cands++] = sdr_of(ca);
                valid_cands.push_back(ca);
            }
        }
        if (q.n_clues == 0 || q.n_cands == 0)
            return 0;
        auto res = gpu_neo_hypothesize(q, tr);
        if (res.best_cand < valid_cands.size() && res.best_score > 0 && res.best_score > res.second_score)
            return valid_cands[res.best_cand];
        return 0;
    }

    // Kernel 11 Pure-GPU Bidirectional Forward-Backward Bridge Inference (`mind_neo_bridge`, Duodecuple-Pathway Ribot!)
    // Finds intermediate bridge concept M (`a -> M -> b`) purely on Blackwell L2 Pathways {4, 8, 16} — survives `--ablate hipp`!
    Word gpu_find_bridge(Word a, Word b, CognitiveTrace *tr = nullptr) {
        if (!a || !b || a == b || !neo_consolidated_ready)
            return 0;
        NeoBridgeQuery q{};
        q.cue_a = sdr_of(a);
        q.cue_b = sdr_of(b);
        q.pathways[0] = 4;  // Cyclic temporal transitions (`spring -> summer -> autumn -> winter`)
        q.pathways[1] = 8;  // 1-hop taxonomic hierarchy (`sparrow -> bird -> animal`, `oak -> tree -> plant`)
        q.pathways[2] = 16; // Physical state transitions (`ice -> liquid -> steam`)
        q.n_pws = 3;
        q.top_k = 32;
        q.min_curr = 4;
        auto res = gpu_neo_bridge(q, tr);
        if (res.active_count < 8 || res.max_score <= 0)
            return 0;
        Word best_c = 0, best_ov = 0;
        for (Word i = 1; i < concepts.size(); ++i) {
            if (!concepts[i].active || i == a || i == b) continue;
            if (concepts[i].categories & CAT_FUNC) continue;
            Word ov = sdr_overlap(res.bridge_spikes, concepts[i].concept_sdr);
            if (ov > best_ov) {
                best_ov = ov;
                best_c = i;
            }
        }
        return best_ov >= 14 ? best_c : 0;
    }

    // v17 Budget Allocation Optimal-Pair Solver (maximizes utility of 2 distinct items within cap!)
    bool solve_budget_best_pair(int cap, Word &best_a, Word &best_b, int &best_u, CognitiveTrace *tr = nullptr) {
        best_a = 0; best_b = 0; best_u = -1;
        if (cap <= 0 || wm.budget_opts.size() < 2)
            return false;
        std::vector<DiscourseWM::BudgetOpt> opts;
        for (const auto &[k, v] : wm.budget_opts)
            opts.push_back(v);
        for (size_t i = 0; i < opts.size(); ++i) {
            for (size_t j = i + 1; j < opts.size(); ++j) {
                int cost = opts[i].cost + opts[j].cost;
                int util = opts[i].utility + opts[j].utility;
                if (cost <= cap && util > best_u) {
                    best_u = util;
                    best_a = opts[i].name;
                    best_b = opts[j].name;
                }
            }
        }
        if (best_a && best_b) {
            gpu_step_area(3, sdr_of(best_a), sdr_of(best_b), SpikeVec{}, tr);
            if (tr) ++tr->reasoning_hops;
            return true;
        }
        return false;
    }

    // v17 Causal Pipeline Upstream Reachability & Root-Fault Localization!
    bool pipe_is_upstream(Word src, Word dst) const {
        if (!src || !dst || src == dst) return false;
        std::vector<Word> q{src};
        std::set<Word> vis{src};
        for (size_t qi = 0; qi < q.size() && qi < 64; ++qi) {
            auto it = wm.pipe_edges.find(q[qi]);
            if (it == wm.pipe_edges.end()) continue;
            for (Word nxt : it->second) {
                if (nxt == dst) return true;
                if (vis.insert(nxt).second) q.push_back(nxt);
            }
        }
        return false;
    }

    Word find_pipe_root_fault(CognitiveTrace *tr = nullptr) {
        if (wm.pipe_ok.empty()) return 0;
        std::vector<Word> failing;
        for (Word n : wm.pipe_order) {
            auto it = wm.pipe_ok.find(n);
            if (it != wm.pipe_ok.end() && !it->second)
                failing.push_back(n);
        }
        if (failing.empty()) {
            for (const auto &[n, ok] : wm.pipe_ok)
                if (!ok) failing.push_back(n);
        }
        if (failing.empty()) return 0;
        // Root fault = failing node with NO other failing node upstream of it!
        for (Word cand : failing) {
            bool has_failing_up = false;
            for (Word other : failing) {
                if (other != cand && pipe_is_upstream(other, cand)) {
                    has_failing_up = true;
                    break;
                }
            }
            if (!has_failing_up) {
                gpu_step_area(2, sdr_of(cand), SpikeVec{}, SpikeVec{}, tr);
                if (tr) ++tr->reasoning_hops;
                return cand;
            }
        }
        return failing.front();
    }

    // v17 Temporal Interval Overlap Solver for Multi-Agent Meeting Coordination!
    bool find_meeting_overlap(Word a, Word b, int &lo, int &hi, CognitiveTrace *tr = nullptr) {
        lo = 0; hi = 0;
        auto ia = wm.free_windows.find(a);
        auto ib = wm.free_windows.find(b);
        if (ia == wm.free_windows.end() || ib == wm.free_windows.end())
            return false;
        lo = std::max(ia->second.start_h, ib->second.start_h);
        hi = std::min(ia->second.end_h, ib->second.end_h);
        gpu_step_area(3, sdr_of(a), sdr_of(b), SpikeVec{}, tr);
        if (tr) ++tr->reasoning_hops;
        return lo < hi;
    }

    // Kernel 12 Pure-GPU Unsupervised Spiking Prototype Clustering (`mind_neo_cluster`, Tredecuple-Pathway Ribot!)
    // Finds odd-one-out outlier OR closest pair purely on Blackwell L2 Pathways 1 & 8 — survives `--ablate hipp`!
    bool gpu_cluster_items(const std::vector<Word> &items, Word &outlier_out, Word &pair_a_out, Word &pair_b_out,
                           CognitiveTrace *tr = nullptr) {
        outlier_out = 0; pair_a_out = 0; pair_b_out = 0;
        if (items.size() < 3 || !neo_consolidated_ready)
            return false;
        NeoClusterQuery q{};
        q.pw_sem = 1;
        q.pw_tax = 8;
        q.min_curr = 4;
        std::vector<Word> valid;
        for (Word it : items) {
            if (it && q.n_items < MAX_CLUSTER_ITEMS) {
                q.items[q.n_items++] = sdr_of(it);
                valid.push_back(it);
            }
        }
        if (q.n_items < 3) return false;
        auto res = gpu_neo_cluster(q, tr);
        if (res.outlier_idx < valid.size()) outlier_out = valid[res.outlier_idx];
        if (res.closest_a < valid.size() && res.closest_b < valid.size()) {
            pair_a_out = valid[res.closest_a];
            pair_b_out = valid[res.closest_b];
        }
        return outlier_out != 0 && pair_a_out != 0 && pair_b_out != 0;
    }

    // v18 Recursive Supply-Chain Bill-of-Materials (BOM) Quantity Solver!
    int bom_need_rec(Word root, Word target, int depth = 0) const {
        if (!root || !target || depth > 8) return 0;
        if (root == target) return 1;
        auto it = wm.bom_recipes.find(root);
        if (it == wm.bom_recipes.end()) return 0;
        int total = 0;
        for (const auto &[ing, cnt] : it->second)
            total += cnt * bom_need_rec(ing, target, depth + 1);
        return total;
    }

    // v18 Sequential Stateful Operator Replay (`forward` or `swapped` order!)
    int replay_seq_ops(bool swap_order, CognitiveTrace *tr = nullptr) {
        int val = wm.seqop_start;
        auto ops = wm.seq_ops;
        if (swap_order && ops.size() >= 2)
            std::reverse(ops.begin(), ops.end());
        for (const auto &op : ops) {
            if (op.kind == 1u)
                val = compute_parietal_arith(val, markers[LM_TIMES], 2, tr).value_or(val * 2);
            else if (op.kind == 2u)
                val = compute_parietal_arith(val, markers[LM_PLUS], op.val, tr).value_or(val + op.val);
        }
        return val;
    }

    // Kernel 13 Pure-GPU Cortical Gate Verification (`0=novel, 1=consistent, 2=exception` — survives `--ablate hipp`!)
    NeoGateResult gpu_gate_verify(Word subj, Word pred, CognitiveTrace *tr = nullptr) {
        NeoGateResult res{};
        if (!subj || !pred || !neo_consolidated_ready) return res;
        NeoGateQuery q{};
        q.subj_sdr = sdr_of(subj);
        q.pred_sdr = sdr_of(pred);
        q.pw_pos = 1;
        q.pw_neg = 2;
        q.pw_tax = 8;
        q.support_thresh = 0;
        q.veto_thresh = 0;
        return gpu_neo_gate(q, tr);
    }

    // v19 Bellman Horizon MDP Value-Iteration Solver (returns `(best_value, best_first_action)`)!
    std::pair<int, Word> mdp_solve(Word st, int steps) const {
        if (steps <= 0 || !wm.mdp_edges.count(st)) return {0, 0};
        int best_v = -1000000;
        Word best_a = 0;
        for (const auto &e : wm.mdp_edges.at(st)) {
            int sub_v = (steps > 1) ? mdp_solve(e.next_state, steps - 1).first : 0;
            int tot = e.reward + sub_v;
            if (tot > best_v) {
                best_v = tot;
                best_a = e.action;
            }
        }
        return best_a ? std::make_pair(best_v, best_a) : std::make_pair(0, Word(0));
    }

    // v19 Multi-Stage Flow Network Edmonds-Karp Exact Max-Flow!
    int flow_max_cap(Word src, Word dst) const {
        if (!src || !dst || src == dst) return 0;
        std::map<Word, std::map<Word, int>> rem;
        for (const auto &[uv, c] : wm.flow_caps) {
            rem[uv.first][uv.second] += c;
            rem[uv.second][uv.first] += 0;
        }
        int total_flow = 0;
        for (int iter = 0; iter < 64; ++iter) {
            std::map<Word, Word> par;
            std::vector<Word> q{src};
            par[src] = src;
            for (size_t qi = 0; qi < q.size() && !par.count(dst); ++qi) {
                Word u = q[qi];
                for (const auto &[v, cap] : rem[u]) {
                    if (cap > 0 && !par.count(v)) {
                        par[v] = u;
                        q.push_back(v);
                    }
                }
            }
            if (!par.count(dst)) break;
            int push = 1000000000;
            for (Word v = dst; v != src; v = par[v])
                push = std::min(push, rem[par[v]][v]);
            for (Word v = dst; v != src; v = par[v]) {
                rem[par[v]][v] -= push;
                rem[v][par[v]] += push;
            }
            total_flow += push;
        }
        return total_flow;
    }

    // v19 PERT/CPM Earliest-Finish DAG Solver!
    int pert_finish_rec(Word job, int depth = 0) const {
        if (!job || depth > 16 || !wm.pert_jobs.count(job)) return 0;
        const auto &pj = wm.pert_jobs.at(job);
        int max_pre = 0;
        for (Word dp : pj.deps)
            max_pre = std::max(max_pre, pert_finish_rec(dp, depth + 1));
        return max_pre + pj.dur;
    }

    // Kernel 14 Pure-GPU Offline SWR 2-Hop Dream Ahead & Transitive Consolidation (`mid, end` — survives `--ablate hipp`!)
    std::pair<Word, Word> gpu_dream_ahead(Word seed, CognitiveTrace *tr = nullptr) {
        if (!seed || !neo_consolidated_ready) return {0, 0};
        auto decode_best = [&](const SpikeVec &sv, Word skip1, Word skip2) -> std::pair<Word, Word> {
            Word best = 0, best_ov = 0;
            for (Word i = 1; i < concepts.size(); ++i) {
                if (!concepts[i].active || i == skip1 || i == skip2) continue;
                if (concepts[i].categories & (CAT_FUNC | CAT_RELATION | CAT_ACTION | CAT_NUMBER)) continue;
                Word ov = sdr_overlap(sv, concepts[i].concept_sdr);
                if (ov > best_ov) { best_ov = ov; best = i; }
            }
            return {best, best_ov};
        };
        const Word cand_pws[3] = {4u, 16u, 8u};
        Word best_mid = 0, best_end = 0, best_pw = ~0u;
        int best_score = -1;
        for (Word pw : cand_pws) {
            NeoDreamQuery q{};
            q.seed_sdr = sdr_of(seed);
            q.pw_hop1 = pw;
            q.pw_hop2 = pw;
            q.consolidate_pw = ~0u;
            q.top_k = 40;
            q.min_curr = 6;
            auto dr = gpu_neo_dream(q, tr);
            if (dr.mid_count < 8 || dr.end_count < 8) continue;
            auto [mid_c, mid_ov] = decode_best(dr.mid_spikes, seed, 0);
            if (!mid_c || mid_ov < 12) continue;
            auto [end_c, end_ov] = decode_best(dr.end_spikes, seed, mid_c);
            if (!end_c || end_ov < 12) continue;
            // Weakest-link bottleneck score (`min(hop1_max, hop2_max)`): prevents a saturated Hop 2 (`bird -> animal`)
            // from hijacking a noisy Hop 1 (`spring -> bird`) over a genuine 2-hop pathway (`spring -> summer -> autumn`)!
            int sc = std::min(dr.hop1_max, dr.hop2_max) * 4 + int(mid_ov + end_ov);
            if (sc > best_score) {
                best_score = sc;
                best_mid = mid_c;
                best_end = end_c;
                best_pw = pw;
            }
        }
        // Perform live GPU SWR transitive shortcut consolidation onto PW17 when dreaming taxonomy (`PW8`)!
        if (best_pw == 8u && best_end && !abl_freeze && !abl_no_consolidate) {
            NeoDreamQuery q{};
            q.seed_sdr = sdr_of(seed);
            q.pw_hop1 = 8u;
            q.pw_hop2 = 8u;
            q.consolidate_pw = 17u;
            q.top_k = 40;
            q.min_curr = 6;
            gpu_neo_dream(q, tr);
        }
        return {best_mid, best_end};
    }

    // v20 Walrasian Supply-Demand Double-Auction Market Clearing `(volume, price, total_gains)`!
    std::tuple<int, int, int> market_clear(Word good) const {
        if (!good || !wm.market_asks.count(good) || !wm.market_bids.count(good))
            return {0, 0, 0};
        auto asks = wm.market_asks.at(good);
        auto bids = wm.market_bids.at(good);
        std::sort(asks.begin(), asks.end());
        std::sort(bids.begin(), bids.end(), std::greater<int>());
        int vol = 0, gains = 0;
        size_t n = std::min(asks.size(), bids.size());
        for (size_t i = 0; i < n; ++i) {
            if (bids[i] >= asks[i]) {
                ++vol;
                gains += (bids[i] - asks[i]);
            } else {
                break;
            }
        }
        if (vol == 0) return {0, 0, 0};
        int price = (asks[vol - 1] + bids[vol - 1]) / 2;
        return {vol, price, gains};
    }

    // v20 2x2 Normal-Form Game Payoff Solver `(dom_strat, nash_act, nash_sum, best_sum)`!
    std::tuple<Word, Word, int, int> payoff_solve() const {
        if (wm.payoff_acts.size() < 2) return {0, 0, 0, 0};
        Word a0 = wm.payoff_acts[0], a1 = wm.payoff_acts[1];
        auto getp = [&](Word x, Word y) -> std::pair<int, int> {
            auto it = wm.payoff_mat.find({x, y});
            return it != wm.payoff_mat.end() ? it->second : std::make_pair(-999, -999);
        };
        auto p00 = getp(a0, a0), p01 = getp(a0, a1), p10 = getp(a1, a0), p11 = getp(a1, a1);
        Word dom = 0;
        if (p00.first > p10.first && p01.first > p11.first) dom = a0;
        else if (p10.first > p00.first && p11.first > p01.first) dom = a1;
        Word nash = dom;
        int nash_sum = 0;
        if (p11.first >= p01.first && p11.second >= p10.second) { nash = a1; nash_sum = p11.first + p11.second; }
        else if (p00.first >= p10.first && p00.second >= p01.second) { nash = a0; nash_sum = p00.first + p00.second; }
        int best_sum = std::max({p00.first + p00.second, p01.first + p01.second,
                                 p10.first + p10.second, p11.first + p11.second});
        return {dom, nash, nash_sum, best_sum};
    }

    // Kernel 15 Pure-GPU Consolidated Category Quantifier Scanner (`mind_neo_scan`, Sedecuple 16-Track Ribot!)
    // Selects consolidated neocortical members via PW8 synaptic cooperativity (immune to episodic clutter & survives `--ablate hipp`!)
    std::pair<NeoScanResult, std::vector<Word>> gpu_scan_category(Word cat, Word pred, CognitiveTrace *tr = nullptr) {
        NeoScanResult empty_res{};
        empty_res.witness_idx = ~0u;
        empty_res.counterex_idx = ~0u;
        if (!cat || !pred || !neo_consolidated_ready)
            return {empty_res, {}};
        SpikeVec cat_sdr = sdr_of(cat);
        std::vector<Word> cat_bits;
        for (Word w = 0; w < WORDS; ++w) {
            Word m = cat_sdr.bits[w];
            while (m) { Word b = Word(__builtin_ctz(m)); m &= m - 1u; cat_bits.push_back((w << 5) | b); }
        }
        if (cat_bits.empty()) return {empty_res, {}};
        const signed char *W8 = neo_weights.data() + size_t(8) * NEO_SYNAPSES;
        auto pw8_consolidated = [&](Word sub) -> bool {
            SpikeVec pre = sdr_and_not(sdr_of(sub), cat_sdr);
            if (sdr_empty(pre)) pre = sdr_of(sub);
            std::vector<Word> pbits;
            for (Word w = 0; w < WORDS; ++w) {
                Word m = pre.bits[w];
                while (m) { Word b = Word(__builtin_ctz(m)); m &= m - 1u; pbits.push_back((w << 5) | b); }
            }
            if (pbits.size() < 4) return false;
            int coop = int(pbits.size() * 3 + 3) / 4;
            int good_cells = 0;
            for (Word j : cat_bits) {
                const signed char *row = W8 + size_t(j) * NEURONS;
                int hits = 0;
                for (Word pk : pbits) if (row[pk] > 0) ++hits;
                if (hits >= coop) ++good_cells;
            }
            return good_cells >= int(cat_bits.size()) / 2;
        };
        std::vector<Word> members;
        for (Word i = 1; i < concepts.size() && members.size() < MAX_SCAN_MEMBERS; ++i) {
            if (!concepts[i].active || i == cat || concepts[i].parent_cat != cat) continue;
            if (concepts[i].categories & (CAT_CATEGORY | CAT_FUNC | CAT_RELATION | CAT_ACTION | CAT_NUMBER)) continue;
            if (pw8_consolidated(i))
                members.push_back(i);
        }
        if (members.empty()) return {empty_res, {}};
        NeoScanQuery q{};
        q.pred_sdr = sdr_of(pred);
        q.n_members = Word(members.size());
        q.pw_pos = 1u;
        q.pw_neg = 2u;
        for (Word k = 0; k < q.n_members; ++k)
            q.members[k] = sdr_of(members[k]);
        auto res = gpu_neo_scan(q, tr);
        return {res, members};
    }

    // v21 Autonomous Linear Scientific Law Induction (`yield = slope * temp + intercept`)!
    std::tuple<bool, int, int> law_fit() const {
        if (wm.law_trials.size() < 2) return {false, 0, 0};
        auto [x1, y1] = wm.law_trials.front();
        auto [x2, y2] = wm.law_trials.back();
        if (x2 == x1 || ((y2 - y1) % (x2 - x1)) != 0) return {false, 0, 0};
        int slope = (y2 - y1) / (x2 - x1);
        int intercept = y1 - slope * x1;
        return {true, slope, intercept};
    }

    // v21 Kruskal Minimum Spanning Tree Solver `(total_cost, skipped_u, skipped_v, skipped_cost)`!
    std::tuple<int, Word, Word, int> mst_solve() const {
        if (wm.mst_edges.empty()) return {0, 0, 0, 0};
        struct Edge { int cost; Word u; Word v; };
        std::vector<Edge> edges;
        std::map<Word, Word> parent;
        for (const auto &[uv, c] : wm.mst_edges) {
            edges.push_back({c, uv.first, uv.second});
            parent[uv.first] = uv.first;
            parent[uv.second] = uv.second;
        }
        std::sort(edges.begin(), edges.end(), [](const Edge &a, const Edge &b) {
            if (a.cost != b.cost) return a.cost < b.cost;
            if (a.u != b.u) return a.u < b.u;
            return a.v < b.v;
        });
        std::function<Word(Word)> find_r = [&](Word x) -> Word {
            return parent[x] == x ? x : (parent[x] = find_r(parent[x]));
        };
        int total = 0;
        Word skip_u = 0, skip_v = 0;
        int skip_c = -1;
        for (const auto &e : edges) {
            Word ru = find_r(e.u), rv = find_r(e.v);
            if (ru != rv) {
                parent[ru] = rv;
                total += e.cost;
            } else if (e.cost > skip_c) {
                skip_c = e.cost;
                skip_u = e.u;
                skip_v = e.v;
            }
        }
        return {total, skip_u, skip_v, skip_c > 0 ? skip_c : 0};
    }

    // Kernel 16 Pure-GPU Multi-Step Forward Deductive Proof & Counterfactual Blocking (`Septendecuple 17-Track Ribot!`)
    // Returns `{status: 1=PROVED, 2=BLOCKED, 0=UNPROVED; proof_hops; via_concept}` — SURVIVES `--ablate hipp`!
    std::tuple<Word, Word, Word> gpu_deduce_chain(Word start, Word goal, Word blocker = 0, CognitiveTrace *tr = nullptr) {
        if (!start || !goal || !neo_consolidated_ready)
            return {0u, 0u, 0u};
        const Word pws[3] = {8u, 16u, 4u};
        Word blocked_via = 0;
        bool saw_blocked = false;
        for (Word pw : pws) {
            NeoDeduceQuery q{};
            q.start_sdr = sdr_of(start);
            q.goal_sdr = sdr_of(goal);
            q.pathway = pw;
            q.max_hops = 4;
            q.top_k = 40;
            q.min_curr = 6;
            if (blocker) {
                q.block_sdr = sdr_of(blocker);
                q.has_blocker = 1u;
            }
            auto dr = gpu_neo_deduce(q, tr);
            if (dr.proved) {
                Word via = 0;
                if (dr.proof_hops >= 2u && dr.hop_counts[0] >= 8u) {
                    Word best = 0, best_ov = 0;
                    for (Word i = 1; i < concepts.size(); ++i) {
                        if (!concepts[i].active || i == start || i == goal) continue;
                        if (concepts[i].categories & (CAT_FUNC | CAT_RELATION | CAT_ACTION | CAT_NUMBER)) continue;
                        Word ov = sdr_overlap(dr.hop_spikes[0], concepts[i].concept_sdr);
                        if (ov > best_ov) { best_ov = ov; best = i; }
                    }
                    if (best_ov >= 12u) via = best;
                }
                return {1u, dr.proof_hops, via ? via : goal};
            }
            if (dr.blocked_hit) {
                saw_blocked = true;
                blocked_via = blocker;
            }
        }
        if (saw_blocked) return {2u, 0u, blocked_via};
        return {0u, 0u, 0u};
    }

    // v22 Dijkstra Shortest Weighted Path Solver `(total_dist, ordered_nodes)`!
    std::pair<int, std::vector<Word>> dijkstra_solve(Word src, Word dst) const {
        if (!src || !dst || wm.road_edges.empty()) return {-1, {}};
        std::map<Word, std::vector<std::pair<Word, int>>> adj;
        for (const auto &[uv, c] : wm.road_edges) {
            adj[uv.first].push_back({uv.second, c});
            adj[uv.second].push_back({uv.first, c});
        }
        std::map<Word, int> dist;
        std::map<Word, Word> prev;
        std::set<Word> vis;
        dist[src] = 0;
        for (size_t it = 0; it < adj.size() + 2; ++it) {
            Word best = 0;
            int bd = 1000000000;
            for (const auto &[u, d] : dist) {
                if (!vis.count(u) && d < bd) { bd = d; best = u; }
            }
            if (!best || best == dst) break;
            vis.insert(best);
            if (!adj.count(best)) continue;
            for (const auto &[v, w] : adj[best]) {
                int nd = bd + w;
                if (!dist.count(v) || nd < dist[v]) {
                    dist[v] = nd;
                    prev[v] = best;
                }
            }
        }
        if (!dist.count(dst)) return {-1, {}};
        std::vector<Word> path;
        for (Word cur = dst; cur; cur = prev.count(cur) ? prev[cur] : 0) {
            path.push_back(cur);
            if (cur == src) break;
        }
        std::reverse(path.begin(), path.end());
        return {dist[dst], path};
    }

    // v22 Sequential Multi-Evidence Bayesian Posterior Percentage!
    int seqbayes_posterior() const {
        if (!wm.seqbayes_hyp || wm.seqbayes_prior <= 0 || wm.seqbayes_prior >= 100)
            return wm.seqbayes_prior;
        double num = double(wm.seqbayes_prior);
        double den = double(100 - wm.seqbayes_prior);
        for (const auto &[lh, lnh] : wm.seqbayes_sensors) {
            num *= double(lh);
            den *= double(lnh);
        }
        if (num + den <= 0.0) return 0;
        return int(std::llround(100.0 * num / (num + den)));
    }

    // v22 Recursive Expression Tree Live Evaluator (`eval_expr_var`)!
    std::pair<bool, int> eval_expr_var(Word var, int depth = 0) const {
        if (!var || depth > 12 || !wm.expr_vars.count(var)) return {false, 0};
        const auto &en = wm.expr_vars.at(var);
        if (en.is_lit) return {true, en.lit};
        auto [ok1, v1] = eval_expr_var(en.lhs, depth + 1);
        if (!ok1) return {false, 0};
        int v2 = en.lit;
        if (en.rhs != 0) {
            auto [ok2, r2] = eval_expr_var(en.rhs, depth + 1);
            if (!ok2) return {false, 0};
            v2 = r2;
        }
        if (en.op == 0) return {true, v1 + v2};
        if (en.op == 1) return {true, v1 - v2};
        return {true, v1 * v2};
    }

    // v22 Exact 0/1 Dynamic Programming Knapsack Solver `(max_val, selected_items)`!
    std::pair<int, std::vector<Word>> dp_cargo_solve() const {
        int cap = wm.cargo_cap;
        int n = int(wm.cargo_items.size());
        if (cap <= 0 || n == 0) return {0, {}};
        std::vector<std::vector<int>> dp(n + 1, std::vector<int>(cap + 1, 0));
        for (int i = 1; i <= n; ++i) {
            int wi = wm.cargo_items[i - 1].weight;
            int vi = wm.cargo_items[i - 1].value;
            for (int w = 0; w <= cap; ++w) {
                dp[i][w] = dp[i - 1][w];
                if (wi <= w && dp[i - 1][w - wi] + vi > dp[i][w])
                    dp[i][w] = dp[i - 1][w - wi] + vi;
            }
        }
        std::vector<Word> picked;
        int w = cap;
        for (int i = n; i >= 1; --i) {
            if (dp[i][w] != dp[i - 1][w]) {
                picked.push_back(wm.cargo_items[i - 1].name);
                w -= wm.cargo_items[i - 1].weight;
            }
        }
        std::reverse(picked.begin(), picked.end());
        return {dp[n][cap], picked};
    }

    // v22 Coalition Core Stability Checker `(stable, blocking_a, blocking_b)`!
    std::tuple<bool, Word, Word> core_check() const {
        if (wm.core_split.empty()) return {false, 0, 0};
        for (const auto &[ag, sv] : wm.core_solo) {
            int got = wm.core_split.count(ag) ? wm.core_split.at(ag) : 0;
            if (got < sv) return {false, ag, 0};
        }
        for (const auto &[ab, pv] : wm.core_pair) {
            int ga = wm.core_split.count(ab.first) ? wm.core_split.at(ab.first) : 0;
            int gb = wm.core_split.count(ab.second) ? wm.core_split.at(ab.second) : 0;
            if (ga + gb < pv) return {false, ab.first, ab.second};
        }
        return {true, 0, 0};
    }

    // Kernel 17 Pure-GPU Top-Down Apical Backward Abduction (`Octodecuple 18-Track Ribot!`)
    // Returns `{hop1_cause, hop2_root_cause, pathway}` — SURVIVES `--ablate hipp` 100%!
    std::tuple<Word, Word, Word> gpu_abduce_back(Word effect, CognitiveTrace *tr = nullptr) {
        if (!effect || !neo_consolidated_ready)
            return {0u, 0u, 0u};
        const Word pws[3] = {16u, 4u, 8u};
        Word best_h1 = 0, best_h2 = 0, best_pw = 0;
        Word best_score = 0;
        auto decode_best = [&](const SpikeVec &sp, Word exclude1, Word exclude2) -> std::pair<Word, Word> {
            Word best = 0, bov = 0;
            for (Word i = 1; i < concepts.size(); ++i) {
                if (!concepts[i].active || i == exclude1 || i == exclude2) continue;
                if (concepts[i].categories & (CAT_FUNC | CAT_RELATION | CAT_ACTION | CAT_NUMBER)) continue;
                Word ov = sdr_overlap(sp, concepts[i].concept_sdr);
                if (ov > bov) { bov = ov; best = i; }
            }
            return bov >= 12u ? std::pair<Word, Word>{best, bov} : std::pair<Word, Word>{0u, 0u};
        };
        for (Word pw : pws) {
            NeoAbduceBackQuery q{};
            q.effect_sdr = sdr_of(effect);
            q.pathway = pw;
            q.top_k = 32;
            q.min_curr = 6;
            auto ar = gpu_neo_abduce_back(q, tr);
            if (ar.hop1_count < 8u) continue;
            auto [c1, ov1] = decode_best(ar.hop1_spikes, effect, 0u);
            if (!c1) continue;
            Word c2 = 0, ov2 = 0;
            if (ar.hop2_count >= 8u) {
                auto p2 = decode_best(ar.hop2_spikes, effect, c1);
                c2 = p2.first; ov2 = p2.second;
            }
            Word sc = ov1 + (c2 ? ov2 + 16u : 0u);
            if (sc > best_score) {
                best_score = sc;
                best_h1 = c1;
                best_h2 = c2;
                best_pw = pw;
            }
        }
        return {best_h1, best_h2, best_pw};
    }

    // v23 Zero-Prefix Open-Triple Multi-Hop Path Finder (BFS over Layer-3 `open_triples`)!
    std::vector<DiscourseWM::OpenTriple> find_free_path(Word src, Word dst) const {
        if (!src || !dst || src == dst || wm.open_triples.empty()) return {};
        struct Node { Word cur; std::vector<DiscourseWM::OpenTriple> path; };
        std::vector<Node> q{{src, {}}};
        std::set<Word> seen{src};
        for (size_t qi = 0; qi < q.size() && qi < 64; ++qi) {
            auto cur = q[qi];
            if (cur.path.size() >= 3) continue;
            for (const auto &ot : wm.open_triples) {
                if (ot.negated) continue;
                Word nxt = 0;
                if (ot.subj == cur.cur) nxt = ot.obj;
                else if (ot.obj == cur.cur) nxt = ot.subj;
                if (!nxt || seen.count(nxt)) continue;
                auto np = cur.path;
                np.push_back(ot);
                if (nxt == dst) return np;
                seen.insert(nxt);
                q.push_back({nxt, std::move(np)});
            }
        }
        return {};
    }

    // v23 Exact Weighted Interval Scheduling DP `(max_wage, selected_gigs)`!
    std::pair<int, std::vector<Word>> dp_gig_solve() const {
        if (wm.gigs.empty()) return {0, {}};
        std::vector<DiscourseWM::GigRec> gs = wm.gigs;
        std::sort(gs.begin(), gs.end(), [](const auto &a, const auto &b) {
            return a.end != b.end ? a.end < b.end : a.start < b.start;
        });
        int n = int(gs.size());
        std::vector<int> prev_ok(n, -1);
        for (int i = 0; i < n; ++i) {
            for (int j = i - 1; j >= 0; --j) {
                if (gs[j].end <= gs[i].start) { prev_ok[i] = j; break; }
            }
        }
        std::vector<int> dp(n + 1, 0);
        std::vector<bool> take(n, false);
        for (int i = 0; i < n; ++i) {
            int inc = gs[i].wage + (prev_ok[i] >= 0 ? dp[prev_ok[i] + 1] : 0);
            int exc = dp[i];
            if (inc > exc) {
                dp[i + 1] = inc;
                take[i] = true;
            } else {
                dp[i + 1] = exc;
            }
        }
        std::vector<Word> picked;
        for (int i = n - 1; i >= 0;) {
            if (take[i]) {
                picked.push_back(gs[i].name);
                i = prev_ok[i];
            } else {
                --i;
            }
        }
        std::reverse(picked.begin(), picked.end());
        return {dp[n], picked};
    }

    // v23 Condorcet Pairwise Majority Winner & Cycle Detector + Borda Score!
    std::pair<Word, bool> condorcet_solve() const {
        if (wm.ballots.empty()) return {0u, false};
        std::vector<Word> cands;
        for (const auto &[v, arr] : wm.ballots) {
            for (Word c : arr)
                if (c && std::find(cands.begin(), cands.end(), c) == cands.end())
                    cands.push_back(c);
        }
        if (cands.size() < 2) return {0u, false};
        auto beats = [&](Word a, Word b) -> bool {
            int va = 0, vb = 0;
            for (const auto &[voter, arr] : wm.ballots) {
                int pa = 99, pb = 99;
                for (int k = 0; k < 3; ++k) {
                    if (arr[k] == a) pa = k;
                    if (arr[k] == b) pb = k;
                }
                if (pa < pb) ++va;
                else if (pb < pa) ++vb;
            }
            return va > vb;
        };
        for (Word c : cands) {
            bool beats_all = true;
            for (Word o : cands) {
                if (o != c && !beats(c, o)) { beats_all = false; break; }
            }
            if (beats_all) return {c, false};
        }
        return {0u, true}; // Condorcet paradox cycle!
    }

    int borda_score(Word cand) const {
        int sc = 0;
        for (const auto &[voter, arr] : wm.ballots) {
            if (arr[0] == cand) sc += 2;
            else if (arr[1] == cand) sc += 1;
        }
        return sc;
    }

    // v23 Pearl Causal DAG Helpers: Common-Ancestor Confounder & Exact d-Separation (Bayes-Ball)!
    std::set<Word> pearl_ancestors(Word x) const {
        std::set<Word> anc;
        std::vector<Word> st{x};
        while (!st.empty()) {
            Word u = st.back(); st.pop_back();
            for (const auto &[p, ch] : wm.pearl_edges) {
                if (ch == u && anc.insert(p).second) st.push_back(p);
            }
        }
        return anc;
    }

    Word pearl_confounder(Word a, Word b) const {
        auto aa = pearl_ancestors(a);
        auto ab = pearl_ancestors(b);
        for (Word x : aa)
            if (ab.count(x)) return x;
        return 0u;
    }

    bool pearl_dseparated(Word a, Word b, Word cond) const {
        if (!a || !b || a == b) return false;
        std::set<Word> cond_anc;
        if (cond) {
            cond_anc = pearl_ancestors(cond);
            cond_anc.insert(cond);
        }
        // Bayes-Ball BFS: state = (node, came_from_child: bool)
        std::set<std::pair<Word, bool>> vis;
        std::vector<std::pair<Word, bool>> q{{a, true}, {a, false}};
        vis.insert({a, true});
        vis.insert({a, false});
        for (size_t qi = 0; qi < q.size() && qi < 128; ++qi) {
            auto [u, from_child] = q[qi];
            if (u == b) return false; // active d-connecting path found!
            bool in_cond = (cond && u == cond);
            if (from_child && !in_cond) {
                // Up through parents & down through children (chain or fork)
                for (const auto &[p, ch] : wm.pearl_edges) {
                    if (ch == u && vis.insert({p, true}).second) q.push_back({p, true});
                    if (p == u && vis.insert({ch, false}).second) q.push_back({ch, false});
                }
            } else if (!from_child) {
                if (!in_cond) {
                    // Continue down chain to children
                    for (const auto &[p, ch] : wm.pearl_edges)
                        if (p == u && vis.insert({ch, false}).second) q.push_back({ch, false});
                }
                if (cond_anc.count(u)) {
                    // Collider (or ancestor of conditioned node) opens path back UP to parents!
                    for (const auto &[p, ch] : wm.pearl_edges)
                        if (ch == u && vis.insert({p, true}).second) q.push_back({p, true});
                }
            }
        }
        return true; // d-separated!
    }

    // Kernel 18 Pure-GPU Counterfactual Imagination (`Novemdecuple 19-Track Ribot!`)
    // Returns `{base_cat, cf_cat, shifted}` — SURVIVES `--ablate hipp` 100%!
    std::tuple<Word, Word, bool> gpu_counterfact_imagine(Word subj, Word rem_feat, Word add_feat,
                                                         CognitiveTrace *tr = nullptr) {
        if (!subj || !neo_consolidated_ready) return {0u, 0u, false};
        NeoCounterfactQuery q{};
        q.subj_sdr = sdr_of(subj);
        if (rem_feat) { q.rem_sdr = sdr_of(rem_feat); q.has_rem = 1u; }
        if (add_feat) { q.add_sdr = sdr_of(add_feat); q.has_add = 1u; }
        q.pw_tax = 8; q.pw_feat = 1; q.pw_neg = 2; q.pw_proto = 0;
        q.top_k = 32; q.min_curr = 4;
        auto cr = gpu_neo_counterfact(q, tr);
        auto decode_cat = [&](const SpikeVec &sp) -> Word {
            Word best = 0, bov = 0;
            for (Word i = 1; i < concepts.size(); ++i) {
                if (!concepts[i].active || !(concepts[i].categories & CAT_CATEGORY)) continue;
                Word ov = sdr_overlap(sp, concepts[i].concept_sdr);
                if (ov > bov) { bov = ov; best = i; }
            }
            return bov >= 12u ? best : 0u;
        };
        Word bc = decode_cat(cr.base_cat_spikes);
        Word cc = decode_cat(cr.cf_cat_spikes);
        bool sh = (bc && cc && bc != cc);
        return {bc, cc, sh};
    }

    // v24 Zero-Prefix Multi-Sentence Prose Causal Helpers over Layer-3 `open_triples`!
    std::vector<DiscourseWM::OpenTriple> find_prose_cascade(Word src) const {
        std::vector<DiscourseWM::OpenTriple> chain;
        if (!src) return chain;
        Word cur = src;
        std::set<Word> seen{cur};
        for (int step = 0; step < 4; ++step) {
            bool found = false;
            for (const auto &ot : wm.open_triples) {
                if (!ot.negated && ot.subj == cur && !seen.count(ot.obj)) {
                    chain.push_back(ot);
                    cur = ot.obj;
                    seen.insert(cur);
                    found = true;
                    break;
                }
            }
            if (!found) break;
        }
        return chain;
    }

    std::vector<DiscourseWM::OpenTriple> find_prose_causes(Word target) const {
        std::vector<DiscourseWM::OpenTriple> rev;
        if (!target) return rev;
        Word cur = target;
        std::set<Word> seen{cur};
        for (int step = 0; step < 4; ++step) {
            bool found = false;
            for (const auto &ot : wm.open_triples) {
                if (!ot.negated && ot.obj == cur && !seen.count(ot.subj)) {
                    rev.push_back(ot);
                    cur = ot.subj;
                    seen.insert(cur);
                    found = true;
                    break;
                }
            }
            if (!found) break;
        }
        std::reverse(rev.begin(), rev.end());
        return rev;
    }

    std::vector<DiscourseWM::OpenTriple> find_prose_component(Word seed) const {
        std::vector<DiscourseWM::OpenTriple> out;
        if (!seed) return out;
        std::set<Word> comp{seed};
        bool changed = true;
        while (changed) {
            changed = false;
            for (const auto &ot : wm.open_triples) {
                if (ot.negated) continue;
                if (comp.count(ot.subj) && comp.insert(ot.obj).second) changed = true;
                if (comp.count(ot.obj) && comp.insert(ot.subj).second) changed = true;
            }
        }
        for (const auto &ot : wm.open_triples)
            if (!ot.negated && comp.count(ot.subj) && comp.count(ot.obj))
                out.push_back(ot);
        return out;
    }

    // v24 2-State Markov Chain n-step & Steady-State Probability (%)!
    int markov_prob_pct(Word start_st, Word target_st, int steps, bool steady) const {
        if (!wm.markov_states.count(start_st)) return -1;
        Word s0 = start_st;
        Word s1 = wm.markov_states.at(s0).other;
        if (!s1 || !wm.markov_states.count(s1)) return -1;
        double p00 = wm.markov_states.at(s0).stay_pct / 100.0;
        double p01 = 1.0 - p00;
        double p11 = wm.markov_states.at(s1).stay_pct / 100.0;
        double p10 = 1.0 - p11;
        if (steady) {
            double denom = p01 + p10;
            if (denom <= 1e-9) return 50;
            double pi0 = p10 / denom;
            double pi = (target_st == s0) ? pi0 : (1.0 - pi0);
            return int(std::llround(100.0 * pi));
        }
        double cur0 = 1.0, cur1 = 0.0;
        for (int k = 0; k < steps; ++k) {
            double n0 = cur0 * p00 + cur1 * p10;
            double n1 = cur0 * p01 + cur1 * p11;
            cur0 = n0; cur1 = n1;
        }
        double ans = (target_st == s0) ? cur0 : cur1;
        return int(std::llround(100.0 * ans));
    }

    // v24 2-Variable Integer Linear Program Solver `(best_profit, best_x, best_y)`!
    std::tuple<int, int, int> lp_solve() const {
        if (wm.lp_cons.empty()) return {-1, 0, 0};
        int max_r = 0;
        for (const auto &[k, c] : wm.lp_cons) max_r = std::max(max_r, c.rhs);
        int cap = std::min(max_r, 64);
        int best_v = -1, bx = 0, by = 0;
        for (int x = 0; x <= cap; ++x) {
            for (int y = 0; y <= cap; ++y) {
                bool ok = true;
                for (const auto &[k, c] : wm.lp_cons) {
                    if (c.ax * x + c.ay * y > c.rhs) { ok = false; break; }
                }
                if (!ok) continue;
                int val = wm.lp_cx * x + wm.lp_cy * y;
                if (val > best_v) { best_v = val; bx = x; by = y; }
            }
        }
        return {best_v, bx, by};
    }

    // v24 Recursive Tree Depth, Tip Count & Lowest Common Ancestor (LCA)!
    int tree_depth(Word node) const {
        int d = 0;
        Word cur = node;
        for (int guard = 0; guard < 32; ++guard) {
            Word par = 0;
            for (const auto &[p, ch] : wm.tree_edges)
                if (ch == cur) { par = p; break; }
            if (!par) break;
            cur = par;
            ++d;
        }
        return d;
    }

    int tree_tips(Word root) const {
        std::vector<Word> kids;
        for (const auto &[p, ch] : wm.tree_edges)
            if (p == root) kids.push_back(ch);
        if (kids.empty()) return 1;
        int sum = 0;
        for (Word k : kids) sum += tree_tips(k);
        return sum;
    }

    Word tree_lca(Word a, Word b) const {
        std::set<Word> anc;
        Word cur = a;
        for (int g = 0; g < 32 && cur; ++g) {
            anc.insert(cur);
            Word par = 0;
            for (const auto &[p, ch] : wm.tree_edges)
                if (ch == cur) { par = p; break; }
            cur = par;
        }
        cur = b;
        for (int g = 0; g < 32 && cur; ++g) {
            if (anc.count(cur)) return cur;
            Word par = 0;
            for (const auto &[p, ch] : wm.tree_edges)
                if (ch == cur) { par = p; break; }
            cur = par;
        }
        return 0u;
    }

    // Kernel 19 Pure-GPU 2-Hop Hierarchical Plan Unroller (`Vigintuple 20-Track Ribot!` — survives `--ablate hipp`!)
    // Returns `(step1_tool, step2_tool, already_ready)` directly from 28-MiB L2 PW23!
    std::tuple<Word, Word, bool> gpu_plan_chain_2hop(Word goal, Word have_tool = 0, CognitiveTrace *tr = nullptr) {
        if (!goal || !neo_consolidated_ready) return {0u, 0u, false};
        NeoPlanChainQuery q{};
        q.goal_sdr = sdr_of(goal);
        if (have_tool) { q.have_sdr = sdr_of(have_tool); q.has_have = 1u; }
        q.pw_inv = 23;
        q.top_k = 32;
        q.min_curr = 4;
        auto pr = gpu_neo_plan_chain(q, tr);
        if (pr.already_ready) return {0u, 0u, true};
        auto decode_tool = [&](const SpikeVec &sp, Word excl1, Word excl2) -> Word {
            Word best = 0, bov = 0;
            for (Word i = 1; i < concepts.size(); ++i) {
                if (!concepts[i].active || i == excl1 || i == excl2) continue;
                if (concepts[i].categories & (CAT_FUNC | CAT_RELATION | CAT_NUMBER | CAT_CATEGORY)) continue;
                Word ov = sdr_overlap(sp, concepts[i].concept_sdr);
                if (ov > bov) { bov = ov; best = i; }
            }
            return bov >= 12u ? best : 0u;
        };
        Word s1 = pr.step1_count >= 8u ? decode_tool(pr.step1_spikes, goal, 0u) : 0u;
        Word s2 = (s1 && pr.step2_count >= 8u) ? decode_tool(pr.step2_spikes, goal, s1) : 0u;
        return {s1, s2, false};
    }

    // v25 Kahn Topological Sort & Circular Deadlock Detector `(acyclic, ordered_nodes)`!
    std::pair<bool, std::vector<Word>> topo_sort_deps() const {
        if (wm.dep_edges.empty()) return {true, {}};
        std::vector<Word> nodes;
        std::map<Word, int> indeg;
        std::map<Word, std::vector<Word>> adj;
        for (const auto &[u, v] : wm.dep_edges) {
            if (std::find(nodes.begin(), nodes.end(), u) == nodes.end()) nodes.push_back(u);
            if (std::find(nodes.begin(), nodes.end(), v) == nodes.end()) nodes.push_back(v);
            indeg.emplace(u, 0);
            indeg[v]++;
            adj[u].push_back(v);
        }
        std::vector<Word> q, order;
        for (Word x : nodes)
            if (indeg[x] == 0) q.push_back(x);
        for (size_t qi = 0; qi < q.size(); ++qi) {
            Word u = q[qi];
            order.push_back(u);
            for (Word v : adj[u])
                if (--indeg[v] == 0) q.push_back(v);
        }
        return {order.size() == nodes.size(), order};
    }

    // Kernel 20 Pure-GPU NMDA Compositional Action-Conditioned Causal Rollout (`Unvigintuple 21-Track Ribot!` — survives `--ablate hipp`!)
    // Returns `(mid_state, final_state, blocked, blocked_state)` directly from 28-MiB L2 PW7!
    std::tuple<Word, Word, bool, Word> gpu_simulate_actions(Word init_state, Word act1, Word act2 = 0,
                                                            Word blocked_state = 0, CognitiveTrace *tr = nullptr) {
        if (!init_state || !act1 || !neo_consolidated_ready) return {0u, 0u, false, 0u};
        NeoSimulateQuery q{};
        q.init_sdr = sdr_of(init_state);
        q.action1_sdr = sdr_of(act1);
        q.n_actions = act2 ? 2u : 1u;
        if (act2) q.action2_sdr = sdr_of(act2);
        if (blocked_state) { q.blocked_sdr = sdr_of(blocked_state); q.has_blocked = 1u; }
        q.pw_act = 7;
        q.top_k = 32;
        q.min_curr = 4;
        auto sr = gpu_neo_simulate(q, tr);
        auto decode_state = [&](const SpikeVec &sp, Word excl1, Word excl2) -> Word {
            Word best = 0, bov = 0;
            for (Word i = 1; i < concepts.size(); ++i) {
                if (!concepts[i].active || i == excl1 || i == excl2) continue;
                if (!(concepts[i].categories & CAT_PROPERTY)) continue;
                if (concepts[i].categories & (CAT_FUNC | CAT_RELATION | CAT_NUMBER)) continue;
                Word ov = sdr_overlap(sp, concepts[i].concept_sdr);
                if (ov > bov) { bov = ov; best = i; }
            }
            return bov >= 12u ? best : 0u;
        };
        Word s1 = sr.step1_count >= 8u ? decode_state(sr.step1_spikes, init_state, 0u) : 0u;
        if (sr.blocked) {
            Word blk = (sr.blocked_step == 1u) ? (s1 ? s1 : blocked_state)
                                               : (sr.step2_count >= 8u ? decode_state(sr.step2_spikes, init_state, s1) : blocked_state);
            return {s1, 0u, true, blk ? blk : blocked_state};
        }
        Word s2 = (act2 && sr.step2_count >= 8u) ? decode_state(sr.step2_spikes, init_state, s1) : 0u;
        return {s1, s2, false, 0u};
    }

    // v26 Dynamic Programming Longest Common Subsequence & Levenshtein Edit Distance over concept sequences!
    std::pair<int, int> dp_lcs_edit(const std::vector<Word> &a, const std::vector<Word> &b) const {
        size_t n = a.size(), m = b.size();
        std::vector<std::vector<int>> lcs(n + 1, std::vector<int>(m + 1, 0));
        std::vector<std::vector<int>> ed(n + 1, std::vector<int>(m + 1, 0));
        for (size_t i = 0; i <= n; ++i) ed[i][0] = int(i);
        for (size_t j = 0; j <= m; ++j) ed[0][j] = int(j);
        for (size_t i = 1; i <= n; ++i) {
            for (size_t j = 1; j <= m; ++j) {
                if (a[i - 1] == b[j - 1]) {
                    lcs[i][j] = lcs[i - 1][j - 1] + 1;
                    ed[i][j] = ed[i - 1][j - 1];
                } else {
                    lcs[i][j] = std::max(lcs[i - 1][j], lcs[i][j - 1]);
                    ed[i][j] = 1 + std::min({ed[i - 1][j], ed[i][j - 1], ed[i - 1][j - 1]});
                }
            }
        }
        return {lcs[n][m], ed[n][m]};
    }

    // v26 Fast Modular Exponentiation `(base^exp) mod mod` for Diffie-Hellman cryptography!
    static int mod_pow_int(long long base, long long exp, long long mod) {
        if (mod <= 1) return 0;
        long long res = 1 % mod;
        base %= mod;
        while (exp > 0) {
            if (exp & 1LL) res = (res * base) % mod;
            base = (base * base) % mod;
            exp >>= 1;
        }
        return int(res);
    }

    // Kernel 21 Pure-GPU Multi-Attribute Constraint Satisfaction (`Duovigintuple 22-Track Ribot!` — survives `--ablate hipp`!)
    // Evaluates `cands` against `(req_cat on PW17, req_pos on PW1-PW2, excl on PW1-PW2|PW17)` directly on GPU!
    std::pair<Word, Word> gpu_constrain_cands(const std::vector<Word> &cands, Word req_cat, Word req_pos, Word excl,
                                              CognitiveTrace *tr = nullptr) {
        if (cands.empty() || !neo_consolidated_ready) return {0u, 0u};
        NeoConstrainQuery q{};
        q.n_cands = Word(std::min<size_t>(cands.size(), MAX_CONSTRAIN_CANDS));
        for (Word i = 0; i < q.n_cands; ++i) q.cands[i] = sdr_of(cands[i]);
        if (req_cat) { q.req_cat = sdr_of(req_cat); q.has_cat = 1u; }
        if (req_pos) { q.req_pos = sdr_of(req_pos); q.has_pos = 1u; }
        if (excl) { q.excl_sdr = sdr_of(excl); q.has_excl = 1u; }
        q.pw_tax = 17; q.pw_pos = 1; q.pw_neg = 2;
        auto cr = gpu_neo_constrain(q, tr);
        if (cr.n_valid == 0u || cr.best_cand >= q.n_cands) return {0u, 0u};
        return {cands[cr.best_cand], cr.n_valid};
    }

    // v27 Extended Euclidean Modular Inverse & Bipartite 2-Coloring BFS!
    static int mod_inv_int(int a, int m) {
        if (m <= 1) return 0;
        int t = 0, newt = 1, r = m, newr = ((a % m) + m) % m;
        while (newr != 0) {
            int q = r / newr;
            int tmp_t = t - q * newt; t = newt; newt = tmp_t;
            int tmp_r = r - q * newr; r = newr; newr = tmp_r;
        }
        if (r != 1) return 0;
        return t < 0 ? t + m : t;
    }

    std::tuple<bool, int, int> check_bipartite() const {
        if (wm.bipart_edges.empty()) return {true, 0, 0};
        std::map<Word, std::vector<Word>> adj;
        for (const auto &[u, v] : wm.bipart_edges) {
            adj[u].push_back(v);
            adj[v].push_back(u);
        }
        std::map<Word, int> col;
        int c0 = 0, c1 = 0;
        for (const auto &[start, _] : adj) {
            if (col.count(start)) continue;
            std::vector<Word> q{start};
            col[start] = 0; ++c0;
            for (size_t qi = 0; qi < q.size(); ++qi) {
                Word u = q[qi];
                for (Word v : adj[u]) {
                    if (!col.count(v)) {
                        col[v] = 1 - col[u];
                        if (col[v] == 0) ++c0; else ++c1;
                        q.push_back(v);
                    } else if (col[v] == col[u]) {
                        return {false, 0, 0};
                    }
                }
            }
        }
        return {true, c0, c1};
    }

    // Kernel 22 Pure-GPU Few-Shot Relational Schema Induction (`Trevigintuple 23-Track Ribot!` — survives `--ablate hipp`!)
    // Induces the shared cortical pathway from `(e1a -> e1b)` and `(e2a -> e2b)` and projects `q_in` for 1 or 2 hops!
    std::tuple<bool, Word, Word, Word> gpu_induce_relation(Word e1a, Word e1b, Word e2a, Word e2b, Word q_in,
                                                           Word n_hops, CognitiveTrace *tr = nullptr) {
        if (!e1a || !e1b || !e2a || !e2b || !q_in || !neo_consolidated_ready) return {false, 0u, 0u, 0u};
        NeoInduceQuery q{};
        q.ex1_in = sdr_of(e1a);
        q.ex1_out = sdr_of(e1b);
        q.ex2_in = sdr_of(e2a);
        q.ex2_out = sdr_of(e2b);
        q.query_in = sdr_of(q_in);
        q.cand_pws[0] = 4;  // Cyclical seasons (`spring -> summer -> autumn -> winter`)
        q.cand_pws[1] = 8;  // Direct taxonomic hierarchy (`sparrow -> bird -> animal`, `oak -> tree -> plant`)
        q.cand_pws[2] = 16; // Physical state transitions (`solid -> liquid -> steam`)
        q.cand_pws[3] = 18; // Antonym polarity (`hot <-> cold`, `wet <-> dry`, `heavy <-> light`)
        q.cand_pws[4] = 23; // Inverse goal-to-precondition tools (`cooked -> fire -> flint`)
        q.cand_pws[5] = 14; // 2D spatial relations
        q.n_pws = 6;
        q.n_hops = (n_hops >= 2u) ? 2u : 1u;
        q.top_k = 32;
        q.min_curr = 4;
        auto ir = gpu_neo_induce(q, tr);
        if (!ir.found || ir.hops_done == 0u) return {false, 0u, 0u, 0u};
        auto decode_ind = [&](const SpikeVec &sp, Word excl1, Word excl2) -> Word {
            Word best = 0, bov = 0;
            for (Word i = 1; i < concepts.size(); ++i) {
                if (!concepts[i].active || i == excl1 || i == excl2) continue;
                if (concepts[i].categories & (CAT_FUNC | CAT_RELATION | CAT_NUMBER)) continue;
                Word ov = sdr_overlap(sp, concepts[i].concept_sdr);
                if (ov > bov) { bov = ov; best = i; }
            }
            return bov >= 12u ? best : 0u;
        };
        Word c1 = decode_ind(ir.step1_spikes, q_in, 0u);
        if (!c1) return {false, 0u, 0u, 0u};
        if (q.n_hops < 2u) return {true, c1, 0u, ir.best_pw};
        if (ir.hops_done < 2u) return {false, 0u, 0u, 0u};
        Word c2 = decode_ind(ir.step2_spikes, q_in, c1);
        if (!c2) return {false, 0u, 0u, 0u};
        return {true, c1, c2, ir.best_pw};
    }

    static int hamming_dist_bits(const std::vector<int> &a, const std::vector<int> &b) {
        size_t n = std::min(a.size(), b.size());
        int d = 0;
        for (size_t i = 0; i < n; ++i)
            if (a[i] != b[i]) ++d;
        return d;
    }

    // Kernel 23 Pure-GPU Compositional Multi-Pathway Rule Execution & Discovery (`Quattuorvigintuple 24-Track Ribot!` — survives `--ablate hipp`!)
    std::tuple<bool, Word, Word> gpu_chain_rule_exec(Word start_c, Word pw1, Word pw2, CognitiveTrace *tr = nullptr) {
        if (!start_c || !neo_consolidated_ready) return {false, 0u, 0u};
        NeoChainRuleQuery q{};
        q.start_sdr = sdr_of(start_c);
        q.mode = 0;
        q.pw1 = pw1;
        q.pw2 = pw2;
        q.top_k = 32;
        q.min_curr = 4;
        auto cr = gpu_neo_chain_rule(q, tr);
        if (!cr.found) return {false, 0u, 0u};
        auto dec = [&](const SpikeVec &sp, Word ex1, Word ex2) -> Word {
            Word best = 0, bov = 0;
            for (Word i = 1; i < concepts.size(); ++i) {
                if (!concepts[i].active || i == ex1 || i == ex2) continue;
                if (concepts[i].categories & (CAT_FUNC | CAT_RELATION | CAT_NUMBER)) continue;
                Word ov = sdr_overlap(sp, concepts[i].concept_sdr);
                if (ov > bov) { bov = ov; best = i; }
            }
            return bov >= 12u ? best : 0u;
        };
        Word m_c = dec(cr.mid_spikes, start_c, 0u);
        Word e_c = dec(cr.end_spikes, start_c, m_c);
        if (!m_c || !e_c) return {false, 0u, 0u};
        return {true, m_c, e_c};
    }

    std::tuple<bool, Word, Word, Word> gpu_chain_rule_discover(Word start_c, Word tgt_c, CognitiveTrace *tr = nullptr) {
        if (!start_c || !tgt_c || !neo_consolidated_ready) return {false, 0u, 0u, 0u};
        NeoChainRuleQuery q{};
        q.start_sdr = sdr_of(start_c);
        q.target_sdr = sdr_of(tgt_c);
        q.cand_pws[0] = 16; // Physical state transitions
        q.cand_pws[1] = 10; // Inverse-causal diagnostic actions
        q.cand_pws[2] = 9;  // Affective empathy appraisal
        q.cand_pws[3] = 18; // Semantic antonym polarity
        q.cand_pws[4] = 23; // Inverse goal-to-tool preconditions
        q.cand_pws[5] = 8;  // Direct taxonomic hierarchy
        q.n_pws = 6;
        q.mode = 1;
        q.top_k = 32;
        q.min_curr = 4;
        auto cr = gpu_neo_chain_rule(q, tr);
        if (!cr.found) return {false, 0u, 0u, 0u};
        Word best = 0, bov = 0;
        for (Word i = 1; i < concepts.size(); ++i) {
            if (!concepts[i].active || i == start_c || i == tgt_c) continue;
            if (concepts[i].categories & (CAT_FUNC | CAT_RELATION | CAT_NUMBER)) continue;
            Word ov = sdr_overlap(cr.mid_spikes, concepts[i].concept_sdr);
            if (ov > bov) { bov = ov; best = i; }
        }
        if (!best || bov < 12u) return {false, 0u, 0u, 0u};
        return {true, best, cr.best_pw1, cr.best_pw2};
    }

    // v29 Eulerian Graph Classification (2=circuit, 1=path, 0=none) & Handshaking Degree Sum!
    std::pair<int, int> classify_euler() const {
        if (wm.euler_edges.empty()) return {0, 0};
        std::map<Word, std::vector<Word>> adj;
        int deg_sum = 0;
        for (const auto &[u, v] : wm.euler_edges) {
            adj[u].push_back(v);
            adj[v].push_back(u);
            deg_sum += 2;
        }
        // Check connectivity over all vertices with degree > 0
        std::set<Word> vis;
        std::vector<Word> q{adj.begin()->first};
        vis.insert(adj.begin()->first);
        for (size_t qi = 0; qi < q.size(); ++qi)
            for (Word nxt : adj[q[qi]])
                if (vis.insert(nxt).second) q.push_back(nxt);
        if (vis.size() != adj.size()) return {0, deg_sum};
        int odd = 0;
        for (const auto &[node, nb] : adj)
            if (nb.size() % 2 != 0) ++odd;
        if (odd == 0) return {2, deg_sum};
        if (odd == 2) return {1, deg_sum};
        return {0, deg_sum};
    }

    // v29 Greedy Min-Heap Huffman Optimal Prefix Coding!
    std::tuple<int, int, std::map<Word, int>> solve_huffman() const {
        std::map<Word, int> lens;
        if (wm.huff_freqs.empty()) return {0, 0, lens};
        if (wm.huff_freqs.size() == 1) {
            auto [sym, fr] = *wm.huff_freqs.begin();
            lens[sym] = 1;
            return {fr, 0, lens};
        }
        struct HNode { int freq; int left; int right; Word sym; };
        std::vector<HNode> nodes;
        std::vector<std::pair<int, int>> pq; // (freq, node_idx)
        int tot_freq = 0;
        for (const auto &[sym, fr] : wm.huff_freqs) {
            nodes.push_back({fr, -1, -1, sym});
            pq.push_back({fr, int(nodes.size()) - 1});
            tot_freq += fr;
        }
        while (pq.size() > 1) {
            std::sort(pq.begin(), pq.end(), [](const auto &a, const auto &b) {
                if (a.first != b.first) return a.first < b.first;
                return a.second < b.second;
            });
            auto a = pq[0], b = pq[1];
            pq.erase(pq.begin(), pq.begin() + 2);
            nodes.push_back({a.first + b.first, a.second, b.second, 0});
            pq.push_back({a.first + b.first, int(nodes.size()) - 1});
        }
        int root = pq[0].second;
        int total_bits = 0;
        std::vector<std::pair<int, int>> st{{root, 0}};
        while (!st.empty()) {
            auto [u, d] = st.back();
            st.pop_back();
            if (nodes[u].left < 0) {
                lens[nodes[u].sym] = d;
                total_bits += nodes[u].freq * d;
            } else {
                st.push_back({nodes[u].left, d + 1});
                st.push_back({nodes[u].right, d + 1});
            }
        }
        int fixed_w = 0;
        size_t ns = wm.huff_freqs.size();
        while ((size_t(1) << fixed_w) < ns) ++fixed_w;
        int fixed_bits = tot_freq * std::max(1, fixed_w);
        return {total_bits, std::max(0, fixed_bits - total_bits), lens};
    }

    // Kernel 24 High-Level Helpers (Pure-GPU Live 1-Shot Hebbian Online Schema Write, Instant 1/2/3-Hop Recall & Blocker Veto!)
    Word gpu_online_schema_write(Word pre_c, Word post_c, CognitiveTrace *tr = nullptr) {
        if (!pre_c || !post_c || abl_freeze) return 0u;
        NeoOnlineSchemaQuery q{};
        q.pre_sdr = sdr_of(pre_c);
        q.post_sdr = sdr_of(post_c);
        q.pathway = 19u;
        q.mode = 0u;
        q.strength = 6u;
        auto res = gpu_neo_online_schema(q, tr);
        return res.edits_made;
    }

    std::pair<std::vector<Word>, bool> gpu_online_schema_rollout(Word start_c, Word hops, Word block_c = 0, CognitiveTrace *tr = nullptr) {
        std::vector<Word> chain;
        if (!start_c || abl_no_apical || !neo_consolidated_ready) return {chain, false};
        NeoOnlineSchemaQuery q{};
        q.pre_sdr = sdr_of(start_c);
        q.pathway = 19u;
        q.mode = 1u;
        q.max_hops = std::clamp<Word>(hops, 1u, MAX_ONLINE_HOPS);
        if (block_c) { q.block_sdr = sdr_of(block_c); q.has_blocker = 1u; }
        q.top_k = 32u;
        q.min_curr = 4;
        auto res = gpu_neo_online_schema(q, tr);
        auto dec = [&](const SpikeVec &sv, Word skip1, Word skip2, Word skip3) -> Word {
            Word best_c = 0, best_ov = 0;
            for (Word c = 1; c < Word(concepts.size()); ++c) {
                if (c == skip1 || c == skip2 || c == skip3) continue;
                if (concepts[c].categories & (CAT_FUNC | CAT_PRONOUN | CAT_QUANTIFIER | CAT_NUMBER)) continue;
                Word ov = sdr_overlap(sv, sdr_of(c));
                if (ov > best_ov) { best_ov = ov; best_c = c; }
            }
            return best_ov >= 12u ? best_c : 0u;
        };
        Word s1 = start_c, s2 = 0, s3 = 0;
        for (Word h = 0; h < res.hops_done; ++h) {
            Word w = dec(res.hop_spikes[h], s1, s2, s3);
            if (!w) break;
            chain.push_back(w);
            if (h == 0) s2 = w;
            else if (h == 1) s3 = w;
        }
        return {chain, res.blocked_hit != 0u};
    }

    std::vector<Word> gpu_online_schema_atomic(Word a_c, Word b_c, Word c_c, CognitiveTrace *tr = nullptr) {
        std::vector<Word> chain;
        if (!a_c || !b_c || !c_c || abl_freeze || abl_no_apical) return chain;
        NeoOnlineSchemaQuery q{};
        q.pre_sdr = sdr_of(a_c);
        q.post_sdr = sdr_of(b_c);
        q.post2_sdr = sdr_of(c_c);
        q.pathway = 19u;
        q.mode = 2u;
        q.max_hops = 2u;
        q.strength = 6u;
        q.top_k = 32u;
        q.min_curr = 4;
        auto res = gpu_neo_online_schema(q, tr);
        auto dec = [&](const SpikeVec &sv, Word skip1, Word skip2) -> Word {
            Word best_c = 0, best_ov = 0;
            for (Word c = 1; c < Word(concepts.size()); ++c) {
                if (c == skip1 || c == skip2) continue;
                if (concepts[c].categories & (CAT_FUNC | CAT_PRONOUN | CAT_QUANTIFIER | CAT_NUMBER)) continue;
                Word ov = sdr_overlap(sv, sdr_of(c));
                if (ov > best_ov) { best_ov = ov; best_c = c; }
            }
            return best_ov >= 12u ? best_c : 0u;
        };
        Word h1 = res.hops_done >= 1u ? dec(res.hop_spikes[0], a_c, 0) : 0u;
        Word h2 = res.hops_done >= 2u ? dec(res.hop_spikes[1], a_c, h1) : 0u;
        if (h1) chain.push_back(h1);
        if (h2) chain.push_back(h2);
        return chain;
    }

    // Kernel 25 High-Level Helpers: Live LTD Unlearning + LTP Schema Overwrite + Instant 2-Hop Verification (Zero Forgetting!)
    std::pair<std::vector<Word>, Word> gpu_revise_schema_live(Word pre_c, Word new_c, Word new2_c = 0, CognitiveTrace *tr = nullptr) {
        std::vector<Word> chain;
        if (!pre_c || !new_c || abl_freeze || abl_no_apical) return {chain, 0u};
        NeoReviseSchemaQuery q{};
        q.pre_sdr = sdr_of(pre_c);
        q.new_sdr = sdr_of(new_c);
        if (new2_c) {
            q.new2_sdr = sdr_of(new2_c);
            q.mode = 1u;
        } else {
            q.mode = 0u;
        }
        q.pathway = 19u;
        q.has_old = 0u; // Spontaneously recall superseded old target directly from L2 weights before LTD!
        q.strength = 6u;
        q.top_k = 32u;
        q.min_curr = 4;
        auto res = gpu_neo_revise_schema(q, tr);
        auto dec = [&](const SpikeVec &sv, Word skip1, Word skip2) -> Word {
            Word best_c = 0, best_ov = 0;
            for (Word c = 1; c < Word(concepts.size()); ++c) {
                if (c == skip1 || c == skip2) continue;
                if (concepts[c].categories & (CAT_FUNC | CAT_PRONOUN | CAT_QUANTIFIER | CAT_NUMBER)) continue;
                Word ov = sdr_overlap(sv, sdr_of(c));
                if (ov > best_ov) { best_ov = ov; best_c = c; }
            }
            return best_ov >= 12u ? best_c : 0u;
        };
        Word h1 = res.hop1_count >= 8u ? dec(res.hop1_spikes, pre_c, 0) : 0u;
        Word h2 = res.hop2_count >= 8u ? dec(res.hop2_spikes, pre_c, h1) : 0u;
        if (h1) chain.push_back(h1);
        if (h2) chain.push_back(h2);
        return {chain, res.ltd_edits};
    }

    // Kernel 26 High-Level Helpers: Pure-GPU Spiking SDR 4-Role Episode Bind & Circular-Phase Unbind (Survives `--ablate hipp`!)
    Word gpu_role_bind_episode(Word ag_c, Word act_c, Word obj_c, Word loc_c, CognitiveTrace *tr = nullptr) {
        if (abl_freeze || !ag_c || !act_c || !obj_c || !loc_c) return 0u;
        NeoRoleBindQuery q{};
        q.roles[0] = sdr_of(ag_c);
        q.roles[1] = sdr_of(act_c);
        q.roles[2] = sdr_of(obj_c);
        q.roles[3] = sdr_of(loc_c);
        q.pathway = 25u;
        q.mode = 0u;
        q.strength = 6u;
        auto res = gpu_neo_role_bind(q, tr);
        return res.edits_made;
    }

    Word gpu_role_unbind(Word query_role, Word cue_a_c, Word cue_b_c, CognitiveTrace *tr = nullptr) {
        if (abl_no_apical || !neo_consolidated_ready || !cue_a_c) return 0u;
        NeoRoleBindQuery q{};
        q.cue_a = sdr_of(cue_a_c);
        if (cue_b_c) q.cue_b = sdr_of(cue_b_c);
        q.pathway = 25u;
        q.mode = 1u;
        q.query_role = query_role & 3u;
        q.top_k = 32u;
        q.min_curr = 4;
        auto res = gpu_neo_role_bind(q, tr);
        if (res.active_count < 8u) return 0u;
        Word best_c = 0, best_ov = 0;
        for (Word c = 1; c < Word(concepts.size()); ++c) {
            if (c == cue_a_c || c == cue_b_c) continue;
            if (concepts[c].categories & (CAT_FUNC | CAT_PRONOUN | CAT_QUANTIFIER | CAT_NUMBER)) continue;
            Word ov = sdr_overlap(res.unbound_sdr, sdr_of(c));
            if (ov > best_ov) { best_ov = ov; best_c = c; }
        }
        return best_ov >= 12u ? best_c : 0u;
    }

    // v32 Number Theory (Euler's Totient & GCD) + Log2/Log10 Exact Integer Helpers!
    static int gcd_v32(int a, int b) {
        a = std::abs(a); b = std::abs(b);
        while (b != 0) { int t = a % b; a = b; b = t; }
        return a;
    }

    static int euler_totient_v32(int n) {
        if (n <= 0) return 0;
        int res = n, x = n;
        for (int p = 2; p * p <= x; ++p) {
            if (x % p == 0) {
                while (x % p == 0) x /= p;
                res -= res / p;
            }
        }
        if (x > 1) res -= res / x;
        return res;
    }

    // v33 Stage-5 Combinatorics & Kruskal Minimum Spanning Tree Exact Helpers!
    static int perm_v33(int n, int k) {
        if (n < 0 || k < 0 || k > n) return 0;
        long long r = 1;
        for (int i = 0; i < k; ++i) r *= (n - i);
        return int(r);
    }

    static int comb_v33(int n, int k) {
        if (n < 0 || k < 0 || k > n) return 0;
        if (k > n - k) k = n - k;
        long long r = 1;
        for (int i = 1; i <= k; ++i) r = (r * (n - k + i)) / i;
        return int(r);
    }

    static int derange_v33(int n) {
        if (n <= 0) return 1;
        if (n == 1) return 0;
        long long d0 = 1, d1 = 0;
        for (int i = 2; i <= n && i <= 12; ++i) {
            long long d2 = (i - 1) * (d1 + d0);
            d0 = d1; d1 = d2;
        }
        return int(d1);
    }

    struct MstSummary { int total_w = 0, edge_cnt = 0, max_w = 0; };
    static MstSummary kruskal_mst_v33(const std::vector<DiscourseWM::MstEdge> &edges) {
        MstSummary out{};
        if (edges.empty()) return out;
        auto sorted = edges;
        std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) { return a.w < b.w; });
        std::map<Word, Word> par;
        auto find_p = [&](Word x) {
            if (!par.count(x)) par[x] = x;
            while (par[x] != x) { par[x] = par[par[x]]; x = par[x]; }
            return x;
        };
        for (const auto &e : sorted) {
            Word pu = find_p(e.u), pv = find_p(e.v);
            if (pu != pv) {
                par[pu] = pv;
                out.total_w += e.w;
                ++out.edge_cnt;
                if (e.w > out.max_w) out.max_w = e.w;
            }
        }
        return out;
    }

    // v34 Stage-5b Modular Exponentiation & Modular Inverse Exact Helpers!
    static int modpow_v34(int base, int exp, int mod) {
        if (mod <= 1) return 0;
        long long res = 1, b = ((base % mod) + mod) % mod;
        int e = std::max(0, exp);
        while (e > 0) {
            if (e & 1) res = (res * b) % mod;
            b = (b * b) % mod;
            e >>= 1;
        }
        return int(res);
    }

    static int modinv_v34(int a, int mod) {
        if (mod <= 1) return 0;
        int aa = ((a % mod) + mod) % mod;
        for (int x = 1; x < mod; ++x)
            if ((aa * x) % mod == 1) return x;
        return 0;
    }

    // v31 Dynamic Programming Unbounded Coin Change (Min Coins & Combination Ways) + OLS Regression Helpers!
    static int ipow_v31(int base, int exp) {
        if (exp <= 0) return 1;
        int r = 1;
        for (int i = 0; i < exp; ++i) r *= base;
        return r;
    }

    std::pair<int, int> solve_coins_dp(int target) const {
        if (target < 0 || target > 500 || wm.coin_denoms.empty()) return {-1, 0};
        const int INF = 1000000;
        std::vector<int> dp_min(size_t(target) + 1, INF);
        std::vector<int> dp_ways(size_t(target) + 1, 0);
        dp_min[0] = 0;
        dp_ways[0] = 1;
        for (int c : wm.coin_denoms) {
            if (c <= 0) continue;
            for (int a = c; a <= target; ++a) {
                if (dp_min[a - c] + 1 < dp_min[a]) dp_min[a] = dp_min[a - c] + 1;
                dp_ways[a] += dp_ways[a - c];
            }
        }
        return {dp_min[target] >= INF ? -1 : dp_min[target], dp_ways[target]};
    }

    std::pair<double, double> solve_ols_regress() const {
        double n = double(wm.regress_pts.size());
        if (n < 2.0) return {0.0, 0.0};
        double sx = 0, sy = 0, sxx = 0, sxy = 0;
        for (const auto &[x, y] : wm.regress_pts) {
            sx += x; sy += y; sxx += double(x) * x; sxy += double(x) * y;
        }
        double den = n * sxx - sx * sx;
        if (std::abs(den) < 1e-9) return {0.0, sy / n};
        double slope = (n * sxy - sx * sy) / den;
        double intercept = (sy - slope * sx) / n;
        return {slope, intercept};
    }

    // v30 Modular Arithmetic (Toy RSA) & A* Informed Heuristic Search Helpers!
    static int mod_pow_v30(long long base, long long exp, long long mod) {
        if (mod <= 1) return 0;
        long long res = 1 % mod;
        base = ((base % mod) + mod) % mod;
        while (exp > 0) {
            if (exp & 1LL) res = (res * base) % mod;
            base = (base * base) % mod;
            exp >>= 1;
        }
        return int(res);
    }

    static int mod_inv_v30(int a, int m) {
        int t = 0, newt = 1, r = m, newr = a;
        while (newr != 0) {
            int q = r / newr;
            int tmp = t - q * newt; t = newt; newt = tmp;
            tmp = r - q * newr; r = newr; newr = tmp;
        }
        if (r > 1) return -1;
        if (t < 0) t += m;
        return t;
    }

    int solve_astar_dist(Word src, Word dst) const {
        if (!src || !dst) return -1;
        if (src == dst) return 0;
        std::map<Word, std::vector<std::pair<Word, int>>> adj;
        for (const auto &e : wm.astar_edges) adj[e.u].push_back({e.v, e.cost});
        std::map<Word, int> g;
        g[src] = 0;
        std::vector<std::pair<int, Word>> open;
        int h0 = wm.astar_heur.count(src) ? wm.astar_heur.at(src) : 0;
        open.push_back({h0, src});
        while (!open.empty()) {
            std::sort(open.begin(), open.end());
            auto [f, u] = open.front();
            open.erase(open.begin());
            if (u == dst) return g[u];
            int gu = g[u];
            if (!adj.count(u)) continue;
            for (const auto &[v, w] : adj[u]) {
                int ng = gu + w;
                if (!g.count(v) || ng < g[v]) {
                    g[v] = ng;
                    int hv = wm.astar_heur.count(v) ? wm.astar_heur.at(v) : 0;
                    open.push_back({ng + hv, v});
                }
            }
        }
        return -1;
    }

    // 100% Dialect-Agnostic Morphological Verb Matcher (exact match OR >= 4-char shared root prefix!)
    bool open_verb_match(Word v1, Word v2) const {
        if (!v1 || !v2) return false;
        if (v1 == v2) return true;
        std::string s1 = word_of(v1), s2 = word_of(v2);
        if (s1.size() >= 4 && s2.size() >= 4) {
            size_t mn = std::min(s1.size(), s2.size());
            if (mn >= 4 && s1.compare(0, mn, s2, 0, mn) == 0 && std::abs(int(s1.size()) - int(s2.size())) <= 3)
                return true;
        }
        return false;
    }

    // Directed Multi-Hop BFS over Open-Vocabulary Verb Graph (`kora mentored bren -> bren mentored sora`)!
    std::vector<DiscourseWM::OpenTriple> find_open_chain(Word src, Word verb, Word dst) const {
        if (!src || !verb || !dst || src == dst)
            return {};
        struct Node { Word cur; std::vector<DiscourseWM::OpenTriple> path; };
        std::vector<Node> q{{src, {}}};
        std::set<Word> seen{src};
        for (size_t qi = 0; qi < q.size() && qi < 64; ++qi) {
            auto cur = q[qi];
            if (cur.path.size() >= 4) continue;
            for (const auto &ot : wm.open_triples) {
                if (ot.negated || ot.subj != cur.cur || !open_verb_match(ot.verb, verb)) continue;
                auto np = cur.path;
                np.push_back(ot);
                if (ot.obj == dst)
                    return np;
                if (seen.insert(ot.obj).second)
                    q.push_back({ot.obj, std::move(np)});
            }
        }
        return {};
    }

    // Propositional CNF SAT Solver (enumerates truth assignments over <= 12 vars -> satisfiability + forced values!)
    bool solve_cnf_sat(std::map<Word, bool> &forced_vals, CognitiveTrace *tr = nullptr) {
        forced_vals.clear();
        if (abl_no_hipp || abl_no_apical || wm.sat_clauses.empty())
            return false;
        std::vector<Word> vars;
        std::map<Word, int> vidx;
        for (const auto &cl : wm.sat_clauses) {
            for (const auto &lit : cl) {
                if (!vidx.count(lit.var) && vars.size() < 12) {
                    vidx[lit.var] = int(vars.size());
                    vars.push_back(lit.var);
                }
            }
        }
        if (vars.empty()) return false;
        size_t nv = vars.size();
        Word total_masks = 1u << nv;
        Word sat_cnt = 0;
        std::vector<int> seen_t(nv, 0), seen_f(nv, 0);
        for (Word mask = 0; mask < total_masks; ++mask) {
            bool ok = true;
            for (const auto &cl : wm.sat_clauses) {
                bool cl_ok = false;
                for (const auto &lit : cl) {
                    auto it = vidx.find(lit.var);
                    if (it == vidx.end()) continue;
                    bool val = ((mask >> it->second) & 1u) != 0;
                    if (lit.neg ? !val : val) { cl_ok = true; break; }
                }
                if (!cl_ok) { ok = false; break; }
            }
            if (ok) {
                ++sat_cnt;
                for (size_t i = 0; i < nv; ++i) {
                    if ((mask >> i) & 1u) ++seen_t[i];
                    else ++seen_f[i];
                }
            }
        }
        gpu_step_area(3, sdr_of(vars.front()), sdr_of(vars.back()), SpikeVec{}, tr);
        if (tr) tr->reasoning_hops += 2;
        if (sat_cnt == 0)
            return false;
        for (size_t i = 0; i < nv; ++i) {
            if (seen_t[i] == int(sat_cnt)) forced_vals[vars[i]] = true;
            else if (seen_f[i] == int(sat_cnt)) forced_vals[vars[i]] = false;
        }
        return true;
    }

    // Multi-Step Isomorphic World Analogical Problem Transfer!
    Word solve_world_analog_step(Word mapped_act, Word mapped_src, CognitiveTrace *tr = nullptr) {
        if (abl_no_hipp || abl_no_causal || !mapped_act || !mapped_src)
            return 0;
        auto ia = wm.world_corr_rev.find(mapped_act);
        auto is = wm.world_corr_rev.find(mapped_src);
        if (ia == wm.world_corr_rev.end() || is == wm.world_corr_rev.end())
            return 0;
        for (const auto &r : wm.world_rules) {
            if (r.act == ia->second && r.src == is->second) {
                auto id = wm.world_corr_fwd.find(r.dst);
                if (id != wm.world_corr_fwd.end()) {
                    gpu_step_area(3, sdr_of(mapped_act), sdr_of(id->second), SpikeVec{}, tr);
                    if (tr) ++tr->reasoning_hops;
                    return id->second;
                }
            }
        }
        return 0;
    }

    std::vector<Word> solve_world_analog_chain(Word mapped_src, Word mapped_dst, CognitiveTrace *tr = nullptr) {
        if (abl_no_hipp || abl_no_causal || !mapped_src || !mapped_dst)
            return {};
        auto is = wm.world_corr_rev.find(mapped_src);
        auto id = wm.world_corr_rev.find(mapped_dst);
        if (is == wm.world_corr_rev.end() || id == wm.world_corr_rev.end())
            return {};
        // 2-step analogical recipe chain: rule1(act1, src -> mid) + rule2(mid -> dst) (either mid acts on something or transforms directly)
        for (const auto &r1 : wm.world_rules) {
            if (r1.src != is->second) continue;
            for (const auto &r2 : wm.world_rules) {
                if ((r2.act == r1.dst || r2.src == r1.dst) && r2.dst == id->second) {
                    auto f1 = wm.world_corr_fwd.find(r1.act);
                    auto f2 = wm.world_corr_fwd.find(r2.act);
                    if (f1 != wm.world_corr_fwd.end() && f2 != wm.world_corr_fwd.end()) {
                        gpu_step_area(3, sdr_of(f1->second), sdr_of(f2->second), SpikeVec{}, tr);
                        if (tr) tr->reasoning_hops += 2;
                        return {f1->second, f2->second};
                    }
                }
            }
        }
        return {};
    }

    // -----------------------------------------------------------------------
    // v5 Iterative Bipartite Constraint-Satisfaction Puzzle Solver (Einstein/Zebra!)
    // -----------------------------------------------------------------------
    Word solve_csp_location(Word target_item, CognitiveTrace *tr = nullptr) {
        if (abl_no_hipp || !target_item)
            return 0;
        Word rel_in = markers[LM_IN];
        Word self_id = markers[LM_SELF];
        for (int gidx = int(wm.csp_groups.size()) - 1; gidx >= 0; --gidx) {
            const auto &grp = wm.csp_groups[gidx];
            int t_row = -1;
            for (size_t i = 0; i < grp.items.size(); ++i)
                if (grp.items[i] == target_item) t_row = int(i);
            if (t_row < 0 || grp.items.size() != grp.places.size() || grp.items.empty())
                continue;

            size_t N = grp.items.size();
            std::vector<std::vector<int>> poss(N, std::vector<int>(N, 1));
            // Seed matrix from Hippocampal positive & negative clues!
            for (size_t i = 0; i < N; ++i) {
                for (size_t j = 0; j < N; ++j) {
                    HippResult hr = query_fact(grp.items[i], rel_in, grp.places[j], 0, self_id, false, R_OBJ, ~0u, tr);
                    if (hr.negated_index != ~0u && hr.negated_score >= hr.best_score) {
                        poss[i][j] = 0;
                    } else if (hr.best_index != ~0u && hr.negated_index == ~0u) {
                        for (size_t k = 0; k < N; ++k) {
                            if (k != j) poss[i][k] = 0;
                            if (k != i) poss[k][j] = 0;
                        }
                    }
                }
            }
            // Iterative spiking winner-take-all constraint propagation!
            bool changed = true;
            for (int iter = 0; iter < 16 && changed; ++iter) {
                changed = false;
                // Row singletons: if item i has only 1 possible place j -> eliminate place j from all other items!
                for (size_t i = 0; i < N; ++i) {
                    int cnt = 0, last_j = -1;
                    for (size_t j = 0; j < N; ++j) if (poss[i][j]) { ++cnt; last_j = int(j); }
                    if (cnt == 1) {
                        for (size_t k = 0; k < N; ++k) {
                            if (k != i && poss[k][last_j]) { poss[k][last_j] = 0; changed = true; }
                        }
                    }
                }
                // Column singletons: if place j has only 1 possible item i -> eliminate other places for item i!
                for (size_t j = 0; j < N; ++j) {
                    int cnt = 0, last_i = -1;
                    for (size_t i = 0; i < N; ++i) if (poss[i][j]) { ++cnt; last_i = int(i); }
                    if (cnt == 1) {
                        for (size_t k = 0; k < N; ++k) {
                            if (k != j && poss[last_i][k]) { poss[last_i][k] = 0; changed = true; }
                        }
                    }
                }
            }
            int cnt = 0, win_j = -1;
            for (size_t j = 0; j < N; ++j) if (poss[t_row][j]) { ++cnt; win_j = int(j); }
            if (cnt == 1 && win_j >= 0) {
                if (tr) tr->reasoning_hops += 2;
                return grp.places[win_j];
            }
        }
        return 0;
    }

    SpikeVec entity_state_sdr(Word entity, CognitiveTrace *tr = nullptr, bool allow_hyp = false) {
        SpikeVec st{};
        if (!entity)
            return st;
        Word rel_p = markers[LM_HAS_PROP];
        for (const auto &e : engrams) {
            if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) || (e.flags & ENG_NEGATED))
                continue;
            if (!allow_hyp && (e.flags & ENG_HYPOTHETICAL))
                continue;
            if (sdr_overlap(e.role[R_SUBJ], sdr_of(entity)) >= 18 &&
                sdr_overlap(e.role[R_REL], sdr_of(rel_p)) >= 18 &&
                sdr_overlap(e.role[R_PERSP], sdr_of(markers[LM_SELF])) >= 16) {
                // If hypothetical overrides an antonym, remove the real antonym spikes!
                if (e.flags & ENG_HYPOTHETICAL) {
                    Word hp = decode_sdr(e.role[R_OBJ], 14);
                    if (hp && hp < concepts.size() && concepts[hp].antonym_concept)
                        st = sdr_and_not(st, sdr_of(concepts[hp].antonym_concept));
                }
                st = sdr_or(st, e.role[R_OBJ]);
            }
        }
        gpu_step_area(1, sdr_of(entity), st, SpikeVec{}, tr);
        return st;
    }

    SpikeVec entity_category_sdr(Word entity, CognitiveTrace *tr = nullptr) {
        SpikeVec cat = sdr_of(entity);
        for (Word anc : get_ancestors(entity, tr))
            cat = sdr_or(cat, sdr_of(anc));
        return cat;
    }

    SpikeVec available_inventory_sdr(Word exclude_item = 0) const {
        SpikeVec inv{};
        Word rel_has = markers[LM_HAS];
        Word self_id = markers[LM_SELF];
        for (const auto &e : engrams) {
            if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) || (e.flags & ENG_NEGATED))
                continue;
            if (e.quantity != QTY_NONE && e.quantity <= 0)
                continue;
            if (rel_has && self_id && sdr_overlap(e.role[R_SUBJ], sdr_of(self_id)) >= 18 &&
                sdr_overlap(e.role[R_REL], sdr_of(rel_has)) >= 18) {
                if (exclude_item && sdr_overlap(e.role[R_OBJ], sdr_of(exclude_item)) >= 18)
                    continue;
                inv = sdr_or(inv, e.role[R_OBJ]);
            }
        }
        return inv;
    }

    // -----------------------------------------------------------------------
    // Hierarchical Cross-Object Tool-Crafting Planner (GPU Causal Rollout!)
    // If direct plan fails due to a missing tool (e.g. "cook stew" needs "fire"),
    // searches causal schemas for a producer of that tool ("strike wood -> fire"),
    // finds a world entity of that category ("tinder"), plans the prerequisite
    // subgoal on GPU, then plans the main goal with the crafted tool!
    // -----------------------------------------------------------------------
    struct HierPlanStep {
        Word action = 0;
        Word target = 0;
        Word schema_id = ~0u;
    };
    struct HierPlanResult {
        bool found = false;
        bool hierarchical = false;
        Word crafted_tool = 0;
        Word missing_tool = 0;
        std::vector<HierPlanStep> steps;
        PlanResult direct_pr{};
    };

    HierPlanResult plan_hierarchical_with_inv(Word target, Word goal, const SpikeVec &inv,
                                              CognitiveTrace *tr = nullptr, bool allow_hyp = false) {
        HierPlanResult out{};
        if (abl_no_causal || !target || !goal)
            return out;
        SpikeVec init_st = entity_state_sdr(target, tr, allow_hyp);
        SpikeVec t_cat = entity_category_sdr(target, tr);
        PlanQuery pq{};
        pq.init_state = init_st;
        pq.target_cat = t_cat;
        pq.inventory = inv;
        pq.goal_pos = sdr_of(goal);
        pq.max_depth = std::max(6u, max_plan_depth(genome));
        PlanResult pr = gpu_plan(pq, tr);
        out.direct_pr = pr;
        if (pr.found) {
            out.found = true;
            for (Word k = 0; k < pr.length; ++k) {
                Word sid = pr.schema_ids[k];
                Word act_c = decode_sdr(schemas[sid].action, 14);
                out.steps.push_back({act_c, target, sid});
            }
            return out;
        }
        // Inspect schemas that can achieve `goal` (or operate on `t_cat`) for missing tools!
        std::set<Word> cand_tools;
        for (const auto &cs : schemas) {
            if (!(cs.flags & 1u) || sdr_empty(cs.tool_req))
                continue;
            bool cat_ok = sdr_empty(cs.target_cat) || sdr_overlap(t_cat, cs.target_cat) >= 14;
            if (!cat_ok)
                continue;
            SpikeVec miss = sdr_and_not(cs.tool_req, inv);
            if (!sdr_empty(miss)) {
                Word mt = decode_sdr(miss, 12);
                if (mt)
                    cand_tools.insert(mt);
            }
        }
        for (Word mt : cand_tools) {
            out.missing_tool = mt;
            // Find a causal schema whose `produces` supplies missing tool `mt`!
            for (const auto &prod_cs : schemas) {
                if (!(prod_cs.flags & 1u) || sdr_empty(prod_cs.produces))
                    continue;
                if (sdr_overlap(prod_cs.produces, sdr_of(mt)) < 14)
                    continue;
                Word prod_cat = decode_sdr(prod_cs.target_cat, 14);
                Word prod_eff = decode_sdr(prod_cs.add_eff, 12);
                if (!prod_cat || !prod_eff)
                    continue;
                // Find a known entity in the world belonging to `prod_cat` (e.g. `tinder` is a `wood`)!
                Word helper_ent = 0;
                Word isa = markers[LM_IS_A];
                for (const auto &e : engrams) {
                    if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) || (e.flags & ENG_NEGATED))
                        continue;
                    if (sdr_overlap(e.role[R_REL], sdr_of(isa)) >= 18 &&
                        sdr_overlap(e.role[R_OBJ], sdr_of(prod_cat)) >= 16) {
                        Word ent = decode_sdr(e.role[R_SUBJ], 14);
                        if (ent && ent != target && !(concepts[ent].categories & CAT_CATEGORY)) {
                            helper_ent = ent;
                            break;
                        }
                    }
                }
                if (!helper_ent)
                    continue;
                // Subgoal GPU rollout: plan how to make `helper_ent` reach `prod_eff` (producing `mt`)!
                PlanQuery sub_q{};
                sub_q.init_state = entity_state_sdr(helper_ent, tr, allow_hyp);
                sub_q.target_cat = entity_category_sdr(helper_ent, tr);
                sub_q.inventory = inv;
                sub_q.goal_pos = sdr_of(prod_eff);
                sub_q.max_depth = std::max(6u, max_plan_depth(genome));
                PlanResult sub_pr = gpu_plan(sub_q, tr);
                if (!sub_pr.found)
                    continue;
                // Now plan main goal on GPU with newly crafted tool `mt` in inventory!
                PlanQuery main_q = pq;
                main_q.inventory = sdr_or(inv, sdr_of(mt));
                PlanResult main_pr = gpu_plan(main_q, tr);
                if (!main_pr.found)
                    continue;
                out.found = true;
                out.hierarchical = true;
                out.crafted_tool = mt;
                out.direct_pr = main_pr;
                for (Word k = 0; k < sub_pr.length; ++k) {
                    Word sid = sub_pr.schema_ids[k];
                    out.steps.push_back({decode_sdr(schemas[sid].action, 14), helper_ent, sid});
                }
                for (Word k = 0; k < main_pr.length; ++k) {
                    Word sid = main_pr.schema_ids[k];
                    out.steps.push_back({decode_sdr(schemas[sid].action, 14), target, sid});
                }
                if (tr)
                    tr->reasoning_hops += 2;
                return out;
            }
        }
        return out;
    }

    HierPlanResult plan_hierarchical(Word target, Word goal, CognitiveTrace *tr = nullptr, bool allow_hyp = false) {
        return plan_hierarchical_with_inv(target, goal, available_inventory_sdr(), tr, allow_hyp);
    }

    // -----------------------------------------------------------------------
    // Spatial Cognitive Map Pathfinding (BFS over Hippocampal Spatial Engrams!)
    // -----------------------------------------------------------------------
    std::vector<Word> find_spatial_path(Word start_place, Word goal_place, CognitiveTrace *tr = nullptr) {
        std::vector<Word> path;
        if (abl_no_hipp || !start_place || !goal_place)
            return path;
        if (start_place == goal_place)
            return {start_place};

        // Build undirected navigable adjacency from all active spatial relations (near, connected_to, north_of, etc.)
        std::map<Word, std::vector<Word>> adj;
        for (const auto &e : engrams) {
            if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) || (e.flags & ENG_NEGATED) ||
                (e.flags & (ENG_IMPLIES | ENG_DISJUNCTION)))
                continue;
            Word r = decode_sdr(e.role[R_REL], 16);
            if (!r || r >= concepts.size())
                continue;
            // Spatial relations: symmetric (near/connected_to) OR directional inverse (north_of/south_of/east_of/west_of)
            bool is_spatial = ((concepts[r].relation_meta & META_SYMMETRIC) != 0 && r != markers[LM_OPPOSITE]) ||
                              (concepts[r].inverse_concept != 0) || (r == markers[LM_CONNECTED]);
            if (!is_spatial)
                continue;
            Word u = decode_sdr(e.role[R_SUBJ], 14);
            Word v = decode_sdr(e.role[R_OBJ], 14);
            if (u && v && u != v && (concepts[u].categories & CAT_PLACE) && (concepts[v].categories & CAT_PLACE)) {
                adj[u].push_back(v);
                adj[v].push_back(u);
            }
        }
        std::vector<Word> q{start_place};
        std::map<Word, Word> parent;
        parent[start_place] = 0;
        bool reached = false;
        for (size_t qi = 0; qi < q.size() && qi < 128; ++qi) {
            Word cur = q[qi];
            if (cur == goal_place) {
                reached = true;
                break;
            }
            for (Word nxt : adj[cur]) {
                if (!parent.count(nxt)) {
                    parent[nxt] = cur;
                    q.push_back(nxt);
                }
            }
        }
        if (!reached)
            return path;
        for (Word cur = goal_place; cur != 0; cur = parent[cur])
            path.push_back(cur);
        std::reverse(path.begin(), path.end());
        gpu_step_area(2, sdr_of(start_place), sdr_of(goal_place), SpikeVec{}, tr);
        if (tr)
            tr->reasoning_hops += Word(path.size() > 0 ? path.size() - 1 : 0);
        return path;
    }

    // -----------------------------------------------------------------------
    // Downward Taxonomic Exemplar Recall & Enumeration ("name a living", "list all fish")
    // -----------------------------------------------------------------------
    std::vector<Word> find_exemplars(Word category, bool list_all, CognitiveTrace *tr = nullptr) {
        std::vector<Word> out;
        if (abl_no_hipp || !category)
            return out;
        Word isa = markers[LM_IS_A];
        // Build downward IS_A map: parent -> children
        std::map<Word, std::vector<Word>> children;
        for (const auto &e : engrams) {
            if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) || (e.flags & ENG_NEGATED))
                continue;
            if (sdr_overlap(e.role[R_REL], sdr_of(isa)) < 18)
                continue;
            Word sub = decode_sdr(e.role[R_SUBJ], 14);
            Word sup = decode_sdr(e.role[R_OBJ], 14);
            if (sub && sup && sub != sup)
                children[sup].push_back(sub);
        }
        // BFS downward to find concrete individual exemplars first (before abstract subcategories!)
        std::vector<Word> q{category};
        std::set<Word> seen{category};
        std::vector<Word> individuals, leaves, nonleaves;
        for (size_t qi = 0; qi < q.size() && qi < 128; ++qi) {
            Word cur = q[qi];
            for (Word ch : children[cur]) {
                if (seen.count(ch))
                    continue;
                seen.insert(ch);
                q.push_back(ch);
                if (children[ch].empty()) {
                    if (!(concepts[ch].categories & CAT_CATEGORY))
                        individuals.push_back(ch);
                    else
                        leaves.push_back(ch);
                } else {
                    nonleaves.push_back(ch);
                }
            }
        }
        const auto &pick = !individuals.empty() ? individuals : (!leaves.empty() ? leaves : nonleaves);
        for (Word x : pick) {
            out.push_back(x);
            if (!list_all && out.size() >= 1)
                break;
            if (out.size() >= 6)
                break;
        }
        if (!out.empty()) {
            gpu_step_area(1, sdr_of(category), sdr_of(out[0]), SpikeVec{}, tr);
            if (tr) ++tr->reasoning_hops;
        }
        return out;
    }

    // -----------------------------------------------------------------------
    // Parietal Number-Line Mental Arithmetic (fires Area 2 Parietal Spikes!)
    // -----------------------------------------------------------------------
    std::optional<int> compute_parietal_arith(int a, Word op, int b, CognitiveTrace *tr = nullptr) {
        if (a == QTY_NONE || b == QTY_NONE || !op)
            return std::nullopt;
        int res = 0;
        if (op == markers[LM_PLUS]) res = a + b;
        else if (op == markers[LM_MINUS]) res = a - b;
        else if (op == markers[LM_TIMES]) res = a * b;
        else if (op == markers[LM_DIV]) {
            if (b == 0) return std::nullopt;
            res = a / b;
        } else {
            return std::nullopt;
        }
        SpikeVec va = cochlear_encode(std::to_string(a), genome.wiring[2]);
        SpikeVec vb = cochlear_encode(std::to_string(b), genome.wiring[2]);
        gpu_step_area(2, va, sdr_of(op), vb, tr);
        if (tr) ++tr->reasoning_hops;
        return res;
    }

    // -----------------------------------------------------------------------
    // Universal Counterexample Refutation & Existential Witness Reasoning!
    // "Can all birds fly?" -> NO (counterexample: penguin is a bird, cannot fly!)
    // "Can some birds swim?" -> YES (witness: penguin!)
    // "Are all birds animals?" -> YES (universal taxonomy!)
    // -----------------------------------------------------------------------
    struct QuantResult {
        int answer = 0; // +1=YES, -1=NO, 0=UNKNOWN
        Word witness = 0;
        bool is_counterexample = false;
    };

    QuantResult check_quantified(Word cat, Word rel, Word obj, bool universal, CognitiveTrace *tr = nullptr) {
        QuantResult qr{};
        if (abl_no_hipp || !cat || !rel || !obj)
            return qr;
        Word isa = markers[LM_IS_A];
        Word self_id = markers[LM_SELF];

        // Collect all known members of category `cat`
        std::vector<Word> members;
        std::set<Word> seen;
        for (const auto &e : engrams) {
            if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) || (e.flags & ENG_NEGATED))
                continue;
            if (sdr_overlap(e.role[R_REL], sdr_of(isa)) >= 18 &&
                sdr_overlap(e.role[R_OBJ], sdr_of(cat)) >= 16) {
                Word m_id = decode_sdr(e.role[R_SUBJ], 14);
                if (m_id && !seen.count(m_id)) {
                    seen.insert(m_id);
                    members.push_back(m_id);
                }
            }
        }

        if (universal) {
            // 1. Search for ANY counterexample member violating (x, rel, obj)!
            for (Word m_id : members) {
                HippResult direct = query_fact(m_id, rel, obj, 0, self_id, false, R_OBJ, ~0u, tr);
                if (direct.negated_index != ~0u && direct.negated_score >= direct.best_score) {
                    qr = {-1, m_id, true};
                    if (tr) ++tr->reasoning_hops;
                    return qr;
                }
                if (rel == markers[LM_HAS_PROP] && obj < concepts.size() && concepts[obj].antonym_concept) {
                    HippResult ant = query_fact(m_id, rel, concepts[obj].antonym_concept, 0, self_id, false, R_OBJ, ~0u, tr);
                    if (ant.best_index != ~0u) {
                        qr = {-1, m_id, true};
                        if (tr) ++tr->reasoning_hops;
                        return qr;
                    }
                }
            }
            // 2. Otherwise check if `cat` itself satisfies `(cat, rel, obj)` universally!
            auto inh = check_inherited(cat, rel, obj, self_id, tr);
            if (inh.answer != 0) {
                qr = {inh.answer, cat, false};
                return qr;
            }
        } else {
            // Existential ("some"): search for ANY positive witness member!
            for (Word m_id : members) {
                auto inh = check_inherited(m_id, rel, obj, self_id, tr);
                if (inh.answer > 0) {
                    qr = {1, m_id, false};
                    if (tr) ++tr->reasoning_hops;
                    return qr;
                }
            }
            auto inh_cat = check_inherited(cat, rel, obj, self_id, tr);
            if (inh_cat.answer > 0)
                return {1, members.empty() ? cat : members[0], false};
            if (inh_cat.answer < 0)
                return {-1, cat, false};
        }
        return qr;
    }

    // -----------------------------------------------------------------------
    // Fast Contrastive Comparison (35x fewer GPU queries!)
    // Finds contrasting capability OR property using ONLY engrams touching a, b, or ancestors!
    // -----------------------------------------------------------------------
    struct ContrastResult {
        Word shared_cat = 0;
        Word contrast_rel = 0;
        Word contrast_val = 0;
        bool swap_ab = false;
    };

    ContrastResult compare_concepts_fast(Word a, Word b, CognitiveTrace *tr = nullptr) {
        ContrastResult cr{};
        if (!a || !b)
            return cr;
        auto aa = get_ancestors(a, tr);
        auto ab = get_ancestors(b, tr);
        for (Word ca : aa) {
            if (std::find(ab.begin(), ab.end(), ca) != ab.end()) {
                cr.shared_cat = ca;
                break;
            }
        }
        std::set<Word> scope{a, b};
        for (Word x : aa) scope.insert(x);
        for (Word x : ab) scope.insert(x);

        Word rel_can = markers[LM_CAN];
        Word rel_prop = markers[LM_HAS_PROP];
        Word self_id = markers[LM_SELF];
        std::set<Word> cand_acts, cand_props;
        for (const auto &e : engrams) {
            if (!(e.flags & ENG_ACTIVE) || (e.flags & ENG_SUPERSEDED) || (e.flags & (ENG_IMPLIES | ENG_DISJUNCTION)))
                continue;
            Word s = decode_sdr(e.role[R_SUBJ], 18);
            if (!s || !scope.count(s))
                continue;
            if (sdr_overlap(e.role[R_REL], sdr_of(rel_can)) >= 18) {
                Word act = decode_sdr(e.role[R_OBJ], 14);
                if (act) cand_acts.insert(act);
            } else if (sdr_overlap(e.role[R_REL], sdr_of(rel_prop)) >= 18) {
                Word pr = decode_sdr(e.role[R_OBJ], 14);
                if (pr) cand_props.insert(pr);
            }
        }
        for (Word act : cand_acts) {
            auto ia = check_inherited(a, rel_can, act, self_id, tr);
            auto ib = check_inherited(b, rel_can, act, self_id, tr);
            if (ia.answer != 0 && ib.answer != 0 && ia.answer != ib.answer) {
                cr.contrast_rel = rel_can;
                cr.contrast_val = act;
                cr.swap_ab = (ia.answer < 0);
                return cr;
            }
        }
        for (Word pr : cand_props) {
            auto ia = check_inherited(a, rel_prop, pr, self_id, tr);
            auto ib = check_inherited(b, rel_prop, pr, self_id, tr);
            if (ia.answer != 0 && ib.answer != 0 && ia.answer != ib.answer) {
                cr.contrast_rel = rel_prop;
                cr.contrast_val = pr;
                cr.swap_ab = (ia.answer < 0);
                return cr;
            }
        }
        return cr;
    }

    // -----------------------------------------------------------------------
    // Binary Checkpoint Save & Load (`.cmind` v8 — includes 6-MiB Neocortex & 2D Place-Cell Map!)
    // -----------------------------------------------------------------------
    void save(const std::string &path) const {
        std::ofstream out(path, std::ios::binary);
        if (!out)
            throw std::runtime_error("cannot write cortical mind checkpoint " + path);
        const char magic[8] = {'A', 'X', 'C', 'M', 'N', 'D', '5', '8'};
        out.write(magic, 8);
        auto put = [&](const auto &v) { out.write(reinterpret_cast<const char *>(&v), sizeof(v)); };
        put(genome);
        put(areas);
        put(markers);
        put(clock);
        put(generation);
        put(stage);
        put(lifetime_spikes);
        put(lifetime_edits);
        put(sleep_cycles_done);
        put(dialect_seed);
        Word sov_flag = is_sov_dialect ? 1u : 0u;
        put(sov_flag);

        Word nc = Word(concepts.size());
        put(nc);
        out.write(reinterpret_cast<const char *>(concepts.data()), std::streamsize(nc * sizeof(ConceptNode)));

        Word ne = Word(engrams.size());
        put(ne);
        if (ne)
            out.write(reinterpret_cast<const char *>(engrams.data()), std::streamsize(ne * sizeof(Engram)));

        Word ns = Word(schemas.size());
        put(ns);
        if (ns)
            out.write(reinterpret_cast<const char *>(schemas.data()), std::streamsize(ns * sizeof(CausalSchema)));

        Word ng = Word(constructions.size());
        put(ng);
        if (ng)
            out.write(reinterpret_cast<const char *>(constructions.data()), std::streamsize(ng * sizeof(Construction)));

        Word nf = Word(filler_concepts.size());
        put(nf);
        for (Word f : filler_concepts)
            put(f);

        Word nh = Word(learned_hedges.size());
        put(nh);
        for (const auto &h : learned_hedges) {
            Word hl = Word(h.size());
            put(hl);
            for (Word x : h)
                put(x);
        }

        Word nct = Word(learned_contractions.size());
        put(nct);
        for (const auto &[k, vec] : learned_contractions) {
            Word kl = Word(k.size());
            put(kl);
            out.write(k.data(), std::streamsize(kl));
            Word vl = Word(vec.size());
            put(vl);
            for (const auto &s : vec) {
                Word sl = Word(s.size());
                put(sl);
                out.write(s.data(), std::streamsize(sl));
            }
        }

        Word na = Word(alias_to_concept.size());
        put(na);
        for (const auto &[k, v] : alias_to_concept) {
            Word len = Word(k.size());
            put(len);
            out.write(k.data(), std::streamsize(len));
            put(v);
        }

        // Persist 3-MiB GPU Neocortical Synaptic Weights + Trust Ledger!
        Word neo_flag = neo_consolidated_ready ? 1u : 0u;
        put(neo_flag);
        if (neo_flag) {
            // Compact sparse serialization of non-zero neocortical synapses!
            Word nnz = 0;
            for (signed char w : neo_weights)
                if (w != 0) ++nnz;
            put(nnz);
            for (Word idx = 0; idx < Word(neo_weights.size()); ++idx) {
                if (neo_weights[idx] != 0) {
                    put(idx);
                    put(neo_weights[idx]);
                }
            }
        }
        // Persist 2D Place-Cell Landmark Map!
        Word nlm = Word(map_landmarks.size());
        put(nlm);
        for (const auto &[cid, xy] : map_landmarks) {
            put(cid);
            put(xy.first);
            put(xy.second);
        }
        put(map_bridge_concept);
        put(map_pass_concept);
        Word br_flag = map_bridge_broken ? 1u : 0u;
        put(br_flag);
        if (!out)
            throw std::runtime_error("failed writing cortical mind checkpoint " + path);
    }

    void load(const std::string &path) {
        std::ifstream in(path, std::ios::binary);
        if (!in)
            throw std::runtime_error("cannot read cortical mind checkpoint " + path);
        char magic[8]{};
        in.read(magic, 8);
        if (std::memcmp(magic, "AXCMND58", 8) != 0)
            throw std::runtime_error("invalid cortical mind checkpoint format: " + path);
        auto get = [&](auto &v) { in.read(reinterpret_cast<char *>(&v), sizeof(v)); };
        get(genome);
        get(areas);
        get(markers);
        get(clock);
        get(generation);
        get(stage);
        get(lifetime_spikes);
        get(lifetime_edits);
        get(sleep_cycles_done);
        get(dialect_seed);
        Word sov_flag = 0;
        get(sov_flag);
        is_sov_dialect = sov_flag != 0;

        Word nc = 0;
        get(nc);
        if (nc < 1 || nc > MAX_CONCEPTS + 1)
            throw std::runtime_error("corrupt concept count in " + path);
        concepts.resize(nc);
        in.read(reinterpret_cast<char *>(concepts.data()), std::streamsize(nc * sizeof(ConceptNode)));

        Word ne = 0;
        get(ne);
        if (ne > MAX_ENGRAMS)
            throw std::runtime_error("corrupt engram count in " + path);
        engrams.resize(ne);
        if (ne)
            in.read(reinterpret_cast<char *>(engrams.data()), std::streamsize(ne * sizeof(Engram)));

        Word ns = 0;
        get(ns);
        if (ns > MAX_CAUSAL)
            throw std::runtime_error("corrupt causal schema count in " + path);
        schemas.resize(ns);
        if (ns)
            in.read(reinterpret_cast<char *>(schemas.data()), std::streamsize(ns * sizeof(CausalSchema)));

        Word ng = 0;
        get(ng);
        if (ng > MAX_CONSTRUCTIONS)
            throw std::runtime_error("corrupt construction count in " + path);
        constructions.resize(ng);
        if (ng)
            in.read(reinterpret_cast<char *>(constructions.data()), std::streamsize(ng * sizeof(Construction)));

        filler_concepts.clear();
        Word nf = 0;
        get(nf);
        for (Word i = 0; i < nf; ++i) {
            Word f = 0;
            get(f);
            filler_concepts.insert(f);
        }

        learned_hedges.clear();
        Word nh = 0;
        get(nh);
        for (Word i = 0; i < nh; ++i) {
            Word hl = 0;
            get(hl);
            std::vector<Word> h(hl);
            for (Word j = 0; j < hl; ++j)
                get(h[j]);
            learned_hedges.push_back(std::move(h));
        }

        learned_contractions.clear();
        Word nct = 0;
        get(nct);
        for (Word i = 0; i < nct; ++i) {
            Word kl = 0;
            get(kl);
            std::string k(kl, '\0');
            in.read(k.data(), std::streamsize(kl));
            Word vl = 0;
            get(vl);
            std::vector<std::string> vec;
            vec.reserve(vl);
            for (Word j = 0; j < vl; ++j) {
                Word sl = 0;
                get(sl);
                std::string s(sl, '\0');
                in.read(s.data(), std::streamsize(sl));
                vec.push_back(std::move(s));
            }
            learned_contractions[std::move(k)] = std::move(vec);
        }

        alias_to_concept.clear();
        Word na = 0;
        get(na);
        for (Word i = 0; i < na; ++i) {
            Word len = 0;
            get(len);
            std::string k(len, '\0');
            in.read(k.data(), std::streamsize(len));
            Word v = 0;
            get(v);
            alias_to_concept[k] = v;
        }
        neo_weights.assign(size_t(NEO_PATHWAYS) * NEO_SYNAPSES, 0);
        Word neo_flag = 0;
        get(neo_flag);
        neo_consolidated_ready = neo_flag != 0;
        if (neo_consolidated_ready) {
            Word nnz = 0;
            get(nnz);
            for (Word i = 0; i < nnz; ++i) {
                Word idx = 0;
                signed char w = 0;
                get(idx);
                get(w);
                if (idx < neo_weights.size())
                    neo_weights[idx] = w;
            }
        }
        map_landmarks.clear();
        Word nlm = 0;
        get(nlm);
        for (Word i = 0; i < nlm; ++i) {
            Word cid = 0, x = 0, y = 0;
            get(cid);
            get(x);
            get(y);
            map_landmarks[cid] = {x, y};
        }
        get(map_bridge_concept);
        get(map_pass_concept);
        Word br_flag = 0;
        get(br_flag);
        map_bridge_broken = br_flag != 0;
        if (!in)
            throw std::runtime_error("truncated cortical mind checkpoint " + path);
        gpu_dirty_engrams = true;
        gpu_dirty_schemas = true;
        gpu_dirty_neo = true;
        if (session)
            sync_genome_to_gpu();
    }
};

} // namespace axion::mind
