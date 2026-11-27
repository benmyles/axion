#pragma once
#include "life_io.hpp"
#include "sequence_chorus.cuh"
#include <cstring>
#include <fstream>
#include <vector>

namespace axion::life::storage {
// At a season boundary all acquired contacts die with their owners. The eggs
// and the physical season counter suffice to resume the next generation.
struct ChorusEggs {
    struct Header {
        char magic[8]="AXEGGS1";
        Word version=1,count=0,next=0,seed=0,days=0,play=0,cells=0,arm=0,drift=0;
    } header;
    std::vector<chorus::Heredity> eggs;
    template<class F> void chunks(F visit) {
        visit(&header,sizeof(header));visit(eggs.data(),eggs.size()*sizeof(eggs[0]));
    }
    void read(const std::string &path) {
        std::ifstream f(path,std::ios::binary|std::ios::ate);
        if (!f) throw std::runtime_error("cannot open chorus eggs "+path);
        auto bytes=f.tellg();f.seekg(0);
        if (!f.read(reinterpret_cast<char *>(&header),sizeof(header)) || std::memcmp(header.magic,"AXEGGS1",8) ||
            header.version!=1 || !header.count || header.count>65536 || header.count%32 ||
            !header.days || header.days>4096 || !header.play || header.play>trace_slots ||
            !header.cells || header.cells>sequence_cells || header.arm>2 || header.drift>1 ||
            bytes!=std::streamoff(sizeof(header)+header.count*sizeof(chorus::Heredity)+sizeof(Count)))
            throw std::runtime_error("invalid chorus checkpoint");
        eggs.resize(header.count);
        Count hash=digest(hash_start,&header,sizeof(header)),stored=0;
        if (!f.read(reinterpret_cast<char *>(eggs.data()),eggs.size()*sizeof(eggs[0])) ||
            !f.read(reinterpret_cast<char *>(&stored),sizeof(stored)) ||
            stored!=digest(hash,eggs.data(),eggs.size()*sizeof(eggs[0]))) throw std::runtime_error("chorus checkpoint checksum mismatch");
        for (const auto &g:eggs) if (g.alive>1 || (g.alive && g.founder>=header.count)) throw std::runtime_error("invalid chorus lineage");
        journal::checkpoint(path,stored,false);
    }
};
}
