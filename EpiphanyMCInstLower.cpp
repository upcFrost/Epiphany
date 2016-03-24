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
#include "EpiphanyTargetMachine.h"
#include "MCTargetDesc/EpiphanyMCExpr.h"
#include "Utils/EpiphanyBaseInfo.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Mangler.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

MCSymbol *EpiphanyMCInstLower::
GetGlobalAddressSymbol(const MachineOperand &MO) const {
  switch (MO.getTargetFlags()) {
    case 0: 
      break;
    default:
      llvm_unreachable("Unknown target flag on GV operand");
  }
  
  return Printer.getSymbol(MO.getGlobal());
}

MCSymbol *EpiphanyMCInstLower::
GetExternalSymbolSymbol(const MachineOperand &MO) const {
  switch (MO.getTargetFlags()) {
    case 0: 
      break;
    default:
      llvm_unreachable("Unknown target flag on GV operand");
  }
  
  return Printer.GetExternalSymbolSymbol(MO.getSymbolName());
}

MCSymbol *EpiphanyMCInstLower::
GetJumpTableSymbol(const MachineOperand &MO) const {
  const DataLayout *DL = Printer.TM.getDataLayout();
  SmallString<256> Name;
  raw_svector_ostream(Name) << DL->getPrivateGlobalPrefix() << "CPI"
                            << Printer.getFunctionNumber() << '_'
                            << MO.getIndex();

  switch (MO.getTargetFlags()) {
    case 0: 
      break;
    default: 
      llvm_unreachable("Unknown target flag on GV operand");
  }
  
  // Create a symbol for the name.
  return Ctx.getOrCreateSymbol(Name);
}

MCSymbol *EpiphanyMCInstLower::
GetConstantPoolIndexSymbol(const MachineOperand &MO) const {
  const DataLayout *DL = Printer.TM.getDataLayout();
  SmallString<256> Name;
  raw_svector_ostream(Name) << DL->getPrivateGlobalPrefix() << "CPI"
                            << Printer.getFunctionNumber() << '_'
                            << MO.getIndex();

  switch (MO.getTargetFlags()) {
    case 0: 
      break;
    default: 
      llvm_unreachable("Unknown target flag on GV operand");
  }

  // Create a symbol for the name.
  return Ctx.getOrCreateSymbol(Name);
}

MCSymbol *EpiphanyMCInstLower::
GetBlockAddressSymbol(const MachineOperand &MO) const {
  switch (MO.getTargetFlags()) {
    case 0: 
      break;
    default: 
      llvm_unreachable("Unknown target flag on GV operand");
  }

  return Printer.GetBlockAddressSymbol(MO.getBlockAddress());
}



MCOperand EpiphanyMCInstLower::
LowerSymbolOperand(const MachineOperand &MO, MCSymbol *Sym) const {

  const MCExpr *Expr = MCSymbolRefExpr::create(Sym, Ctx);

  switch (MO.getTargetFlags()) {
  case EpiphanyII::MO_LO16:
    Expr = EpiphanyMCExpr::createLo16(Expr, Ctx);
    break;
  case EpiphanyII::MO_HI16:
    Expr = EpiphanyMCExpr::createHi16(Expr, Ctx);
    break;
  case EpiphanyII::MO_NO_FLAG:
    // Expr is already correct
    break;
  default:
    llvm_unreachable("Unexpected MachineOperand flag - lowersymboloperand");
  }

  if (!MO.isJTI() && MO.getOffset())
    Expr = MCBinaryExpr::createAdd(Expr,
                                   MCConstantExpr::create(MO.getOffset(), Ctx),
                                   Ctx);

  return MCOperand::createExpr(Expr);
}

bool EpiphanyMCInstLower::lowerOperand(const MachineOperand &MO,
                                       MCOperand &MCOp) const {
  switch (MO.getType()) {
    default: 
      llvm_unreachable("unknown operand type");
    case MachineOperand::MO_Register:
      if (MO.isImplicit())
        return false;
      assert(!MO.getSubReg() && "Subregs should be eliminated!");
      MCOp = MCOperand::createReg(MO.getReg());
      break;
    case MachineOperand::MO_Immediate:
      MCOp = MCOperand::createImm(MO.getImm());
      break;
    case MachineOperand::MO_FPImmediate: {// a bit hacky, see arm
      APFloat Val = MO.getFPImm()->getValueAPF();
      bool ignored;
      Val.convert(APFloat::IEEEdouble, APFloat::rmTowardZero, &ignored);
      MCOp = MCOperand::createFPImm(Val.convertToDouble());
      break;
     }
    case MachineOperand::MO_BlockAddress:
      MCOp = LowerSymbolOperand(MO, GetBlockAddressSymbol(MO));
      break;
    case MachineOperand::MO_ExternalSymbol:
      MCOp = LowerSymbolOperand(MO, GetExternalSymbolSymbol(MO));
      break;
    case MachineOperand::MO_GlobalAddress:
      MCOp = LowerSymbolOperand(MO, GetGlobalAddressSymbol(MO));
      break;
    case MachineOperand::MO_MachineBasicBlock:
      MCOp = MCOperand::createExpr(MCSymbolRefExpr::create(
                                   MO.getMBB()->getSymbol(), Ctx));
      break;
    case MachineOperand::MO_JumpTableIndex:
      MCOp = LowerSymbolOperand(MO, GetJumpTableSymbol(MO));
      break;
    case MachineOperand::MO_ConstantPoolIndex:
      MCOp = LowerSymbolOperand(MO, GetConstantPoolIndexSymbol(MO));
      break;
    case MachineOperand::MO_RegisterMask:
      // Ignore call clobbers
      return false;
    }
    return true;
}

void EpiphanyMCInstLower::Lower(const MachineInstr *MI, MCInst &OutMI) const {
  OutMI.setOpcode(MI->getOpcode());
  
  for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
    MCOperand MCOp;  
    if (lowerOperand(MI->getOperand(i), MCOp))
      OutMI.addOperand(MCOp);
  }
}

