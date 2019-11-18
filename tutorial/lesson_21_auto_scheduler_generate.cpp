// Halide tutorial lesson 21: Auto-Scheduler

// So far we have written Halide schedules by hand, but it is also possible to
// ask Halide to suggest a reasonable schedule. We call this auto-scheduling.
// This lesson demonstrates how to use the auto-scheduler to generate a
// copy-pasteable CPU schedule that can be subsequently improved upon.

// On linux or os x, you can compile and run it like so:

// g++ lesson_21_auto_scheduler_generate.cpp ../tools/GenGen.cpp -g -std=c++11 -fno-rtti -I ../include -L ../bin -lHalide -lpthread -ldl -o lesson_21_generate
// export LD_LIBRARY_PATH=../bin   # For linux
// export DYLD_LIBRARY_PATH=../bin # For OS X
// ./lesson_21_generate -o . -g auto_schedule_gen -f auto_schedule_false -e static_library,h,schedule target=host auto_schedule=false
// ./lesson_21_generate -o . -g auto_schedule_gen -f auto_schedule_true -e static_library,h,schedule target=host auto_schedule=true machine_params=32,16777216,40
// g++ lesson_21_auto_scheduler_run.cpp -std=c++11 -I ../include -I ../tools auto_schedule_false.a auto_schedule_true.a -ldl -lpthread -o lesson_21_run
// ./lesson_21_run

// If you have the entire Halide source tree, you can also build it by
// running:
//    make tutorial_lesson_21_auto_scheduler_run
// in a shell with the current directory at the top of the halide
// source tree.

#include "Halide.h"
#include <stdio.h>

using namespace Halide;

// We will define a generator to auto-schedule.
class AutoScheduled : public Halide::Generator<AutoScheduled> {
public:
      Input<Buffer<uint8_t>>  input{"input", 2};
    Output<Buffer<uint8_t>> output{"output", 2};

  int ksize = 3;
  int imgsize = 62;

    void generate() {
        /* THE ALGORITHM */

        //Var x("x"), y("y");

        Func kernel("kernel");
        Func conv("conv");
        RDom r(0, ksize,               0, ksize);

        kernel(x,y) = 0;
        kernel(0,0) = 17;      kernel(0,1) = 4;      kernel(0,2) = 6;
        kernel(1,0) = 7;      kernel(1,1) = 19;       kernel(1,2) = 4;
        kernel(2,0) = 5;      kernel(2,1) = 21;      kernel(2,2) = 15;

        conv(x, y) = 0;

        Func hw_input("hw_input");
        hw_input(x, y) = cast<uint16_t>(input(x, y));
        conv(x, y)  += kernel(r.x, r.y) * hw_input(x + r.x, y + r.y);
        //conv(x,y) =
        //  kernel(0,0)*hw_input(x,y) +
        //  kernel(1,0)*hw_input(x+1,y) +
        //  kernel(2,0)*hw_input(x+2,y) +
        //  kernel(0,1)*hw_input(x,y+1) +
        //  kernel(1,1)*hw_input(x+1,y+1) +
        //  kernel(2,1)*hw_input(x+2,y+1) +
        //  kernel(0,2)*hw_input(x,y+2) +
        //  kernel(1,2)*hw_input(x+1,y+2) +
        //  kernel(2,2)*hw_input(x+2,y+2);

        Func hw_output("hw_output");
        hw_output(x, y) = cast<uint8_t>(conv(x, y));
        output(x, y) = hw_output(x,y);

    }

    void schedule() {
        if (auto_schedule) {
            // The auto-scheduler requires estimates on all the input/output
            // sizes and parameter values in order to compare different
            // alternatives and decide on a good schedule.

            // To provide estimates (min and extent values) for each dimension
            // of the input images ('input', 'filter', and 'bias'), we use the
            // set_bounds_estimate() method. set_bounds_estimate() takes in
            // (min, extent) of the corresponding dimension as arguments.
            input.dim(0).set_bounds_estimate(0, 64);
            input.dim(1).set_bounds_estimate(0, 64);
            //input.dim(2).set_bounds_estimate(0, 2);

            // input.dim(0).set_bounds_estimate(0, 1024);
            // input.dim(1).set_bounds_estimate(0, 1024);
            // input.dim(2).set_bounds_estimate(0, 3);

            // To provide estimates on the parameter values, we use the
            // set_estimate() method.
            // factor.set_estimate(2.0f);

            // To provide estimates (min and extent values) for each dimension
            // of pipeline outputs, we use the estimate() method. estimate()
            // takes in (dim_name, min, extent) as arguments.
            // output1.estimate(x, 0, 1024)
            //        .estimate(y, 0, 1024);

            // output2.estimate(x, 0, 1024)
            //        .estimate(y, 0, 1024);
            output.estimate(x, 0, 62)
                  .estimate(y, 0, 62);
                  //.estimate(w, 0, 4);

            // Technically, the estimate values can be anything, but the closer
            // they are to the actual use-case values, the better the generated
            // schedule will be.

            // To auto-schedule the the pipeline, we don't have to do anything else:
            // every Generator implicitly has a GeneratorParam named "auto_schedule";
            // if this is set to true, Halide will call auto_schedule() on all of
            // our pipeline's outputs automatically.

            // Every Generator also implicitly has a GeneratorParams named "machine_params",
            // which allows you to specify characteristics of the machine architecture
            // for the auto-scheduler; it's generally specified in your Makefile.
            // If none is specified, the default machine parameters for a generic CPU
            // architecture will be used by the auto-scheduler.

            // Let's see some arbitrary but plausible values for the machine parameters.
            //
            //      const int kParallelism = 32;
            //      const int kLastLevelCacheSize = 16 * 1024 * 1024;
            //      const int kBalance = 40;
            //      MachineParams machine_params(kParallelism, kLastLevelCacheSize, kBalance);
            //
            // The arguments to MachineParams are the maximum level of parallelism
            // available, the size of the last-level cache (in KB), and the ratio
            // between the cost of a miss at the last level cache and the cost
            // of arithmetic on the target architecture, in that order.

            // Note that when using the auto-scheduler, no schedule should have
            // been applied to the pipeline; otherwise, the auto-scheduler will
            // throw an error. The current auto-scheduler cannot handle a
            // partially-scheduled pipeline.

            // If HL_DEBUG_CODEGEN is set to 3 or greater, the schedule will be dumped
            // to stdout (along with much other information); a more useful way is
            // to add "schedule" to the -e flag to the Generator. (In CMake and Bazel,
            // this is done using the "extra_outputs" flag.)

            // The generated schedule that is dumped to file is an actual
            // Halide C++ source, which is readily copy-pasteable back into
            // this very same source file with few modifications. Programmers
            // can use this as a starting schedule and iteratively improve the
            // schedule. Note that the current auto-scheduler is only able to
            // generate CPU schedules and only does tiling, simple vectorization
            // and parallelization. It doesn't deal with line buffering, storage
            // reordering, or factoring reductions.

            // At the time of writing, the auto-scheduler will produce the
            // following schedule for the estimates and machine parameters
            // declared above when run on this pipeline:
            //
            // Var x_i("x_i");
            // Var x_i_vi("x_i_vi");
            // Var x_i_vo("x_i_vo");
            // Var x_o("x_o");
            // Var x_vi("x_vi");
            // Var x_vo("x_vo");
            // Var y_i("y_i");
            // Var y_o("y_o");
            //
            // Func Ix = pipeline.get_func(4);
            // Func Iy = pipeline.get_func(7);
            // Func gray = pipeline.get_func(3);
            // Func harris = pipeline.get_func(14);
            // Func output1 = pipeline.get_func(15);
            // Func output2 = pipeline.get_func(16);
            //
            // {
            //     Var x = Ix.args()[0];
            //     Ix
            //         .compute_at(harris, x_o)
            //         .split(x, x_vo, x_vi, 8)
            //         .vectorize(x_vi);
            // }
            // {
            //     Var x = Iy.args()[0];
            //     Iy
            //         .compute_at(harris, x_o)
            //         .split(x, x_vo, x_vi, 8)
            //         .vectorize(x_vi);
            // }
            // {
            //     Var x = gray.args()[0];
            //     gray
            //         .compute_at(harris, x_o)
            //         .split(x, x_vo, x_vi, 8)
            //         .vectorize(x_vi);
            // }
            // {
            //     Var x = harris.args()[0];
            //     Var y = harris.args()[1];
            //     harris
            //         .compute_root()
            //         .split(x, x_o, x_i, 256)
            //         .split(y, y_o, y_i, 128)
            //         .reorder(x_i, y_i, x_o, y_o)
            //         .split(x_i, x_i_vo, x_i_vi, 8)
            //         .vectorize(x_i_vi)
            //         .parallel(y_o)
            //         .parallel(x_o);
            // }
            // {
            //     Var x = output1.args()[0];
            //     Var y = output1.args()[1];
            //     output1
            //         .compute_root()
            //         .split(x, x_vo, x_vi, 8)
            //         .vectorize(x_vi)
            //         .parallel(y);
            // }
            // {
            //     Var x = output2.args()[0];
            //     Var y = output2.args()[1];
            //     output2
            //         .compute_root()
            //         .split(x, x_vo, x_vi, 8)
            //         .vectorize(x_vi)
            //         .parallel(y);
            // }

        } else {
            // This is where you would declare the schedule you have written by
            // hand or paste the schedule generated by the auto-scheduler.
            // We will use a naive schedule here to compare the performance of
            // the autoschedule with a basic schedule.
        }
    }
private:
    Var x{"x"}, y{"y"};
};

// As in lesson 15, we register our generator and then compile this
// file along with tools/GenGen.cpp.
HALIDE_REGISTER_GENERATOR(AutoScheduled, auto_schedule_gen)

// After compiling this file, see how to use it in
// lesson_21_auto_scheduler_run.cpp
