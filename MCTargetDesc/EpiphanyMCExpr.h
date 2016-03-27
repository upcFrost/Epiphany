//==- EpiphanyMCExpr.h - Epiphany specific MC expression classes --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes Epiphany-specific MCExprs, used for modifiers like
// ":lo12:" or ":gottprel_g1:".
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_EPIPHANYMCEXPR_H
#define LLVM_EPIPHANYMCEXPR_H

#include "llvm/MC/MCExpr.h"

namespace llvm {
class EpiphanyMCExpr : public MCTargetExpr {
public:
  enum VariantKind {
    VK_EPIPHANY_None,
    VK_EPIPHANY_LO16,     // :lo12:
    VK_EPIPHANY_HI16,     // :lo12:
  };
  
private:
  const MCExpr *Expr;
  const VariantKind Kind;

  explicit EpiphanyMCExpr(const MCExpr *Expr, VariantKind Kind)
    : Expr(Expr), Kind(Kind) {}

public:
  /// @name Construction
  /// @{

  static const EpiphanyMCExpr *create(const MCExpr *Expr, VariantKind Kind, 
                                     MCContext &Ctx);

  static const EpiphanyMCExpr *createLo16(const MCExpr *Expr, MCContext &Ctx) {
    return create(Expr, VK_EPIPHANY_LO16, Ctx);
  }
  static const EpiphanyMCExpr *createHi16(const MCExpr *Expr, MCContext &Ctx) {
    return create(Expr, VK_EPIPHANY_HI16, Ctx);
  }

  /// @}
  /// @name Accessors
  /// @{

  /// getOpcode - Get the kind of this expression.
  VariantKind getKind() const { return Kind; }

  /// getSubExpr - Get the child of this expression.
  const MCExpr *getSubExpr() const { return Expr; }

  /// @}

  void printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const override;
  
  void visitUsedExpr(MCStreamer &Streamer) const override;
  
  bool evaluateAsRelocatableImpl(MCValue &Res,
                                 const MCAsmLayout *Layout,
                                 const MCFixup *Fixup) const override;
  void AddValueSymbols(MCAssembler *) const;
  MCSection *findAssociatedSection() const override {
    return getSubExpr()->findAssociatedSection();
  }

  void fixELFSymbolsInTLSFixups(MCAssembler &Asm) const override;

  static bool classof(const MCExpr *E) {
    return E->getKind() == MCExpr::Target;
  }

  static bool classof(const EpiphanyMCExpr *) { return true; }

};
} // end namespace llvm

#endif
