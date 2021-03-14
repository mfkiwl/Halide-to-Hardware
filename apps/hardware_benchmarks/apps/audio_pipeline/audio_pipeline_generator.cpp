#include "Halide.h"

namespace {
  using namespace Halide;
  using namespace Halide::ConciseCasts;
  using namespace std;

  class AudioPipeline : public Halide::Generator<AudioPipeline> {
    
  public:
    Input<Buffer<int16_t>>  in1{"in1", 2};
    Input<Buffer<int16_t>>  in2{"in2", 2};
    Output<Buffer<int16_t>> output{"output", 2};

    void generate() {
		
	  float sample_freq = 48000;
	  int num_samples = 868346;
	  float PI = atan(1)*4;


	  Var t, c;

      Func hw_input1, hw_input2, hw_output;
	  Func normalize, scale_up;
	  
      hw_input1(t, c) = in1(t, c);
      hw_input2(t, c) = in2(t, c);
	  
	  normalize(t, c) = cast<float>(hw_input1(t, c)) / 32767.0f + cast<float>(hw_input2(t, c)) / 32767.0f;
	  scale_up(t, c) = cast<int16_t>(clamp(3.0f * normalize(t, c), -1.0f, 1.0f) * 32767.0f);

	  hw_output(t, c) = scale_up(t, c);
	  output(t, c) = hw_output(t, c);


	  if (get_target().has_feature(Target::Clockwork)) {
		  
		Var ti, to;  
		  
        output.bound(t, 0, num_samples);
        output.bound(c, 0, 1);

        hw_output.compute_root();

        hw_output.split(t, to, ti, num_samples)
          .reorder(ti, c, to)
          .hw_accelerate(ti, to);


        scale_up.compute_at(hw_output, to);
        normalize.compute_at(hw_output, to);


        hw_input2.stream_to_accelerator();
        hw_input1.stream_to_accelerator();
          
      } else 
	  {    
			scale_up.compute_root();
			normalize.compute_root();
      }
    }
	
	
  };

}

HALIDE_REGISTER_GENERATOR(AudioPipeline, audio_pipeline)