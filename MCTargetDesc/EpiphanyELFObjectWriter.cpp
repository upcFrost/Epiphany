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
  : MCELFObjectTargetWriter(/*_is64Bit=false*/ false, OSABI, ELF::EM_EPIPHANY,
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
    Type = ELF::R_EPIPHANY_32;
    break;
  case Epiphany::fixup_Epiphany_32:
    Type = ELF::R_EPIPHANY_32;
    break;
  case Epiphany::fixup_Epiphany_GPREL16:
    Type = ELF::R_EPIPHANY_GPREL16;
    break;
  case Epiphany::fixup_Epiphany_GOT:
    Type = ELF::R_EPIPHANY_GOT16;
    break;
  case Epiphany::fixup_Epiphany_HI16:
    Type = ELF::R_EPIPHANY_HI16;
    break;
  case Epiphany::fixup_Epiphany_LO16:
    Type = ELF::R_EPIPHANY_LO16;
    break;
  case Epiphany::fixup_Epiphany_GOT_HI16:
    Type = ELF::R_EPIPHANY_GOT_HI16;
    break;
  case Epiphany::fixup_Epiphany_GOT_LO16:
    Type = ELF::R_EPIPHANY_GOT_LO16;
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
  switch (Type) {
  default:
    return true;

  case ELF::R_EPIPHANY_GOT16:
  // For Epiphany pic mode, I think it's OK to return true but I didn't confirm.
  //  llvm_unreachable("Should have been handled already");
    return true;

  // These relocations might be paired with another relocation. The pairing is
  // done by the static linker by matching the symbol. Since we only see one
  // relocation at a time, we have to force them to relocate with a symbol to
  // avoid ending up with a pair where one points to a section and another
  // points to a symbol.
  case ELF::R_EPIPHANY_HI16:
  case ELF::R_EPIPHANY_LO16:
  // R_EPIPHANY_32 should be a relocation record, I don't know why Mips set it to 
  // false.
  case ELF::R_EPIPHANY_32:
    return true;

  case ELF::R_EPIPHANY_GPREL16:
    return false;
  }
}

MCObjectWriter *llvm::createEpiphanyELFObjectWriter(raw_pwrite_stream &OS,
                                                uint8_t OSABI,
                                                bool IsLittleEndian) {
  MCELFObjectTargetWriter *MOTW = new EpiphanyELFObjectWriter(OSABI);
  return createELFObjectWriter(MOTW, OS, IsLittleEndian);
}

