//===-- Epiphany.h - Top-level interface for Epiphany representation ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in
// the LLVM Epiphany back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EPIPHANY_EPIPHANY_H
#define LLVM_LIB_TARGET_EPIPHANY_EPIPHANY_H

#include "EpiphanyConfig.h"
#include "MCTargetDesc/EpiphanyMCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace EpiphanyCC {
  // Epiphany-specific condition codes
  enum CondCodes {
    COND_EQ = 0x0,
    COND_NE = 0x1,
    COND_GTU = 0x2,
    COND_GTEU = 0x3,
    COND_LTEU = 0x4,
    COND_LTU = 0x5,
    COND_GT = 0x6,
    COND_GTE = 0x7,
    COND_LT = 0x8,
    COND_LTE = 0x9,
    COND_BEQ = 0xA,
    COND_BNE = 0xB,
    COND_BLT = 0xC,
    COND_BLTE = 0xD,
    COND_NONE = 0xE,
    COND_L = 0xF
  };
}

namespace llvm {
  class EpiphanyTargetMachine;
  class FunctionPass;

  FunctionPass *createEpiphanyFpuConfigPass();
  FunctionPass *createEpiphanyLoadStoreOptimizationPass();
  FunctionPass *createEpiphanyVregLoadStoreOptimizationPass();
  FunctionPass *createEpiphanyHardwareLoopsPass();

} // end namespace llvm;

#endif
