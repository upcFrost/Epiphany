//===-- EpiphanyELFObjectWriter.cpp - Epiphany ELF Writer -------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyConfig.h"

#include "MCTargetDesc/EpiphanyBaseInfo.h"
#include "MCTargetDesc/EpiphanyFixupKinds.h"
#include "MCTargetDesc/EpiphanyMCTargetDesc.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"
#include <list>

using namespace llvm;

namespace {
  class EpiphanyELFObjectWriter : public MCELFObjectTargetWriter {
  public:
    EpiphanyELFObjectWriter(uint8_t OSABI);

    ~EpiphanyELFObjectWriter() override;

    unsigned getRelocType(MCContext &Ctx, const MCValue &Target,
                        const MCFixup &Fixup, bool IsPCRel) const override;
    bool needsRelocateWithSymbol(const MCSymbol &Sym,
                                 unsigned Type) const override;
  };
}

EpiphanyELFObjectWriter::EpiphanyELFObjectWriter(uint8_t OSABI)
  : MCELFObjectTargetWriter(/*_is64Bit=false*/ false, OSABI, ELF::EM_ADAPTEVA_EPIPHANY,
                            /*HasRelocationAddend*/ false) {}

EpiphanyELFObjectWriter::~EpiphanyELFObjectWriter() {}

//@GetRelocType {
unsigned EpiphanyELFObjectWriter::getRelocType(MCContext &Ctx,
                                           const MCValue &Target,
                                           const MCFixup &Fixup,
                                           bool IsPCRel) const {
  // determine the type of the relocation
  unsigned Type = (unsigned)ELF::R_EPIPHANY_NONE;
  unsigned Kind = (unsigned)Fixup.getKind();

  switch (Kind) {
  default:
    llvm_unreachable("invalid fixup kind!");
  case FK_Data_4:
  case Epiphany::fixup_Epiphany_32:
    Type = ELF::R_EPIPHANY_32;
    break;
  case Epiphany::fixup_Epiphany_HIGH:
    Type = ELF::R_EPIPHANY_HIGH;
    break;
  case Epiphany::fixup_Epiphany_LOW:
    Type = ELF::R_EPIPHANY_LOW;
    break;
  case Epiphany::fixup_Epiphany_SIMM8:
    Type = ELF::R_EPIPHANY_SIMM8;
    break;
  case Epiphany::fixup_Epiphany_SIMM24:
    Type = ELF::R_EPIPHANY_SIMM24;
    break;
  }

  return Type;
}
//@GetRelocType }

bool
EpiphanyELFObjectWriter::needsRelocateWithSymbol(const MCSymbol &Sym,
                                             unsigned Type) const {
  // FIXME: This is extremelly conservative. This really needs to use a
  // whitelist with a clear explanation for why each realocation needs to
  // point to the symbol, not to the section.
  return true;
}

MCObjectWriter *llvm::createEpiphanyELFObjectWriter(raw_pwrite_stream &OS,
                                                uint8_t OSABI,
                                                bool IsLittleEndian) {
  MCELFObjectTargetWriter *MOTW = new EpiphanyELFObjectWriter(OSABI);
  return createELFObjectWriter(MOTW, OS, IsLittleEndian);
}

