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
#include "llvm/MC/MCTargetOptions.h"
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
  class raw_pwrite_stream;

  extern Target TheEpiphanyTarget;

  MCCodeEmitter *createEpiphanyMCCodeEmitterEL(const MCInstrInfo &MCII,
      const MCRegisterInfo &MRI,
      MCContext &Ctx);

  MCAsmBackend *createEpiphanyAsmBackendEL32(const Target &T,
      const MCRegisterInfo &MRI,
      const Triple &TT, StringRef CPU, const MCTargetOptions &Options);

  MCObjectWriter *createEpiphanyELFObjectWriter(raw_pwrite_stream &OS,
      uint8_t OSABI,
      bool IsLittleEndian);


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
