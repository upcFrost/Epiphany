//===-- EpiphanyMachineFuctionInfo.cpp - Epiphany machine function info -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file just contains the anchor for the EpiphanyMachineFunctionInfo to
// force vtable emission.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyMachineFunction.h"

#include "EpiphanyInstrInfo.h"
#include "EpiphanySubtarget.h"
#include "llvm/IR/Function.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

using namespace llvm;

bool FixGlobalBaseReg;

EpiphanyMachineFunctionInfo::~EpiphanyMachineFunctionInfo() {}

void EpiphanyMachineFunctionInfo::anchor() { }

unsigned EpiphanyMachineFunctionInfo::getGlobalBaseReg() {
  // Return if it has already been initialized.
  if (GlobalBaseReg)
    return GlobalBaseReg;

  EpiphanySubtarget const &STI =
      static_cast<const EpiphanySubtarget &>(MF.getSubtarget());

  const TargetRegisterClass *RC = &Epiphany::GPR32RegClass;
  return GlobalBaseReg = MF.getRegInfo().createVirtualRegister(RC);
}
