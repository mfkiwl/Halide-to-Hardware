#include "CostFunction.h"

#include <iostream>
#include "Func.h"
#include "IRVisitor.h"
#include "IRMutator.h"
#include "IROperator.h"
#include "Scope.h"
#include "Debug.h"
#include "Substitute.h"
#include "IRPrinter.h"
#include "Simplify.h"
#include "Bounds.h"
#include "ExprUsesVar.h"
#include "Var.h"

#include <algorithm>

namespace Halide {
namespace Internal {

using std::ostream;
using std::endl;
using std::string;
using std::vector;
using std::ostringstream;
using std::ofstream;
using std::cout;

int compute_cost(Stmt s) {
  CostFunction cf;
  s.accept(&cf);
  return cf.area;
}

}  // namespace Internal
}  // namespace Halide
