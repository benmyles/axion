#pragma once
#include "composition_island.cuh"
#include "life_io.hpp"
#include <cstring>
#include <fstream>
#include <vector>

namespace axion::life::storage {
struct ShelterNest {
    struct Header {
        char magic[8]="AXNEST1";
        Word version=1,count=0,season=0,seed=0,dialect=0,rounds=0,days=0,wire=0,drift=0;
    } header;
    std::vector<market::Genome> eggs;
    template<class F> void chunks(F visit) {visit(&header,sizeof(header));visit(eggs.data(),eggs.size()*sizeof(eggs[0]));}
    void read(const std::string &path) {
        std::ifstream f(path,std::ios::binary|std::ios::ate);if (!f) throw std::runtime_error("cannot open the shelter brood");
        auto bytes=f.tellg();f.seekg(0);
        if (!f.read(reinterpret_cast<char *>(&header),sizeof(header)) || std::memcmp(header.magic,"AXNEST1",8) || header.version!=1 ||
            !header.count || header.count>8192 || header.count%32 || header.dialect>2 || header.rounds<2 || header.rounds>12 ||
            !header.days || header.days>2048 || header.wire>4 || header.drift>1 ||
            bytes!=std::streamoff(sizeof(header)+header.count*sizeof(market::Genome)+sizeof(Count))) throw std::runtime_error("invalid shelter brood");
        Count hash=digest(hash_start,&header,sizeof(header)),stored=0;eggs.resize(header.count);
        if (!f.read(reinterpret_cast<char *>(eggs.data()),eggs.size()*sizeof(eggs[0])) || !f.read(reinterpret_cast<char *>(&stored),sizeof(stored)) ||
            stored!=digest(hash,eggs.data(),eggs.size()*sizeof(eggs[0]))) throw std::runtime_error("shelter brood checksum mismatch");
        for (auto &egg:eggs) if (egg.alive>1 || (egg.alive && egg.founder>=header.count)) throw std::runtime_error("invalid shelter genealogy");
        journal::checkpoint(path,stored,false);
    }
};
}
