#include <cstdio>
#include "hardware_process_helper.h"
#include "hardware_image_helpers.h"
#include "halide_image_io.h"

#if defined(WITH_CPU)
   #include "audio_pipeline.h"
#endif

#if defined(WITH_CLOCKWORK)
    #include "rdai_api.h"
    #include "clockwork_sim_platform.h"
    #include "audio_pipeline_clockwork.h"
#endif

using namespace Halide::Tools;
using namespace Halide::Runtime;
using namespace std;

int main( int argc, char **argv ) {
  std::map<std::string, std::function<void()>> functions;
  ManyInOneOut_ProcessController<int16_t> processor("audio_pipeline", {"in1.png", "in2.png"});

  #if defined(WITH_CPU)
      auto cpu_process = [&]( auto &proc ) {
        audio_pipeline( proc.inputs["in1.png"], proc.inputs["in2.png"], proc.output );
      };
      functions["cpu"] = [&](){ cpu_process( processor ); } ;
  #endif
  
  #if defined(WITH_CLOCKWORK)
      auto clockwork_process = [&]( auto &proc ) {
        RDAI_Platform *rdai_platform = RDAI_register_platform( &rdai_clockwork_sim_ops );
        if ( rdai_platform ) {
          printf( "[RUN_INFO] found an RDAI platform\n" );
          audio_pipeline( proc.inputs["in1.png"], proc.inputs["in2.png"], proc.output );
          RDAI_unregister_platform( rdai_platform );
        } else {
          printf("[RUN_INFO] failed to register RDAI platform!\n");
        }
      };
      functions["clockwork"] = [&](){ clockwork_process( processor ); };
  #endif


  processor.run_calls = functions;
  
  
  AudioFile<int16_t> f1;
  readAudioData<int16_t>("radio_test.wav", &f1);
  
  AudioFile<int16_t> f2;
  readAudioData<int16_t>("lecture_test.wav", &f2);
  

  processor.inputs["in1.png"]   = f1.data;
  processor.inputs["in2.png"]   = f2.data;
  
  auto in1 = processor.inputs["in1.png"];
  auto in2 = processor.inputs["in2.png"];
  
  
  cout << "*********************************************************" << endl;
  cout << "radio_test: " << in1.dim(0).extent() << "   " << in1.dim(1).extent() << endl;
  cout << "lecture_test: " << in2.dim(0).extent() << "   " << in2.dim(1).extent() << endl;
  cout << "*********************************************************" << endl;
  
  
  processor.output  = Buffer<int16_t>(in1.dim(0).extent(), in1.dim(1).extent());
  processor.inputs_preset = true;

  auto ret_value = processor.process_command(argc, argv);


  AudioFile<int16_t> outAudio;
  outAudio.header = f1.header;
  outAudio.data = processor.output;
  writeAudioData<int16_t>("bin/output.wav", outAudio);
  
  auto out = processor.output;

  return ret_value;
}