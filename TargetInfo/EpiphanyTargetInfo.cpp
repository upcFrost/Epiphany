//===-- EpiphanyTargetInfo.cpp - Epiphany Target Implementation -----------===//
//
//                     The LLVM Compiler Infrastructure
//
//===----------------------------------------------------------------------===//

#include "Epiphany.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

Target llvm::TheEpiphanyTarget;

extern "C" void LLVMInitializeEpiphanyTargetInfo() {
  RegisterTarget<Triple::epiphany> X(TheEpiphanyTarget, "epiphany", "Epiphany");
}
