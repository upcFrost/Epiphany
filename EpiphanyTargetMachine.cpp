//===-- EpiphanyTargetMachine.cpp - Define TargetMachine for Epiphany -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the implementation of the EpiphanyTargetMachine
// methods. Principally just setting up the passes needed to generate correct
// code on this architecture.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyTargetMachine.h"
#include "Epiphany.h"

#include "EpiphanySubtarget.h"
#include "EpiphanyTargetObjectFile.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "epiphany"

extern "C" void LLVMInitializeEpiphanyTarget() {
  RegisterTargetMachine<EpiphanyTargetMachine> X(TheEpiphanyTarget);
}

EpiphanyTargetMachine::EpiphanyTargetMachine(const Target &T, const Triple &TT,
                                           StringRef CPU, StringRef FS,
                                           const TargetOptions &Options,
                                           Reloc::Model RM, CodeModel::Model CM,
                                           CodeGenOpt::Level OL)
      : LLVMTargetMachine(T, "e-p:32:32-i8:8:8-i16:16:16-i32:32:32-f32:32:32-i64:64:64-f64:64:64-s64:64:64-S64:64:64-a0:32:32", 
                          TT, CPU, FS, Options, RM, CM, OL),
        TLOF(make_unique<EpiphanyLinuxTargetObjectFile>()),
        ABI(EpiphanyABIInfo::computeTargetABI()),
        Subtarget(TT, CPU, FS, *this) {
  initAsmInfo();
}

EpiphanyTargetMachine::~EpiphanyTargetMachine() {}

namespace {
//@EpiphanyPassConfig
/// Epiphany Code Generator Pass Configuration Options.
class EpiphanyPassConfig : public TargetPassConfig {
public:
  EpiphanyPassConfig(EpiphanyTargetMachine *TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  EpiphanyTargetMachine &getEpiphanyTargetMachine() const {
    return getTM<EpiphanyTargetMachine>();
  }
};
} // namespace

TargetPassConfig *EpiphanyTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new EpiphanyPassConfig(this, PM);
}
