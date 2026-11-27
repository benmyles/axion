#pragma once
#include "life.hpp"

namespace axion::life {
struct JunctionTrial {
    Word sets=0,rounds=12,delay=2,practice_delay=2,noise=0,practice_noise=0,visible=0,seed=883173,motivation=0,offset=0;
    Word traversal=0,variable=0,places=1,lifetime_clock=1;
};
struct PassageScore { Count naive,acquired,surprise,revised,observed,moved,covered; };
struct ForageLedger {
    Count at_spring,away,depart_spring,depart_away,returning,depart_returning,meals,found,first_arrival;
};
}
