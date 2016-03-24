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

#define DEBUG_TYPE "epiphany-subtarget"
#include "EpiphanySubtarget.h"
#include "EpiphanyRegisterInfo.h"
#include "MCTargetDesc/EpiphanyMCTargetDesc.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/SmallVector.h"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "EpiphanyGenSubtargetInfo.inc"

using namespace llvm;

EpiphanySubtarget &
EpiphanySubtarget::initializeSubtargetDependencies(StringRef CPU, StringRef FS)
{
    ParseSubtargetFeatures(CPU, FS);
    return *this;
}

EpiphanySubtarget::EpiphanySubtarget(const Triple &TT, StringRef CPU, StringRef FS, const TargetMachine &TM)
  : EpiphanyGenSubtargetInfo(TT, CPU, FS)
  , TargetTriple(TT)
  , FrameLowering()
  , InstrInfo(initializeSubtargetDependencies(CPU, FS))
  , TLInfo(TM, *this)
{}

bool EpiphanySubtarget::GVIsIndirectSymbol(const GlobalValue *GV,
                                          Reloc::Model RelocM) const {
  if (RelocM == Reloc::Static)
    return false;

  return !GV->hasLocalLinkage() && !GV->hasHiddenVisibility();
}
