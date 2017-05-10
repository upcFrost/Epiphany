//===-- EpiphanyMCExpr.cpp - Epiphany specific MC expression classes --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Epiphany.h"

#if CH >= CH5_1

#include "EpiphanyMCExpr.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCObjectStreamer.h"
#include "llvm/MC/MCSymbolELF.h"
#include "llvm/Support/ELF.h"

using namespace llvm;

#define DEBUG_TYPE "cpu0mcexpr"

const EpiphanyMCExpr *EpiphanyMCExpr::create(EpiphanyMCExpr::EpiphanyExprKind Kind,
                                     const MCExpr *Expr, MCContext &Ctx) {
  return new (Ctx) EpiphanyMCExpr(Kind, Expr);
}

const EpiphanyMCExpr *EpiphanyMCExpr::create(const MCSymbol *Symbol, EpiphanyMCExpr::EpiphanyExprKind Kind,
                         MCContext &Ctx) {
  const MCSymbolRefExpr *MCSym =
      MCSymbolRefExpr::create(Symbol, MCSymbolRefExpr::VK_None, Ctx);
  return new (Ctx) EpiphanyMCExpr(Kind, MCSym);
}

const EpiphanyMCExpr *EpiphanyMCExpr::createGpOff(EpiphanyMCExpr::EpiphanyExprKind Kind,
                                          const MCExpr *Expr, MCContext &Ctx) {
  return create(Kind, create(CEK_None, create(CEK_PCREL16, Expr, Ctx), Ctx), Ctx);
}

void EpiphanyMCExpr::printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const {
  int64_t AbsVal;

  switch (Kind) {
  case CEK_None:
  case CEK_Special:
    llvm_unreachable("CEK_None and CEK_Special are invalid");
    break;
  case CEK_HIGH:
    OS << "%high";
    break;
  case CEK_LOW:
    OS << "%low";
    break;
  case CEK_SIMM8:
  case CEK_SIMM24:
    break;
  case CEK_PCREL8:
  case CEK_PCREL16:
  case CEK_PCREL32:
    OS << "%pcrel";
    break;
  default:
    llvm_unreachable("Unknown kind: " + Kind);
  }

  OS << '(';
  if (Expr->evaluateAsAbsolute(AbsVal))
    OS << AbsVal;
  else
    Expr->print(OS, MAI, true);
  OS << ')';
}

bool
EpiphanyMCExpr::evaluateAsRelocatableImpl(MCValue &Res,
                                      const MCAsmLayout *Layout,
                                      const MCFixup *Fixup) const {
  return getSubExpr()->evaluateAsRelocatable(Res, Layout, Fixup);
}

void EpiphanyMCExpr::visitUsedExpr(MCStreamer &Streamer) const {
  Streamer.visitUsedExpr(*getSubExpr());
}

void EpiphanyMCExpr::fixELFSymbolsInTLSFixups(MCAssembler &Asm) const {
  switch (getKind()) {
  case CEK_None:
  case CEK_Special:
    llvm_unreachable("CEK_None and CEK_Special are invalid");
    break;
  case CEK_HIGH:
  case CEK_LOW:
  case CEK_SIMM8:
  case CEK_SIMM24:
  case CEK_PCREL8:
  case CEK_PCREL16:
  case CEK_PCREL32:
    break;
  }
}

bool EpiphanyMCExpr::isGpOff(EpiphanyExprKind &Kind) const {
  if (const EpiphanyMCExpr *S1 = dyn_cast<const EpiphanyMCExpr>(getSubExpr())) {
    if (const EpiphanyMCExpr *S2 = dyn_cast<const EpiphanyMCExpr>(S1->getSubExpr())) {
      EpiphanyExprKind kind = S2->getKind();
      if (S1->getKind() == CEK_None && ((kind == CEK_PCREL8) || (kind == CEK_PCREL16) || (kind == CEK_PCREL32))) {
        Kind = getKind();
        return true;
      }
    }
  }
  return false;
}

#endif
