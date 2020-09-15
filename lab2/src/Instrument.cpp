#include "Instrument.h"

using namespace llvm;

namespace instrument {

static const char *SanitizerFunctionName = "__sanitize__";
static const char *CoverageFunctionName = "__coverage__";

/*
 * Implement divide-by-zero sanitizer.
 */
void instrumentSanitize(Module *M, Instruction &I) {

  const DebugLoc &Debug = I.getDebugLoc();
  /* If the object is NULL, skip */
  if(!Debug) {
    return;
  }
  
  /* Get the divisor of the instruction */
  Value* Divisor = I.getOperand(1);

  /* Load sanitizer instruction */
  LLVMContext& Ctx = M->getContext();
  Value* NewValue = M->getOrInsertFunction(SanitizerFunctionName,
		                           Type::getVoidTy(Ctx),
					   Type::getInt32Ty(Ctx),
					   Type::getInt32Ty(Ctx),
					   Type::getInt32Ty(Ctx));
  Function* NewFunction = cast<Function>(NewValue);
  /* Populate arguments divisor, line, col */
  std::vector<Value*> Args;
  Args.push_back(Divisor);
  Args.push_back(ConstantInt::get(Type::getInt32Ty(Ctx), Debug.getLine(), true));
  Args.push_back(ConstantInt::get(Type::getInt32Ty(Ctx), Debug.getCol(), true));
  CallInst *Call = CallInst::Create(NewFunction, Args, "", &I);
  Call->setCallingConv(CallingConv::C);
  Call->setTailCall(true);
}

/*
 * Implement code coverage instrumentation.
 */
void instrumentCoverage(Module *M, Instruction &I) {
  
  const DebugLoc &Debug = I.getDebugLoc();
  /* If the object is NULL, skip */
  if(!Debug) {
    return;
  }
  /* Load coverage function into LLVM */
  LLVMContext& Ctx = M->getContext();
  Value* NewValue = M->getOrInsertFunction(CoverageFunctionName,
		                           Type::getVoidTy(Ctx),
					   Type::getInt32Ty(Ctx),
					   Type::getInt32Ty(Ctx));
  Function* NewFunction = cast<Function>(NewValue);
  /* Populate arguments line, col */
  std::vector<Value*> Args;
  Args.push_back(ConstantInt::get(Type::getInt32Ty(Ctx), Debug.getLine(), true));
  Args.push_back(ConstantInt::get(Type::getInt32Ty(Ctx), Debug.getCol(), true));
  CallInst *Call = CallInst::Create(NewFunction, Args, "", &I);
  Call->setCallingConv(CallingConv::C);
  Call->setTailCall(true);

}

bool Instrument::runOnFunction(Function &F) {
  /* Function inherits this method to get the Module from GlobalValue parent class */
  Module* ParentModule = F.getParent ();
  for (inst_iterator It = inst_begin(F), E = inst_end(F); It != E; ++It){
    /* Check if it belongs to div related operations as per https://piazza.com/class/kdtbmqthpx22d?cid=47
     * Note that this checks against sdiv, udiv, srem, and urem (srem, urem not appearing in this assignment)
     * Here I just call the wrapping API in https://llvm.org/doxygen/classllvm_1_1Instruction.html
     * One could also just call getOpcode or getOpcodeName and match it against target subset instructions signatures,
     * i.e., udiv, sdiv, which are enough for this assignment though
     */
    if(It->isIntDivRem()) {
      instrumentSanitize(ParentModule, *It);
    }
    instrumentCoverage(ParentModule, *It);
  }    
  return true;
}

char Instrument::ID = 1;
static RegisterPass<Instrument>
    X("Instrument", "Instrumentations for Dynamic Analysis", false, false);

} // namespace instrument
