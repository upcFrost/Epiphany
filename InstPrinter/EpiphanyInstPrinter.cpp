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

#include "MCTargetDesc/EpiphanyMCExpr.h"
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
  OS << StringRef(getRegisterName(RegNo)).lower();
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
  Op.getExpr() -> print(O, &MAI, true);
}

void EpiphanyInstPrinter::printUnsignedImm(const MCInst *MI, unsigned OpNum,
                                           raw_ostream &O) {
  const MCOperand &MO = MI->getOperand(OpNum);
  if (MO.isImm())
    O << '#' << (unsigned short int)MO.getImm();
  else
    printOperand(MI, OpNum, O);
}

void EpiphanyInstPrinter::printMemOperand(const MCInst *MI, unsigned opNum, raw_ostream &O) {
  O << "[";
  printOperand(MI, opNum, O);
  O << ",";
  printOperand(MI, opNum+1, O);
  O << "]";
}
