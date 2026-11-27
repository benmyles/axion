#pragma once
#include "cuda_driver.hpp"

namespace axion::life::runtime {
using namespace cuda;
using Graph = struct CUgraph_st *;
using GraphExec = struct CUgraphExec_st *;
struct GraphApi {
    Result (*create_stream)(Stream *, unsigned);
    Result (*destroy_stream)(Stream);
    Result (*begin)(Stream, int);
    Result (*end)(Stream, Graph *);
    Result (*instantiate)(GraphExec *, Graph, unsigned long long);
    Result (*launch)(GraphExec, Stream);
    Result (*destroy_exec)(GraphExec);
    Result (*destroy)(Graph);
    void bind(Driver &d) {
        d.load(create_stream, "cuStreamCreate"); d.load(destroy_stream, "cuStreamDestroy_v2");
        d.load(begin, "cuStreamBeginCapture_v2"); d.load(end, "cuStreamEndCapture");
        d.load(instantiate, "cuGraphInstantiateWithFlags"); d.load(launch, "cuGraphLaunch");
        d.load(destroy_exec, "cuGraphExecDestroy"); d.load(destroy, "cuGraphDestroy");
    }
};
}
