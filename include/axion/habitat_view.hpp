#pragma once
#include "life.hpp"
#if !defined(__CUDA__)
#include <memory>
#include <string>
#endif
namespace axion::life {
// Four directional receptor currents in clockwise order from north.
struct HabitatSignals { Word food[4],water[4],food_bearing,water_bearing; };
struct ResidentView { Word slot,founder,born,generation,position; };
#if !defined(__CUDA__)
class HabitatObserver {
public:
    virtual ~HabitatObserver()=default;
    virtual Word side() const=0;
    virtual Word tick() const=0;
    virtual HabitatSignals observe(Word x,Word y)=0;
    virtual bool locate(ResidentView &animal,Word x,Word y,bool nearest)=0;
    virtual void advance(Word steps)=0;
    virtual void save(const std::string &path)=0;
};
std::unique_ptr<HabitatObserver> open_habitat(const std::string &path);
#endif
}
