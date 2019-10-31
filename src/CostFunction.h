#ifndef HALIDE_COST_FUNCTION_H
#define HALIDE_COST_FUNCTION_H

/** \file
 *
 * Evaluate an IR graph's cost for autoscheduling purposes.
 */

#include "Bounds.h"
#include "Debug.h"
#include "ExprUsesVar.h"
#include "IRMutator.h"
#include "IROperator.h"
#include "IRPrinter.h"
#include "IRVisitor.h"
#include "Scope.h"
#include "Simplify.h"
#include "Substitute.h"
#include "Var.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <set>


namespace Halide {
namespace Internal {

/** Compute the cost of a pipeline by searching over the IR graph
 * looks at: area */
class CostFunction : public IRVisitor {
  using IRVisitor::visit;

  void visit(const Add *op) override {
    area += 1;
    IRVisitor::visit(op);
  }

  void visit(const Mul *op) override {
    area += 100;
    IRVisitor::visit(op);
  } 

public:
  int area = 0;
  int energy = 0;
};

/** Run the CostFunction and return the cost */
int compute_cost(Stmt s);

}  // namespace Internal
}  // namespace Halide

#endif
