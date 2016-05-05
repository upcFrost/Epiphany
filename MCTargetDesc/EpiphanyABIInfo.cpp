//===---- EpiphanyABIInfo.cpp - Information about Epiphany ABI's ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyConfig.h"

#include "EpiphanyABIInfo.h"
#include "EpiphanyRegisterInfo.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

namespace {
static const MCPhysReg E16IntRegs[5] = {Epiphany::V1, Epiphany::V2, Epiphany::V3, Epiphany::V4, Epiphany::V5};
}

const ArrayRef<MCPhysReg> EpiphanyABIInfo::GetByValArgRegs() const {
  if (IsE16())
    return makeArrayRef(E16IntRegs);
  llvm_unreachable("Unhandled ABI");
}

const ArrayRef<MCPhysReg> EpiphanyABIInfo::GetVarArgRegs() const {
  if (IsE16())
    return makeArrayRef(E16IntRegs);
  llvm_unreachable("Unhandled ABI");
}

unsigned EpiphanyABIInfo::GetCalleeAllocdArgSizeInBytes(CallingConv::ID CC) const {
  if (IsE16())
    return 0;
  llvm_unreachable("Unhandled ABI");
}

EpiphanyABIInfo EpiphanyABIInfo::computeTargetABI() {
  EpiphanyABIInfo abi(ABI::Unknown);

  // TODO: Probably generations should be used, but i don't have any other Epiphany chips atm
  if (true)
    abi = ABI::E16;
  // Assert exactly one ABI was chosen.
  assert(abi.ThisABI != ABI::Unknown);

  return abi;
}

unsigned EpiphanyABIInfo::GetStackPtr() const {
  return Epiphany::SP;
}

unsigned EpiphanyABIInfo::GetFramePtr() const {
  return Epiphany::FP;
}

unsigned EpiphanyABIInfo::GetEhDataReg(unsigned I) const {
  static const unsigned EhDataReg[] = {
    Epiphany::A1, Epiphany::A2, Epiphany::A3, Epiphany::A4
  };

  return EhDataReg[I];
}

int EpiphanyABIInfo::EhDataRegSize() const {
  if (ThisABI == ABI::E16)
    return 0;
}