#include "PreprocessHWLoops.h"

#include "IRMutator.h"
#include "Simplify.h"
#include "RemoveTrivialForLoops.h"
#include "HWUtils.h"
#include "UnrollLoops.h"

#include "coreir.h"

using namespace std;
using namespace CoreIR;

namespace Halide {

  namespace Internal {


    bool MemoryInfoCollector::isROM(const std::string& name) const {
      return CoreIR::elem(name, roms());
    }

    class InnermostLoopChecker : public IRGraphVisitor {
      public:
        bool foundSubLoop;

        InnermostLoopChecker() : foundSubLoop(false) {}

      protected:

        using IRGraphVisitor::visit;

        void visit(const For* f) override {
          foundSubLoop = true;
        }
    };

bool isInnermostLoop(const For* f) {
  InnermostLoopChecker c;
  f->body->accept(&c);
  return !c.foundSubLoop;
}

class LetPusher : public IRMutator {
  public:

    vector<const LetStmt*> letStack;

  protected:
    using IRMutator::visit;

    Stmt visit(const LetStmt* let) override {
      letStack.push_back(let);
      auto res = IRMutator::visit(let);
      letStack.pop_back();

      return res;
    }

    Stmt visit(const For* f) override {
      if (isInnermostLoop(f)) {
        cout << "Found innermost loop with var: " << f->name << endl;
        Stmt lBody = f->body;
        for (int i = ((int) letStack.size()) - 1; i >= 0; i--) {
          auto let = letStack[i];
          lBody = LetStmt::make(let->name, let->value, lBody);
        }

        return For::make(f->name, f->min, f->extent, f->for_type, f->device_api, lBody);
      } else {
        return IRMutator::visit(f);
      }
    }
};

class LetEraser : public IRMutator {

  public:

  protected:

    using IRMutator::visit;
    
    Stmt visit(const LetStmt* let) override {
      auto newBody = this->mutate(let->body);
      return newBody;
    }

    Stmt visit(const For* f) override {
      if (isInnermostLoop(f)) {
        return f;
      } else {
        return IRMutator::visit(f);
      }
    }
};

    int MemoryInfoCollector::romValue(const std::string& name, const int addr) {
      internal_assert(memOps.size() > 0);
      for (int i = ((int) memOps.size()) - 1; i >= 0; i--) {
        auto op = memOps[i];
        if (op.isStore() && is_const(op.store->index)) {
          int a = *as_const_int(op.store->index);
          if (a == addr) {
            internal_assert(is_const(op.store->value));
            return *as_const_int(op.store->value);
          }
        }
      }
      internal_assert(false);
      return 0;
    }

class ROMReadOptimizer : public IRMutator {
  public:

    MemoryInfoCollector mic;

    using IRMutator::visit;

    Expr visit(const Load* ld) {
      if (mic.isROM(ld->name) &&
          is_const(ld->index)) {
        int addr = *as_const_int(ld->index);
        int value = mic.romValue(ld->name, addr);
        return IntImm::make(Int(32), value);
      }
      
      return IRMutator::visit(ld);
    }
};


  Stmt preprocessHWLoops(const Stmt& stmt) {
    LetPusher pusher;
    Stmt pushed = pusher.mutate(stmt);
    LetEraser letEraser;
    Stmt r = letEraser.mutate(pushed);
    ROMReadOptimizer romReadOptimizer;

    MemoryInfoCollector mic;
    r.accept(&mic);
    romReadOptimizer.mic = mic;
    auto romOpt = romReadOptimizer.mutate(r);
    return romOpt;
  }

  class  FuncOpCollector : public IRGraphVisitor {
    public:
      using IRGraphVisitor::visit;
      map<string, vector<const Provide*> > provides;
      map<string, vector<const Call*> > calls;

      Expr last_provide_to(const std::string& name, const vector<Expr>& args) const {
        vector<const Provide*> ps = map_find(name, provides);
        CoreIR::reverse(ps);
        for (auto p : ps) {
          bool args_match = true;
          internal_assert(p->args.size() == args.size());
          for (size_t i = 0; i < p->args.size(); i++) {
            auto pa = p->args.at(i);
            auto a = args.at(i);
            if (!is_const(pa) || !is_const(a)) {
              args_match = false;
              break;
            }
            if (id_const_value(pa) != id_const_value(a)) {
              args_match = false;
              break;
            }
          }

          if (args_match) {
            internal_assert(p->values.size() == 1);
            return p->values.at(0);
          }
        }

        internal_assert(false) << "Could not find provide to location: " << name << "\n";
          //<< args << end;
        return args[0];
      }

      void visit(const Provide* p) override {
        map_insert(provides, p->name, p);
        IRGraphVisitor::visit(p);
      }
      
      void visit(const Load* p) override {
        cout << "Found load from:" << p->name << endl;
        //map_insert(calls, p->name, p);
        IRGraphVisitor::visit(p);
      }

      void visit(const Call* p) override {
        cout << "Found call to:" << p->name << endl;
        map_insert(calls, p->name, p);

        IRGraphVisitor::visit(p);
      }

      bool is_rom(const std::string& func) const {
        if (!contains_key(func, provides)) {
          // If we have no information about this function assume it is
          // not going to be stored in a rom
          return false;
        }

        auto ps = map_find(func, provides);
        for (auto p : ps) {
          for (auto v : p->values) {
            if (!is_const(v)) {
              return false;
            }
          }
          for (auto a : p->args) {
            if (!is_const(a)) {
              return false;
            }
          }
        }
        return true;
      }
  };

  class ROMLoadFolder : public IRMutator {
    public:

      FuncOpCollector mic;

      using IRMutator::visit;

      Expr visit(const Call* ld) override {
        //cout << "Mutating: " << ld->name << endl;
        //internal_assert(false);

        Expr newLd = IRMutator::visit(ld);

        if (ld->call_type != Call::CallType::Halide) {
          return newLd;
          //return Call::make(ld->type, ld->name, ld->args, ld->call_type,
              //ld->func,
              //ld->value_index,
              //ld->image,
              //ld->param);

        }
        if (mic.is_rom(ld->name)) {
          cout << "found call to rom: " << ld->name << endl;
          //internal_assert(false) << ld->name << " is rom!\n";
          bool all_const_addr = true;
          for (auto a : ld->args) {
            if (!is_const(a)) {
              cout << "\targument: " << a << " is not const" << endl;
              all_const_addr = false;
              break;
            }
          }

          if (all_const_addr) {
            return mic.last_provide_to(ld->name, ld->args);
          }
        }

        return newLd;
        //return Call::make(ld->type, ld->name, ld->args, ld->call_type,
            //ld->func,
            //ld->value_index,
            //ld->image,
            //ld->param);
      }

  };

  Stmt constant_fold_rom_buffers(const Stmt& stmt) {
    auto pre_simple = simplify(stmt);
    cout << "Pre simplification..." << endl;
    cout << pre_simple << endl;
    Stmt simple = simplify(remove_trivial_for_loops(simplify(unroll_loops(pre_simple))));
    cout << "Simplified code for stmt:" << endl;
    cout << simple << endl;

    FuncOpCollector mic;
    simple.accept(&mic);

    cout << "### New buffers in constant fold ROMs" << endl;
    for (auto b : mic.provides) {
      cout << "\t" << b.second.size() << " Provides to: " << b.first << endl;
      cout << "\t\tis rom " << mic.is_rom(b.first) << endl;
      if (contains_key(b.first, mic.calls)) {
        cout << "\t" << map_find(b.first, mic.calls).size() << " calls to: " << b.first << endl;
      }
    }

    ROMLoadFolder folder;
    folder.mic = mic;
    Stmt replaced = folder.mutate(simple);

    cout << "After ROM simplification..." << endl;
    cout << replaced << endl;

    //internal_assert(false) << "Stopping so dillon can view\n";
    return replaced;
  }


}
}
