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

#include "EpiphanyFrameLowering.h"
#include "EpiphanyISelLowering.h"
#include "EpiphanyInstrInfo.h"
#include "EpiphanySelectionDAGInfo.h"
#include "EpiphanySubtarget.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class EpiphanyTargetMachine : public LLVMTargetMachine {
  EpiphanySubtarget          Subtarget;
  EpiphanyInstrInfo          InstrInfo;
  const DataLayout          DL;
  std::unique_ptr<TargetLoweringObjectFile> TLOF;

public:
  EpiphanyTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                       StringRef FS, const TargetOptions &Options,
                       Reloc::Model RM, CodeModel::Model CM,
                       CodeGenOpt::Level OL);

  const EpiphanyInstrInfo *getInstrInfo() const {
    return &InstrInfo;
  }

  void resetSubtarget(MachineFunction *MF);

  TargetLoweringObjectFile* getObjFileLowering() const override {
    return TLOF.get();
  }

  const EpiphanySubtarget *getSubtargetImpl(const Function &F) const override { return &Subtarget; }

  const DataLayout *getDataLayout() const { return &DL; }

  const TargetRegisterInfo *getRegisterInfo() const {
    return &InstrInfo.getRegisterInfo();
  }
  TargetPassConfig *createPassConfig(PassManagerBase &PM);
};

}

#endif
