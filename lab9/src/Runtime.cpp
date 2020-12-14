#include <iostream>

#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"

#include "SymbolicInterpreter.h"

extern SymbolicInterpreter SI;

z3::expr eval(z3::expr &E) {
  if (E.kind() == Z3_NUMERAL_AST) {
    return E;
  } else {
    MemoryTy Mem = SI.getMemory();
    Address Register(E);
    if (Mem.find(Register) != Mem.end()) {
      return Mem.at(Register);
    } else {
      std::cout << "Warning: Cannot find register " << Register << " in memory "
                << std::endl;
      return E;
    }
  }
}

// An instance of the Address class represents a symbolic memory address 
// All these instructions utilizes the symbolic expression in the stack (register or constant)

// R: ID of the register
// Ptr: the address of a newly allocated physical memory block
extern "C" void __DSE_Alloca__(int R, int *Ptr) {
  MemoryTy &Mem = SI.getMemory();
  Address Register(R);
  z3::expr SE = SI.getContext().int_val((uintptr_t)Ptr);
  Mem.insert(std::make_pair(Register, SE));
}

// X: physical memory address
extern "C" void __DSE_Store__(int *X) {
  // assumes that there exists a symbolic expression of its value operand (constant or register) on top of the stack
  MemoryTy &Mem = SI.getMemory();
  Address Addr(X);
  z3::expr SE = eval(SI.getStack().top());
  SI.getStack().pop();
  Mem.erase(Addr);
  Mem.insert(std::make_pair(Addr, SE));
}

// Y: ID of the register
// X: address of the physical memory block of which value will be loaded to the register
extern "C" void __DSE_Load__(int Y, int *X) {
  MemoryTy &Mem = SI.getMemory();
  Address Register(Y);
  // X is the physical memory, so we have to read its value there
  Address Addr(X);
  z3::expr SE = SI.getContext();
  if(Mem.find(Addr)!=Mem.end()) {
    SE = Mem.at(Addr); 
  } 
  else {
    SE = SI.getContext().int_val((uintptr_t)X);
  }
  if(Mem.find(Register)!=Mem.end()) {
    Mem.erase(Register);
  }
  Mem.insert(std::make_pair(Register, SE));
}

extern "C" void __DSE_ICmp__(int R, int Op) {
  MemoryTy &Mem = SI.getMemory();
  Address Register(R);
  z3::expr SE = SI.getContext();
  z3::expr SE2 = eval(SI.getStack().top());
  SI.getStack().pop();
  z3::expr SE1 = eval(SI.getStack().top());
  SI.getStack().pop();
  // Use C++ like operators for Z3 expressions (operator-overloading)
  // https://z3prover.github.io/api/html/class_microsoft_1_1_z3_1_1_arith_expr.html
  if(Op==llvm::CmpInst::Predicate::ICMP_EQ) {
    // Note that the second argument is pushed to stack after first operand
    SE = (SE1==SE2);
  } else if(Op==llvm::CmpInst::Predicate::ICMP_NE) {
    SE = (SE1!=SE2); 
  } else if(Op==llvm::CmpInst::Predicate::ICMP_UGT || Op==llvm::CmpInst::Predicate::ICMP_SGT) {
    SE = (SE1>SE2); 
  } else if(Op==llvm::CmpInst::Predicate::ICMP_UGE || Op==llvm::CmpInst::Predicate::ICMP_SGE) {
    SE = (SE1>=SE2); 
  } else if(Op==llvm::CmpInst::Predicate::ICMP_ULT || Op==llvm::CmpInst::Predicate::ICMP_SLT) {
    SE = (SE1<SE2); 
  } else if(Op==llvm::CmpInst::Predicate::ICMP_ULE || Op==llvm::CmpInst::Predicate::ICMP_SLE) {
    SE = (SE1<=SE2); 
  }
  Mem.erase(Register);
  Mem.insert(std::make_pair(Register, SE));
}

extern "C" void __DSE_BinOp__(int R, int Op) {
  MemoryTy &Mem = SI.getMemory();
  Address Register(R);
  z3::expr SE = SI.getContext();
  z3::expr SE2 = eval(SI.getStack().top());
  SI.getStack().pop();
  z3::expr SE1 = eval(SI.getStack().top());
  SI.getStack().pop();
  if(Op==llvm::Instruction::Add) {
    SE = (SE1+SE2); 
  } else if(Op==llvm::Instruction::Sub) {
    SE = (SE1-SE2); 
  } else if(Op==llvm::Instruction::Mul) {
    SE = (SE1*SE2); 
  } else if(Op==llvm::Instruction::UDiv) {
    SE = (SE1/SE2); 
  } else if(Op==llvm::Instruction::SDiv) {
    SE = (SE1/SE2); 
  } else if(Op==llvm::Instruction::URem || Op==llvm::Instruction::SRem) {
    // There is no overloading for modulo: https://github.com/Z3Prover/z3/issues/2487
    SE = (z3::mod(SE1, SE2));
  }
  Mem.erase(Register);
  Mem.insert(std::make_pair(Register, SE));
}
