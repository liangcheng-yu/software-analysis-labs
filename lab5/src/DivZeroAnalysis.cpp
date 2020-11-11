#include "DivZeroAnalysis.h"
namespace dataflow {
//===----------------------------------------------------------------------===//
// DivZero Analysis Implementation
//===----------------------------------------------------------------------===//
/**
 * Implement your data-flow analysis with pointer-aliasing functionality. 
 * PART 1: Handle function arguments and any CallInst iff it returns an int.
 * PART 2: Handle StoreInst and LoadInst, accounting for may-aliases when pointers are present
 */

// Comment out to disable errs() print
// #define DEBUG 1
// Uncomment to enable cmp optimization
#define CMP_OPT 1

// Helper API to get the domain value for the given Value object
Domain* getDomainFromValue(Value* value, const Memory* In) {
  // First check if it could be most precise
  if(ConstantInt* constInt = dyn_cast<ConstantInt>(value)) {
    if(constInt->isZero()) {
      return new Domain(Domain::Zero);
    } else {
      return new Domain(Domain::NonZero);
    }
  } 
  // If value can be found
  if(In->find(variable(value))!=In->end()) {
    return In->find(variable(value))->second;
  } else {
    // If value doesn't exist in InMemory, return the least precise top element
    return new Domain(Domain::MaybeZero);
  }
}

/* Given CmpInst, return its abstract value */
Domain* evalCmpInst(CmpInst* cmpInst, const Memory* In) {
  // First find the abstract value for the two operands
  Domain* domain1 = new Domain();
  Domain* domain2 = new Domain();
  ConstantInt* constInt1 = dyn_cast<ConstantInt>(cmpInst->getOperand(0));
  ConstantInt* constInt2 = dyn_cast<ConstantInt>(cmpInst->getOperand(1));
  // Note: don't do constInt1->getSExtValue() to get the actual int value
  // With our abstract domain, the actual value is invisible
  if(constInt1) {
    if(constInt1->isZero()) {
      domain1->Value = Domain::Zero; 
    } else {
      domain1->Value = Domain::NonZero;
    } 
  } 
  // Otherwise, it could be variables 
  else {
    domain1 = getDomainFromValue(cmpInst->getOperand(0), In);
  } 
  if(constInt2) {
    if(constInt2->isZero()) {
      domain2->Value = Domain::Zero; 
    } else {
      domain2->Value = Domain::NonZero;
    } 
  } 
  // Otherwise, it could be variables 
  else {
    domain2 = getDomainFromValue(cmpInst->getOperand(1), In);
  } 

  // Now decide the abstract value of the current instruction/variable
  // Regardless of the cond, if any of the operand is Uninit, the result is Uninit
  if(domain1->Value==Domain::Uninit || domain2->Value==Domain::Uninit) {
    return new Domain(Domain::Uninit);
  }
  // Also, if any of the operand is MaybeZero (ambiguous), the result is MaybeZero
  if(domain1->Value==Domain::MaybeZero || domain2->Value==Domain::MaybeZero) {
    return new Domain(Domain::MaybeZero);
  }
  
  // Now both operands are either Zero or NonZero
  // We need to compute the abstract value based on the type of the condition
  llvm::CmpInst::Predicate pred = cmpInst->getPredicate();
  // Brute force full list of CmpInst cond based on LLVM primer
  // ugt: unsigned greater than
  // uge: unsigned greater or equal
  // ult: unsigned less than
  // ule: unsigned less or equal
  // sgt: signed greater than
  // sge: signed greater or equal
  // slt: signed less than
  // sle: signed less or equal
  // Identify non MaybeZero cases
  /* eq: equal */
  if(pred==CmpInst::Predicate::ICMP_EQ) {
    if(domain1->Value==Domain::Zero && domain2->Value==Domain::Zero) {
      return new Domain(Domain::NonZero);
    } else if(domain1->Value==Domain::Zero && domain2->Value==Domain::NonZero) {
      return new Domain(Domain::Zero);
    } else if(domain1->Value==Domain::NonZero && domain2->Value==Domain::Zero) {
      return new Domain(Domain::Zero);
    }
#ifdef CMP_OPT
    else if(constInt1 && constInt2) {
      if(constInt1->getSExtValue()==constInt2->getSExtValue()) {
        return new Domain(Domain::NonZero);
      } else {
        return new Domain(Domain::Zero);
      }
    }
#endif
  } 
  /* ne: not equal */
  else if(pred==CmpInst::Predicate::ICMP_NE) {
    if(domain1->Value==Domain::Zero && domain2->Value==Domain::Zero) {
      return new Domain(Domain::Zero);
    } else if(domain1->Value==Domain::Zero && domain2->Value==Domain::NonZero) {
      return new Domain(Domain::NonZero);
    } else if(domain1->Value==Domain::NonZero && domain2->Value==Domain::Zero) {
      return new Domain(Domain::NonZero);
    }
#ifdef CMP_OPT
    else if(constInt1 && constInt2) {
      if(constInt1->getSExtValue()==constInt2->getSExtValue()) {
        return new Domain(Domain::Zero);
      } else {
        return new Domain(Domain::NonZero);
      }
    } 
#endif
  } else if(pred==CmpInst::Predicate::ICMP_UGT || pred==CmpInst::Predicate::ICMP_SGT) {
    // Even NonZero and Zero, we can not say the result is NonZero
    if(domain1->Value==Domain::Zero && domain2->Value==Domain::Zero) {
      return new Domain(Domain::Zero);
    } 
#ifdef CMP_OPT
    else if(constInt1 && constInt2) {
      if(constInt1->getSExtValue()>constInt2->getSExtValue()) {
        return new Domain(Domain::NonZero);
      } else {
        return new Domain(Domain::Zero);
      }
    }
    else if(constInt1 && domain2->Value==Domain::Zero) {
      if(constInt1->getSExtValue()>0) {
        return new Domain(Domain::NonZero);
      } else {
        return new Domain(Domain::Zero);
      }
    }
    else if(domain1->Value==Domain::Zero && constInt2) {
      if(0>constInt2->getSExtValue()) {
        return new Domain(Domain::NonZero);
      } else {
        return new Domain(Domain::Zero);
      }
    }
#endif
  } else if(pred==CmpInst::Predicate::ICMP_UGE || pred==CmpInst::Predicate::ICMP_SGE) {
    if(domain1->Value==Domain::Zero && domain2->Value==Domain::Zero) {
      return new Domain(Domain::NonZero);
    }
#ifdef CMP_OPT
    else if(constInt1 && constInt2) {
      if(constInt1->getSExtValue()>=constInt2->getSExtValue()) {
        return new Domain(Domain::NonZero);
      } else {
        return new Domain(Domain::Zero);
      }
    }
    else if(constInt1 && domain2->Value==Domain::Zero) {
      if(constInt1->getSExtValue()>=0) {
        return new Domain(Domain::NonZero);
      } else {
        return new Domain(Domain::Zero);
      }
    }
    else if(domain1->Value==Domain::Zero && constInt2) {
      if(0>=constInt2->getSExtValue()) {
        return new Domain(Domain::NonZero);
      } else {
        return new Domain(Domain::Zero);
      }
    }
#endif
  } else if(pred==CmpInst::Predicate::ICMP_ULT || pred==CmpInst::Predicate::ICMP_SLT) {
    if(domain1->Value==Domain::Zero && domain2->Value==Domain::Zero) {
      return new Domain(Domain::Zero);
    } 
#ifdef CMP_OPT
    else if(constInt1 && constInt2) {
      if(constInt1->getSExtValue()<constInt2->getSExtValue()) {
        return new Domain(Domain::NonZero);
      } else {
        return new Domain(Domain::Zero);
      }
    }
    else if(constInt1 && domain2->Value==Domain::Zero) {
      if(constInt1->getSExtValue()<0) {
        return new Domain(Domain::NonZero);
      } else {
        return new Domain(Domain::Zero);
      }
    }
    else if(domain1->Value==Domain::Zero && constInt2) {
      if(0<constInt2->getSExtValue()) {
        return new Domain(Domain::NonZero);
      } else {
        return new Domain(Domain::Zero);
      }
    }
#endif
  } else if(pred==CmpInst::Predicate::ICMP_ULE || pred==CmpInst::Predicate::ICMP_SLE) {
    if(domain1->Value==Domain::Zero && domain2->Value==Domain::Zero) {
      return new Domain(Domain::NonZero);
    }
#ifdef CMP_OPT
    else if(constInt1 && constInt2) {
      if(constInt1->getSExtValue()<=constInt2->getSExtValue()) {
        return new Domain(Domain::NonZero);
      } else {
        return new Domain(Domain::Zero);
      }
    }
    else if(constInt1 && domain2->Value==Domain::Zero) {
      if(constInt1->getSExtValue()<=0) {
        return new Domain(Domain::NonZero);
      } else {
        return new Domain(Domain::Zero);
      }
    }
    else if(domain1->Value==Domain::Zero && constInt2) {
      if(0<=constInt2->getSExtValue()) {
        return new Domain(Domain::NonZero);
      } else {
        return new Domain(Domain::Zero);
      }
    }
#endif
  }
  // If no clue, return top element
  return new Domain(Domain::MaybeZero);
}

/* This function can used to evaluate Instruction::PHI */
Domain *evalPhiNode(PHINode *PHI, const Memory *Mem) {
  Value* cv = PHI->hasConstantValue();
  if(cv){
    ConstantInt* constInt = dyn_cast<ConstantInt>(cv);
    if(constInt->isZero()) {
      return new Domain(Domain::Zero);
    } else {
      return new Domain(Domain::NonZero);
    }
  }
  unsigned int n = PHI->getNumIncomingValues();
  Domain* joined = NULL;
  for(unsigned int i = 0; i < n; i++){
    // eval PHI->getIncomingValue(i), translate it to a Domain
    Value* valuei = PHI->getIncomingValue(i);
    Domain* domaini = NULL;
    if(ConstantInt* constInt  = dyn_cast<ConstantInt>(valuei)) {
      if(constInt->isZero()) {
        domaini = new Domain(Domain::Zero);
      } else {
        domaini = new Domain(Domain::NonZero);
      } 
    }else {
      if(Mem->find(variable(valuei)) != Mem->end()) {
        domaini = new Domain(Mem->find(variable(valuei))->second->Value); 
      } else {
        // No clue, hence top
        domaini = new Domain(Domain::MaybeZero);
      }
    }
    if(!joined){
      joined = domaini;
    }
    joined = Domain::join(joined, domaini);
  }
  return joined;
}

/* This function is intended to return the union of two Memory objects (M1 and M2), accounting for Domain values.
 * M1 is assumed to be the primary Memory to return */
Memory* join(Memory *M1, Memory *M2) {
  // Iterate each variable->abstract value pair
  for(auto iter2=M2->begin(); iter2!=M2->end(); iter2++) {
    auto iter1 = M1->find(iter2->first);
    if(iter1 != M1->end()) {
      // We need to merge the two facts
      iter1->second = Domain::join(iter1->second, iter2->second);
    } else {
      M1->insert(std::pair<std::string, Domain*>(iter2->first, iter2->second));
    }
  } 
  return M1;
}

/* Return true if the two memories M1 and M2 are equal */
bool equal(Memory *M1, Memory *M2) {
  // First check if the size equals
  if(M1->size() != M2->size()) {
    return false;
  }
  // Now check if each element in M2 exists in M1, return false when one counterexample is found
  for(auto iter2=M2->begin(); iter2!=M2->end(); iter2++) {
    auto iter1 = M1->find(iter2->first);
    if(iter1 != M1->end()) {
      // Also need to check if the Domain is the same
      if(!Domain::equal(*(iter1->second), *(iter2->second))) {
        return false; 
      }
    } else {
      return false;
    } 
  }
  return true;
}

/* Flow abstract domain from all predecessors of I into the In Memory object for I. */
void DivZeroAnalysis::flowIn(Instruction *I, Memory *In) {
  // join function provided
  std::vector<Instruction*> predInst = getPredecessors(I);
  // Before merge, clear In since the algorithm update for In is stateless
  // Otherwise, it may remember some old state which will join to weird MaybeZero, e.g., in simple1.c
  // If it is first instruction, don't clean up
  if(predInst.size()!=0) {
    In->clear();
  }
  for(Instruction* inst : predInst) {
    // Get OutMem for inst, which always exists
    if(OutMap.find(inst) != OutMap.end()) {
      // We need to merge OutMap[inst] and In
      In = join(In, OutMap[inst]);
    }
  }
}

// Create a transfer function that updates the Out Memory based on In Memory and the instruction type/parameters */
void DivZeroAnalysis::transfer(Instruction *I, const Memory *In, Memory *NOut,
                               PointerAnalysis *PA,
                               SetVector<Value *> PointerSet) {
/* Given the current InMemory and Instruction, update the Out Memory. 
 * This API is called by the main doAnalysis algorithm.
 * Note that the reference lib iterates from the back of the instruction.
 **/
  // Before calculating KILL[n]/GEN[n], first mirror IN[n] to OUT[n]
  NOut->clear();
  NOut->insert(In->begin(), In->end());
  // We consider the following instructions, no need for LoadInst, StoreInst in this lab
  /* BinaryOperator: add, sub, mul, udiv, sdiv (the complete list in LLVM primer) */
  if(BinaryOperator* binaryInst = dyn_cast<BinaryOperator>(I)) {
    // Get the two operands' abstract value
    Domain* domain0 = getDomainFromValue(I->getOperand(0), In);
    Domain* domain1 = getDomainFromValue(I->getOperand(1), In);
    Domain* domain2;
    // Calculate the resulting abstract value
    if(binaryInst->getOpcode()==Instruction::Add) {
      domain2 = Domain::add(domain0, domain1); 
    } else if(binaryInst->getOpcode()==Instruction::Sub) {
      domain2 = Domain::sub(domain0, domain1); 
    } else if(binaryInst->getOpcode()==Instruction::Mul) {
      domain2 = Domain::mul(domain0, domain1); 
    } else if(binaryInst->getOpcode()==Instruction::UDiv) {
      domain2 = Domain::div(domain0, domain1);
    } else if(binaryInst->getOpcode()==Instruction::SDiv) {
      domain2 = Domain::div(domain0, domain1);
    } else {
      // Do not need to handle XOR, OR, AND ...
      domain2 = new Domain(Domain::MaybeZero);
      return;
    }
    // If OutMap doesn't have variable I, insert the new I=>abstract value pair
    if(NOut->find(variable(I)) != NOut->end()) {
      (*NOut)[variable(I)] = domain2; 
    }
    // otherwise, update the new abstract value
    else {
      NOut->insert(std::pair<std::string, Domain*>(variable(I), domain2));
    }
  }
  /* Cast instruction */
  else if(CastInst* castInst = dyn_cast<CastInst>(I)) {
    // Get the only operand's abstract value
    Domain* domain1 = getDomainFromValue(I->getOperand(0), In);
    // Again, update OutMap with the new abstract value
    if(NOut->find(variable(I)) != NOut->end()) {
      (*NOut)[variable(I)] = domain1;
    } else {
      NOut->insert(std::pair<std::string, Domain*>(variable(I), domain1));
    }
  } 
  /* Comapre instruction: imcp eq, ne, slt, sgt, sge etc */
  else if(CmpInst* cmpInst = dyn_cast<CmpInst>(I)) {
    // Compare instruction is more complicated, hence, we extract a standalone API
    Domain* domain1 = evalCmpInst(cmpInst, In);
    if(NOut->find(variable(I)) != NOut->end()) {
      (*NOut)[variable(I)] = domain1;
    } else {
      NOut->insert(std::pair<std::string, Domain*>(variable(I), domain1));
    }
  }
//  else if(BranchInst* Branch = dyn_cast<BranchInst>(I)) {
    // We don't need to intepret the conditionals, though doing so is more accurate 
    // Example BranchInst: br i1 %cmp, label %IfEqual, label %IfUnequal
//  }
  /* User input via getchar() */
  else if(isInput(I)) {
    // The user input should be initialized to least precise abstract value, i.e., top
    if(NOut->find(variable(I)) != NOut->end()) {
      (*NOut)[variable(I)] = new Domain(Domain::MaybeZero);
    } else {
      NOut->insert(std::pair<std::string, Domain*>(variable(I), new Domain(Domain::MaybeZero)));
    }
  }
  /* Phi node (merge point due to SSA) */
  else if (PHINode *phiNode = dyn_cast<PHINode>(I)) {
    Domain* domain1 = evalPhiNode(phiNode, In);
    if(NOut->find(variable(I)) != NOut->end()) {
      (*NOut)[variable(I)] = domain1;
    } else {
      NOut->insert(std::pair<std::string, Domain*>(variable(I), domain1));
    }
  }
  else if(CallInst *CI = dyn_cast<CallInst>(I)) {
    if(CI->getCalledFunction()->getReturnType()->isIntegerTy()) {
      Domain* domain1 = new Domain(Domain::MaybeZero);
      if(NOut->find(variable(I)) != NOut->end()) {
	(*NOut)[variable(I)] = domain1;
      } else {
        NOut->insert(std::pair<std::string, Domain*>(variable(I), domain1));
      }
    } 
  }
  else if(AllocaInst *AI = dyn_cast<AllocaInst>(I)) {
    /* No abstract state changes for AllocaInst */
  }
  else if(StoreInst *SI = dyn_cast<StoreInst>(I)) {
    // Example: store i32 %div, i32* %x, align 4
    // 		getValueOperand()  %div = sdiv i32 1, %2
    // 		getPointerOperand()  %x = alloca i32, align 4
    // Example: store i32* %a, i32** %d, align 8
    // 		getValueOperand()  %a = alloca i32, align 4
    // 		getPointerOperand()  %d = alloca i32*, align 8
    // Example: store i32 0, i32* %0, align 4
    // 		getValueOperand()  i32 0
    // 		getPointerOperand()  %0 = load i32*, i32** %c, align 8
    Domain* joined = new Domain(Domain::MaybeZero);
    if(SI->getValueOperand()->getType()->isIntegerTy()) {
      ConstantInt *constInt = dyn_cast<ConstantInt>(SI->getValueOperand());
      if(constInt) {
        if(constInt->isZero()) {
          joined = new Domain(Domain::Zero);
	} else {
	  joined = new Domain(Domain::NonZero);
	}
      }
      else {
//	// Could be: %div = sdiv i32 1, %2
        auto iter = In->find(variable(SI->getValueOperand()));
	if(iter!=In->end()) {
	  joined = iter->second;
	}
      }
      // Join all alias with PointerOperand efficiently (with a single loop)
      // Assign it and make sure all alias to the same object are in sync
      std::string pointerOperandVariable = variable(SI->getPointerOperand());
      for(Value* value : PointerSet) {
        std::string variableTmp = variable(value);
        if(PA->alias(variableTmp, pointerOperandVariable)) {
          auto iter = NOut->find(variableTmp);
          if(iter!=NOut->end()) {
	    (*NOut)[variableTmp] = Domain::join(joined, iter->second);
          } else {
	    NOut->insert(std::pair<std::string, Domain*>(variableTmp, joined));
	  }
        }
      }
    }
  } else if(LoadInst *LI = dyn_cast<LoadInst>(I)) {
    /* The pointer analysis was done a priori, no need to check alias since load only writes to a new variable
     * only need to update the abstract value for variable LoadInst */
    // Inherit the abstract value of the existing variable  
    Domain* newDomain = new Domain(Domain::MaybeZero);
//    if(LI->getType()->isPointerTy()) {
      // Example: %1 = load i32*, i32** %d, align 8
      // getPointerOperand(): %d = alloca i32*, align 8
    if(LI->getType()->isIntegerTy()) {
      // Example: %3 = load i32, i32* %x, align 4
      // getPointerOperand(): %x = alloca i32, align 4
      auto iter = In->find(variable(LI->getPointerOperand()));
      if(iter!=In->end()) {
        newDomain = iter->second;
      }   
      if(NOut->find(variable(I))!=NOut->end()) {
//        (*NOut)[variable(I)] = newDomain;
      } else {
        NOut->insert(std::pair<std::string, Domain*>(variable(I), newDomain));
      }
    }
  }
}

/* For a given instruction, update WorkSet as needed */
void DivZeroAnalysis::flowOut(Instruction *I, Memory *Pre, Memory *Post, SetVector <Instruction *> &WorkSet) {
  if(equal(Pre, Post)) {
    WorkSet.remove(I);
  } else {
    // Flow new changes to successors
    std::vector<Instruction*> succInst = getSuccessors(I);
    for(auto inst : succInst) {
      WorkSet.insert(inst);
    }
  }
}

/* Reaching definitions analysis implementation with chaotic algorithm (forward may)

Note that the chaotic algorithm doesn't specify the order of traversal.
Actually the iteration order doesn't matter for correctness of the algorithm, only for time complexity.

Basic Workflow:
  - Visit instruction in WorkSet
  - For each visited instruction I, construct its In memory by joining all memory sets of incoming flows (predecessors of I)
  - Based on the type of instruction I and the In memory, populate the NOut memory. Take the pointer analysis into consideration for this step.
  - Based on the previous Out memory and the current Out memory, check if there is a difference between the two and
     flow the memory set appropriately to all successors of I and update WorkSet accordingly
*/
void DivZeroAnalysis::doAnalysis(Function &F, PointerAnalysis *PA) {
  SetVector<Instruction *> workSet;
  SetVector<Value *> pointerSet;
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    workSet.insert(&(*I));
    pointerSet.insert(&(*I));
  }
  /* Initialize the memory with function arguments */
  Memory* argMemory = new Memory;
  for(auto arg = F.arg_begin(); arg != F.arg_end(); ++arg) {
    // Don't just dyn_cast<ConstantInt>
    if(arg->getType()->isIntegerTy()) {
       // No need to extract constInt->getValue()
       argMemory->insert(std::pair<std::string, Domain*>(variable(arg), new Domain(Domain::MaybeZero)));
    }
  }
  InMap[&(*inst_begin(F))] = argMemory;
  // Stop when workSet is empty
  while(!workSet.empty()) {
    // Fetch an instruction from workSet, fetch front is much faster (7s) than back (>2min), use front for leaderboard
    Instruction* inst = workSet.back();
    // Call flowIn to join all OutMem of predessors to InMem of current inst
    auto iterIn = InMap.find(inst);
    if(iterIn!=InMap.end()) {
      flowIn(inst, iterIn->second);
      auto iterOut = OutMap.find(inst);
      if(iterOut!=OutMap.end()) {
	// Keep a record of the OutMem before transfer
        Memory prevOutMemory = (*(iterOut->second));
        transfer(inst, iterIn->second, iterOut->second, PA, pointerSet);
        flowOut(inst, &prevOutMemory, iterOut->second, workSet);
      }
    }
  }
}

/* Given an instruction, decide if it incurs a divide-by-zero error based on abstract value.
 * Note that the decision is sound and could misclassify some good programs as buggy.
 */
bool DivZeroAnalysis::check(Instruction *I) {
#ifdef DEBUG
  errs() << "========== " << *I << "==========\n";
  errs() << "---------- InMap -----------\n";
  for(auto iter : *InMap[I]) {
    errs() << iter.first << " => " << *(iter.second) << "\n";
  }
  errs() << "---------- OutMap -----------\n";
  for(auto iter : *OutMap[I]) {
    errs() << iter.first << " => " << *(iter.second) << "\n";
  }
#endif
  // We consider both signed div and unsigned div instructions
  if(I->getOpcode() == Instruction::UDiv || I->getOpcode() == Instruction::SDiv) {
    Value* operand1 = I->getOperand(1);
    // Get the memory for I and then the abstract value for operand1
    // Alternative is to access the second of the iterator (which is a pair)
    Memory* memory = InMap[I];
    if(memory->find(variable(operand1)) != memory->end()) {
      Domain* domain = (*memory)[variable(operand1)];
      // Return true if the abstract domain of the operand is zero of maybe zero
      if(domain->Value == Domain::Zero || domain->Value == Domain::MaybeZero) {
	return true;
      }
    } else {
      // We can't find the variable in memory for ConstantInt
      ConstantInt *constInt = dyn_cast<ConstantInt>(operand1);
      if(constInt) {
	if(constInt->isZero()) {
	  return true;
	}
      }
    }
  }
  // We are confident when return false
  return false;
}

char DivZeroAnalysis::ID = 1;
static RegisterPass<DivZeroAnalysis> X("DivZero", "Divide-by-zero Analysis",
                                       false, false);
} // namespace dataflow
