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

// In some cases we will need to print shifted Imm, e.g. for load/store instructions
static unsigned getShift(unsigned int OpCode) {
  unsigned Shift = 0;
  switch (OpCode) {
    case Epiphany::LDRi16_r16:
    case Epiphany::STRi16_r16:
    case Epiphany::LDRi16_r32:
    case Epiphany::STRi16_r32:
      Shift = 1;
      break;
    case Epiphany::LDRi32_r16:
    case Epiphany::STRi32_r16:
    case Epiphany::LDRi32_r32:
    case Epiphany::STRi32_r32:
      Shift = 2;
      break;
    case Epiphany::LDRi64:
    case Epiphany::STRi64:
    case Epiphany::LDRf64:
    case Epiphany::STRf64:
      Shift = 3;
      break;
  }

  return Shift;
}

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
    // In some cases we will need to print shifted Imm, e.g. for load/store instructions
    O << '#' << (Op.getImm() >> getShift(MI->getOpcode()));
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

void EpiphanyInstPrinter::printPostModifyOperand(const MCInst *MI, unsigned opNum, raw_ostream &O) {
  O << "[";
  printOperand(MI, opNum, O);
  O << "],";
  printOperand(MI, opNum+1, O);
}

void EpiphanyInstPrinter::printCondCode(const MCInst *MI, unsigned OpNo,
    raw_ostream &O) {
  unsigned CC = MI->getOperand(OpNo).getImm();

  switch (CC) {
    default:
      llvm_unreachable("Unsupported CC code");
    case EpiphanyCC::COND_EQ:
      O << "eq";
      break;
    case EpiphanyCC::COND_NE:
      O << "ne";
      break;
    case EpiphanyCC::COND_GTU:
      O << "gtu";
      break;
    case EpiphanyCC::COND_GTEU:
      O << "gteu";
      break;
    case EpiphanyCC::COND_LTEU:
      O << "lteu";
      break;
    case EpiphanyCC::COND_LTU:
      O << "ltu";
      break;
    case EpiphanyCC::COND_GT:
      O << "gt";
      break;
    case EpiphanyCC::COND_GTE:
      O << "gte";
      break;
    case EpiphanyCC::COND_LT:
      O << "lt";
      break;
    case EpiphanyCC::COND_LTE:
      O << "lte";
      break;
    case EpiphanyCC::COND_BEQ:
      O << "beq";
      break;
    case EpiphanyCC::COND_BNE:
      O << "bne";
      break;
    case EpiphanyCC::COND_BLT:
      O << "blt";
      break;
    case EpiphanyCC::COND_BLTE:
      O << "blte";
      break;
    case EpiphanyCC::COND_NONE:
      O << "";
      break;
    case EpiphanyCC::COND_L:
      O << "l";
      break;
  }
}
