#pragma once
#include "dialogue.hpp"

namespace axion::life {
struct CommonsSettings { Word count,seed,rations,volatility,price,policy,choice,credit,soma,form,exposures,reserve,receipts,channel,ear_learning,auditory_form,kin,focused,optics,geometry,joint,calories,shuffled,audit,projection;
    Word culture=0,tutoring=0,report_wire=0,report_cost=0,event_age=0,cues[2]{},teachers=0,report_cache=1,alternating=0;
    Word route_depth=2,route_reuse=1,tidal_feedback=1,current_seed=0,mentoring=0;
    Word flora=0,pollen=4,observers=1,garden_cache=1,garden_model=0,mixed_plots=1; };
struct SeedBed {Apparatus field{};Word ancestor=0,edits=0,migrated=0,dormant_meals=0;};
struct CropPolicy {Count planes[3]{};Word dirty=1,builds=0;};
struct GardenSpeech {Count names[32]{},words[25]{},verdicts[3]{};};
struct EstuarySpeech {Count places[32]{},routes[3]{},verdicts[3]{};};
struct CulturalIsland {Reports teacher{};Word accepted=0,improvised=0,words=0,investigated=0;};
struct CulturalSample {Utterance voice{};Word accepted=0,improvised=0,words=0,investigated=0,forms=0;};
struct Forager {
    Word energy,day,receipt,offspring,meals,missed,contested,experiments,diagnostic,after_miss,lived,expense,meal_updates,unopened;
};
struct AcousticState { Word heard,decoded,calls,heard_calls,readings,accurate,private_traps,private_hits,prior_call; };
struct Reservoir { Word stock[4],switches,phase,regrown,consumed,law; };
struct AcousticIsland { Word speech[2][4][5],retinas[2],audit[4]; };
struct CommonsTotals { Count count[19]; };
}
