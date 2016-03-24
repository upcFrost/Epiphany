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

#include "EpiphanySubtarget.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Target/TargetFrameLowering.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class EpiphanyTargetMachine : public LLVMTargetMachine {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  EpiphanySubtarget          Subtarget;

public:
  EpiphanyTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                       StringRef FS, const TargetOptions &Options,
                       Reloc::Model RM, CodeModel::Model CM,
                       CodeGenOpt::Level OL);
  ~EpiphanyTargetMachine() override;

  const EpiphanySubtarget *getSubtargetImpl(const Function &F) const override { 
    return &Subtarget; 
  }

  const DataLayout *getDataLayout() const { return &DL; }

  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;
  
  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
};

}

#endif
