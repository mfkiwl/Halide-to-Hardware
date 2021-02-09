#include "HWBufferRename.h"
#include "HWBufferSimplifications.h"
#include "IRMutator.h"
#include "IROperator.h"
#include "Simplify.h"
#include "Substitute.h"
#include "UnrollLoops.h"

using std::map;
using std::string;
using std::vector;

namespace Halide {
namespace Internal {

class RenameRealize : public IRMutator {
  const string &orig_name;
  const string &new_name;

  using IRMutator::visit;

  Stmt visit(const ProducerConsumer *op) override {
    if (op->name == orig_name) {
      Stmt new_body = mutate(op->body);
      return ProducerConsumer::make(new_name, op->is_producer, new_body);
    } else {
      return IRMutator::visit(op);
    }
  }
  
  Expr visit(const Call *op) override {
    if (op->name == orig_name ) {
      vector<Expr> new_args;
      for (auto arg : op->args) {
        Expr new_arg = mutate(arg);
        new_args.emplace_back(new_arg);
      }
      //return Call::make(op->type, new_name, new_args, op->call_type);
      return Call::make(op->type, new_name, new_args, Call::Intrinsic);
    } else {
      return IRMutator::visit(op);
    }
  }

  Stmt visit(const Provide *op) override {
    if (op->name == orig_name ) {
      vector<Expr> new_values;
      for (auto value : op->values) {
        Expr new_value = mutate(value);
        new_values.emplace_back(new_value);
      }
      vector<Expr> new_args;
      for (auto arg : op->args) {
        Expr new_arg = mutate(arg);
        new_args.emplace_back(new_arg);
      }
      
      return Provide::make(new_name, new_values, new_args);
    } else {
      return IRMutator::visit(op);
    }
  }

public:
  RenameRealize(const string &o, const string &n)
    : orig_name(o), new_name(n) {}
};


class RenameStencilRealizes : public IRMutator {
  const map<string, Function> &env;
  bool in_xcel;
  
    using IRMutator::visit;

    Stmt visit(const Realize *op) override {
      string funcname;
      if (ends_with(op->name, ".0")) { // complexfuncs get separated with .0 and .1
        funcname = op->name.substr(0, op->name.find(".0"));
      } else if (ends_with(op->name, ".1")) {
        funcname = op->name.substr(0, op->name.find(".1"));
      } else {
        funcname = op->name;
      }

      // do this for share_output, share_input. They aren't in env
      if (starts_with(funcname, "share")) {
        internal_assert(env.count(funcname) == 0);
        string realize_name = op->name + ".stencil";
        //std::cout << op->name << " getting new name " << realize_name << " with type=" << realization_type << std::endl;
        Stmt new_body = RenameRealize(op->name, realize_name).mutate(op->body);
        new_body = mutate(new_body);
        return Realize::make(realize_name, op->types, op->memory_type,
                             op->bounds, op->condition, new_body);
      }
      
      //std::cout << "looking for realize " << op->name << " using name " << funcname << std::endl;
      internal_assert(env.count(funcname) > 0) << "looking for realize " + op->name + " using name " + funcname;
      auto func = env.at(funcname);

      // ROMs should be flattened, so don't append ".stencil"
      auto realization_type = identify_realization(unroll_loops(Stmt(op)), op->name);

      // ROMs tagget as Stack should not be flattened
      bool tagged_rom = func.schedule().memory_type() == MemoryType::Stack;

      if (tagged_rom ||
          (realization_type != ROM_REALIZATION &&
           realization_type != CONSTS_REALIZATION &&
           (in_xcel || func.schedule().is_accelerator_input()))) {
        string realize_name = op->name + ".stencil";
        //std::cout << op->name << " getting new name " << realize_name << " with type=" << realization_type << std::endl;
        Stmt new_body = RenameRealize(op->name, realize_name).mutate(op->body);
        new_body = mutate(new_body);
        return Realize::make(realize_name, op->types, op->memory_type,
                             op->bounds, op->condition, new_body);
        
      } else if (realization_type == CONSTS_REALIZATION &&
                 (in_xcel || func.schedule().is_accelerator_input())) {
        string realize_name = op->name + ".const.stencil";
        //std::cout << op->name << " getting new name " << realize_name << " with type=" << realization_type << std::endl;
        Stmt new_body = RenameRealize(op->name, realize_name).mutate(op->body);
        new_body = mutate(new_body);
        return Realize::make(realize_name, op->types, op->memory_type,
                             op->bounds, op->condition, new_body);
        
      } else {

        if (!func.schedule().is_accelerated()) {
          return IRMutator::visit(op);
        }

        // start of accelerated portion
        string realize_name = op->name + ".stencil";
        Stmt new_body = RenameRealize(op->name, realize_name).mutate(op->body);

        in_xcel = true;
        new_body = mutate(new_body);
        in_xcel = false;
        
        return Realize::make(realize_name, op->types, op->memory_type,
                             op->bounds, op->condition, new_body);
      }
    }

public:
  RenameStencilRealizes(const map<string, Function> &env) : env(env), in_xcel(false){ }
};

Stmt rename_hwbuffers(Stmt s, const map<string, Function> &env) {
    auto renamed = RenameStencilRealizes(env).mutate(s);
    return renamed;
}

}  // namespace Internal
}  // namespace Halide
