#include "Halide.h"

namespace {

using namespace Halide;
using namespace ConciseCasts;

class UnitTestFPComp : public Halide::Generator<UnitTestFPComp> {
public:
    Input<Buffer<uint8_t>>  input{"input", 2};
    Output<Buffer<uint8_t>> output{"output", 2};

    void generate() {
        /* THE ALGORITHM */

        Var x("x"), y("y");

        Func hw_input("hw_input");
        hw_input(x, y) = cast<float>(input(x, y));

        Func lt, gt, le, ge, eq_t, neq_t;
        Expr const_k = float(100.3f);
        lt(x,y) = hw_input(x,y) < const_k;
        ge(x,y) = hw_input(x,y) >= Expr(float(80.2f));
        le(x,y) = hw_input(x,y) <= Expr(float(42.42f));
        gt(x,y) = hw_input(x,y) > Expr(float(9.6f));
        neq_t(x,y)= hw_input(x,y) != Expr(float(6.f));
        eq_t(x,y) = hw_input(x,y) == Expr(float(66.f));

        Func hw_output("hw_output");
        hw_output(x, y) = select((lt(x,y) && ge(x,y)) ||
                                 (le(x,y) && gt(x,y) && neq_t(x,y)) ||
                                 eq_t(x,y),
                                 255, 0);
        output(x, y) = cast<uint8_t>(hw_output(x,y));

        /* THE SCHEDULE */
        if (get_target().has_feature(Target::CoreIR)) {
          Var xi,yi, xo,yo;
          
          hw_input.compute_root();
          hw_output.compute_root();
          
          hw_output.tile(x,y, xo,yo, xi,yi, 64, 64)
            .hw_accelerate(xi, xo);

          hw_input.stream_to_accelerator();
          
        } else {  // schedule to CPU
          output.compute_root();
        }
        
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(UnitTestFPComp, fp_comp)
