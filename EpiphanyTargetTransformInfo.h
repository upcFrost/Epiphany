//===-- EpiphanyTargetTransformInfo.h - Epiphany specific TTI ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file a TargetTransformInfo::Concept conforming object specific to the
/// Epiphany target machine. It uses the target's detailed information to
/// provide more precise answers to certain TTI queries, while letting the
/// target independent and default TTI implementations handle the rest.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EPIPHANY_EPIPHANYTARGETTRANSFORMINFO_H
#define LLVM_LIB_TARGET_EPIPHANY_EPIPHANYTARGETTRANSFORMINFO_H

#include "Epiphany.h"
#include "EpiphanyTargetMachine.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/BasicTTIImpl.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class EpiphanyTargetLowering;

class EpiphanyTTIImpl final : public BasicTTIImplBase<EpiphanyTTIImpl> {
  typedef BasicTTIImplBase<EpiphanyTTIImpl> BaseT;
  typedef TargetTransformInfo TTI;
  friend BaseT;

  const EpiphanySubtarget *ST;
  const EpiphanyTargetLowering *TLI;

  const EpiphanySubtarget *getST() const { return ST; }
  const EpiphanyTargetLowering *getTLI() const { return TLI; }


  static inline int getFullRateInstrCost() {
    return TargetTransformInfo::TCC_Basic;
  }

public:
  explicit EpiphanyTTIImpl(const EpiphanyTargetMachine *TM, const Function &F)
    : BaseT(TM, F.getParent()->getDataLayout()),
      ST(TM->getSubtargetImpl(F)),
      TLI(ST->getTargetLowering()) {}

  bool hasBranchDivergence() { return true; }

  void getUnrollingPreferences(Loop *L, TTI::UnrollingPreferences &UP);

  TTI::PopcntSupportKind getPopcntSupport(unsigned TyWidth) {
    assert(isPowerOf2_32(TyWidth) && "Ty width must be power of 2");
    return TTI::PSK_FastHardware;
  }

  unsigned getNumberOfRegisters(bool Vector);
  unsigned getRegisterBitWidth(bool Vector);
  unsigned getMinVectorRegisterBitWidth();
  unsigned getLoadStoreVecRegBitWidth(unsigned AddrSpace) const;
  unsigned getMaxInterleaveFactor(unsigned VF);

  // Instruction costs
  int getVectorInstrCost(unsigned Opcode, Type *Val, unsigned Index);
  int getMemoryOpCost(unsigned Opcode, Type *Src, unsigned Alignment,
                      unsigned AddressSpace);
  int getMaskedMemoryOpCost(unsigned Opcode, Type *Src, unsigned Alignment,
                            unsigned AddressSpace);
  int getGatherScatterOpCost(unsigned Opcode, Type *DataTy, Value *Ptr,
                             bool VariableMask, unsigned Alignment);
  int getAddressComputationCost(Type *PtrTy, ScalarEvolution *SE,
                                const SCEV *Ptr);
  int getArithmeticInstrCost(
    unsigned Opcode, Type *Ty,
    TTI::OperandValueKind Opd1Info = TTI::OK_AnyValue,
    TTI::OperandValueKind Opd2Info = TTI::OK_AnyValue,
    TTI::OperandValueProperties Opd1PropInfo = TTI::OP_None,
    TTI::OperandValueProperties Opd2PropInfo = TTI::OP_None,
    ArrayRef<const Value *> Args = ArrayRef<const Value *>());

  unsigned getCFInstrCost(unsigned Opcode);

  unsigned getVectorSplitCost() { return 0; }
};

} // end namespace llvm

#endif
