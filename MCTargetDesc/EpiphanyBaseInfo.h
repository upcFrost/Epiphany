//=== EpiphanyBaseInfo.h - Top level definitions for Epiphany MC *- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains small standalone helper functions and enum definitions for
// the Epiphany target useful for the compiler back-end and the MC libraries.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_LIB_TARGET_Epiphany_MCTARGETDESC_EpiphanyBASEINFO_H
#define LLVM_LIB_TARGET_Epiphany_MCTARGETDESC_EpiphanyBASEINFO_H

#include "EpiphanyConfig.h"

#include "EpiphanyMCTargetDesc.h"
#include "EpiphanyFixupKinds.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/ErrorHandling.h"

namespace llvm {

/// Epiphany - This namespace holds all of the target specific flags that
/// instruction info tracks.
//@Epiphany
namespace EpiphanyII {
  /// Target Operand Flag enum.
  enum TOF {
    //===------------------------------------------------------------------===//
    // Epiphany Specific MachineOperand flags.

    MO_NO_FLAG,

    /// MO_GPREL - Represents the offset from the current gp value to be used
    /// for the relocatable object file being produced.
    MO_GPREL,

    /// MO_GOT_HI16/LO16 - Relocations used for large GOTs.
    MO_HIGH,
    MO_LOW
  }; // enum TOF {

  enum {
    //===------------------------------------------------------------------===//
    // Instruction encodings.  These are the standard/most common forms for
    // Epiphany instructions.
    //

    // Normal-pseudo - TSFlags bit 0
    // Pseudo - This represents an instruction that is a pseudo instruction
    // or one that has not been implemented yet.  It is illegal to code generate
    // it, but tolerated for intermediate implementation stages.
    Normal   = 0,
    Pseudo   = 1,
    FormMask = 1,
    // Size masks - TSFlags bits 1 and 2
    SizeMask = 6
  };
}

}

#endif
