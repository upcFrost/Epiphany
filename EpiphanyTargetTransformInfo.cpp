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

unsigned EpiphanyTTIImpl::getMinVectorRegisterBitWidth() {
  return 32;
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

unsigned EpiphanyTTIImpl::getMaxInterleaveFactor(unsigned VF) {
  return 1;
}

// FIXME: setting all costs to 1 for now
int EpiphanyTTIImpl::getArithmeticInstrCost(
    unsigned Opcode, Type *Ty,  
    TTI::OperandValueKind Op1Info, TTI::OperandValueKind Op2Info,
    TTI::OperandValueProperties Opd1PropInfo,
    TTI::OperandValueProperties Opd2PropInfo,
    ArrayRef<const Value *> Args) { return 1; }
int EpiphanyTTIImpl::getVectorInstrCost(unsigned Opcode, Type *Val, unsigned Index) { return 1; }
int EpiphanyTTIImpl::getMemoryOpCost(unsigned Opcode, Type *Src, unsigned Alignment,
    unsigned AddressSpace) { return 1; }
int EpiphanyTTIImpl::getMaskedMemoryOpCost(unsigned Opcode, Type *Src, unsigned Alignment,
    unsigned AddressSpace) { return 1; }
int EpiphanyTTIImpl::getGatherScatterOpCost(unsigned Opcode, Type *DataTy, Value *Ptr,
    bool VariableMask, unsigned Alignment) { return 1; }
int EpiphanyTTIImpl::getAddressComputationCost(Type *PtrTy, ScalarEvolution *SE,
    const SCEV *Ptr) { return 1; }
unsigned EpiphanyTTIImpl::getCFInstrCost(unsigned Opcode) { return 1; }

