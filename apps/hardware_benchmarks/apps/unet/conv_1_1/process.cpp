#include <cstdio>

#include "conv_1_1.h"

#include "halide_benchmark.h"
#include "HalideBuffer.h"

using namespace Halide::Tools;
using namespace Halide::Runtime;

int main(int argc, char **argv) {
  Buffer<uint8_t> input(16, 16, 32);
  Buffer<uint8_t> output(16, 16, 32);


  for (int c = 0; c < input.channels(); c++)
  for (int h = 0; h < input.height(); h++)
  for (int w = 0; w < input.width(); w++) {
    input(w, h, c) = (uint8_t)rand();
  }
  

  printf("Start compile!\n");
  conv_1_1(input, output);
  printf("Finish compile, start running!\n");

  // Timing code

  // Manually-tuned version
  double min_t_manual = benchmark(100, 20, [&]() {
      conv_1_1(input, output);
  });
  printf("Manually-tuned time: %gms\n", min_t_manual * 1e3);

  return 0;
  
}
