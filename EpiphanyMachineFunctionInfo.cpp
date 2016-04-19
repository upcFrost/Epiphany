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

#include "EpiphanyMachineFunctionInfo.h"

#include "EpiphanyInstrInfo.h"
#include "EpiphanySubtarget.h"
#include "llvm/IR/Function.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

using namespace llvm;

bool FixGlobalBaseReg;

// class EpiphanyCallEntry.
EpiphanyCallEntry::EpiphanyCallEntry(StringRef N) {
#ifndef NDEBUG
  Name = N;
  Val = nullptr;
#endif
}

EpiphanyCallEntry::EpiphanyCallEntry(const GlobalValue *V) {
#ifndef NDEBUG
  Val = V;
#endif
}

bool EpiphanyCallEntry::isConstant(const MachineFrameInfo *) const {
  return false;
}

bool EpiphanyCallEntry::isAliased(const MachineFrameInfo *) const {
  return false;
}

bool EpiphanyCallEntry::mayAlias(const MachineFrameInfo *) const {
  return false;
}

void EpiphanyCallEntry::printCustom(raw_ostream &O) const {
  O << "EpiphanyCallEntry: ";
#ifndef NDEBUG
  if (Val)
    O << Val->getName();
  else
    O << Name;
#endif
}

EpiphanyFunctionInfo::~EpiphanyFunctionInfo() {}

void EpiphanyMachineFunctionInfo::anchor() { }
