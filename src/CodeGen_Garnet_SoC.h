#ifndef HALIDE_CODEGEN_GARNET_SOC_H
#define HALIDE_CODEGEN_GARNET_SOC_H

/** \file
 *
 * Defines a code generator for GarnetSoC
 */
#include "CodeGen_C.h"
#include "Module.h"
#include "Scope.h"

namespace Halide {

    namespace Internal {

        /**
         * This codegen emits C code for GarnetSoC from given Halide stmt
         * that contains hardware accelerators.
         * The interface to the hardware accelerator is replaced
         * with runtime Linux driver calls.
         */
        class CodeGen_Garnet_SoC : public CodeGen_C {
        public:

            CodeGen_Garnet_SoC( std::ostream &dest,
                                Target target,
                                OutputKind outputKind = OutputKind::CImplementation);

        protected:
            std::vector<std::string> cma_buffers;

            using CodeGen_C::visit;

            void visit(const Realize *);
            void visit(const ProducerConsumer *);
            void visit(const Call *);
        };
    }
}

#endif //HALIDE_CODEGEN_GARNET_SOC_H
