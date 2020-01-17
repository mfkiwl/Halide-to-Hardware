#ifndef HALIDE_UBUFFER_REWRITES_H
#define HALIDE_UBUFFER_REWRITES_H

#include "coreir.h"

#include "ExtractHWBuffers.h"
#include "HWUtils.h"
#include "IR.h"

namespace Halide {
  namespace Internal {

    class HWBufferWrapper {
      public:
        CoreIR::Module* mod;
        std::map<string, const Provide*> writePorts;
        std::map<string, const Call*> readPorts;
    };

    std::map<std::string, HWBufferWrapper>
    synthesize_hwbuffers(const Stmt& stmt, const std::map<std::string, Function>& env, std::vector<HWXcel>& xcels);
  }
}

#endif
