//===-- EpiphanySubtarget.cpp - Epiphany Subtarget Information --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Epiphany specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "EpiphanySubtarget.h"
#include "EpiphanyRegisterInfo.h"
#include "MCTargetDesc/EpiphanyMCTargetDesc.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/SmallVector.h"

using namespace llvm;

#define DEBUG_TYPE "epiphany-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "EpiphanyGenSubtargetInfo.inc"

EpiphanySubtarget &
EpiphanySubtarget::initializeSubtargetDependencies(StringRef CPU, StringRef FS) {
  ParseSubtargetFeatures("generic", FS);
  return *this;
}


EpiphanySubtarget::EpiphanySubtarget(const Triple &TT, const std::string &CPU, 
				     const std::string &FS, const TargetMachine &TM)
  : EpiphanyGenSubtargetInfo(TT, CPU, FS), FrameLowering(),
      InstrInfo(initializeSubtargetDependencies(CPU, FS)), 
      TSInfo(), TLInfo(TM, *this) {}

bool EpiphanySubtarget::GVIsIndirectSymbol(const GlobalValue *GV,
                                          Reloc::Model RelocM) const {
  if (RelocM == Reloc::Static)
    return false;

  return !GV->hasLocalLinkage() && !GV->hasHiddenVisibility();
}
