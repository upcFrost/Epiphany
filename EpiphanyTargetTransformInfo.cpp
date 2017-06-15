//===-- EpiphanyTargetTransformInfo.cpp - Epiphany specific TTI pass ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// \file
// This file implements a TargetTransformInfo analysis pass specific to the
// Epiphany target machine. It uses the target's detailed information to provide
// more precise answers to certain TTI queries, while letting the target
// independent and default TTI implementations handle the rest.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyTargetTransformInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/CodeGen/BasicTTIImpl.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/CostTable.h"
#include "llvm/Target/TargetLowering.h"

using namespace llvm;

unsigned EpiphanyTTIImpl::getNumberOfRegisters(bool Vec) {
  if (Vec)
    return 32; // Only even regs

  return 64;
}

unsigned EpiphanyTTIImpl::getRegisterBitWidth(bool Vector) {
  return Vector ? 64 : 32;
}

unsigned EpiphanyTTIImpl::getLoadStoreVecRegBitWidth(unsigned AddrSpace) const {
  return 64;
}

void EpiphanyTTIImpl::getUnrollingPreferences(Loop *L,
                                            TTI::UnrollingPreferences &UP) {
  UP.Threshold = 64; // 8 * min hw loop, assuming inst const = 1
  UP.MaxCount = 8;
  UP.Partial = true;
}

