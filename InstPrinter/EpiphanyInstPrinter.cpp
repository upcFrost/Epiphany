//==-- EpiphanyInstPrinter.cpp - Convert Epiphany MCInst to assembly syntax --==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints an Epiphany MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyInstPrinter.h"

#include "EpiphanyInstrInfo.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#define PRINT_ALIAS_INSTR
#include "EpiphanyGenAsmWriter.inc"

void EpiphanyInstPrinter::printRegName(raw_ostream &OS, unsigned RegNo) const {
  OS << getRegisterName(RegNo);
}

void EpiphanyInstPrinter::printInst(const MCInst *MI, raw_ostream &O,
                                   StringRef Annot,
                                   const MCSubtargetInfo &STI) {
// Try to print any aliases first.
if (!printAliasInstr(MI, O))
    //- printInstruction(MI, O) defined in EpiphanyGenAsmWriter.inc which came from 
    //   Epiphany.td indicate.
    printInstruction(MI, O);
  printAnnotation(O, Annot);
}

//@printExpr {
static void printExpr(const MCExpr *Expr, const MCAsmInfo *MAI,
                      raw_ostream &OS) {
//@printExpr body {
  int Offset = 0;
  const MCSymbolRefExpr *SRE;

  if (const MCBinaryExpr *BE = dyn_cast<MCBinaryExpr>(Expr)) {
    SRE = dyn_cast<MCSymbolRefExpr>(BE->getLHS());
    const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(BE->getRHS());
    assert(SRE && CE && "Binary expression must be sym+const.");
    Offset = CE->getValue();
  } else {
    SRE = cast<MCSymbolRefExpr>(Expr);
    assert(SRE && "Unexpected MCExpr type");
  }

  MCSymbolRefExpr::VariantKind Kind = SRE->getKind();

  switch (Kind) {
  default:                                 llvm_unreachable("Invalid kind!");
  case MCSymbolRefExpr::VK_None:           break;
  }

  SRE->getSymbol().print(OS, MAI);

  if (Offset) {
    if (Offset > 0)
      OS << '+';
    OS << Offset;
  }
//}
}

void EpiphanyInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                      raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg()) {
    printRegName(O, Op.getReg());
    return;
  }
  
  if (Op.isImm()) {
    O << '#' << Op.getImm();
    return;
  }
  
  assert(Op.isExpr() && "unknown operand kind in printOperand");
  printExpr(Op.getExpr(), &MAI, O);
}

void EpiphanyInstPrinter::printUnsignedImm(const MCInst *MI, unsigned OpNum,
                                           raw_ostream &O) {
  const MCOperand &MO = MI->getOperand(OpNum);
  if (MO.isImm())
    O << '#' << (unsigned short int)MO.getImm();
  else
    printOperand(MI, OpNum, O);
}
