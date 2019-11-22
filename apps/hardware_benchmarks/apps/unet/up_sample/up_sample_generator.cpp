#include "Halide.h"

namespace {

using namespace Halide;

class NearestNeighborKernel : public Halide::Generator<NearestNeighborKernel> {
public:
    Input<Buffer<int8_t>>  input{"input", 3};
    Output<Buffer<uint8_t>> output{"output", 3};

    void generate() {
        /* THE ALGORITHM */
        int factor = 2;
        Expr width = input.dim(0).extent();
        Expr height = input.dim(1).extent();

        Var x("x"), y("y"), z("z");

        Func nearest_neighbor("nearest_neighbor");

        Func hw_input("hw_input");
        hw_input(x, y, z) = cast<int16_t>(input(x, y, z));

        nearest_neighbor(x, y, z) = hw_input(x / factor, y / factor, z);

        Func hw_output("hw_output");
        hw_output(x, y, z) = cast<uint8_t>(nearest_neighbor(x, y, z));
        output(x, y, z) = hw_output(x, y, z);

        /* THE SCHEDULE */
        if (get_target().has_feature(Target::CoreIR)) {
            nearest_neighbor(x, 0, 20);
            nearest_neighbor(y, 0, 20);
            nearest_neighbor(z, 0, 128);
            output.bound(x, 0, 20);
            output.bound(y, 0, 20);
            output.bound(z, 0, 128);
            
            Var xi, yi, xo, yo;
            hw_input.compute_root();
            hw_output.compute_root();

            hw_output.tile(x, y, xo, yo, xi, yi, 64, 64)
              .reorder(xi,yi,z,xo,yo)
              .hw_accelerate(xi, xo);

            nearest_neighbor.linebuffer();

            hw_input.stream_to_accelerator();
        } else { // schedule to CPU
            nearest_neighbor.compute_root();
        }
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(NearestNeighborKernel, up_sample)
