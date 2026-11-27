#pragma once
#include "model.hpp"

namespace axion::life {
// The two inherited tables specify the next low and high contact bits for
// every combination of old state, presynaptic cue, retinal spike and clarity.
struct RecollectionGenes { Word low=0,high=0,expression=3; };
struct Recollection { RecollectionGenes genes{};Word weights=0; };
// Six auditory action fields. Each has sixteen two-bit transition contacts.
// The visual and prospective circuits share the individual's inherited rule.
struct Imagery { Word forms[6]{},weights[6]{}; };
struct Projection { Word paths=0x8421u,actions=0; };
static_assert(sizeof(Imagery)==48);
static_assert(sizeof(Recollection)==16);
enum class RecallCut : Word { intact, fixed, crossed, source, erased, last, immediate };
struct RecallScore { Word one,delay,noise,revision,faint,empty; };
struct MemoryProof { Word reachable,checked[4],failed[4],first[4]; };
#if !defined(__CUDA__)
int recollection_command(int argc,char **argv);
int attention_command(int argc,char **argv);
int imagery_command(int argc,char **argv);
#endif
}
