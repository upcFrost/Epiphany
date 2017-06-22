//=== EpiphanyTargetMachine.h - Define TargetMachine for Epiphany -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the Epiphany specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_EPIPHANYTARGETMACHINE_H
#define LLVM_EPIPHANYTARGETMACHINE_H

#include "EpiphanyConfig.h"

#include "MCTargetDesc/EpiphanyABIInfo.h"
#include "EpiphanySubtarget.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/Target/TargetFrameLowering.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class formatted_raw_ostream;
class EpiphanyRegisterInfo;

class EpiphanyTargetMachine : public LLVMTargetMachine {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  EpiphanyABIInfo ABI;
  EpiphanySubtarget Subtarget;

public:
  EpiphanyTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                       StringRef FS, const TargetOptions &Options,
                       Optional<Reloc::Model> RM, CodeModel::Model CM,
                       CodeGenOpt::Level OL);
  ~EpiphanyTargetMachine() override;

  const EpiphanySubtarget *getSubtargetImpl() const { 
    return &Subtarget; 
  }

  const EpiphanySubtarget *getSubtargetImpl(const Function &F) const override {
    return &Subtarget;
  };

  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;
  
  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }

  //TargetIRAnalysis getTargetIRAnalysis() override;
  
  const EpiphanyABIInfo &getABI() const { return ABI; }
  //const DataLayout *getDataLayout() const { return &DL; }
};

} // namespace llvm

#endif
