#include "Instrument.h"

using namespace llvm;

namespace instrument {

void instrumentDSEInit(Module *M, Function &F, Instruction &I) {
  // First invoke __DSE_init__ that initializes input if input.txt exists
  // otherwise, use random inputs
  LLVMContext &Ctx = M->getContext();
  Value *Fun = M->getOrInsertFunction(DSEInitFunctionName, Type::getVoidTy(Ctx));
  if (Function *NewF = dyn_cast<Function>(Fun)) {
    // Insert CallInst of __DSE_init__ before the original first instruction
    std::vector<Value*> Args; 
    CallInst *Call = CallInst::Create(Fun, Args, "", &I);
    Call->setCallingConv(CallingConv::C);
    Call->setTailCall(true);
  } else {
    errs() << "WARN: invalid function\n";
  }
}

void instrumentDSEAlloca(Module *M, AllocaInst *AI) {
  LLVMContext &Ctx = M->getContext();
  std::vector<Value *> Args;
  Value *V = ConstantInt::get(Type::getInt32Ty(Ctx), getRegisterID(AI));
  Args.push_back(V);
  Args.push_back(AI);
  Type *ArgsTypes[] = {Type::getInt32Ty(Ctx), Type::getInt32PtrTy(Ctx)};
  FunctionType *FType = FunctionType::get(Type::getVoidTy(Ctx), ArgsTypes, false);
  Value *Fun = M->getOrInsertFunction(DSEAllocaFunctionName, FType);
  if (Function *F = dyn_cast<Function>(Fun)) {
      // Insert right after the current AI instruction
      CallInst *Call = CallInst::Create(Fun, Args, "", AI->getNextNonDebugInstruction());
      Call->setCallingConv(CallingConv::C);
      Call->setTailCall(true);
  } else {
      errs() << "WARN: invalid function\n";
  }
}

void instrumentDSEStore(Module *M, StoreInst *SI) {
  LLVMContext &Ctx = M->getContext();
  std::vector<Value *> Args;
  Args.push_back(SI->getPointerOperand());
  Type *ArgsTypes[] = {Type::getInt32PtrTy(Ctx)};
  FunctionType *FType =
      FunctionType::get(Type::getVoidTy(Ctx), ArgsTypes, false);
  Value *Fun = M->getOrInsertFunction(DSEStoreFunctionName, FType);
  if (Function *F = dyn_cast<Function>(Fun)) {
    CallInst *Call = CallInst::Create(Fun, Args, "", SI);
    Call->setCallingConv(CallingConv::C);
    Call->setTailCall(true);
  } else {
    errs() << "WARN: invalid function\n";
  }
}

void instrumentDSELoad(Module *M, LoadInst *LI) {
  // Same signature with Alloca
  LLVMContext &Ctx = M->getContext();
  std::vector<Value *> Args;
  Value *V = ConstantInt::get(Type::getInt32Ty(Ctx), getRegisterID(LI));
  Args.push_back(V);
  Args.push_back(LI->getPointerOperand());
  Type *ArgsTypes[] = {Type::getInt32Ty(Ctx), Type::getInt32PtrTy(Ctx)};
  FunctionType *FType = FunctionType::get(Type::getVoidTy(Ctx), ArgsTypes, false);
  Value *Fun = M->getOrInsertFunction(DSELoadFunctionName, FType);
  if (Function *F = dyn_cast<Function>(Fun)) {
      CallInst *Call = CallInst::Create(Fun, Args, "", LI);
      Call->setCallingConv(CallingConv::C);
      Call->setTailCall(true);
  } else {
      errs() << "WARN: invalid function\n";
  }
}

void instrumentDSEConst(Module *M, Value *V, Instruction *I) {
  LLVMContext &Ctx = M->getContext();
  std::vector<Value *> Args;
  Args.push_back(V);
  Value *Fun = M->getOrInsertFunction(DSEConstFunctionName,
		                      Type::getVoidTy(Ctx),
				      Type::getInt32Ty(Ctx));
  if (Function *F = dyn_cast<Function>(Fun)) {
    CallInst *Call = CallInst::Create(Fun, Args, "", I);
    Call->setCallingConv(CallingConv::C);
    Call->setTailCall(true);
  } else {
    errs() << "WARN: invalid function\n";
  }
}

void instrumentDSERegister(Module *M, Value *V, Instruction *I) {
  LLVMContext &Ctx = M->getContext();
  std::vector<Value *> Args;
  Value *NewV = ConstantInt::get(Type::getInt32Ty(Ctx), getRegisterID(V));
  Args.push_back(NewV);
  Value *Fun = M->getOrInsertFunction(DSERegisterFunctionName,
		                      Type::getVoidTy(Ctx),
				      Type::getInt32Ty(Ctx));
  if (Function *F = dyn_cast<Function>(Fun)) {
    CallInst *Call = CallInst::Create(Fun, Args, "", I);
    Call->setCallingConv(CallingConv::C);
    Call->setTailCall(true);
  } else {
    errs() << "WARN: invalid function\n";
  }
}

void instrumentDSEICmp(Module *M, ICmpInst *I) {
  // ID of the register, LLVM opcode
  LLVMContext &Ctx = M->getContext();
  std::vector<Value *> Args;
  Value *V = ConstantInt::get(Type::getInt32Ty(Ctx), getRegisterID(I));
  Args.push_back(V);
  Value *Opt = ConstantInt::get(Type::getInt32Ty(Ctx), I->getPredicate());
  Args.push_back(Opt);
  Type *ArgsTypes[] = {Type::getInt32Ty(Ctx), Type::getInt32Ty(Ctx)};
  FunctionType *FType = FunctionType::get(Type::getVoidTy(Ctx), ArgsTypes, false);
  Value *Fun = M->getOrInsertFunction(DSEICmpFunctionName, FType);
  if (Function *F = dyn_cast<Function>(Fun)) {
      CallInst *Call = CallInst::Create(Fun, Args, "", I);
      Call->setCallingConv(CallingConv::C);
      Call->setTailCall(true);
  } else {
      errs() << "WARN: invalid function\n";
  }
}

void instrumentDSEBranch(Module *M, BranchInst *BI) {
  // Branch ID of the BranchInst, the Register ID of the BranchInst condition and the condition of the BranchInst
  LLVMContext &Ctx = M->getContext();
  std::vector<Value *> Args;
  Value *V1 = ConstantInt::get(Type::getInt32Ty(Ctx), getBranchID(BI));
  Args.push_back(V1);
  Value *V2 = ConstantInt::get(Type::getInt32Ty(Ctx), getRegisterID(BI->getCondition()));
  Args.push_back(V2);
  Args.push_back(BI->getCondition());
  Type *ArgsTypes[] = {Type::getInt32Ty(Ctx), Type::getInt32Ty(Ctx), BI->getCondition()->getType()};
  FunctionType *FType = FunctionType::get(Type::getVoidTy(Ctx), ArgsTypes, false);
  Value *Fun = M->getOrInsertFunction(DSEBranchFunctionName, FType);
  if (Function *F = dyn_cast<Function>(Fun)) {
    CallInst *Call = CallInst::Create(Fun, Args, "", BI);
    Call->setCallingConv(CallingConv::C);
    Call->setTailCall(true);
  } else {
    errs() << "WARN: invalid function\n";
  }
}

void instrumentDSEBinOp(Module *M, BinaryOperator *BO) {
  // Similar to ICmpInst: ID of the register, LLVM opcode
  LLVMContext &Ctx = M->getContext();
  std::vector<Value *> Args;
  Value *V = ConstantInt::get(Type::getInt32Ty(Ctx), getRegisterID(BO));
  Args.push_back(V);
  Value *Opt = ConstantInt::get(Type::getInt32Ty(Ctx), BO->getOpcode());
  Args.push_back(Opt);
  Type *ArgsTypes[] = {Type::getInt32Ty(Ctx), Type::getInt32Ty(Ctx)};
  FunctionType *FType = FunctionType::get(Type::getVoidTy(Ctx), ArgsTypes, false);
  Value *Fun = M->getOrInsertFunction(DSEBinOpFunctionName, FType);
  if (Function *F = dyn_cast<Function>(Fun)) {
      CallInst *Call = CallInst::Create(Fun, Args, "", BO);
      Call->setCallingConv(CallingConv::C);
      Call->setTailCall(true);
  } else {
      errs() << "WARN: invalid function\n";
  }
}

/*
 * Implement your instrumentation for dynamic symbolic execution engine
 */
bool Instrument::runOnFunction(Function &F) {
  Module* ParentModule = F.getParent ();
  instrumentDSEInit(ParentModule, F, *inst_begin(F)); 
  // Now pass each instructions
  for (inst_iterator It = inst_begin(F), E = inst_end(F); It != E; ++It){  
    if(AllocaInst *AI = dyn_cast<AllocaInst>(&(*It))) {
      instrumentDSEAlloca(ParentModule, AI);
    } else if(StoreInst *SI = dyn_cast<StoreInst>(&(*It))) {
      // Assume operand is always Integer, either Constant or Register
      if(dyn_cast<Constant>(SI->getValueOperand())!=NULL) {
	// If Constant
        instrumentDSEConst(ParentModule, SI->getValueOperand(), SI);
      } else {
	// O.w., it is Register with our assumption of input programs
        instrumentDSERegister(ParentModule, SI->getValueOperand(), SI); 
      }
      instrumentDSEStore(ParentModule, SI);
    } else if(LoadInst *LI = dyn_cast<LoadInst>(&(*It))) {
      instrumentDSELoad(ParentModule, LI); 
    } else if(ICmpInst * CI = dyn_cast<ICmpInst>(&(*It))) {
      // First operates on the two operands
      if(dyn_cast<Constant>(CI->getOperand(0))!=NULL) {
        instrumentDSEConst(ParentModule, CI->getOperand(0), CI); 
      } else {
        instrumentDSERegister(ParentModule, CI->getOperand(0), CI); 
      }
      if(dyn_cast<Constant>(CI->getOperand(1))!=NULL) {
        instrumentDSEConst(ParentModule, CI->getOperand(1), CI); 
      } else {
        instrumentDSERegister(ParentModule, CI->getOperand(1), CI); 
      }
      instrumentDSEICmp(ParentModule, CI);
    } else if(BranchInst* BI = dyn_cast<BranchInst>(&(*It))) {
      // Skip unconditional branch
      if(BI->isUnconditional()) {
	continue;
      }
      // Again, first operate on its operand
      if(dyn_cast<Constant>(BI->getOperand(0))!=NULL) {
        instrumentDSEConst(ParentModule, BI->getOperand(0), BI); 
      } else {
        instrumentDSERegister(ParentModule, BI->getOperand(0), BI); 
      }
      instrumentDSEBranch(ParentModule, BI);
    } else if(BinaryOperator* BO = dyn_cast<BinaryOperator>(&(*It))) {
      // Again, first operate on the two operands
      if(dyn_cast<Constant>(BO->getOperand(0))!=NULL) {
        instrumentDSEConst(ParentModule, BO->getOperand(0), BO); 
      } else {
        instrumentDSERegister(ParentModule, BO->getOperand(0), BO); 
      }
      if(dyn_cast<Constant>(BO->getOperand(1))!=NULL) {
        instrumentDSEConst(ParentModule, BO->getOperand(1), BO); 
      } else {
        instrumentDSERegister(ParentModule, BO->getOperand(1), BO); 
      }
      instrumentDSEBinOp(ParentModule, BO);
    }
  }
  return true;
}

char Instrument::ID = 1;
static RegisterPass<Instrument>
    X("Instrument", "Instrumentations for Dynamic Symbolic Execution", false,
      false);

} // namespace instrument
