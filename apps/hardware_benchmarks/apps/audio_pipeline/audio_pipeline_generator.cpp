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

	  float sample_freq = 48000;
	  int num_samples = 868346;
	  float PI = atan(1)*4;
	  
	  float azi1 = -0.1f;
	  float elev1 = 3.14f / 2.0f;
	  float dis1 = 1.0f;
	  
	  float azi2 = 1.0f;
	  float elev2 = 0.0f;
	  float dis2 = 5.0f;
	  
	  float fSinAzim1 = sinf(azi1);
	  float fCosAzim1 = cosf(azi1);
	  float fSinElev1 = sinf(elev1);
	  float fCosElev1 = cosf(elev1);
	  float fSin2Azim1 = sinf(2.0f * azi1);
	  float fCos2Azim1 = cosf(2.0f * azi1);
	  float fSin2Elev1 = sinf(2.0f * elev1);
	  float fCos2Elev1 = cosf(2.0f * elev1);
	  
	  float fSinAzim2 = sinf(azi2);
	  float fCosAzim2 = cosf(azi2);
	  float fSinElev2 = sinf(elev2);
	  float fCosElev2 = cosf(elev2);
	  float fSin2Azim2 = sinf(2.0f * azi2);
	  float fCos2Azim2 = cosf(2.0f * azi2);
	  float fSin2Elev2 = sinf(2.0f * elev2);
	  float fCos2Elev2 = cosf(2.0f * elev2);
	  
	  float fSqrt32 = sqrt(3.f) / 2.0f;
	  float fSqrt58 = sqrt(5.0f / 8.0f);
	  float fSqrt152 = sqrt(15.0f) / 2.0f;
	  float fSqrt38 = sqrt(3.0f / 8.0f);
	  
	  
	  float coef1_1 = 1.0f;
	  float coef1_2 = fSinAzim1 * fCosElev1;
	  float coef1_3 = fSinElev1;
	  float coef1_4 = fCosAzim1 * fCosElev1;
	  float coef1_5 = fSqrt32 * (fSin2Azim1 * powf(fCosElev1, 2.0f));
	  float coef1_6 = fSqrt32 * (fSinAzim1 * fSin2Elev1);
	  float coef1_7 = 1.5f * powf(fSinElev1, 2.0f) - 0.5f;
	  float coef1_8 = fSqrt32 * (fCosAzim1 * fSin2Elev1);
	  float coef1_9 = fSqrt32 * (fCos2Azim1 * powf(fCosElev1, 2.0f));
	  float coef1_10 = fSqrt58 * (sinf(3.0f * azi1) * powf(fCosElev1, 3.0f));
	  float coef1_11 = fSqrt152 * (fSin2Azim1 * fSinElev1 * powf(fCosElev1, 2.0f));
	  float coef1_12 = fSqrt38 * (fSinAzim1 * fCosElev1 * (5.0f * powf(fSinElev1, 2.0f) - 1.0f));
	  float coef1_13 = fSinElev1 * (5.0f * powf(fSinElev1, 2.0f) - 3.0f) * 0.5f;
	  float coef1_14 = fSqrt38 * (fCosAzim1 * fCosElev1 * (5.0f * powf(fSinElev1, 2.0f) - 1.0f));
	  float coef1_15 = fSqrt152 * (fCos2Azim1 * fSinElev1 * powf(fCosElev1, 2.0f));
	  float coef1_16 = fSqrt58 * (cosf(3.0f * azi1) * powf(fCosElev1, 3.0f));



	  float coef2_1 = 1.0f;
	  float coef2_2 = fSinAzim2 * fCosElev2;
	  float coef2_3 = fSinElev2;
	  float coef2_4 = fCosAzim2 * fCosElev2;
	  float coef2_5 = fSqrt32 * (fSin2Azim2 * powf(fCosElev2, 2.0f));
	  float coef2_6 = fSqrt32 * (fSinAzim2 * fSin2Elev2);
	  float coef2_7 = 1.5f * powf(fSinElev2, 2.0f) - 0.5f;
	  float coef2_8 = fSqrt32 * (fCosAzim2 * fSin2Elev2);
	  float coef2_9 = fSqrt32 * (fCos2Azim2 * powf(fCosElev2, 2.0f));
	  float coef2_10 = fSqrt58 * (sinf(3.0f * azi2) * powf(fCosElev2, 3.0f));
	  float coef2_11 = fSqrt152 * (fSin2Azim2 * fSinElev2 * powf(fCosElev2, 2.0f));
	  float coef2_12 = fSqrt38 * (fSinAzim2 * fCosElev2 * (5.0f * powf(fSinElev2, 2.0f) - 1.0f));
	  float coef2_13 = fSinElev2 * (5.0f * powf(fSinElev2, 2.0f) - 3.0f) * 0.5f;
	  float coef2_14 = fSqrt38 * (fCosAzim2 * fCosElev2 * (5.0f * powf(fSinElev2, 2.0f) - 1.0f));
	  float coef2_15 = fSqrt152 * (fCos2Azim2 * fSinElev2 * powf(fCosElev2, 2.0f));
	  float coef2_16 = fSqrt58 * (cosf(3.0f * azi2) * powf(fCosElev2, 3.0f));


    void generate() {
	  Var t, c;

      Func hw_input1, hw_input2, hw_output;
	  
	  Func normalize1, normalize2;
	  
	  Func ambisonic1_1, ambisonic2_1, ambisonic1;
	  Func ambisonic1_2, ambisonic2_2, ambisonic2;
	  Func ambisonic1_3, ambisonic2_3, ambisonic3;
	  Func ambisonic1_4, ambisonic2_4, ambisonic4;
	  Func ambisonic1_5, ambisonic2_5, ambisonic5;
	  Func ambisonic1_6, ambisonic2_6, ambisonic6;
	  Func ambisonic1_7, ambisonic2_7, ambisonic7;
	  Func ambisonic1_8, ambisonic2_8, ambisonic8;
	  Func ambisonic1_9, ambisonic2_9, ambisonic9;
	  Func ambisonic1_10, ambisonic2_10, ambisonic10;
	  Func ambisonic1_11, ambisonic2_11, ambisonic11;
	  Func ambisonic1_12, ambisonic2_12, ambisonic12;
	  Func ambisonic1_13, ambisonic2_13, ambisonic13;
	  Func ambisonic1_14, ambisonic2_14, ambisonic14;
	  Func ambisonic1_15, ambisonic2_15, ambisonic15;
	  Func ambisonic1_16, ambisonic2_16, ambisonic16;
	  
	  Func scale_up;
	  
      hw_input1(t, c) = in1(t, c);
      hw_input2(t, c) = in2(t, c);
	  
	  normalize1(t, c) = cast<float>(hw_input1(t, c)) / 32767.0f;
	  normalize2(t, c) = cast<float>(hw_input2(t, c)) / 32767.0f;
	  
	  
	  ambisonic1_1(t, c) = normalize1(t, c) * coef1_1; // W
	  ambisonic1_2(t, c) = normalize1(t, c) * coef1_2; // Y
      ambisonic1_3(t, c) = normalize1(t, c) * coef1_3; // Z
      ambisonic1_4(t, c) = normalize1(t, c) * coef1_4; // X
      ambisonic1_5(t, c) = normalize1(t, c) * coef1_5; // V
      ambisonic1_6(t, c) = normalize1(t, c) * coef1_6; // T
      ambisonic1_7(t, c) = normalize1(t, c) * coef1_7; // R
      ambisonic1_8(t, c) = normalize1(t, c) * coef1_8; // S
      ambisonic1_9(t, c) = normalize1(t, c) * coef1_9; // U
      ambisonic1_10(t, c) = normalize1(t, c) * coef1_10; // Q
      ambisonic1_11(t, c) = normalize1(t, c) * coef1_11; // O
      ambisonic1_12(t, c) = normalize1(t, c) * coef1_12; // M
      ambisonic1_13(t, c) = normalize1(t, c) * coef1_13; // K
      ambisonic1_14(t, c) = normalize1(t, c) * coef1_14; // L
      ambisonic1_15(t, c) = normalize1(t, c) * coef1_15; // N
      ambisonic1_16(t, c) = normalize1(t, c) * coef1_16; // P
	  
	  
	  ambisonic2_1(t, c) = normalize2(t, c) * coef2_1; // W
	  ambisonic2_2(t, c) = normalize2(t, c) * coef2_2; // Y
      ambisonic2_3(t, c) = normalize2(t, c) * coef2_3; // Z
      ambisonic2_4(t, c) = normalize2(t, c) * coef2_4; // X
      ambisonic2_5(t, c) = normalize2(t, c) * coef2_5; // V
      ambisonic2_6(t, c) = normalize2(t, c) * coef2_6; // T
      ambisonic2_7(t, c) = normalize2(t, c) * coef2_7; // R
      ambisonic2_8(t, c) = normalize2(t, c) * coef2_8; // S
      ambisonic2_9(t, c) = normalize2(t, c) * coef2_9; // U
      ambisonic2_10(t, c) = normalize2(t, c) * coef2_10; // Q
      ambisonic2_11(t, c) = normalize2(t, c) * coef2_11; // O
      ambisonic2_12(t, c) = normalize2(t, c) * coef2_12; // M
      ambisonic2_13(t, c) = normalize2(t, c) * coef2_13; // K
      ambisonic2_14(t, c) = normalize2(t, c) * coef2_14; // L
      ambisonic2_15(t, c) = normalize2(t, c) * coef2_15; // N
      ambisonic2_16(t, c) = normalize2(t, c) * coef2_16; // P
	  
	  
	  
	  ambisonic1(t, c) = ambisonic1_1(t, c) + ambisonic2_1(t, c);
	  ambisonic2(t, c) = ambisonic1_2(t, c) + ambisonic2_2(t, c);
	  ambisonic3(t, c) = ambisonic1_3(t, c) + ambisonic2_3(t, c);
	  ambisonic4(t, c) = ambisonic1_4(t, c) + ambisonic2_4(t, c);
	  ambisonic5(t, c) = ambisonic1_5(t, c) + ambisonic2_5(t, c);
	  ambisonic6(t, c) = ambisonic1_6(t, c) + ambisonic2_6(t, c);
	  ambisonic7(t, c) = ambisonic1_7(t, c) + ambisonic2_7(t, c);
	  ambisonic8(t, c) = ambisonic1_8(t, c) + ambisonic2_8(t, c);
	  ambisonic9(t, c) = ambisonic1_9(t, c) + ambisonic2_9(t, c);
	  ambisonic10(t, c) = ambisonic1_10(t, c) + ambisonic2_10(t, c);
	  ambisonic11(t, c) = ambisonic1_11(t, c) + ambisonic2_11(t, c);
	  ambisonic12(t, c) = ambisonic1_12(t, c) + ambisonic2_12(t, c);
	  ambisonic13(t, c) = ambisonic1_13(t, c) + ambisonic2_13(t, c);
	  ambisonic14(t, c) = ambisonic1_14(t, c) + ambisonic2_14(t, c);
	  ambisonic15(t, c) = ambisonic1_15(t, c) + ambisonic2_15(t, c);
	  ambisonic16(t, c) = ambisonic1_16(t, c) + ambisonic2_16(t, c);
	  
	  
	  
	  
	  
	  
	  scale_up(t, c) = cast<int16_t>(clamp(ambisonic2_1(t, c) +
										   ambisonic2_2(t, c) +
										   ambisonic2_3(t, c) +
										   ambisonic2_4(t, c) +
										   ambisonic2_5(t, c) +
										   ambisonic2_6(t, c) +
										   ambisonic2_7(t, c) +
										   ambisonic2_8(t, c) +
										   ambisonic2_9(t, c) +
										   ambisonic2_10(t, c) +
										   ambisonic2_11(t, c) +
										   ambisonic2_12(t, c) +
										   ambisonic2_13(t, c) +
										   ambisonic2_14(t, c) +
										   ambisonic2_15(t, c) +
										   ambisonic2_16(t, c), -1.0f, 1.0f) * 32767.0f);

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
        normalize2.compute_at(hw_output, to);
        normalize1.compute_at(hw_output, to);


        hw_input2.stream_to_accelerator();
        hw_input1.stream_to_accelerator();
          
      } else 
	  {    
			scale_up.compute_root();
			normalize2.compute_root();
			normalize1.compute_root();
      }
    }
	
	
  };

}

HALIDE_REGISTER_GENERATOR(AudioPipeline, audio_pipeline)