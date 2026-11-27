#pragma once
#include "axion/commons.hpp"
#include "life_io.hpp"
#include <cstring>
#include <fstream>
#include <vector>

namespace axion::life::storage {
struct SeedBank {
    struct Header {char magic[8]="AXSEED1";Word count=0,focus=0;} header;
    std::vector<SeedBed> beds;
    template<class F> void chunks(F visit) {header.count=Word(beds.size());visit(&header,sizeof(header));visit(beds.data(),beds.size()*sizeof(SeedBed));}
    void read(const std::string &path) {
        std::ifstream in(path,std::ios::binary);if (!in.read(reinterpret_cast<char *>(&header),sizeof(header)) ||
            std::memcmp(header.magic,"AXSEED1",8) || !header.count || header.count>32768 || header.focus>=header.count) throw std::runtime_error("invalid seed bank");
        beds.resize(header.count);Count stored=0;
        if (!in.read(reinterpret_cast<char *>(beds.data()),beds.size()*sizeof(SeedBed)) || !in.read(reinterpret_cast<char *>(&stored),sizeof(stored)) || in.peek()!=std::ifstream::traits_type::eof())
            throw std::runtime_error("incomplete seed bank");
        Count hash=digest(digest(hash_start,&header,sizeof(header)),beds.data(),beds.size()*sizeof(SeedBed));
        if (hash!=stored) throw std::runtime_error("seed bank checksum mismatch");
        auto valid=[](Word gate){Word v=gate-1;return gate && (v&7)<6 && ((v>>3)&7)<6 && (v>>6)<16;};
        for (const auto &bed:beds) {
            if (bed.field.present!=~0u || bed.field.dormant>63 || !valid(bed.field.food)) throw std::runtime_error("invalid physical seed");
            for (auto x:bed.field.plots) if (x>63) throw std::runtime_error("invalid archived plot");
            for (auto &row:bed.field.gates) for (auto g:row) if (!valid(g)) throw std::runtime_error("invalid physical kernel");
        }
        journal::checkpoint(path,hash,false);
    }
};
}
