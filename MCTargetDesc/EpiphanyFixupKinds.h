//===-- EpiphanyFixupKinds.h - Epiphany Specific Fixup Entries ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EPIPHANY_MCTARGETDESC_EPIPHANYFIXUPKINDS_H
#define LLVM_LIB_TARGET_EPIPHANY_MCTARGETDESC_EPIPHANYFIXUPKINDS_H

#include "EpiphanyConfig.h"

#include "llvm/MC/MCFixup.h"

namespace llvm {
  namespace Epiphany {
    // Although most of the current fixup types reflect a unique relocation
    // one can have multiple fixup types for a given relocation and thus need
    // to be uniquely named.
    //
    // This table *must* be in the save order of
    // MCFixupKindInfo Infos[Epiphany::NumTargetFixupKinds]
    // in EpiphanyAsmBackend.cpp.
    //@Fixups {
    enum Fixups {
      //@ Pure upper 32 bit fixup resulting in - R_EPIPHANY_32.
      fixup_Epiphany_32 = FirstTargetFixupKind,

      // Pure upper 16 bit fixup resulting in - R_EPIPHANY_HIGH.
      fixup_Epiphany_HIGH,

      // Pure lower 16 bit fixup resulting in - R_EPIPHANY_LOW.
      fixup_Epiphany_LOW,

      // Branch displacement
      fixup_Epiphany_SIMM8,
      fixup_Epiphany_SIMM24,

      // Marker
      LastTargetFixupKind,
      NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
    };
    //@Fixups }
  } // namespace Epiphany
} // namespace llvm

#endif // LLVM_EPIPHANY_EPIPHANYFIXUPKINDS_H
