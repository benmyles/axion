#pragma once
#include "life.hpp"
#if !defined(__CUDA__)
#include <array>
#include <functional>
#include <map>
#include <string>
#include <vector>
#endif

namespace axion::journal {
using life::Count;
struct FieldTile { unsigned short food,alive; Word lineage; };
struct Animal {
    Word slot,position,founder,born,generation,energy,body,muscles,heading,tape;
    Word expression,effectors_low,effectors_high,navigation_rule;
    life::TraceGenes temporal_rule;
};
struct BroodScore { Count total,best; };
#if !defined(__CUDA__)
struct Moment {
    Count elapsed,tick,passage,alive,families,generation,births,deaths,moves,edits,grazed,bitten,signals;
};
struct Frame { Moment time; std::vector<Animal> residents; };
struct Score { std::string name; Count correct,total; };
struct BreedingPoint { Word generation,animals; BroodScore score; };
struct Checkpoint { std::string path; Count checksum; bool saved; };
struct Specimen {
    std::string path,role;Count checksum=0;
    std::vector<std::string> senses,circuits;
    std::map<std::string,Count> measures;
};
struct Viewpoint {
    Count after;Word tick,x,y,side;
    std::array<Word,4> food,water;
    Word slot,founder,born,generation;
};
struct Report {
    std::string command,source,habitat,metabolism,device,archive;
    std::map<std::string,Count> metrics;
    std::vector<std::string> args,lines;
    std::vector<Moment> points;
    std::vector<Frame> frames;
    std::vector<FieldTile> tiles;
    std::vector<Score> scores;
    std::vector<Word> actions,transitions,conditional,interviews;
    std::vector<Word> perceptions;
    std::vector<std::array<Word,8>> commons_residents;
    std::vector<Viewpoint> viewpoints;
    std::map<Word,Count> journeys;
    std::vector<BreedingPoint> breeding;
    std::vector<Checkpoint> checkpoints;
    std::vector<Specimen> specimens;
    std::array<Count,8> motors{};
    Word side=0,resolution=0;
    void observe(Moment moment);
    void action(Word motor,Word pins,Word event);
    void turn(Word words,Word first,Word second,Word request,Word view,Word sound,Word shutters,Word dim,Word retinal,Word attention=0);
    void line(std::string entry);
};
Report *current();
void checkpoint(const std::string &path,Count checksum,bool saved);
int execute(int argc,char **argv,const std::function<int(int,char **)> &command);
#endif
}
