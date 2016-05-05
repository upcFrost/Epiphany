//===-- EpiphanyMCTargetDesc.h - Epiphany Target Descriptions -----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
//===----------------------------------------------------------------------===//
//
// This file provides Epiphany specific target descriptions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EPIPHANY_MCTARGETDESC_EPIPHANYMCTARGETDESC_H
#define LLVM_LIB_TARGET_EPIPHANY_MCTARGETDESC_EPIPHANYMCTARGETDESC_H

#include "EpiphanyConfig.h"
#include "llvm/Support/DataTypes.h"

namespace llvm {
class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCObjectWriter;
class MCRegisterInfo;
class MCSubtargetInfo;
class StringRef;
class Target;
class Triple;
class raw_ostream;

extern Target TheEpiphanyTarget;

} // End llvm namespace

// Defines symbolic names for Epiphany registers.  This defines a mapping from
// register name to register number.
#define GET_REGINFO_ENUM
#include "EpiphanyGenRegisterInfo.inc"

// Defines symbolic names for the Epiphany instructions.
#define GET_INSTRINFO_ENUM
#include "EpiphanyGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "EpiphanyGenSubtargetInfo.inc"

#endif