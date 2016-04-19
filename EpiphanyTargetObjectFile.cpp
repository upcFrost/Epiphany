//===-- EpiphanyTargetObjectFile.cpp - Epiphany Object Info -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file deals with any Epiphany specific requirements on object files.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyTargetObjectFile.h"

#include "EpiphanySubtarget.h"
#include "EpiphanyTargetMachine.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ELF.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

void EpiphanyTargetObjectFile::Initialize(MCContext &Ctx,
                                         const TargetMachine &TM) {
  TargetLoweringObjectFileELF::Initialize(Ctx, TM);
  InitializeELF(TM.Options.UseInitArray);
  
  this->TM = &static_cast<const EpiphanyTargetMachine &>(TM);
}