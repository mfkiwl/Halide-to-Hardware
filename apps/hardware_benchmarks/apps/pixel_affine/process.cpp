#include "pixel_affine.h"

#include <cstdlib>
#include <string>
#include <iostream>
#include <limits>

#include "HalideBuffer.h"
#include "halide_image_io.h"
using namespace Halide::Tools;
using namespace Halide::Runtime;

void process(Buffer<float> &input,
             Buffer<float> &coefficient,
             Buffer<float> &output) {
    // shape sanity check
    // get input size
    int input_width = input.dim(0).extent();
    int input_height = input.dim(1).extent();
    int input_channel = input.dim(2).extent();
    // get coefficient size
    int coefficient_width = coefficient.dim(0).extent();
    int coefficient_height = coefficient.dim(1).extent();
    int coefficient_channel_out = coefficient.dim(2).extent();
    int coefficient_channel_in = coefficient.dim(3).extent();
    // get output size
    int output_width = output.dim(0).extent();
    int output_height = output.dim(1).extent();
    int output_channel = output.dim(2).extent();
    // check width
    if (input_width != output_width) {
        std::cerr << "input_width: " << input_width << std::endl;
        std::cerr << "output_width: " << output_width << std::endl;
    };
    if (input_width != coefficient_width) {
        std::cerr << "input_width: " << input_width << std::endl;
        std::cerr << "coefficient_width: " << coefficient_width << std::endl;
    };
    if (output_width != coefficient_width) {
        std::cerr << "output_width: " << output_width << std::endl;
        std::cerr << "coefficient_width: " << coefficient_width << std::endl;
    };
    // check height
    if (input_height != output_height) {
        std::cerr << "input_height: " << input_height << std::endl;
        std::cerr << "output_height: " << output_height << std::endl;
    };
    if (input_height != coefficient_height) {
        std::cerr << "input_height: " << input_height << std::endl;
        std::cerr << "coefficient_height: " << coefficient_height << std::endl;
    };
    if (output_height != coefficient_height) {
        std::cerr << "output_height: " << output_height << std::endl;
        std::cerr << "coefficient_height: " << coefficient_height << std::endl;
    };
    // check channel
    if (input_channel != 3) {
        std::cerr << "input_channel: " << input_channel << std::endl;
    };
    if (output_channel != 3) {
        std::cerr << "output_channel: " << output_channel << std::endl;
    };
    if ((coefficient_channel_out != 3) || (coefficient_channel_in != 4)) {
        std::cerr << "transform shape: (" << coefficient_channel_out;
        std::cerr <<"," << coefficient_channel_out << ")" << std::endl;
    }
    // real work
    pixel_affine(input, coefficient, output);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage:\n\t./process in.png out.png " << std::endl;
        return 1;
    }

    // read float input
    Buffer<float> input = Halide::Tools::load_and_convert_image(argv[1]);
    // create transform
    Buffer<float> coefficient(input.width(), input.height(), 3, 4);
    for (int y = 0; y < coefficient.height(); ++y) {
        for (int x = 0; x < coefficient.width(); ++x) {
            // red
            coefficient(x, y, 0, 0) = 0;
            coefficient(x, y, 0, 1) = 0.9;
            coefficient(x, y, 0, 2) = 0;
            coefficient(x, y, 0, 3) = 0.01;
            // green
            coefficient(x, y, 1, 0) = 0.8;
            coefficient(x, y, 1, 1) = 0;
            coefficient(x, y, 1, 2) = 0;
            coefficient(x, y, 1, 3) = 0.01;
            // blue
            coefficient(x, y, 2, 0) = 0.3;
            coefficient(x, y, 2, 1) = 0.3;
            coefficient(x, y, 2, 2) = 0.3;
            coefficient(x, y, 2, 3) = 0.01;
        }
    }

    Buffer<float> output(input.width(), input.height(), input.channels());
    process(input, coefficient, output);

    convert_and_save_image(output, argv[2]);

    return 0;
}
