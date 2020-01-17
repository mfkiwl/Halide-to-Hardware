#include "CoreIR_Mapper_Compute.h"

#include "coreir.h"

#include "coreir/libs/commonlib.h"
#include "coreir/libs/float.h"

#include "CoreIR_Libs.h"
#include "CodeGen_CoreIR_Target.h"
#include "CodeGen_CoreHLS.h"
#include "Closure.h"
#include "IRMutator.h"
#include "Simplify.h"
#include "RemoveTrivialForLoops.h"
#include "HWUtils.h"
#include "UBufferRewrites.h"
#include "UnrollLoops.h"

using namespace std;
using namespace CoreIR;

namespace Halide {
  namespace Internal {

    class ProduceFinder : public IRGraphVisitor {
      public:

        using IRGraphVisitor::visit;

        const ProducerConsumer* r;
        string target;

        ProduceFinder(const std::string& target_) : r(nullptr), target(target_) {}

        void visit(const ProducerConsumer* rl) override {
          cout << "Searching realize: " << rl->name << " for " << target << endl;
          if (rl->name == target && rl->is_producer) {
            r = rl;
          } else {
            rl->body->accept(this);
          }
        }
    };

    class ComputeExtractor : public IRMutator {
      public:
        using IRMutator::visit;

        int ld_inst;
        int st_inst;

        map<const Provide*, int> provideNums;
        map<const Call*, int> callNums;

        ComputeExtractor() : ld_inst(0), st_inst(0) {}

        Expr visit(const Call* p) override {
          callNums[p] = ld_inst;
          auto fresh = Call::make(p->type,
              "compute_input.stencil",
              {Expr(0), Expr(ld_inst)},
              p->call_type);

          ld_inst++;
          return fresh;
        }

        Stmt visit(const Provide* p) override {
          //internal_assert(!contains_key(p, provideNums)) << "Duplicate provide: " <<
            //Provide::make(p->name, p->values, p->args) << "\n";

          provideNums[p] = st_inst;
          vector<Expr> vals;
          for (auto v : p->values) {
            vals.push_back(this->mutate(v));
          }
          auto fresh = Provide::make("compute_result.stencil", vals, {Expr(0), Expr(st_inst)});
          st_inst++;
          return fresh;
        }

        Stmt visit(const For* f) override {
          Stmt body = this->mutate(f->body);
          return body;
        }

        Stmt visit(const Realize* a) override {
          return this->mutate(a->body);
        }

        Stmt visit(const ProducerConsumer* a) override {
          return this->mutate(a->body);
        }

        Stmt visit(const AssertStmt* a) override {
          return Evaluate::make(0);
        }
    };

    class ComputeUnit {
      public:
        CoreIR::Module* mod;
        std::map<const Variable*, CoreIR::Wireable*> vars;
        std::map<const Call*, CoreIR::Wireable*> inputs;
        std::map<const Provide*, CoreIR::Wireable*> outputs;
    };
    
    ComputeUnit generate_compute_unit(Stmt& simple, std::map<std::string, Function>& env) {
      //internal_assert(active_ctx != nullptr);
      auto context = get_coreir_ctx();
      auto ns = context->getNamespace("global");

      string hw_name = "";
      for (auto f : env) {
        cout << "Checking f: " << f.first << " to see if it is accelerator output" << endl;
        if (f.second.schedule().is_accelerator_output()) {
          hw_name = f.first;
          break;
        }
      }
      if (hw_name != "") {
        internal_assert(hw_name != "") << "No function to accelerate\n";

        RecordType* mtp = context->Record({{"clk", context->Named("coreir.clkIn")}, {"rst", context->BitIn()}});
        auto m = ns->newModuleDecl("m", mtp);
        auto mDef = m->newModuleDef();

        ProduceFinder rFinder(hw_name);
        simple->accept(&rFinder);
        internal_assert(rFinder.r != nullptr);

        vector<pair<string, CoreIR::Type*> > compute_fields{{"clk", context->Named("coreir.clkIn")}, {"rst", context->BitIn()}};
        ComputeExtractor ce;
        Stmt compute_only = simplify(ce.mutate(rFinder.r->body));
        cout << "Compute logic..." << endl;
        cout << compute_only << endl;
        HardwareInfo info;
        info.interfacePolicy =
          HW_INTERFACE_POLICY_COMPUTE_UNIT;

        //Expr inLen((int) ce.callNums.size());
        Expr inLen((int) ce.ld_inst);
        Range rng(Expr((int) 0), inLen);
        Range z(Expr((int) 0), Expr((int) 1));
        vector<CoreIR_Argument> compute_args;
        Stencil_Type inTp{Stencil_Type::StencilContainerType::AxiStream,
          Int(16),
          {z, rng},
          0};
        compute_args.push_back({"compute_input.stencil", true, false, Int(16), inTp});

        //Expr outLen((int) ce.provideNums.size());
        Expr outLen((int) ce.st_inst);
        cout << "compute_result outLen = " << outLen << endl;
        cout << "---- Provides" << endl;
        for (auto pr : ce.provideNums) {
          cout << "\t" << pr.second << endl;
        }
        cout << "compute st nums       = " << ce.st_inst << endl;
        Range outRng(Expr((int) 0), outLen);
        Stencil_Type outTp{Stencil_Type::StencilContainerType::AxiStream,
          Int(16),
          {z, outRng},
          0};
        compute_args.push_back({"compute_result.stencil", true, true, Int(16), outTp});
        Closure interface;
        compute_only.accept(&interface);
        cout << "Interface..." << endl;
        cout << "\tExternal vars..." << endl;
        for (auto v : interface.vars) {
          cout << "\t\t" << v.first << endl;
          compute_args.push_back({v.first, false, false, v.second, outTp});
        }

        //cout << "\t# external buffers = " << interface.buffers.size() << endl;
        //internal_assert(interface.buffers.size() == 0);
        auto compute_unit =
          createCoreIRForStmt(context, info, compute_only, "compute_unit", compute_args);
        mDef->addInstance("compute_unit", compute_unit);


        m->setDef(mDef);
        cout << "Output module" << endl;
        m->print();
        return {m};
      }


      //deleteContext(context);
      return {nullptr};
    }

    void generate_mapped_buffers(Stmt& stmt,
        std::map<std::string, Function>& env,
        std::vector<HWXcel>& buf_xcels) {
      auto pre_simple = simplify(stmt);
      cout << "Pre simplification..." << endl;
      cout << pre_simple << endl;
      Stmt simple = simplify(remove_trivial_for_loops(simplify(unroll_loops(pre_simple))));
      cout << "Simplified code for stmt:" << endl;
      cout << simple << endl;

      CoreIR::Context* context = newContext();
      internal_assert(context != nullptr);

      CoreIRLoadLibrary_commonlib(context);
      loadHalideLib(context);

      coreir_builder_set_context(context);

      //cout << "active_ctx = " << active_ctx << endl;
      //internal_assert(active_ctx != nullptr);

      synthesize_hwbuffers(simple, env, buf_xcels);
      ComputeUnit cu = generate_compute_unit(simple, env);
      if (cu.mod != nullptr) {
        cout << "Compute unit..." << endl;
        cu.mod->print();
      }

      //active_ctx = nullptr;
      deleteContext(context);
    }



  }
}
