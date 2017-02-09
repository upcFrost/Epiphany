//===-- EpiphanyMCExpr.h - Epiphany specific MC expression classes ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EPIPHANY_MCTARGETDESC_EPIPHANYMCEXPR_H
#define LLVM_LIB_TARGET_EPIPHANY_MCTARGETDESC_EPIPHANYMCEXPR_H

#include "EpiphanyConfig.h"
#if CH >= CH5_1

#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCValue.h"

namespace llvm {

class EpiphanyMCExpr : public MCTargetExpr {
public:
  enum EpiphanyExprKind {
    CEK_None,
    CEK_HIGH,
    CEK_LOW,
    CEK_GPREL,
    CEK_Special,
  };

private:
  const EpiphanyExprKind Kind;
  const MCExpr *Expr;

  explicit EpiphanyMCExpr(EpiphanyExprKind Kind, const MCExpr *Expr)
    : Kind(Kind), Expr(Expr) {}

public:
  static const EpiphanyMCExpr *create(EpiphanyExprKind Kind, const MCExpr *Expr,
                                  MCContext &Ctx);
  static const EpiphanyMCExpr *create(const MCSymbol *Symbol, 
                                  EpiphanyMCExpr::EpiphanyExprKind Kind, MCContext &Ctx);
  static const EpiphanyMCExpr *createGpOff(EpiphanyExprKind Kind, const MCExpr *Expr,
                                       MCContext &Ctx);

  /// Get the kind of this expression.
  EpiphanyExprKind getKind() const { return Kind; }

  /// Get the child of this expression.
  const MCExpr *getSubExpr() const { return Expr; }

  void printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const override;
  bool evaluateAsRelocatableImpl(MCValue &Res, const MCAsmLayout *Layout,
                                 const MCFixup *Fixup) const override;
  void visitUsedExpr(MCStreamer &Streamer) const override;
  MCFragment *findAssociatedFragment() const override {
    return getSubExpr()->findAssociatedFragment();
  }

  void fixELFSymbolsInTLSFixups(MCAssembler &Asm) const override;

  static bool classof(const MCExpr *E) {
    return E->getKind() == MCExpr::Target;
  }

  bool isGpOff(EpiphanyExprKind &Kind) const;
  bool isGpOff() const {
    EpiphanyExprKind Kind;
    return isGpOff(Kind);
  }
};
} // end namespace llvm

#endif // #if CH >= CH5_1

#endif
