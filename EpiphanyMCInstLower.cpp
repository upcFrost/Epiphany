//===-- EpiphanyMCInstLower.cpp - Convert Epiphany MachineInstr to an MCInst -==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains code to lower Epiphany MachineInstrs to their corresponding
// MCInst records.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyMCInstLower.h"

#include "EpiphanyAsmPrinter.h"
#include "EpiphanyInstrInfo.h"
#include "MCTargetDesc/EpiphanyBaseInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/IR/Mangler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"

using namespace llvm;

EpiphanyMCInstLower::EpiphanyMCInstLower(EpiphanyAsmPrinter &asmprinter)
  : AsmPrinter(asmprinter) {}

void EpiphanyMCInstLower::Initialize(MCContext* C) {
  Ctx = C;
}

static void CreateMCInst(MCInst& Inst, unsigned Opc, const MCOperand& Opnd0,
                         const MCOperand& Opnd1,
                         const MCOperand& Opnd2 = MCOperand()) {
  Inst.setOpcode(Opc);
  Inst.addOperand(Opnd0);
  Inst.addOperand(Opnd1);
  if (Opnd2.isValid())
    Inst.addOperand(Opnd2);
}

// @LowerOperand
MCOperand EpiphanyMCInstLower::LowerOperand(const MachineOperand &MO,
                                       unsigned offset) const {
  switch (MO.getType()) {
    default: 
      llvm_unreachable("unknown operand type");
    case MachineOperand::MO_Register:
      // Ignore all implicit register operands
      if (MO.isImplicit()) break;
      return MCOperand::createReg(MO.getReg());
    case MachineOperand::MO_Immediate:
      return MCOperand::createImm(MO.getImm());
    case MachineOperand::MO_RegisterMask:
      break;
    }
    
    return MCOperand();
}

void EpiphanyMCInstLower::Lower(const MachineInstr *MI, MCInst &OutMI) const {
  OutMI.setOpcode(MI->getOpcode());
  
  for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
    const MachineOperand &MO = MI->getOperand(i);
    MCOperand MCOp = LowerOperand(MO);

    if (MCOp.isValid())
      OutMI.addOperand(MCOp);
  }
}
