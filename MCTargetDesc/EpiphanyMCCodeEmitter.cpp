//===-- EpiphanyMCCodeEmitter.cpp - Convert Epiphany Code to Machine Code ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the EpiphanyMCCodeEmitter class.
//
//===----------------------------------------------------------------------===//
//

#include "EpiphanyMCCodeEmitter.h"

#include "MCTargetDesc/EpiphanyAsmBackend.h"
#include "MCTargetDesc/EpiphanyBaseInfo.h"
#include "MCTargetDesc/EpiphanyFixupKinds.h"
#include "MCTargetDesc/EpiphanyMCExpr.h"
#include "MCTargetDesc/EpiphanyMCTargetDesc.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "mccodeemitter"

#define GET_INSTRMAP_INFO
#include "EpiphanyGenInstrInfo.inc"
#undef GET_INSTRMAP_INFO

namespace llvm {
  MCCodeEmitter *createEpiphanyMCCodeEmitterEL(const MCInstrInfo &MCII,
      const MCRegisterInfo &MRI,
      MCContext &Ctx) {
    return new EpiphanyMCCodeEmitter(MCII, Ctx, true);
  }
} // End of namespace llvm

void EpiphanyMCCodeEmitter::EmitByte(unsigned char C, raw_ostream &OS) const {
  OS << (char)C;
}

void EpiphanyMCCodeEmitter::EmitInstruction(uint64_t Val, unsigned Size, raw_ostream &OS) const {
  // Output the instruction encoding in little endian byte order.
  for (unsigned i = 0; i < Size; ++i) {
    unsigned Shift = IsLittleEndian ? i * 8 : (Size - 1 - i) * 8;
    EmitByte((Val >> Shift) & 0xff, OS);
  }
}

/// encodeInstruction - Emit the instruction.
/// Size the instruction (currently only 4 bytes)
void EpiphanyMCCodeEmitter::encodeInstruction(const MCInst &MI, raw_ostream &OS,
    SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI) const
{
  uint32_t Binary = getBinaryCodeForInstr(MI, Fixups, STI);

  // Check for unimplemented opcodes.
  unsigned Opcode = MI.getOpcode();
  if (!Binary) {
    DEBUG(dbgs() << "Opcode: " << Opcode << "\n");
    DEBUG(dbgs() << "Binary: " << Binary << "\n");
    llvm_unreachable("unimplemented opcode in encodeInstruction()");
  }

  const MCInstrDesc &Desc = MCII.get(MI.getOpcode());
  uint64_t TSFlags = Desc.TSFlags;

  // Pseudo instructions don't get encoded and shouldn't be here
  // in the first place!
  if ((TSFlags & EpiphanyII::FormMask) == EpiphanyII::Pseudo) {
    llvm_unreachable("Pseudo opcode found in encodeInstruction()");
  }

  // Instruction size in bytes
  int Size = Desc.getSize(); // FIXME: Have Desc.getSize() return the correct value!

  EmitInstruction(Binary, Size, OS);
}

/// getBranch24TargetOpValue - Return binary encoding of the branch
/// target operand. If the machine operand requires relocation,
/// record the relocation and return zero.
unsigned EpiphanyMCCodeEmitter::getBranchTargetOpValue(const MCInst &MI, unsigned OpNo,
    SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(OpNo);

  // If destination is already resolved into immediate - nothing to do
  if (MO.isImm()) {
    DEBUG(dbgs() << "\nFixup to immediate: " << MO.getImm() << "\n");
    return MO.getImm();
  }

  // If not imm - it should be an expression, otherwise it's smth strange
  assert(MO.isExpr() && "Strange MO in getJumpTargetOpValue");
  const MCExpr *Expr = MO.getExpr();
  // Get fixup kind and info, then create new fixup
  MCFixupKind FixupKind = MCFixupKind(Epiphany::fixup_Epiphany_SIMM24);
  Fixups.push_back(MCFixup::create(0, Expr, FixupKind));
  return 0;
}

/// getJumpTargetOpValue - Return binary encoding of the jump target operand.
/// If the machine operand requires relocation, record the relocation and return zero.
//@getJumpTargetOpValue {
unsigned EpiphanyMCCodeEmitter::getJumpTargetOpValue(const MCInst &MI, unsigned OpNo,
    SmallVectorImpl<MCFixup> &Fixups, const MCSubtargetInfo &STI) const {
  return 0;
}
//@CH8_1 }

//@getExprOpValue {
unsigned EpiphanyMCCodeEmitter::getExprOpValue(const MCExpr *Expr,SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI) const {
  //@getExprOpValue body {
  int64_t Res;
  if (Expr->evaluateAsAbsolute(Res)) {
    return Res;
  }

  MCExpr::ExprKind Kind = Expr->getKind();
  if (Kind == MCExpr::Constant) {
    return cast<MCConstantExpr>(Expr)->getValue();
  }

  if (Kind == MCExpr::Binary) {
    unsigned Res = getExprOpValue(cast<MCBinaryExpr>(Expr)->getLHS(), Fixups, STI);
    Res += getExprOpValue(cast<MCBinaryExpr>(Expr)->getRHS(), Fixups, STI);
    return Res;
  }

  if (Kind == MCExpr::Target) {
    const EpiphanyMCExpr *EpiphanyExpr = cast<EpiphanyMCExpr>(Expr);

    Epiphany::Fixups FixupKind = Epiphany::Fixups(0);
    switch (EpiphanyExpr->getKind()) {
      default: 
        llvm_unreachable("Unsupported fixup kind for target expression!");
        break;
      case EpiphanyMCExpr::CEK_HIGH:
        FixupKind = Epiphany::fixup_Epiphany_HIGH;
        break;
      case EpiphanyMCExpr::CEK_LOW:
        FixupKind = Epiphany::fixup_Epiphany_LOW;
        break;
      case EpiphanyMCExpr::CEK_GPREL:
        FixupKind = Epiphany::fixup_Epiphany_SIMM24;
        break;
    } // switch
    Fixups.push_back(MCFixup::create(0, Expr, MCFixupKind(FixupKind)));
    return 0;
  }

  // All of the information is in the fixup.
  return 0;
}

/// getMachineOpValue - Return binary encoding of operand. If the machine
/// operand requires relocation, record the relocation and return zero.
unsigned EpiphanyMCCodeEmitter::getMachineOpValue(const MCInst &MI, const MCOperand &MO,
    SmallVectorImpl<MCFixup> &Fixups, const MCSubtargetInfo &STI) const {
  if (MO.isReg()) {
    unsigned Reg = MO.getReg();
    unsigned RegNo = Ctx.getRegisterInfo()->getEncodingValue(Reg);
    return RegNo;
  } else if (MO.isImm()) {
    return static_cast<unsigned>(MO.getImm());
  } else if (MO.isFPImm()) {
    return static_cast<unsigned>(APFloat(MO.getFPImm())
        .bitcastToAPInt().getHiBits(32).getLimitedValue());
  }
  // MO must be an Expr.
  assert(MO.isExpr());
  return getExprOpValue(MO.getExpr(),Fixups, STI);
}

// In some cases we will need to print shifted Imm, e.g. for load/store instructions
static unsigned getShift(unsigned int OpCode) {
  unsigned Shift = 0;
  switch (OpCode) {
    case Epiphany::LDRi16e_r16:
    case Epiphany::STRi16_r16:
    case Epiphany::LDRi16e_r32:
    case Epiphany::STRi16_r32:
    case Epiphany::LDRi16e_idx_add_r16:
    case Epiphany::STRi16_idx_add_r16:
    case Epiphany::LDRi16e_idx_add_r32:
    case Epiphany::STRi16_idx_add_r32:
    case Epiphany::LDRi16e_idx_sub_r32:
    case Epiphany::STRi16_idx_sub_r32:
    case Epiphany::LDRi16e_pm_add_r16:
    case Epiphany::STRi16_pm_add_r16:
    case Epiphany::LDRi16e_pm_add_r32:
    case Epiphany::STRi16_pm_add_r32:
    case Epiphany::LDRi16e_pm_sub_r32:
    case Epiphany::STRi16_pm_sub_r32:
    case Epiphany::LDRi16e_pmd_r32:
    case Epiphany::STRi16_pmd_r32:
      Shift = 1;
      break;
    case Epiphany::LDRi32_r16:
    case Epiphany::STRi32_r16:
    case Epiphany::LDRi32_r32:
    case Epiphany::STRi32_r32:
    case Epiphany::LDRf32:
    case Epiphany::STRf32:
    case Epiphany::LDRi32_idx_add_r16:
    case Epiphany::STRi32_idx_add_r16:
    case Epiphany::LDRi32_idx_add_r32:
    case Epiphany::STRi32_idx_add_r32:
    case Epiphany::LDRi32_idx_sub_r32:
    case Epiphany::STRi32_idx_sub_r32:
    case Epiphany::LDRi32_pm_add_r16:
    case Epiphany::STRi32_pm_add_r16:
    case Epiphany::LDRi32_pm_add_r32:
    case Epiphany::STRi32_pm_add_r32:
    case Epiphany::LDRi32_pm_sub_r32:
    case Epiphany::STRi32_pm_sub_r32:
    case Epiphany::LDRi32_pmd_r32:
    case Epiphany::STRi32_pmd_r32:
      Shift = 2;
      break;
    case Epiphany::LDRi64:
    case Epiphany::LDRi64_pmd:
    case Epiphany::STRi64:
    case Epiphany::STRi64_pmd:
    case Epiphany::LDRf64:
    case Epiphany::STRf64:
      Shift = 3;
      break;
  }

  return Shift;
}

/// getMemEncoding - Return binary encoding of memory related operand.
/// If the offset operand requires relocation, record the relocation.
unsigned EpiphanyMCCodeEmitter::getMemEncoding(const MCInst &MI, unsigned OpNo,
    SmallVectorImpl<MCFixup> &Fixups, const MCSubtargetInfo &STI, bool modOffset) const {
  // Base register is encoded in bits 21-16, offset is encoded in bits 15-0.
  if (!MI.getOperand(OpNo).isReg()) {
    MI.getOperand(OpNo).print(errs());
    llvm_unreachable("Wrong operand type in getMemEncoding");
  }

  // Get register bits
  unsigned RegBits = getMachineOpValue(MI, MI.getOperand(OpNo), Fixups, STI) << 16;
  unsigned OffBits = getMachineOpValue(MI, MI.getOperand(OpNo+1), Fixups, STI);

  if (modOffset) {
    OffBits = OffBits >> getShift(MI.getOpcode());
    // Value should be always greater than 0, sign is regulated by bit 11
    OffBits = (OffBits >> 11) == 0 ? OffBits : (OffBits^0xFFFF) + 1 | (1 << 11);
  }

  return (OffBits & 0xFFFF) | RegBits;
}

#include "EpiphanyGenMCCodeEmitter.inc"

