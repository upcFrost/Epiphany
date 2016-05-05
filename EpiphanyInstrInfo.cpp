//===- EpiphanyInstrInfo.cpp - Epiphany Instruction Information -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Epiphany implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyInstrInfo.h"

#include "EpiphanyTargetMachine.h"
#include "EpiphanyMachineFunctionInfo.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "EpiphanyGenInstrInfo.inc"

// Pin the vtable to this file.
void EpiphanyInstrInfo::anchor() {}

//@EpiphanyInstrInfo {
EpiphanyInstrInfo::EpiphanyInstrInfo(const EpiphanySubtarget &STI)
  : RI(STI), Subtarget(STI) {}
// }