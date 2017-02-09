
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

#include "EpiphanyMachineFunction.h"
#include "Epiphany.h"
#include "EpiphanyRegisterInfo.h"

#include "EpiphanyTargetMachine.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "epiphany-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "EpiphanyGenSubtargetInfo.inc"

/// Select the Epiphany CPU for the given triple and cpu name.
static StringRef selectEpiphanyCPU(Triple TT, StringRef CPU) {
  if (CPU.empty() || CPU == "generic") {
    if (TT.getArch() == Triple::epiphany)
      CPU = "epiphany";
  }
  return CPU;
}

void EpiphanySubtarget::anchor() {}

EpiphanySubtarget::EpiphanySubtarget(const Triple &TT, const std::string &CPU, 
                                     const std::string &FS, const EpiphanyTargetMachine &_TM)
  :   EpiphanyGenSubtargetInfo(TT, CPU, FS), 
      TM(_TM), TargetTriple(TT), TSInfo(), 
      InstrInfo(new EpiphanyInstrInfo(initializeSubtargetDependencies(CPU, FS, TM))),
      FrameLowering(new EpiphanyFrameLowering(*this)),
      TLInfo(new EpiphanyTargetLowering(TM, *this)) {}

EpiphanySubtarget &
EpiphanySubtarget::initializeSubtargetDependencies(StringRef CPU, StringRef FS,
                                                   const TargetMachine &TM) {
  // Get CPU name string
  std::string CPUName = selectEpiphanyCPU(TargetTriple, CPU);
  
  // Parse features string
  ParseSubtargetFeatures(CPUName, FS);
  // Initialize scheduling itinerary for the specified CPU.
  InstrItins = getInstrItineraryForCPU(CPUName);
  
  return *this;
}

bool EpiphanySubtarget::abiUsesSoftFloat() const {
//  return TM->Options.UseSoftFloat;
  return true;
}

const EpiphanyABIInfo &EpiphanySubtarget::getABI() const { return TM.getABI(); }
