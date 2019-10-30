#include <iostream>
#include <limits>

#include "CodeGen_Garnet_SoC.h"
#include "CodeGen_Internal.h"
#include "IROperator.h"
#include "Simplify.h"

namespace Halide {
    namespace Internal {

        using std::ostream;
        using std::endl;
        using std::string;
        using std::vector;
        using std::ostringstream;
        using std::to_string;

        namespace {
            const string zynq_runtime_include = "#include \"halide_hw_api.h\"\n";
        }

        CodeGen_Garnet_SoC::CodeGen_Garnet_SoC( ostream &dest,
                                                Target target,
                                                OutputKind output_kind)
                : CodeGen_C(dest, target, output_kind) {
            stream << zynq_runtime_include;
        }

        void CodeGen_Garnet_SoC::visit(const Realize *op) {
            internal_assert(ends_with(op->name, ".stream") ||
                            ends_with(op->name, ".tap.stencil"));
            open_scope();
            string cma_buffer_name = op->name;
            cma_buffers.push_back(cma_buffer_name);

            do_indent();
            stream << "UBuffer " << print_name(cma_buffer_name) << ";\n";
            // Recurse
            print_stmt(op->body);
            close_scope(cma_buffer_name);
        }

        void CodeGen_Garnet_SoC::visit(const ProducerConsumer *op) {
            if (op->is_producer && starts_with(op->name, "_hls_target.")) {
                // reached the HW boundary
                /* Example C code we want to generate:
                   UBuffer kbufs[3];
                   kbufs[0] = kbuf_in0;
                   kbufs[1] = kbuf_in1;
                   kbufs[2] = kbuf_out;
                   int process_id = halide_hw_launch(kbufs);
                   halide_hw_sync(process_id);
                */
                // TODO check the order of buffer slices is consistent with
                // the order of DMA ports in the driver
                do_indent();
                stream << "UBuffer _cma_bufs[" << cma_buffers.size() << "];\n";
                for (size_t i = 0; i < cma_buffers.size(); i++) {
                    do_indent();
                    stream << "_cma_bufs[" << i << "] = " << print_name(cma_buffers[i]) << ";\n";
                }
                do_indent();
                stream << "int _process_id = halide_hw_launch(_cma_bufs);\n";
                do_indent();
                stream << "halide_hw_sync(_process_id);\n";

                cma_buffers.clear();
            } else {
                CodeGen_C::visit(op);
            }
        }

        void CodeGen_Garnet_SoC::visit(const Call *op) {
            ostringstream rhs;

            // render a CMA alloc call
            if (op->is_intrinsic("halide_hw_alloc")) {
                internal_assert(op->args.size() == 1);
                string buffer = print_expr(op->args[0]);
                rhs << "halide_hw_alloc(" << buffer << ")";
                print_assignment(op->type, rhs.str());
            }
            // render a CMA free call
            else if (op->is_intrinsic("halide_hw_free")) {
                internal_assert(op->args.size() == 1);
                string buffer = print_expr(op->args[0]);
                do_indent();
                stream << "halide_hw_free(" << buffer << ");\n";
            }
            // render an image slicing call
            else if (op->is_intrinsic("stream_subimage")) {
                /* IR:
                   stream_subimage(direction, buffer_var, stream_var,
                               address_of_subimage_origin,
                               dim_0_stride, dim_0_extent, ...)

                   C code:
                   halide_hw_slice_image(&buffer_var, &stream_var, address_of_subimage_origin, width, height);
            */
                internal_assert(op->args.size() >= 6);
                const Variable *buffer_var = op->args[1].as<Variable>();
                internal_assert(buffer_var && buffer_var->type == type_of<struct buffer_t *>());
                string buffer_name = print_expr(op->args[1]);
                string slice_name = print_expr(op->args[2]);
                string address_of_subimage_origin = print_expr(op->args[3]);

                string width, height;
                // TODO check the lower demesion matches the buffer depth
                // TODO static check that the slice is within the bounds of kernel buffer
                size_t arg_length = op->args.size();
                width = print_expr(op->args[arg_length - 3]);
                height = print_expr(op->args[arg_length - 1]);

                do_indent();
                stream << "halide_hw_slice_image("
                       << print_name(buffer_name) << ", &" << print_name(slice_name) << ", "
                       << address_of_subimage_origin << ", " << width << ", " << height << ");\n";
            } else if (op->name == "address_of") {
                std::ostringstream rhs;
                const Load *l = op->args[0].as<Load>();
                internal_assert(op->args.size() == 1 && l);
                rhs << "(("
                    << print_type(l->type.element_of()) // index is in elements, not vectors.
                    << " *)"
                    << print_name(l->name)
                    << " + "
                    << print_expr(l->index)
                    << ")";
                print_assignment(op->type, rhs.str());
            } else if (op->name == "buffer_to_stencil") {
                /**
                 * disguise tap value as buffer and handle that in the kernel driver,
                 * assuming tap values are one-dimensional array
                 * (at least for the current applications).
                 */
                internal_assert(op->args.size() == 2);
                do_indent();
                stream << "halide_hw_buffer_to_stencil("
                       << print_expr(op->args[0]) << ", &"
                       << print_expr(op->args[1]) << ");\n";
                id = "0"; // skip evaluation
            } else {
                CodeGen_C::visit(op);
            }
        }
    }
}
