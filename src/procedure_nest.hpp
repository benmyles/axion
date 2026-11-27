#pragma once
#include "procedure_roost.cuh"
#include "procedure_storage.hpp"
#include "life_io.hpp"
#include <cstring>
#include <fstream>
#include <vector>

namespace axion::life::storage {
struct RoostNest {
    struct Header {
        char magic[8]="AXROOST";
        Word version=3,n=0,epoch=0,seed=0,style=0,rounds=0,steps=0,days=0,cut=0,coins=0,culture=0,storm=0,drift=0;
    } header;
    std::vector<roost::Genome> eggs;
    std::vector<procedure::Mind> mentors;
    std::vector<roost::Terrain> field;
    template<class F> void chunks(F visit) {
        visit(&header,sizeof(header));visit(eggs.data(),eggs.size()*sizeof(eggs[0]));
        visit(mentors.data(),mentors.size()*sizeof(mentors[0]));visit(field.data(),field.size()*sizeof(field[0]));
    }
    void read(const std::string &name) {
        std::ifstream f(name,std::ios::binary|std::ios::ate);if (!f) throw std::runtime_error("cannot open roost brood");
        auto length=f.tellg();f.seekg(0);auto &h=header;
        if (!f.read(reinterpret_cast<char *>(&h),sizeof(h)) || std::memcmp(h.magic,"AXROOST",8) || (h.version<1 || h.version>3) || !h.n || h.n%32 ||
            h.n>16384 || !h.epoch || h.style>2 || h.rounds<2 || h.rounds>14 || h.steps>roost::practice_limit || !h.days || h.days>4096 ||
            h.cut>4+h.version || h.coins>1 || h.culture>2 || h.drift>1 ||
            length!=std::streamoff(sizeof(h)+sizeof(Count)+h.n*sizeof(roost::Genome)+(h.n/32)*(sizeof(procedure::Mind)+sizeof(roost::Terrain))))
            throw std::runtime_error("invalid roost brood header");
        eggs.resize(h.n);mentors.resize(h.n/32);field.resize(h.n/32);Count hash=digest(hash_start,&h,sizeof(h));
        auto load=[&](auto &collection) {
            auto bytes=collection.size()*sizeof(collection[0]);if (!f.read(reinterpret_cast<char *>(collection.data()),bytes)) throw std::runtime_error("incomplete roost brood");
            hash=digest(hash,collection.data(),bytes);
        };
        load(eggs);load(mentors);load(field);Count checksum;
        if (!f.read(reinterpret_cast<char *>(&checksum),sizeof(checksum)) || checksum!=hash) throw std::runtime_error("roost brood checksum mismatch");
        for (const auto &egg:eggs) if (egg.alive>1 || (egg.alive && egg.founder>=h.n)) throw std::runtime_error("invalid roost lineage");
        for (const auto &adult:mentors) procedure::validate(adult);
        journal::checkpoint(name,checksum,false);
    }
};
}
