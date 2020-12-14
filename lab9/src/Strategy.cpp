#include "Strategy.h"

/* Comment to use the DFS search for bonus points
 * Uncomment to use the naive search */
#define NAIVE_SEARCH

#ifdef NAIVE_SEARCH
/* Baseline solution that works for given test cases 
 * Idea: naive negation of the last path condition (i.e., leaf node negation)
 * Drawback: It is not an ideal solution: fail to terminate or fail to explore all paths
 * */
void searchStrategy(z3::expr_vector &OldVec) {
  // Alter the current path formula that will be given to Z3 so that it will derive a new input by solving the new set of z3 constraints
  // z3::expr_vector API: https://z3prover.github.io/api/html/classz3_1_1ast__vector__tpl.html
  auto prev = OldVec.back();
  auto newz3 = !prev;
  OldVec.pop_back();
  OldVec.push_back(newz3);
}
#else
// As mentioned in Piazza, naive solution doesn't always work even for the naive example below
// foo() {
//  if (C1) {
//    if (C2) S1 else S2
//  }  else {
//    if (C3) S3 else S4
//  }
//}
/* An improved search strategy for bonus points
 * Idea: for a given OldVec, negate the nodes along the OldVec paths from leaf to root to explore all possible paths
 * Example: given C1&C2, we push to stack: !C1, C1&!C2
 * */
#include <stack>
#include <set>
// A stack to store the possible paths to solve
static std::stack<z3::expr_vector> PathsToSolveStack;
// A set of explored paths to avoid duplication
static std::set<z3::expr_vector> ExploredPaths;
void searchStrategy(z3::expr_vector &OldVec) {
  // Initialize PrefixVec with the same context
  z3::expr_vector PrefixVec(OldVec.ctx());
  for(int i = 0; i < OldVec.size(); i++) {
    auto NewNode = !OldVec[i];
    // Inherit the prefix conditions
    z3::expr_vector NewVec(OldVec.ctx());
    for(auto Prefix : PrefixVec) {
      NewVec.push_back(Prefix);
    }
    NewVec.push_back(NewNode);
    // Push NewVec to PathVecsToSolve and remember to avoid revisiting
    // Only push NewVec to stack when it hasn't been visited
    if(ExploredPaths.find(NewVec)==ExploredPaths.end()) {
      PathsToSolveStack.push(NewVec);
      ExploredPaths.insert(NewVec);
    }
    // PrefixVec should include Node now
    PrefixVec.push_back(OldVec[i]);
  }
  // The returning OldVec should be the top of stack
  while(!OldVec.empty()) {
    OldVec.pop_back();
  }
  z3::expr_vector RetVec = PathsToSolveStack.top();
  PathsToSolveStack.pop();
  for(auto Node : RetVec) {
    OldVec.push_back(Node);
  }
}
#endif
