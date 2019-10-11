#include "coreir.h"
#include "coreir/simulator/interpreter.h"
#include "coreir/libs/commonlib.h"
#include "coreir/libs/float.h"

#include "Halide.h"

#include "halide_image_io.h"
#include <stdio.h>
#include <iostream>

#include <fstream>

using namespace CoreIR;
using namespace Halide;
using namespace Halide::Tools;
using namespace std;

std::string GREEN = "\033[32m";
std::string RED = "\033[31m";
std::string RESET = "\033[0m";

void runCmd(const std::string& cmd) {
  cout << "Running command: " << cmd << endl;
  int res = system(cmd.c_str());
  assert(res == 0);
}

template<typename T>
void printBuffer(T& inputBuf, std::ostream& out) {
  if (inputBuf.dimensions() <= 2 || inputBuf.channels() == 1) {
    for (int i = inputBuf.top(); i <= inputBuf.bottom(); i++) {
      for (int j = inputBuf.left(); j <= inputBuf.right(); j++) {
        out << inputBuf(j, i) << " ";
      }
      out << endl;
    }
  } else {
    for (int c = 0; c < inputBuf.channels(); c++) {
      out << "------------------------------------------" << endl;
      out << "Channel " << c << endl;
      out << "------------------------------------------" << endl;
      for (int i = inputBuf.top(); i <= inputBuf.bottom(); i++) {
        for (int j = inputBuf.left(); j <= inputBuf.right(); j++) {
          out << inputBuf(j, i, c) << " ";
        }
        out << endl;
      }
    }
  }
}

template<typename T>
class Point {
  public:
    std::vector<T> values;
    std::vector<std::string> names;

    std::vector<std::string> dimensionNames() const { return names; }

    int numDims() { return values.size(); }

    void setCoord(const std::string& name, const T& newVal) {
      auto ind = indexOf(name);
      values[ind] = newVal;
    }

    int indexOf(const std::string& name) const {
      for (int i = 0; i < names.size(); i++) {
        if (names[i] == name) {
          return i;
        }
      }
      assert(false);
    }

    int coord(const std::string& name) const {
      for (int i = 0; i < names.size(); i++) {
        if (names[i] == name) {
          return values[i];
        }
      }
      assert(false);
    }

    Point<T> appendDim(const std::string& name, const int i) {
      Point<T> fresh = *this;
      fresh.names.push_back(name);
      fresh.values.push_back(i);
      return fresh;
    }

};

template<typename T>
std::ostream& operator<<(std::ostream& out, const Point<T>& pt) {
  out << "(";
  for (int i = 0; i < (int) pt.values.size(); i++) {
    out << pt.values[i];
    if (i < ((int) pt.values.size()) - 1) {
      out << ", ";
    }
  }
  out << ")";
  return out;
}

template<typename T>
class CoordinateVector {
  public:

    std::vector<T> values;
    std::vector<std::string> names;
    std::vector<T> bounds;
    std::vector<T> increments;

    bool finished;

    CoordinateVector(vector<std::string> names_, vector<T> bounds_) : names(names_), bounds(bounds_), finished(false) {
      values.resize(names.size());
      increments.resize(names.size());
      for (int i = 0; i < (int) bounds.size(); i++) {
        values[i] = 0;
        increments[i] = 1;
      }
    }

    CoordinateVector(vector<std::string>& names_, vector<T>& bounds_) : names(names_), bounds(bounds_), finished(false) {
      values.resize(names.size());
      for (int i = 0; i < (int) bounds.size(); i++) {
        values[i] = 0;
      }
    }

    int indexOf(const std::string& name) const {
      for (int i = 0; i < (int) names.size(); i++) {
        auto cN = names[i];
        if (cN == name) {
          return i;
        }
      }

      assert(false);
    }

    void setIncrement(const std::string& name, const int inc) {
      int val = indexOf(name);
      increments[val] = inc;
    }

    int coord(const std::string& str) const {
      for (int i = 0; i < (int) names.size(); i++) {
        auto cN = names[i];
        if (cN == str) {
          return values[i];
        }
      }

      assert(false);
    }

    int intervalEnd(const std::string& name) const {
      int ind = indexOf(name);
      return intervalEnd(ind);
    }

    int intervalEnd(const int ind) const {
      return std::min(values[ind] + increments[ind] - 1, bounds[ind]);
    }

    std::string coordString() const {
      std::string str = "{";
      for (int i = 0; i < ((int) bounds.size()); i++) {
        str += "[" + std::to_string(values[i]) + ", " + std::to_string(intervalEnd(i)) + "] : " + std::to_string(bounds[i]);
        if (i < ((int) bounds.size()) - 1) {
          str += ", ";
        }
      }
      str += "}";
      return str;
    }

    std::vector<Point<T> > allPoints(const std::vector<Point<T> >& startingPts, const int i) const {
      assert(startingPts.size() > 0);
      assert(i < ((int) values.size()));

      std::vector<Point<T> > newPoints;
      for (auto pt : startingPts) {
        for (int b = values[i]; b <= intervalEnd(i); b++) {
          newPoints.push_back(pt.appendDim(names[i], b));
        }
      }

      //cout << "Newpoints at " << i << " has size " << newPoints.size() << endl;
      if (i == ((int) values.size()) - 1) {
        return newPoints;
      }

      auto finalPoints = allPoints(newPoints, i + 1);
      return finalPoints;
    }

    int intervalMin(const std::string& name) const {
      return values[indexOf(name)];
    }

    int intervalWidth(const std::string& name) const {
      return increments[indexOf(name)];
    }
    
    Point<T> relativeWindowPosition(const Point<T>& pt) const {
      Point<T> rel = pt;
      for (auto dim : pt.dimensionNames()) {
        rel.setCoord(dim, pt.coord(dim) % intervalWidth(dim));
      }
      return rel;
    }

    std::vector<Point<T> > currentWindowRelative() const {
      auto window = currentWindow();
      vector<Point<T> > relWindow;
      for (auto pt : window) {
        relWindow.push_back(relativeWindowPosition(pt));
      }

      return relWindow;
    }
    
    std::vector<Point<T> > currentWindow() const {
      std::vector<Point<T> > window{{}};
      assert(window.size() == 1);
      auto finalWindow = allPoints(window, 0);
      return finalWindow;
    }
    
    bool allLowerAtMax(const int level) const {
      if (level == ((int) bounds.size()) - 1) {
        return true;
      }

      for (int i = level + 1; i < (int) bounds.size(); i++) {
        if (!atMax(i)) {
          return false;
        }
      }

      return true;
    }

    bool atMax(const int level) const {
      return intervalEnd(level) == bounds[level];
      //bool atM = bounds[level] == values[level];
      ////cout << "atM = " << atM << " for level: " << level << ", bounds = " << bounds[level] << ", value = " << values[level] << endl;
      //return atM;
    }

    bool allAtMax() const {
      return atMax(0) && allLowerAtMax(0);
    }

    bool allDone() const {
      return finished && atMax(0) && allLowerAtMax(0);
    }
 
    void increment(const int index) {
      assert(index < ((int) values.size()));
      values[index] = std::min(bounds[index], values[index] + increments[index]);
    }

    void increment() {
      if (allAtMax() && !allDone()) {
        finished = true;
      }

      if (allDone()) {
        return;
      }

      for (int i = 0; i < (int) bounds.size(); i++) {
        if (allLowerAtMax(i)) {
          increment(i);
          //values[i]++;

          for (int j = i + 1; j < (int) bounds.size(); j++) {
            values[j] = 0;
          }
          break;
        }
      }
    }

};

bool hasClock(CoreIR::Module* m) {
  for (auto f : m->getType()->getRecord()) {
    if (isClockOrNestedClockType(f.second, m->getContext()->Named("coreir.clkIn"))) {
      cout << "Found clock" << endl;
      return true;
    }
  }
  return false;
}

template<typename T>
bool is2D(T& buf) {
  return (buf.dimensions() == 2) || (buf.dimensions() == 3 && buf.channels() == 1);
}

void resetSim(const std::string& inputName, CoreIR::Module* m, SimulatorState& state) {

    state.setValue(inputName, BitVector(16, 0));
    state.setValue("self.in_en", BitVector(1, 0));
    if (hasClock(m)) {
      state.setClock("self.clk", 0, 1);
    }
    state.setValue("self.reset", BitVector(1, 1));

    state.resetCircuit();

    state.setValue("self.reset", BitVector(1, 0));
}

template<typename T>
void runHWKernel(const std::string& inputName, CoreIR::Module* m, Halide::Runtime::Buffer<T>& hwInputBuf, Halide::Runtime::Buffer<T>& outputBuf) {

  SimulatorState state(m);
  resetSim(inputName, m, state);

  assert(is2D(hwInputBuf));

  int maxCycles = 100;
  int cycles = 0;

  std::string outputName = "self.out_0_0";
  CoordinateVector<int> writeIdx({"y", "x", "c"}, {hwInputBuf.height() - 1, hwInputBuf.width() - 1, hwInputBuf.channels() - 1});
  CoordinateVector<int> readIdx({"y", "x", "c"}, {outputBuf.height() - 1, outputBuf.width() - 1, outputBuf.channels() - 1});
  if (!is2D(outputBuf)) {
    readIdx.setIncrement("c", 3);
  }

  while (cycles < maxCycles && !readIdx.allDone()) {
    cout << "Read index = " << readIdx.coordString() << endl;
    cout << "Cycles     = " << cycles << endl;

    run_for_cycle(writeIdx, readIdx,
        hwInputBuf, outputBuf,
        inputName, outputName,
        state);
    cycles++;
  }

}
template<typename T>
void runHWKernel(CoreIR::Module* m, Halide::Runtime::Buffer<T>& hwInputBuf, Halide::Runtime::Buffer<T>& outputBuf) {
  runHWKernel("self.in_arg_0_0_0", m, hwInputBuf, outputBuf);
}

CoreIR::Context* hwContext() {
  CoreIR::Context* context = newContext();
  CoreIRLoadLibrary_commonlib(context);
  CoreIRLoadLibrary_float(context);
  return context;
}

CoreIR::Module* buildModule(CoreIR::Context* context, const std::string& name, std::vector<Argument>& args, const std::string& fName, Func& hwOutput) {
  Target t;
  t = t.with_feature(Target::Feature::CoreIR);
  hwOutput.compile_to_coreir(name, args, fName, t);

  if (!loadFromFile(context, "./conv_3_3_app.json")) {
    cout << "Error: Could not load json for unit test!" << endl;
    context->die();
  }
  context->runPasses({"rungenerators", "flattentypes", "flatten", "wireclocks-coreir"});
  CoreIR::Module* m = context->getNamespace("global")->getModule("DesignTop");
  cout << "Module..." << endl;
  m->print();
  return m;
}

template<typename T>
std::string outputForCoord2D(Point<T>& point) {
  return "self.out_0_0";
}

template<typename T>
std::string outputForCoord3D(Point<T>& point) {
  assert(point.coord("x") == 0);
  assert(point.coord("y") == 0);

  return "self." + std::string("out_") + std::to_string(point.coord("c")) + std::string("_0_0");
}

template<typename T>
void run_for_cycle(CoordinateVector<int>& writeIdx,
    CoordinateVector<int>& readIdx,

    Halide::Runtime::Buffer<T> input,
    Halide::Runtime::Buffer<T> output,
    string input_name,
    string output_name,

    CoreIR::SimulatorState& state
    ) {

  const int x = writeIdx.coord("x");
  const int y = writeIdx.coord("y");
  const int c = writeIdx.coord("c");

  if (!writeIdx.allDone()) {

    state.setValue("self.in_en", BitVector(1, true));
    state.setValue(input_name, BitVector(16, input(x,y,c)));
    //std::cout << "y=" << y << ",x=" << x << " " << hex << "in=" << (int) input(x, y, c) << endl;
    //std::cout << "y=" << y << ",x=" << x << " " << "in=" << (int) input(x, y, c) << endl;

    writeIdx.increment();
  } else {
    state.setValue("self.in_en", BitVector(1, false));
  }
  // propogate to all wires
  state.exeCombinational();

  // read output wire
  bool valid_value = state.getBitVec("self.valid").to_type<bool>();

  cout << "Read window with " << readIdx.currentWindowRelative().size() << " points..." << endl;
  for (auto point : readIdx.currentWindowRelative()) {
    assert(point.numDims() > 0);
    cout << "\tReading output data for point " << point << endl;
  }
  if (valid_value) {

    for (auto point : readIdx.currentWindow()) {
      assert(point.numDims() > 0);
      cout << "\tReading output data for point " << point << endl;
      cout << "\t\trelative position in window = " << readIdx.relativeWindowPosition(point) << endl;
      auto windowPos = readIdx.relativeWindowPosition(point);
      BitVector output_bv;
      if (!is2D(output)) {
        output_bv = state.getBitVec(outputForCoord3D(windowPos));
      } else {
        output_bv = state.getBitVec(outputForCoord2D(windowPos));
      }
      T output_value;
      output_value = output_bv.to_type<T>();
      std::cout << "\tthis one is valid = " << output_bv << ", int = " << output_bv.to_type<int>() << endl;

      //const int xr = readIdx.coord("x");
      //const int yr = readIdx.coord("y");
      //const int cr = readIdx.coord("c");
      
      const int xr = point.coord("x");
      const int yr = point.coord("y");
      const int cr = point.coord("c");
      output(xr, yr, cr) = output_value;
    }
    readIdx.increment();
  }


  //std::cout << "y=" << y << ",x=" << x << " " << "in=" << (state.getBitVec(input_name)) << " out=" << +output_value << " based on bv=" << state.getBitVec(output_name).to_type<int>() << dec << endl;
  //std::cout << "y=" << y << ",x=" << x << " " << hex << "in=" << (state.getBitVec(input_name)) << " out=" << +output_value << " based on bv=" << state.getBitVec(output_name).to_type<int>() << dec << endl;

  // give another rising edge (execute seq)
  state.exeSequential();
}

template<typename T>
void compare_buffers(Halide::Runtime::Buffer<T>& outputBuf, Halide::Buffer<T>& cpuOutput) {
  cout << "Comparing buffers..." << endl;
  cout << "Hardware output" << endl;
  printBuffer(outputBuf, cout);
  
  cout << endl;
  cout << "CPU Output" << endl;
  printBuffer(cpuOutput, cout);

  for (int i = 0; i < outputBuf.width(); i++) {
    for (int j = 0; j < outputBuf.height(); j++) {
      for (int b = 0; b < outputBuf.channels(); b++) {
        if (outputBuf(i, j, b) != cpuOutput(i, j, b)) {
          cout << "Error: Buffers do not match at " << i << ", " << j << ", " << b << endl;
        }
        assert(outputBuf(i, j, b) == cpuOutput(i, j, b));
      }
    }
    cout << endl;
  }

}
using namespace Halide::Internal;

class CodeGen_SoC_Test : public CodeGen_C {
  public:

    bool inHWRegion;

    CodeGen_SoC_Test(std::ostream &dest,
              Target target,
              OutputKind output_kind = CImplementation,
              const std::string &include_guard = "") :
      CodeGen_C(dest, target, output_kind, include_guard) {
        inHWRegion = false;
      }

    ~CodeGen_SoC_Test() {
      //std::cout << "Calling destructor for printer" << endl;
      CodeGen_C::~CodeGen_C();
    }

    void compileForCGRA(const Halide::Module& input) {
      stream << "// CGRA Specific wrapper code" << endl;
      stream << "#include \"cgra_wrapper.h\"" << endl;
      stream << endl;
      CodeGen_SoC_Test::compile(input);
      stream.flush();
    }

    void visit(const Provide* p) {
      stream << "// Found a provide, must be start of something hardware related!" << endl;
    }

    void visit(const Call* c) {
      if (c->name == "stream_subimage") {

        stream << "// Call to stream_subimage, we need to do commands here?..." << endl;
        const StringImm *direction = c->args[0].as<StringImm>();

        string a1 = print_expr(c->args[1]);
        //string a2 = print_expr(c->args[2]);
        //string a3 = print_expr(c->args[3]);

        string val = print_expr(c->args[3].as<Call>()->args[0]);
        if (direction->value == "buffer_to_stream") {
          stream << "accelerator.subimage_to_stream(";
        } else {
          assert(direction->value == "stream_to_buffer");
          stream << "accelerator.stream_to_subimage(";
        }
        
        //stream << a1 << ", " << a2 << ", " << a3;
        stream << a1;
        for (size_t i = 4; i < c->args.size(); i++) {
            stream << ", " << print_expr(c->args[i]);
        }

        stream << ");" << endl;
      } else {
        CodeGen_C::visit(c);
      }
    }

    void visit(const ProducerConsumer* p) {
      if (inHWRegion && p->is_producer) {
        stream << "// Should generate CoreIR for produce: " << p->name << endl;
        // Generate coreir module for this statement...
      } else {
        CodeGen_C::visit(p);
      }
    }

    void visit(const Evaluate* op) {
      if (inHWRegion) {
        stream << "// Evaluating... = " << op->value << endl;
        op->value.accept(this);
      } else {
        CodeGen_C::visit(op);
      }
    }

    void visit(const For* f) {
      stream << "// Entering for loop" << endl;
      CodeGen_C::visit(f);
      stream << "// Leaving for loop" << endl;
    }

    void visit(const Realize* p) {

      //stream << "// Found a realize of " << p->name << ", entering outermost hardware region" << endl;
      //stream << "CGRAWrapper accelerator;" << std::endl;
      
      bool justEntered = inHWRegion == false;
      if (justEntered) {
        stream << "// Found a realize of " << p->name << ", entering outermost hardware region" << endl;
        stream << "std::cout << \"Computing output tile with accelerator\" << std::endl;\n";
        stream << "CGRAWrapper accelerator;" << std::endl;
        inHWRegion = true;
      }

      p->body.accept(this);

      if (justEntered) {
        stream << "// Done with hardware region" << endl;
        inHWRegion = false;
      }
    }
};
void small_demosaic_test() {

  ImageParam input(type_of<uint16_t>(), 2);
  ImageParam output(type_of<uint16_t>(), 3);

  Var x("x"), y("y"), c("c");
  Var xi,yi,zi, xo,yo,zo;

  Func hw_input;
  hw_input(x,y) = cast<uint16_t>(input(x+1,y+1));

  // common patterns: average of four surrounding pixels
  Func neswNeighbors, diagNeighbors;
  // Should this actually be /4 outside of the hw input?
  neswNeighbors(x, y) = (hw_input(x-1, y)   + hw_input(x+1, y) +
      hw_input(x,   y-1) + hw_input(x,   y+1)) / 4;
  diagNeighbors(x, y) = (hw_input(x-1, y-1) + hw_input(x+1, y-1) +
      hw_input(x-1, y+1) + hw_input(x+1, y+1)) / 4;

  // common patterns: average of two adjacent pixels
  Func vNeighbors, hNeighbors;
  vNeighbors(x, y) = (hw_input(x, y-1) + hw_input(x, y+1))/2;
  hNeighbors(x, y) = (hw_input(x-1, y) + hw_input(x+1, y))/2;

  int phase = 0;

  // output pixels depending on image layout.
  // Generally, image looks like
  //    R G R G R G R G
  //    G B G B G B G B
  //    R G R G R G R G
  //    G B G B G B G B
  Func green, red, blue;
  green(x, y) = select((y % 2) == (phase / 2),
      select((x % 2) == (phase % 2), neswNeighbors(x, y), hw_input(x, y)), // First row, RG
      select((x % 2) == (phase % 2), hw_input(x, y),      neswNeighbors(x, y))); // Second row, GB

  red(x, y) = select((y % 2) == (phase / 2),
      select((x % 2) == (phase % 2), hw_input(x, y),   hNeighbors(x, y)), // First row, RG
      select((x % 2) == (phase % 2), vNeighbors(x, y), diagNeighbors(x, y))); // Second row, GB

  blue(x, y) = select((y % 2) == (phase / 2),
      select((x % 2) == (phase % 2), diagNeighbors(x, y), vNeighbors(x, y)), // First row, RG
      select((x % 2) == (phase % 2), hNeighbors(x, y),    hw_input(x, y))); // Second row, GB

  // output all channels
  Func demosaic, hw_output;
  demosaic(x,y,c) = cast<uint16_t>(select(c == 0, red(x, y),
        c == 1, green(x, y),
        blue(x, y)));

  hw_output(x,y,c) = demosaic(x,y,c);
  output(x,y,c) = hw_output(x,y,c);

  // Schedule common
  demosaic.bound(c, 0, 3);
  hw_output.bound(c, 0, 3);
  //output.bound(c, 0, 3);

  hw_input.compute_root();
  hw_output.compute_root();

  Halide::Buffer<uint16_t> inputBuf(200, 200);
  Halide::Runtime::Buffer<uint16_t> hwInputBuf(inputBuf.width(), inputBuf.height(), 1);
  Halide::Runtime::Buffer<uint16_t> outputBuf(inputBuf.width() - 8, inputBuf.height() - 8, 3);
  for (int i = 0; i < inputBuf.width(); i++) {
    for (int j = 0; j < inputBuf.height(); j++) {
      inputBuf(i, j) = rand() % 255;
      hwInputBuf(i, j, 0) = inputBuf(i, j);
    }
  }

  //Creating CPU reference output
  Halide::Buffer<uint16_t> cpuOutput(outputBuf.width(), outputBuf.height(), outputBuf.channels());
  ParamMap rParams;
  rParams.set(input, inputBuf);
  Target t;
  hw_output.realize(cpuOutput, t, rParams);

  cout << "CPU output" << endl;
  printBuffer(cpuOutput, cout);

  // Hardware schedule
  
  hw_output.tile(x, y, xo, yo, xi, yi, 8, 8)
    .reorder(c, xi, yi, xo, yo);

  //hw_input.stream_to_accelerator();
  //hw_output.hw_accelerate(xi, xo);
  hw_output.unroll(c);

  auto context = hwContext();
  vector<Argument> args{input};

  {

    {
      Target t;
      t = t.with_feature(Target::Feature::CoreIR);
      auto mod = hw_output.compile_to_module(args, "hw_demosaic", t);
      cout << "Compiled to module" << endl;
      cout << mod << endl;
      ofstream outFile("demosaic_soc_mini.cpp");
      //CodeGen_SoC_Test testPrinter(cout, t, CodeGen_C::OutputKind::CPlusPlusImplementation);
      CodeGen_SoC_Test testPrinter(outFile, t, CodeGen_C::OutputKind::CPlusPlusImplementation);
      testPrinter.compileForCGRA(mod);
      //outFile.close();
      cout.flush();
    }
    cout << "Done with compiling for CGRA" << endl;
    runCmd("clang++ -std=c++11 demosaic_soc_run.cpp demosaic_soc_mini.cpp cgra_wrapper.cpp -lHalide -L ../../../../bin");
    runCmd("./a.out");
  }

  //assert(false);

  //auto m = buildModule(context, "demosaic_coreir", args, "demosaic", hw_output);

  //runHWKernel("self.in_arg_3_0_0", m, hwInputBuf, outputBuf);
  //cout << "Input buffer" << endl;
  //printBuffer(hwInputBuf, cout);
  //compare_buffers(outputBuf, cpuOutput);
  
  cout << GREEN << "Demosaic test passed" << RESET << endl;
}

void multi_channel_conv_test() {
  ImageParam input(type_of<uint16_t>(), 2);
  ImageParam output(type_of<uint16_t>(), 3);

  Var x("x"), y("y"), z("z");
  Var xi,yi,zi, xo,yo,zo;

  Func kernel("kernel");
  kernel(x) = 0;
  kernel(0) = 0;
  kernel(1) = 1;
  kernel(2) = 2;

  Func conv("conv");
  Func hw_input("hw_input");
  hw_input(x, y) = cast<uint16_t>(input(x, y));
  conv(x, y, z) = hw_input(x, y) + kernel(z);
  
  Func hw_output("hw_output");
  hw_output(x, y, z) = cast<uint16_t>(conv(x, y, z));
  output(x, y, z) = hw_output(x, y, z);

  // Create common elements of the CPU and hardware schedule
  hw_input.compute_root();
  hw_output.compute_root();

  // Creating input data
  Halide::Buffer<uint16_t> inputBuf(4, 4);
  Halide::Runtime::Buffer<uint16_t> hwInputBuf(inputBuf.width(), inputBuf.height(), 1);
  Halide::Runtime::Buffer<uint16_t> outputBuf(inputBuf.width(), inputBuf.height(), 3);
  for (int i = 0; i < inputBuf.width(); i++) {
    for (int j = 0; j < inputBuf.height(); j++) {
      inputBuf(i, j) = rand() % 255;
      hwInputBuf(i, j, 0) = inputBuf(i, j);
    }
  }

  //Creating CPU reference output
  Halide::Buffer<uint16_t> cpuOutput(outputBuf.width(), outputBuf.height(), outputBuf.channels());
  ParamMap rParams;
  rParams.set(input, inputBuf);
  Target t;
  hw_output.realize(cpuOutput, t, rParams);

  cout << "CPU output" << endl;
  printBuffer(cpuOutput, cout);
  
  // Hardware schedule
  int tileSize = 8;
  hw_output.bound(z, 0, 3);
  conv.bound(z, 0, 3);

  hw_output.reorder(z, x, y).tile(x, y, xo, yo, xi, yi, tileSize, tileSize).
    unroll(z).
    hw_accelerate(xi, xo);
  //conv.unroll(z, 3);
  hw_input.stream_to_accelerator();

  cout << "Loop nest.." << endl;
  hw_output.print_loop_nest();
  
  auto context = hwContext();
  vector<Argument> args{input};
  auto m = buildModule(context, "mc_conv_coreir", args, "mc_conv", hw_output);

  runHWKernel(m, hwInputBuf, outputBuf);

  cout << "Input buffer" << endl;
  printBuffer(hwInputBuf, cout);
  compare_buffers(outputBuf, cpuOutput);

  cout << GREEN << "Multi channel conv test passed" << RESET << endl;
}

void clamped_grad_x_test() {

  // Build the app
  ImageParam input(type_of<uint16_t>(), 2);
  ImageParam output(type_of<uint16_t>(), 2);

  Var x("x"), y("y");

  Var xi,yi, xo,yo;

  Func hw_input("hw_input");
  hw_input(x, y) = input(x, y);
  Func padded16;
  padded16(x, y) = cast<uint16_t>(0);
  padded16(x, y) = hw_input(x + 3, y + 3);
  // Sobel filter
  Func grad_x_unclamp, grad_x;
  grad_x_unclamp(x, y) = cast<int16_t>(-padded16(x - 1, y - 1) + padded16(x + 1, y - 1)
      -2*padded16(x - 1, y) + 2*padded16(x + 1, y)
      -padded16(x-1, y+1) + padded16(x + 1, y + 1));

  grad_x(x, y) = clamp(grad_x_unclamp(x, y), -255, 255);

  Func hw_output("hw_output");
  hw_output(x, y) = cast<uint16_t>(grad_x(x, y));
  output(x, y) = hw_output(x,y);

  // Create common elements of the CPU and hardware schedule
  hw_input.compute_root();
  hw_output.compute_root();

  // Creating input data
  Halide::Buffer<uint16_t> inputBuf(32, 32);
  Halide::Runtime::Buffer<uint16_t> hwInputBuf(inputBuf.height(), inputBuf.width(), 1);
  Halide::Runtime::Buffer<uint16_t> outputBuf(16, 16, 1);
  for (int i = 0; i < inputBuf.height(); i++) {
    for (int j = 0; j < inputBuf.width(); j++) {
      inputBuf(i, j) = rand() % 255;
      hwInputBuf(i, j, 0) = inputBuf(i, j);
    }
  }

  {
    Halide::Buffer<uint16_t> paddingOut(4, 4);
    paddingOut.set_min(-3, -3);
    cout << "Padding y range = " << paddingOut.bottom() << ", " << paddingOut.top() << endl;
    ParamMap rParams;
    rParams.set(input, inputBuf);
    Target t;
    padded16.realize(paddingOut, t, rParams);

    cout << "Original" << endl;
    printBuffer(inputBuf, cout);

    cout << "Padded" << endl;
    printBuffer(paddingOut, cout);
  }
  assert(false);
  
  // Creating CPU reference output
  Halide::Buffer<uint16_t> cpuOutput(4, 4);
  ParamMap rParams;
  rParams.set(input, inputBuf);
  Target t;
  hw_output.realize(cpuOutput, t, rParams);

  // Hardware schedule
  padded16.compute_root();
  int tileSize = 16;
  hw_output.tile(x, y, xo, yo, xi, yi, tileSize, tileSize).accelerate({padded16}, xi, xo);
  hw_input.stream_to_accelerator();
  grad_x_unclamp.linebuffer();
  grad_x.linebuffer();
  
  auto context = hwContext();
  vector<Argument> args{input};
  auto m = buildModule(context, "coreir_harris", args, "harris", hw_output);

  runHWKernel(m, hwInputBuf, outputBuf);
  cout << "Input buf" << endl;
  printBuffer(inputBuf, cout);

  compare_buffers(outputBuf, cpuOutput);

  cout << GREEN << "Harris test passed" << RESET << endl;
}

void control_path_test() {
  ImageParam input(type_of<int16_t>(), 2);
  ImageParam output(type_of<int16_t>(), 2);

  Var x("x"), y("y");

  Var xi,yi, xo,yo;

  Func hw_input, hw_output, clamped;
  hw_input(x, y) = input(x, y);
  clamped(x, y) = select(x % 2 == 0, hw_input(x, y), hw_input(x, y) + 1);

  hw_output(x, y) = cast<int16_t>(clamped(x, y));
  output(x, y) = hw_output(x, y);

  // Create common elements of the CPU and hardware schedule
  hw_input.compute_root();
  hw_output.compute_root();

   // Creating input data
   int nRows = 4;
   int nCols = 2;
  Halide::Buffer<int16_t> inputBuf(nCols, nRows);
  Halide::Runtime::Buffer<int16_t> hwInputBuf(inputBuf.width(), inputBuf.height(), 1);
  Halide::Runtime::Buffer<int16_t> outputBuf(inputBuf.width(), inputBuf.height(), 1);
  for (int i = 0; i < inputBuf.height(); i++) {
    for (int j = 0; j < inputBuf.width(); j++) {
      inputBuf(j, i) = 0;
      hwInputBuf(j, i, 0) = inputBuf(j, i);
    }
  }

   //Creating CPU reference output
  Halide::Buffer<int16_t> cpuOutput(outputBuf.width(), outputBuf.height());
  ParamMap rParams;
  rParams.set(input, inputBuf);
  Target t;
  hw_output.realize(cpuOutput, t, rParams);

  // Create HW schedule
  hw_output.tile(x, y, xo, yo, xi, yi, 2, 2).hw_accelerate(xi, xo);
  clamped.linebuffer();
  hw_input.stream_to_accelerator();

  auto context = hwContext();
  vector<Argument> args{input};
  auto m = buildModule(context, "coreir_harris", args, "harris", hw_output);

  runHWKernel("self.in_arg_2_0_0", m, hwInputBuf, outputBuf);

  compare_buffers(outputBuf, cpuOutput);
  deleteContext(context);

  cout << GREEN << "Control path test passed" << RESET << endl;
}

void control_path_xy_test() {
  ImageParam input(type_of<int16_t>(), 2);
  ImageParam output(type_of<int16_t>(), 2);

  Var x("x"), y("y");

  Var xi,yi, xo,yo;

  Func hw_input, hw_output, clamped;
  hw_input(x, y) = input(x, y);
  clamped(x, y) = select(x % 2 == 0, select(y % 2 == 0, hw_input(x, y), hw_input(x, y) + 1),
      select(y % 2 == 0, hw_input(x, y) + 2, hw_input(x, y) + 3));

  hw_output(x, y) = cast<int16_t>(clamped(x, y));
  output(x, y) = hw_output(x, y);

  // Create common elements of the CPU and hardware schedule
  hw_input.compute_root();
  hw_output.compute_root();

   // Creating input data
   int nRows = 4;
   int nCols = 2;
  Halide::Buffer<int16_t> inputBuf(nCols, nRows);
  Halide::Runtime::Buffer<int16_t> hwInputBuf(inputBuf.width(), inputBuf.height(), 1);
  Halide::Runtime::Buffer<int16_t> outputBuf(inputBuf.width(), inputBuf.height(), 1);
  for (int i = 0; i < inputBuf.height(); i++) {
    for (int j = 0; j < inputBuf.width(); j++) {
      inputBuf(j, i) = 0;
      hwInputBuf(j, i, 0) = inputBuf(j, i);
    }
  }

   //Creating CPU reference output
  Halide::Buffer<int16_t> cpuOutput(outputBuf.width(), outputBuf.height());
  ParamMap rParams;
  rParams.set(input, inputBuf);
  Target t;
  hw_output.realize(cpuOutput, t, rParams);

  // Create HW schedule
  hw_output.tile(x, y, xo, yo, xi, yi, 2, 2).hw_accelerate(xi, xo);
  clamped.linebuffer();
  hw_input.stream_to_accelerator();

  auto context = hwContext();
  vector<Argument> args{input};
  auto m = buildModule(context, "coreir_harris", args, "harris", hw_output);

  runHWKernel("self.in_arg_3_0_0", m, hwInputBuf, outputBuf);

  compare_buffers(outputBuf, cpuOutput);
  deleteContext(context);

  cout << GREEN << "Control path xy test passed" << RESET << endl;
}
void mod2_test() {
  ImageParam input(type_of<int16_t>(), 2);
  ImageParam output(type_of<int16_t>(), 2);

  Var x("x"), y("y");

  Var xi,yi, xo,yo;

  Func hw_input, hw_output, clamped;
  hw_input(x, y) = input(x, y);
  clamped(x, y) = hw_input(x, y) % 2;

  hw_output(x, y) = cast<int16_t>(clamped(x, y));
  output(x, y) = hw_output(x, y);

  // Create common elements of the CPU and hardware schedule
  hw_input.compute_root();
  hw_output.compute_root();

   // Creating input data
  Halide::Buffer<int16_t> inputBuf(2, 2);
  Halide::Runtime::Buffer<int16_t> hwInputBuf(inputBuf.height(), inputBuf.width(), 1);
  Halide::Runtime::Buffer<int16_t> outputBuf(2, 2, 1);
  for (int i = 0; i < inputBuf.height(); i++) {
    for (int j = 0; j < inputBuf.width(); j++) {
      inputBuf(i, j) = i + j;
      hwInputBuf(i, j, 0) = inputBuf(i, j);
    }
  }

   //Creating CPU reference output
  Halide::Buffer<int16_t> cpuOutput(2, 2);
  ParamMap rParams;
  rParams.set(input, inputBuf);
  Target t;
  hw_output.realize(cpuOutput, t, rParams);

  // Create HW schedule
  hw_output.tile(x, y, xo, yo, xi, yi, 2, 2).hw_accelerate(xi, xo);
  clamped.linebuffer();
  hw_input.stream_to_accelerator();

  auto context = hwContext();
  vector<Argument> args{input};
  auto m = buildModule(context, "coreir_harris", args, "harris", hw_output);

  runHWKernel(m, hwInputBuf, outputBuf);

  compare_buffers(outputBuf, cpuOutput);
  deleteContext(context);

  cout << GREEN << "Clamp test passed" << RESET << endl;
}
void shiftRight_test() {
  ImageParam input(type_of<int16_t>(), 2);
  ImageParam output(type_of<int16_t>(), 2);

  Var x("x"), y("y");

  Var xi,yi, xo,yo;

  Func hw_input, hw_output, clamped;
  hw_input(x, y) = input(x, y);
  clamped(x, y) = hw_input(x, y) >> 7;

  hw_output(x, y) = cast<int16_t>(clamped(x, y));
  output(x, y) = hw_output(x, y);

  // Create common elements of the CPU and hardware schedule
  hw_input.compute_root();
  hw_output.compute_root();

   // Creating input data
  Halide::Buffer<int16_t> inputBuf(2, 2);
  Halide::Runtime::Buffer<int16_t> hwInputBuf(inputBuf.height(), inputBuf.width(), 1);
  Halide::Runtime::Buffer<int16_t> outputBuf(2, 2, 1);
  for (int i = 0; i < inputBuf.height(); i++) {
    for (int j = 0; j < inputBuf.width(); j++) {
      inputBuf(i, j) = rand();
      hwInputBuf(i, j, 0) = inputBuf(i, j);
    }
  }

   //Creating CPU reference output
  Halide::Buffer<int16_t> cpuOutput(2, 2);
  ParamMap rParams;
  rParams.set(input, inputBuf);
  Target t;
  hw_output.realize(cpuOutput, t, rParams);

  // Create HW schedule
  hw_output.tile(x, y, xo, yo, xi, yi, 2, 2).hw_accelerate(xi, xo);
  clamped.linebuffer();
  hw_input.stream_to_accelerator();

  auto context = hwContext();
  vector<Argument> args{input};
  auto m = buildModule(context, "coreir_harris", args, "harris", hw_output);

  runHWKernel(m, hwInputBuf, outputBuf);

  compare_buffers(outputBuf, cpuOutput);
  deleteContext(context);

  cout << GREEN << "Clamp test passed" << RESET << endl;
}

void clamp_test() {
  ImageParam input(type_of<int16_t>(), 2);
  ImageParam output(type_of<int16_t>(), 2);

  Var x("x"), y("y");

  Var xi,yi, xo,yo;

  Func hw_input, hw_output, clamped;
  hw_input(x, y) = input(x, y);
  clamped(x, y) = clamp(hw_input(x, y), cast<int16_t>(-255), cast<int16_t>(255));

  hw_output(x, y) = cast<int16_t>(clamped(x, y));
  output(x, y) = hw_output(x, y);

  // Create common elements of the CPU and hardware schedule
  hw_input.compute_root();
  hw_output.compute_root();

   // Creating input data
  Halide::Buffer<int16_t> inputBuf(2, 2);
  Halide::Runtime::Buffer<int16_t> hwInputBuf(inputBuf.height(), inputBuf.width(), 1);
  Halide::Runtime::Buffer<int16_t> outputBuf(2, 2, 1);
  for (int i = 0; i < inputBuf.height(); i++) {
    for (int j = 0; j < inputBuf.width(); j++) {
      inputBuf(i, j) = rand();
      hwInputBuf(i, j, 0) = inputBuf(i, j);
    }
  }

   //Creating CPU reference output
  Halide::Buffer<int16_t> cpuOutput(2, 2);
  ParamMap rParams;
  rParams.set(input, inputBuf);
  Target t;
  hw_output.realize(cpuOutput, t, rParams);

  // Create HW schedule
  hw_output.tile(x, y, xo, yo, xi, yi, 2, 2).hw_accelerate(xi, xo);
  clamped.linebuffer();
  hw_input.stream_to_accelerator();

  auto context = hwContext();
  vector<Argument> args{input};
  auto m = buildModule(context, "coreir_harris", args, "harris", hw_output);

  runHWKernel(m, hwInputBuf, outputBuf);

  compare_buffers(outputBuf, cpuOutput);
  deleteContext(context);

  cout << GREEN << "Clamp test passed" << RESET << endl;
}

void small_harris_test() {

  int blockSize = 3;

  // k is a sensitivity parameter for detecting corners.
  // k should vary from 0.04 to 0.15 according to literature.
  int shiftk = 4; // equiv to k = 0.0625

  // Threshold for cornerness measure.
  int threshold = 1;
  // Build the app
  ImageParam input(type_of<uint16_t>(), 2);
  ImageParam output(type_of<uint16_t>(), 2);

  Var x("x"), y("y");

  Var xi,yi, xo,yo;

  Func padded16, padded;
  padded16(x, y) = input(x + 3, y + 3);

  // Sobel filter
  Func grad_x_unclamp, grad_y_unclamp, grad_x, grad_y;
  grad_x_unclamp(x, y) = cast<int16_t>(-padded16(x - 1, y - 1) + padded16(x + 1, y - 1)
      -2*padded16(x - 1, y) + 2*padded16(x + 1, y)
      -padded16(x-1, y+1) + padded16(x + 1, y + 1));

  grad_y_unclamp(x, y) = cast<int16_t>(padded16(x - 1, y + 1) - padded16(x - 1, y - 1) +
      2*padded16(x, y + 1) - 2*padded16(x, y - 1) + 
      padded16(x + 1, y + 1) - padded16(x + 1, y - 1));

  grad_x(x, y) = clamp(grad_x_unclamp(x, y), -255, 255);
  grad_y(x, y) = clamp(grad_y_unclamp(x, y), -255, 255);

  // Product of gradients
  Func grad_xx, grad_yy, grad_xy;
  grad_xx(x, y) = grad_x(x, y) * grad_x(x, y);
  grad_yy(x, y) = grad_y(x, y) * grad_y(x, y);
  grad_xy(x, y) = grad_x(x, y) * grad_y(x, y);

  // Shift gradients
  Func lxx, lyy, lxy;
  lxx(x, y) = grad_xx(x, y) >> 7;
  lyy(x, y) = grad_yy(x, y) >> 7;
  lxy(x, y) = grad_xy(x, y) >> 7;

  // Box filter
  Func lgxx, lgyy, lgxy;
  RDom box(-blockSize / 2, blockSize, -blockSize/2, blockSize);
  lgxx(x, y) += lxx(x + box.x, y + box.y);
  lgyy(x, y) += lyy(x + box.x, y + box.y);
  lgxy(x, y) += lxy(x + box.x, y + box.y);

  Expr lgxx8 = lgxx(x, y) >> 6;
  Expr lgyy8 = lgyy(x, y) >> 6;
  Expr lgxy8 = lgxy(x, y) >> 6;

  // calculate Cim
  // int scale = (1 << (Ksize - 1)) * blockSize
  Func cim;
  Expr det = lgxx8*lgyy8 - lgxy8*lgxy8;
  Expr trace = lgxx8 + lgyy8;
  cim(x, y) = det - (trace*trace >> shiftk);

  // Perform non-maximal suppression
  Func hw_output("hw_output");
  Expr is_max = cim(x, y) > cim(x - 1, y - 1) && cim(x, y) > cim(x, y - 1) &&
    cim(x, y) > cim(x + 1, y - 1) && cim(x, y) > cim(x - 1, y) &&
    cim(x, y) > cim(x + 1, y) && cim(x, y) > cim(x - 1, y  + 1) &&
    cim(x, y) > cim(x, y + 1) && cim(x, y) > cim(x + 1, y + 1);
  hw_output(x, y) = cast<uint16_t>(select(is_max && (cim(x, y) >= threshold), 255, 0));
  output(x, y) = hw_output(x,y);

  // Create common elements of the CPU and hardware schedule

  //hw_input.compute_root();
  hw_output.compute_root();

  // Creating input data
  Halide::Buffer<uint16_t> inputBuf(16, 16);
  Halide::Runtime::Buffer<uint16_t> hwInputBuf(inputBuf.height(), inputBuf.width(), 1);
  Halide::Runtime::Buffer<uint16_t> outputBuf(4, 4, 1);
  for (int i = 0; i < inputBuf.height(); i++) {
    for (int j = 0; j < inputBuf.width(); j++) {
      inputBuf(i, j) = rand() % 255;
      hwInputBuf(i, j, 0) = inputBuf(i, j);
    }
  }
 
  // Creating CPU reference output
  Halide::Buffer<uint16_t> cpuOutput(4, 4);
  ParamMap rParams;
  rParams.set(input, inputBuf);
  Target t;
  hw_output.realize(cpuOutput, t, rParams);
  cout << "CPU output..." << endl;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      cout << (int) cpuOutput(i, j) << " ";
    }
    cout << endl;
  }

  ////int tileSize = 8;
  //hw_output.tile(x,y, xo,yo, xi,yi, 8-4, 8-4)
    //.hw_accelerate(xi, xo);

  //conv1.update()
    //.unroll(r.x)
    //.unroll(r.y);
  //conv1.linebuffer();

  //conv2.update()
    //.unroll(r0.x)
    //.unroll(r0.y);
  //conv2.linebuffer();

  //hw_input.stream_to_accelerator();
  
  //auto context = hwContext();
  //vector<Argument> args{input};
  //auto m = buildModule(context, "coreir_harris", args, "harris", hw_output);
  ////cout << "Module = " << endl;
  //m->print();

  //SimulatorState state(m);
  //state.setValue("self.in_arg_0_0_0", BitVector(16, 0));
  //state.setValue("self.in_en", BitVector(1, 0));
  //state.setClock("self.clk", 0, 1);
  //state.setValue("self.reset", BitVector(1, 1));

  //state.resetCircuit();

  //state.setValue("self.reset", BitVector(1, 0));

  //int maxCycles = 100;
  //int cycles = 0;

  //std::string inputName = "self.in_arg_0_0_0";
  //std::string outputName = "self.out_0_0";
  //CoordinateVector<int> writeIdx({"y", "x", "c"}, {hwInputBuf.height() - 1, hwInputBuf.width() - 1, hwInputBuf.channels() - 1});
  //CoordinateVector<int> readIdx({"y", "x", "c"}, {outputBuf.height() - 1, outputBuf.width() - 1, outputBuf.channels() - 1});
  
  //while (cycles < maxCycles && !readIdx.allDone()) {
    //cout << "Read index = " << readIdx.coordString() << endl;
    //cout << "Cycles     = " << cycles << endl;

    //run_for_cycle(writeIdx, readIdx,
        //hwInputBuf, outputBuf,
        //inputName, outputName,
        //state);
    //cycles++;
  //}

 compare_buffers(outputBuf, cpuOutput);
 cout << GREEN << "Harris test passed" << RESET << endl;
}

void small_cascade_test() {

  cout << "Starting cascade test" << endl;

  ImageParam input(type_of<uint16_t>(), 2);
  ImageParam output(type_of<uint16_t>(), 2);

  Var x("x"), y("y");

  Var xi,yi, xo,yo;
  
  Func kernel("kernel");
  Func conv1("conv1"), conv2("conv2");
  RDom r(0, 3,
      0, 3);

  RDom r0(0, 3,
      0, 3);
  kernel(x,y) = 0;
  kernel(0,0) = 1;      kernel(0,1) = 2;      kernel(0,2) = 1;
  kernel(1,0) = 2;      kernel(1,1) = 4;      kernel(1,2) = 3;
  kernel(2,0) = 1;      kernel(2,1) = 2;      kernel(2,2) = 1;

  conv1(x, y) = 0;

  Func hw_input("hw_input");
  hw_input(x, y) = cast<uint16_t>(input(x, y));
  conv1(x, y)  += kernel(r.x, r.y) * hw_input(x + r.x, y + r.y);
  conv2(x, y)  += kernel(r0.x, r0.y) * conv1(x + r0.x, y + r0.y);
  
  kernel.compute_root();

  Func hw_output("hw_output");
  hw_output(x, y) = cast<uint16_t>(conv2(x, y));
  output(x, y) = hw_output(x,y);

  hw_input.compute_root();
  hw_output.compute_root();

  // Creating input data
  Halide::Buffer<uint16_t> inputBuf(8, 8);
  Halide::Runtime::Buffer<uint16_t> hwInputBuf(8, 8, 1);
  Halide::Runtime::Buffer<uint16_t> outputBuf(4, 4, 1);
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      inputBuf(i, j) = 11;
      hwInputBuf(i, j, 0) = inputBuf(i, j);
    }
  }
 
  // Creating CPU reference output
  Halide::Buffer<uint16_t> cpuOutput(4, 4);
  ParamMap rParams;
  rParams.set(input, inputBuf);
  Target t;
  hw_output.realize(cpuOutput, t, rParams);
  cout << "CPU output..." << endl;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      cout << (int) cpuOutput(i, j) << " ";
    }
    cout << endl;
  }

  //int tileSize = 8;
  hw_output.tile(x,y, xo,yo, xi,yi, 8-4, 8-4)
    .hw_accelerate(xi, xo);

  kernel.compute_at(hw_output, xo);

  conv1.update()
    .unroll(r.x)
    .unroll(r.y);
  conv1.linebuffer();

  conv2.update()
    .unroll(r0.x)
    .unroll(r0.y);
  conv2.linebuffer();

  hw_input.stream_to_accelerator();
  
  auto context = hwContext();
  vector<Argument> args{input};
  auto m = buildModule(context, "coreir_cascade", args, "cascade", hw_output);
  //cout << "Module = " << endl;
  m->print();

  SimulatorState state(m);
  state.setValue("self.in_arg_0_0_0", BitVector(16, 0));
  state.setValue("self.in_en", BitVector(1, 0));
  state.setClock("self.clk", 0, 1);
  state.setValue("self.reset", BitVector(1, 1));

  state.resetCircuit();

  state.setValue("self.reset", BitVector(1, 0));

  int maxCycles = 100;
  int cycles = 0;

  std::string inputName = "self.in_arg_0_0_0";
  std::string outputName = "self.out_0_0";
  CoordinateVector<int> writeIdx({"y", "x", "c"}, {hwInputBuf.height() - 1, hwInputBuf.width() - 1, hwInputBuf.channels() - 1});
  CoordinateVector<int> readIdx({"y", "x", "c"}, {outputBuf.height() - 1, outputBuf.width() - 1, outputBuf.channels() - 1});
  
  while (cycles < maxCycles && !readIdx.allDone()) {
    cout << "Read index = " << readIdx.coordString() << endl;
    cout << "Cycles     = " << cycles << endl;

    run_for_cycle(writeIdx, readIdx,
        hwInputBuf, outputBuf,
        inputName, outputName,
        state);
    cycles++;
  }

  //cout << "final buffer" << endl;
  for (int i = 0; i < outputBuf.height(); i++) {
    for (int j = 0; j < outputBuf.width(); j++) {
      for (int b = 0; b < outputBuf.channels(); b++) {
        //cout << (int) outputBuf(i, j, b) << " ";
        //cout << (int) cpuOutput(i, j, b) << " ";
        assert(outputBuf(i, j, b) == cpuOutput(i, j, b));
      }
    }
    cout << endl;
  }
  cout << GREEN << "Cascade test passed" << RESET << endl;
}

void small_conv_3_3_test() {
  ImageParam input(type_of<uint8_t>(), 2);
  ImageParam output(type_of<uint8_t>(), 2);

  Var x("x"), y("y");

  Func kernel("kernel");
  Func conv("conv");
  RDom r(0, 3,
      0, 3);

  kernel(x,y) = 0;
  kernel(0,0) = 11;      kernel(0,1) = 12;      kernel(0,2) = 13;
  kernel(1,0) = 14;      kernel(1,1) = 0;       kernel(1,2) = 16;
  kernel(2,0) = 17;      kernel(2,1) = 18;      kernel(2,2) = 19;

  conv(x, y) = 0;

  Func hw_input("hw_input");
  hw_input(x, y) = cast<uint16_t>(input(x, y));
  conv(x, y)  += kernel(r.x, r.y) * hw_input(x + r.x, y + r.y);

  Func hw_output("hw_output");
  hw_output(x, y) = cast<uint8_t>(conv(x, y));
  output(x, y) = hw_output(x,y);

  Var xi,yi, xo,yo;

  hw_input.compute_root();
  hw_output.compute_root();

  // Creating input data
  Halide::Buffer<uint8_t> inputBuf(4, 4);
  Halide::Runtime::Buffer<uint8_t> hwInputBuf(4, 4, 1);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      for (int b = 0; b < 1; b++) {
        inputBuf(i, j, b) = i + j*2;
        hwInputBuf(i, j, b) = inputBuf(i, j, b);
      }
    }
  }
 
  // Creating CPU reference output
  Halide::Buffer<uint8_t> cpuOutput(2, 2);
  ParamMap rParams;
  rParams.set(input, inputBuf);
  Target t;
  hw_output.realize(cpuOutput, t, rParams);
  //cout << "CPU output..." << endl;
  //for (int i = 0; i < 2; i++) {
    //for (int j = 0; j < 2; j++) {
      //cout << (int) cpuOutput(i, j) << " ";
    //}
    //cout << endl;
  //}
  
  Halide::Runtime::Buffer<uint8_t> outputBuf(2, 2, 1);
  
  int tileSize = 4;
  hw_output.tile(x,y, xo,yo, xi,yi, tileSize-2, tileSize-2)
    .hw_accelerate(xi, xo);

  conv.update()
    .unroll(r.x, 3)
    .unroll(r.y, 3);
  conv.linebuffer();

  hw_input.stream_to_accelerator();

  // Generate CoreIR
  auto context = hwContext();
  vector<Argument> args{input};
  auto m = buildModule(context, "coreir_conv_3_3", args, "conv_3_3", hw_output);
  cout << "Module = " << endl;
  m->print();

  SimulatorState state(m);
  state.setValue("self.in_arg_0_0_0", BitVector(16, 0));
  state.setValue("self.in_en", BitVector(1, 0));
  state.setClock("self.clk", 0, 1);
  state.setValue("self.reset", BitVector(1, 1));

  state.resetCircuit();

  state.setValue("self.reset", BitVector(1, 0));

  int maxCycles = 100;
  int cycles = 0;
  

  std::string inputName = "self.in_arg_0_0_0";
  std::string outputName = "self.out_0_0";
  CoordinateVector<int> writeIdx({"y", "x", "c"}, {hwInputBuf.height() - 1, hwInputBuf.width() - 1, hwInputBuf.channels() - 1});
  CoordinateVector<int> readIdx({"y", "x", "c"}, {outputBuf.height() - 1, outputBuf.width() - 1, outputBuf.channels() - 1});
  
  while (cycles < maxCycles && !readIdx.allDone()) {
    cout << "Read index = " << readIdx.coordString() << endl;
    cout << "Cycles     = " << cycles << endl;


    run_for_cycle(writeIdx, readIdx,
        hwInputBuf, outputBuf,
        inputName, outputName,
        state);
    cycles++;
  }

  cout << "final buffer" << endl;
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      for (int b = 0; b < 1; b++) {
        cout << (int) outputBuf(i, j, b) << " ";
        assert(outputBuf(i, j, b) == cpuOutput(i, j, b));
      }
    }
    cout << endl;
  }
  deleteContext(context);
 
  cout << GREEN << "Conv 3x3 test passed" << RESET << endl;
}

void pointwise_add_test() {

    Var x, y;
    Var xo, yo, xi, yi;

    ImageParam input(type_of<uint8_t>(), 2);

    Func hwInput("hw_input");
    Func hwOutput("hw_output");
    Func brighter("brighter");
    
    hwInput(x, y) = input(x, y);
    brighter(x, y) = hwInput(x, y) + 10;
    hwOutput(x, y) = brighter(x, y);

    hwInput.compute_root();
    hwOutput.compute_root();

    hwOutput.tile(x, y, xo, yo, xi, yi, 4, 4).hw_accelerate(xi, xo);
    brighter.linebuffer();
    
    hwInput.stream_to_accelerator();
    
    Context* context = newContext();
    vector<Argument> args{input};
    auto m = buildModule(context, "coreir_brighter", args, "brighter", hwOutput);
    SimulatorState state(m);

    state.setValue("self.reset", BitVector(1, 1));
    state.setValue("self.in_en", BitVector(1, 1));
    state.setValue("self.in_arg_0_0_0", BitVector(16, 123));
    
    state.resetCircuit();
    cout << "Starting to execute" << endl;

    state.setValue("self.reset", BitVector(1, 0));
    state.exeCombinational();

    assert(state.getBitVec("self.out_0_0") == BitVec(16, 123 + 10));
    assert(state.getBitVec("self.valid") == BitVec(1, 1));

    deleteContext(context);

    cout << GREEN << "Pointwise add passed!" << RESET << endl;
}

int main(int argc, char **argv) {

  small_demosaic_test();


  assert(false);
  multi_channel_conv_test();
  control_path_test();
  control_path_xy_test();
  //assert(false);
  mod2_test();
  shiftRight_test();
  clamp_test();
  //clamped_grad_x_test();
  pointwise_add_test();
  small_conv_3_3_test();
  small_cascade_test();
  //small_harris_test();
  
  cout << GREEN << "All tests passed" << RESET << endl;
  return 0;
}
