#ifndef HALIDE_MAPPER_COMPUTE_H
#define HALIDE_MAPPER_COMPUTE_H

#include "ExtractHWBuffers.h"
#include "Function.h"
#include "IR.h"

namespace Halide {
  namespace Internal {

    void generate_compute_unit(Stmt& stmt, std::map<std::string, Function>& env);
    void generate_mapped_buffers(Stmt& stmt,
        std::map<std::string, Function>& env,
        std::vector<HWXcel>& xcels);
  }
}

#endif
