#ifndef HALIDE_INJECTCMAINTRINSICS_H
#define HALIDE_INJECTCMAINTRINSICS_H

/** \file
 * Defines the lowering pass that inject CMA-related intrinsics
 */

#include <map>

#include "IR.h"

namespace Halide {

    namespace Internal {
        /**
         * Inject CMA allocation calls for buffers shared
         * between the CPU and Garnet
         * @param s The pipeline's IR
         * @param env Map of function names to Functions in the pipeline
         * @return Pipeline IR with possibly-inserted CMA calls
         */
        Stmt inject_cma_intrinsics(Stmt s, const std::map<std::string, Function> &env);
    }
}

#endif //HALIDE_INJECTCMAINTRINSICS_H
