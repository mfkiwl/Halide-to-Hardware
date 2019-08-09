// Pixel-wise Affine Transform
// [R_{out}, G_{out}, B_{out}] =
// [[M(0, 0), M(0, 1), M(0, 2)],
//  [M(1, 0), M(1, 1), M(1, 2)],    *   [R_{in}, G_{in}, B_{in}]
//  [M(2, 0), M(2, 1), M(2, 2)]]
//  +
//  [Off(0), Off(1), Off(2)]
#include "Halide.h"

using namespace Halide;

class PixelAffineGenerator : public Halide::Generator<PixelAffineGenerator> {
public:
    Input<Buffer<int8_t>> input{"input", 3};
    Input<Buffer<uint8_t>> coefficient{"coefficient", 4};

    Output<Buffer<int8_t>> output{"output", 3};

    void generate() {

        Var x("x"), y("y"), c("c"), d("d");

        Expr height = input.dim(0).extent();
        Expr width = input.dim(1).extent();

        Func hw_input("hw_input");
        hw_input(x, y, c) = cast<int16_t>(input(x, y, c));

        Func hw_coef;
        hw_coef(x, y, c, d) = cast<int16_t>(coefficient(x, y, c, d));

        // apply affine transform
        // add offsets first
        Func affined;
        RDom r(0, 3);
        affined(x, y, c) = hw_coef(x, y, c, 3);
        affined(x, y, c) += hw_coef(x, y, c, r) * hw_input(x, y, r);

        Func hw_output("hw_output");
        hw_output(x, y, c) = cast<int8_t>(affined(x, y, c));
        output(x, y, c) = hw_output(x, y, c);

        output.bound(x, 0, 62);
        output.bound(y, 0, 62);
        output.bound(c, 0, 3);

        hw_coef.bound(x, 0, 62);
        hw_coef.bound(y, 0, 62);
        hw_coef.bound(c, 0, 3);
        hw_coef.bound(d, 0, 4);

        /* THE SCHEDULE */
        // CoreIR schedule
        if (get_target().has_feature(Target::CoreIR)) {
            Var xi,yi, xo,yo;
          
            hw_input.compute_root();
            hw_output.compute_root();

            hw_output.tile(x,y, xo,yo, xi,yi, 62, 62)
                     .reorder(xi,yi,c,xo,yo)
                     .hw_accelerate(xi, xo);

            affined.reorder(x, y, c);
            affined.linebuffer();

            hw_coef.stream_to_accelerator();
            hw_input.stream_to_accelerator();

        }
        // CPU schedule
        else {
            // left blank
        }
    }
};

HALIDE_REGISTER_GENERATOR(PixelAffineGenerator, pixel_affine)