#pragma once
#include "life.hpp"

namespace axion::life {

// One descendant, detached from its territory. The file contains only the
// expressed capacity of tissue. Its time base is the external event stream.
struct Individual {
    Genome genes;
    SynapticLayer layer;
    Contacts contacts;
    Word tiles, bits, delayed_path, outcome_lag, seed, ticks;
    Word due, motor_history, afferent_history[8];
    Word has_dendrites, has_relations, alternate_program;
    Tissue tissue[32];
};
enum class EventKind : Word { trial, step, probe, sample, seek, taste, travel, orient, stride, savour };
struct Encounter { Word receptors, ambient, mask, value, signal; EventKind kind; };
struct Response { Word action, ports, received, matched; };
#if !defined(__CUDA__)
inline const char *neural_prefix(Word level) {
    return level==3 ? "context_" : level==2 ? "wave_" : level==1 ? "linked_" : "";
}
inline unsigned association_stride(Word level,Word programs=1) {
    return level ? sizeof(AssociationLayer)+(level==3 ? sizeof(ContextLayer)+(programs==2 ? sizeof(ExpressionLayer) : 0) : 0) : 0;
}
Individual hatch_from_habitat(const char *file, Word site, Word &origin, AssociationLayer &links, ContextLayer &contexts, ExpressionLayer &expression,bool resident=false,OutcomeLayer *outcome=nullptr,Word *metabolism=nullptr,MapLayer *map=nullptr,Word *navigation=nullptr,TraceLayer *trace=nullptr);
int actor_command(int argc, char **argv);
#endif
} // namespace axion::life
