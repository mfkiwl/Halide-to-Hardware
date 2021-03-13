#include "hardware_process_helper.h"
#include "halide_image_io.h"
#include <chrono>

#if defined(WITH_CPU)
   #include "modexp.h"
#endif

#if defined(WITH_CLOCKWORK)
    #include "rdai_api.h"
    #include "clockwork_sim_platform.h"
    #include "modexp_clockwork.h"
#endif

using namespace std;
using namespace Halide::Tools;
using namespace Halide::Runtime;

int main( int argc, char **argv ) {
  std::map<std::string, std::function<void()>> functions;
  ManyInOneOut_ProcessController<uint16_t> processor("modexp", {"a.png", "c.png"});

  #if defined(WITH_CPU)
      auto cpu_process = [&]( auto &proc ) {
		modexp(proc.inputs["a.png"], proc.inputs["c.png"], proc.output);
      };
      functions["cpu"] = [&](){ cpu_process( processor ); } ;
  #endif
  
  #if defined(WITH_CLOCKWORK)
      auto clockwork_process = [&]( auto &proc ) {
        RDAI_Platform *rdai_platform = RDAI_register_platform( &rdai_clockwork_sim_ops );
        if ( rdai_platform ) {
          printf( "[RUN_INFO] found an RDAI platform\n" );
          modexp(proc.inputs["a.png"], proc.inputs["c.png"], proc.output);
          RDAI_unregister_platform( rdai_platform );
        } else {
          printf("[RUN_INFO] failed to register RDAI platform!\n");
        }
      };
      functions["clockwork"] = [&](){ clockwork_process( processor ); };
  #endif

  // Add all defined functions
  processor.run_calls = functions;
  processor.inputs["a.png"] = Buffer<uint16_t>(1);
  processor.inputs["c.png"] = Buffer<uint16_t>(1);
  
  auto a = processor.inputs["a.png"];
  auto c = processor.inputs["c.png"];
  
  a(0) = 3;
  c(0) = 7;
  // a^100 % c
  
  processor.inputs_preset = true;
  processor.output = Buffer<uint16_t>(1);
  
  
  
  auto start = chrono::steady_clock::now();
  auto out = processor.process_command(argc, argv);
  auto end = chrono::steady_clock::now();
  
  
  
  auto output = processor.output;
  cout << output(0) << endl;
  
  
  cout << chrono::duration_cast<chrono::milliseconds>(end - start).count() << " ms" << endl;
  
  return out;
}