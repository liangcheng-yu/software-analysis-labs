#include "CBIInstrument.h"

using namespace llvm;

namespace instrument {

static const char *CBIBranchFunctionName = "__cbi_branch__";
static const char *CBIReturnFunctionName = "__cbi_return__";

/*
 * Implement instrumentation for the branch scheme of CBI.
 */
void instrumentCBIBranches(Module *M, Function &F, BranchInst &I) {
  const DebugLoc &Debug = I.getDebugLoc();
  /* If the object is NULL, skip */
  if(!Debug) {
    return;
  }
  
  // Skip since no condition can be extracted
  if(I.isUnconditional()) {
    return;
  }

  /* Get the divisor of the instruction */
  Value* Cond = I.getCondition();

  LLVMContext& Ctx = M->getContext();

  // Need to insert an instruction before call to ZExt Cond to i32
  // API: https://llvm.org/doxygen/classllvm_1_1CastInst.html
  // https://llvm.org/doxygen/classllvm_1_1Type.html#a30dd396c5b40cd86c1591872e574ccdf
  CastInst *Cast = NULL;
  if(!Cond->getType()->isIntegerTy(32)) {
    Cast = CastInst::CreateIntegerCast(Cond, Type::getInt32Ty(Ctx), false, "", &I);
  }

  // Insert the call instruction
  Value* NewValue = M->getOrInsertFunction(CBIBranchFunctionName,
		                           Type::getVoidTy(Ctx),
					   Type::getInt32Ty(Ctx),
					   Type::getInt32Ty(Ctx),
					   Type::getInt32Ty(Ctx));
  Function* NewFunction = cast<Function>(NewValue);
  std::vector<Value*> Args;
  Args.push_back(ConstantInt::get(Type::getInt32Ty(Ctx), Debug.getLine(), true));
  Args.push_back(ConstantInt::get(Type::getInt32Ty(Ctx), Debug.getCol(), true));
  if(Cast) {
    Args.push_back(Cast);
  } else {
    Args.push_back(Cond);
  }
  CallInst *Call = CallInst::Create(NewFunction, Args, "", &I);
  Call->setCallingConv(CallingConv::C);
  Call->setTailCall(true);

}

/*
 * Implement instrumentation for the return scheme of CBI.
 */
void instrumentCBIReturns(Module *M, Function &F, CallInst &I) {
  const DebugLoc &Debug = I.getDebugLoc();
  /* If the object is NULL, skip */
  if(!Debug) {
    return;
  }
  // We only care about integer return call instructions
  // Note that CallInstr itself is its return variable
  if(I.getType()->isIntegerTy()) {
    LLVMContext& Ctx = M->getContext();
    // For CallInst, the instructions to be inserted need to be come after it
    Instruction* NextInstr = I.getNextNode();
    // Same API as before, it will intelligently apply ZExt, Trunc etc
    CastInst* Cast = NULL;
    if(!I.getType()->isIntegerTy(32)) {
      Cast = CastInst::CreateIntegerCast(&I, Type::getInt32Ty(Ctx), false, "", NextInstr);
    }

    Value* NewValue = M->getOrInsertFunction(CBIReturnFunctionName,
		                           Type::getVoidTy(Ctx),
					   Type::getInt32Ty(Ctx),
					   Type::getInt32Ty(Ctx),
					   Type::getInt32Ty(Ctx));
    Function* NewFunction = cast<Function>(NewValue);
    std::vector<Value*> Args;
    Args.push_back(ConstantInt::get(Type::getInt32Ty(Ctx), Debug.getLine(), true));
    Args.push_back(ConstantInt::get(Type::getInt32Ty(Ctx), Debug.getCol(), true));
    if(Cast) {
      Args.push_back(Cast);
    } else {
      Args.push_back(&I);   
    }
    CallInst *Call = CallInst::Create(NewFunction, Args, "", NextInstr);
    Call->setCallingConv(CallingConv::C);
    Call->setTailCall(true);
  }
}

bool CBIInstrument::runOnFunction(Function &F) {
  Module* ParentModule = F.getParent ();
  for (inst_iterator It = inst_begin(F), E = inst_end(F); It != E; ++It){
    if(BranchInst *BI = dyn_cast<BranchInst>(&(*It))) {
      instrumentCBIBranches(ParentModule, F, *BI);
    } else if(CallInst *CI = dyn_cast<CallInst>(&(*It))) {
      instrumentCBIReturns(ParentModule, F, *CI);
    }
  }
  return true;
}

char CBIInstrument::ID = 1;
static RegisterPass<CBIInstrument> X("CBIInstrument",
                                     "Instrumentations for CBI", false, false);

} // namespace instrument
