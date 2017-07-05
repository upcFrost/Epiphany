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

#include "EpiphanyISelDAGToDAG.h"
#include "EpiphanySubtarget.h"
#include "EpiphanyTargetObjectFile.h"
#include "EpiphanyTargetTransformInfo.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Vectorize.h"

using namespace llvm;

static cl::opt<bool> EnableSROA(
  "epiphany-sroa",
  cl::desc("Run SROA after promote alloca pass"),
  cl::ReallyHidden,
  cl::init(true));

#define DEBUG_TYPE "epiphany"

extern "C" void LLVMInitializeEpiphanyTarget() {
  RegisterTargetMachine<EpiphanyTargetMachine> X(TheEpiphanyTarget);
}

static Reloc::Model getEffectiveRelocModel(CodeModel::Model CM,
                                           Optional<Reloc::Model> RM) {
  if (!RM.hasValue() || CM == CodeModel::JITDefault)
    return Reloc::Static;
  return *RM;
}

static std::string computeDataLayout(const Triple &TT, StringRef CPU,
                                     const TargetOptions &Options) {
  std::string Ret = "";

  // Always little-endian
  Ret += "e";

  // Pointers are 32 bit 
  Ret += "-p:32:32";

  // Minimal alignment for E16 is byte
  Ret += "-i8:8-i16:16-i32:32-i64:64";

  // Vector alignment
  Ret += "-v32:32-v64:64";
  
  // 32 and 64 bit floats should have natural alignment
  Ret += "-f32:32-f64:64";

  // Native integer is 32 bits
  Ret += "-n32";

  // Stack is 64 bit aligned (don't want to mess with type-based alignment atm)
  Ret += "-S64";

  return Ret;
}

EpiphanyTargetMachine::EpiphanyTargetMachine(const Target &T, const Triple &TT,
                                           StringRef CPU, StringRef FS,
                                           const TargetOptions &Options,
                                           Optional<Reloc::Model> RM, 
                                           CodeModel::Model CM,
                                           CodeGenOpt::Level OL)
      : LLVMTargetMachine(T, computeDataLayout(TT, CPU, Options), 
                          TT, CPU, FS, Options, getEffectiveRelocModel(CM, RM), CM, OL),
        TLOF(make_unique<EpiphanyTargetObjectFile>()),
        ABI(EpiphanyABIInfo::computeTargetABI()),
        Subtarget(TT, CPU, FS, *this) {

  // initAsmInfo will display features by llc -march=cpu0 -mcpu=help on 3.7 but
  // not on 3.6
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

  bool addILPOpts() override;
  bool addInstSelector() override;
  void addIRPasses() override;
  void addCodeGenPrepare() override;
  void addPreRegAlloc() override;
  void addPreSched2() override;
  void addPreEmitPass() override;

  const EpiphanySubtarget &getEpiphanySubtarget() const {
    return *getEpiphanyTargetMachine().getSubtargetImpl();
  }
};
} // namespace

TargetPassConfig *EpiphanyTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new EpiphanyPassConfig(this, PM);
}

void EpiphanyPassConfig::addIRPasses() {
  addPass(createAtomicExpandPass(&getEpiphanyTargetMachine()));
  if (EnableSROA) {
    addPass(createSROAPass());
  }

  TargetPassConfig::addIRPasses();
}

bool EpiphanyPassConfig::addILPOpts() {
  addPass(&EarlyIfConverterID);
  //if (EnableMachineCombinerPass)
    //addPass(&MachineCombinerID);
  return true;
}

bool EpiphanyPassConfig::addInstSelector() {
  addPass(new EpiphanyDAGToDAGISel(getEpiphanyTargetMachine(), getOptLevel()));
  addPass(createEpiphanyFpuConfigPass());
  return false;
}

void EpiphanyPassConfig::addCodeGenPrepare() {
  TargetPassConfig::addCodeGenPrepare();

  addPass(createLoadStoreVectorizerPass());
}

void EpiphanyPassConfig::addPreRegAlloc() {
  addPass(&LiveVariablesID, false);
  addPass(createEpiphanyLoadStoreOptimizationPass());
}

void EpiphanyPassConfig::addPreSched2() {
  addPass(&IfConverterID, false);
}

void EpiphanyPassConfig::addPreEmitPass() {
  addPass(createEpiphanyLoadStoreOptimizationPass());
}

TargetIRAnalysis EpiphanyTargetMachine::getTargetIRAnalysis() {
  return TargetIRAnalysis([this](const Function &F) {
      return TargetTransformInfo(EpiphanyTTIImpl(this, F));
      });
}

