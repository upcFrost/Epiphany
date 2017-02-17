//===- EpiphanyInstrInfo.cpp - Epiphany Instruction Information -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Epiphany implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyInstrInfo.h"

#include "MCTargetDesc/EpiphanyAddressingModes.h"
#include "EpiphanyTargetMachine.h"
#include "EpiphanyMachineFunction.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "epiphany-instrinfo"

#define GET_INSTRINFO_CTOR_DTOR
#include "EpiphanyGenInstrInfo.inc"

// Pin the vtable to this file.
void EpiphanyInstrInfo::anchor() {}

//@EpiphanyInstrInfo {
EpiphanyInstrInfo::EpiphanyInstrInfo(const EpiphanySubtarget &STI)
  : EpiphanyGenInstrInfo(Epiphany::ADJCALLSTACKDOWN, Epiphany::ADJCALLSTACKUP),
  Subtarget(STI), RI(STI) {}

  const EpiphanyRegisterInfo &EpiphanyInstrInfo::getRegisterInfo() const {
    return RI;
  }

//@expandPostRAPseudo
/// Expand Pseudo instructions into real backend instructions
bool EpiphanyInstrInfo::expandPostRAPseudo(MachineInstr &MI) const {
  //@expandPostRAPseudo-body
  MachineBasicBlock &MBB = *MI.getParent();

  switch (MI.getDesc().getOpcode()) {
    case Epiphany::RTS:
      expandRTS(MBB, MI);
      break;
    default:
      return false;
  }

  MBB.erase(MI);
  return true;
}
// }



//-------------------------------------------------------------------
// Branch analysis
//-------------------------------------------------------------------

// Check if the branch behavior is predicated
bool EpiphanyInstrInfo::isUnpredicatedTerminator(const MachineInstr &MI) const {
  // If not terminator - false
  if (!MI.isTerminator()) return false;

  // Conditional branch is a special case
  if (MI.isBranch() && !MI.isBarrier()) {
    return true;
  }

  return !isPredicated(MI);
}

// Analyze if branch can be removed/modified
bool EpiphanyInstrInfo::analyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB, 
    MachineBasicBlock *&FBB, SmallVectorImpl<MachineOperand> &Cond, bool AllowModify) const {
  // Start from the bottom of the block and work up, examining the
  // terminator instructions.
  MachineBasicBlock::iterator I = MBB.end();
  MachineBasicBlock::iterator UnCondBrIter = MBB.end();
  while (I != MBB.begin()) {
    --I;
    // Do not delete debug values
    if (I->isDebugValue())
      continue;

    // Working from the bottom, when we see a non-terminator instruction, we're
    // done.
    if (!isUnpredicatedTerminator(*I)) {      
      break;
    }

    // A terminator that isn't a branch can't easily be handled by this
    // analysis.
    if (!I->isBranch())
      return true;

    // Indirect branches with links are not handled
    if (I->getOpcode() == Epiphany::BL32 || I->getOpcode() == Epiphany::JR32)
      return true;

    // Handle unconditional branches.
    if (I->getOpcode() == Epiphany::BNONE32) {
      UnCondBrIter = I;

      // If modification is not allowed
      if (!AllowModify) {
        TBB = I->getOperand(0).getMBB();
        continue;
      }

      // If the block has any instructions after a JMP, delete them.
      while (std::next(I) != MBB.end()) {
        std::next(I)->eraseFromParent();
      }
      Cond.clear();
      FBB = nullptr;

      // Delete the JMP if it's equivalent to a fall-through.
      if (MBB.isLayoutSuccessor(I->getOperand(0).getMBB())) {
        DEBUG(dbgs()<< "\nErasing the jump to successor block\n";);
        TBB = nullptr;
        I->eraseFromParent();
        I = MBB.end();
        UnCondBrIter = MBB.end();
        continue;
      }

      // TBB is used to indicate the unconditional destination.
      TBB = I->getOperand(0).getMBB();
      continue;
    }

    // Handle conditional branches.
    if (I->getOpcode() != Epiphany::BCC32) {
      continue;
    }
    EpiphanyCC::CondCodes BranchCode = static_cast<EpiphanyCC::CondCodes>(I->getOperand(1).getImm());

    // Working from the bottom, handle the first conditional branch.
    if (Cond.empty()) {
      FBB = TBB;
      TBB = I->getOperand(0).getMBB();
      Cond.push_back(MachineOperand::CreateImm(BranchCode));
      continue;
    }

    // Handle subsequent conditional branches. Only handle the case where all
    // conditional branches branch to the same destination.
    assert(Cond.size() == 1 && "Condition size is bigger than 1");
    assert(TBB && "Target basic block not set");

    // Only handle the case where all conditional branches branch to
    // the same destination.
    if (TBB != I->getOperand(0).getMBB())
      return true;

    EpiphanyCC::CondCodes OldBranchCode = (EpiphanyCC::CondCodes)Cond[0].getImm();
    // If the conditions are the same, we can leave them alone.
    if (OldBranchCode == BranchCode)
      continue;

    return true;
  }

  return false;
}

// RemoveBranch - helper function for branch analysis
// Used with IfConversion pass
unsigned EpiphanyInstrInfo::RemoveBranch(MachineBasicBlock &MBB) const {
  // Branches to handle
  unsigned uncond[] = {Epiphany::BNONE32, Epiphany::BL32, Epiphany::BCC32};
  MachineBasicBlock::iterator I = MBB.end();
  unsigned Count = 0;

  while (I != MBB.begin()) {
    --I;
    // Leave debug values
    if (I->isDebugValue())
      continue;
    // Check if we should handle this branch
    bool isHandled = std::find(std::begin(uncond), std::end(uncond), I->getOpcode()) != std::end(uncond);
    if (!isHandled) {
      break;
    }
    // Remove the branch.
    I->eraseFromParent();
    I = MBB.end();
    ++Count;
  }

  return Count;
}

unsigned EpiphanyInstrInfo::InsertBranch(MachineBasicBlock &MBB,
    MachineBasicBlock *TBB, MachineBasicBlock *FBB,	ArrayRef<MachineOperand> Cond,
    const DebugLoc &DL) const {
  // Shouldn't be a fall through.
  assert(TBB && "InsertBranch must not be told to insert a fallthrough");
  assert((Cond.size() == 1 || Cond.size() == 0) &&
      "Branch conditions have one component!");

  if (Cond.empty()) {
    // Unconditional branch?
    assert(!FBB && "Unconditional branch with multiple successors!");
    BuildMI(&MBB, DL, get(Epiphany::BNONE32)).addMBB(TBB);
    return 1;
  }

  // Conditional branch.
  unsigned Count = 0;
  BuildMI(&MBB, DL, get(Epiphany::BCC32)).addMBB(TBB).addImm(Cond[0].getImm()).addReg(Epiphany::R0);
  ++Count;

  if (FBB) {
    // Two-way Conditional branch. Insert the second branch.
    BuildMI(&MBB, DL, get(Epiphany::BNONE32)).addMBB(FBB);
    ++Count;
  }
  return Count;
}

bool EpiphanyInstrInfo::ReverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const {
  assert(Cond.size() == 1 && "More than 1 condition");
  EpiphanyCC::CondCodes CC = static_cast<EpiphanyCC::CondCodes>(Cond[0].getImm());
  switch(CC) {
    default:
      llvm_unreachable("Wrong branch condition code!");
    case EpiphanyCC::COND_BEQ:
    case EpiphanyCC::COND_BNE:
    case EpiphanyCC::COND_BLT:
    case EpiphanyCC::COND_BLTE:
      llvm_unreachable("Unimplemented reverse conditions");
    case EpiphanyCC::COND_NONE:
    case EpiphanyCC::COND_L:
      llvm_unreachable("Unconditional branch cant be reversed");
    case EpiphanyCC::COND_EQ:
      CC = EpiphanyCC::COND_NE;
      break;
    case EpiphanyCC::COND_NE:
      CC = EpiphanyCC::COND_EQ;
      break;
    case EpiphanyCC::COND_GTU:
      CC = EpiphanyCC::COND_LTEU;
      break;
    case EpiphanyCC::COND_GTEU:
      CC = EpiphanyCC::COND_LTU;
      break;
    case EpiphanyCC::COND_LTEU:
      CC = EpiphanyCC::COND_GTU;
      break;
    case EpiphanyCC::COND_LTU:
      CC = EpiphanyCC::COND_GTEU;
      break;
    case EpiphanyCC::COND_GT:
      CC = EpiphanyCC::COND_LTE;
      break;
    case EpiphanyCC::COND_GTE:
      CC = EpiphanyCC::COND_LT;
      break;
    case EpiphanyCC::COND_LTE:
      CC = EpiphanyCC::COND_GT;
      break;
    case EpiphanyCC::COND_LT:
      CC = EpiphanyCC::COND_GTE;
      break;
  }

  Cond[0].setImm(CC);
  return false;
}

//-------------------------------------------------------------------
// Misc
//-------------------------------------------------------------------
void EpiphanyInstrInfo::insertNoop(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI) const {
  DebugLoc DL;
  BuildMI(MBB, MI, DL, get(Epiphany::NOP));
}

/// Test if the given instruction should be considered a scheduling boundary.
/// This primarily includes labels and terminators.
// Borrowed from Hexagon code
bool EpiphanyInstrInfo::isSchedulingBoundary(const MachineInstr &MI,
    const MachineBasicBlock *MBB, const MachineFunction &MF) const {
  // Debug info is never a scheduling boundary. It's necessary to be explicit
  // due to the special treatment of IT instructions below, otherwise a
  // dbg_value followed by an IT will result in the IT instruction being
  // considered a scheduling hazard, which is wrong. It should be the actual
  // instruction preceding the dbg_value instruction(s), just like it is
  // when debug info is not present.
  if (MI.isDebugValue())
    return false;

  // Throwing call is a boundary.
  if (MI.isCall()) {
    // If any of the block's successors is a landing pad, this could be a
    // throwing call.
    for (auto I : MBB->successors())
      if (I->isEHPad())
        return true;
  }

  // Don't mess around with no return calls.
  unsigned uncond[] = {Epiphany::BNONE32, Epiphany::JR16, Epiphany::JR32};
  bool isNoReturn = std::find(std::begin(uncond), std::end(uncond), MI.getOpcode()) != std::end(uncond);
  if (isNoReturn)
    return true;

  // Terminators and labels can't be scheduled around.
  if (MI.getDesc().isTerminator() || MI.isPosition())
    return true;

  if (MI.isInlineAsm())
    return true;

  return false;

}


//-------------------------------------------------------------------
// Load/Store
//-------------------------------------------------------------------

/// isLoadFromStackSlot - If the specified machine instruction is a direct
/// load from a stack slot, return the virtual or physical register number of
/// the destination along with the FrameIndex of the loaded stack slot.  If
/// not, return 0.  This predicate must return 0 if the instruction has
/// any side effects other than loading from the stack slot.
unsigned EpiphanyInstrInfo::isLoadFromStackSlot(const MachineInstr &MI,
    int &FrameIndex) const {
  // Load instructions
  unsigned inst[] = {
    Epiphany::LDRi8_r32,  Epiphany::LDRi8u_r32, 
    Epiphany::LDRi16_r32, Epiphany::LDRi16u_r32, 
    Epiphany::LDRi32_r32, Epiphany::LDRf32
  };
  DEBUG(dbgs() << "\nisLoadToStackSlot for "; MI.print(dbgs()));
  // Check if current opcode is one of those
  bool found = (std::find(std::begin(inst), std::end(inst), MI.getOpcode()) != std::end(inst));
  // If true, check operands
  if (found) {
    if (MI.getOperand(1).isFI() && MI.getOperand(2).isImm() && MI.getOperand(2).getImm() == 0) {
      FrameIndex = MI.getOperand(1).getIndex();
      return MI.getOperand(0).getReg();
    }
  }
  return 0;
}

/// isStoreToStackSlot - If the specified machine instruction is a direct
/// store to a stack slot, return the virtual or physical register number of
/// the source reg along with the FrameIndex of the loaded stack slot.  If
/// not, return 0.  This predicate must return 0 if the instruction has
/// any side effects other than storing to the stack slot.
unsigned EpiphanyInstrInfo::isStoreToStackSlot(const MachineInstr &MI,
    int &FrameIndex) const {
  unsigned inst[] = {
    Epiphany::STRi8_r32,  Epiphany::STRi16_r32,
    Epiphany::STRi32_r32, Epiphany::STRf32
  };
  DEBUG(dbgs() << "\nisStoreToStackSlot for "; MI.print(dbgs()));
  // Check if current opcode is one of those
  bool found = (std::find(std::begin(inst), std::end(inst), MI.getOpcode()) != std::end(inst));
  // If true, check operands
  if (found) {
    if (MI.getOperand(0).isFI() && MI.getOperand(1).isImm() && MI.getOperand(1).getImm() == 0) {
      FrameIndex = MI.getOperand(0).getIndex();
      return MI.getOperand(2).getReg();
    }
  }
  return 0;
}


void EpiphanyInstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MI, unsigned SrcReg, bool KillSrc, int FrameIdx,
    const TargetRegisterClass *Rd, const TargetRegisterInfo *TRI) const {
  DebugLoc DL;
  // Get instruction, for stack slots (FP/SP) we can only use 32-bit instructions
  unsigned STRi32_r32 = Epiphany::STRi32_r32;

  // Get function and frame info
  if (MI != MBB.end()) DL = MI->getDebugLoc();
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = *MF.getFrameInfo();

  // Get mem operand where to store
  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIdx),
      MachineMemOperand::MOStore, 
      MFI.getObjectSize(FrameIdx),
      MFI.getObjectAlignment(FrameIdx));

  // Build instruction
  BuildMI(MBB, MI, DL, get(STRi32_r32)).addReg(SrcReg, getKillRegState(KillSrc))
    .addFrameIndex(FrameIdx).addImm(0).addMemOperand(MMO);
}

void EpiphanyInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MI, unsigned DestReg, int FrameIdx,
    const TargetRegisterClass *Rd, const TargetRegisterInfo *TRI) const {
  DebugLoc DL;
  // Get instruction, we can only use 32-bit instructions
  unsigned LDRi32_r32 = Epiphany::LDRi32_r32;

  // Get function and frame info
  if (MI != MBB.end()) DL = MI->getDebugLoc();
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = *MF.getFrameInfo();

  // Get mem operand from where to load
  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIdx),
      MachineMemOperand::MOLoad, MFI.getObjectSize(FrameIdx),
      MFI.getObjectAlignment(FrameIdx));

  // Build instruction
  BuildMI(MBB, MI, DL, get(LDRi32_r32), DestReg).addFrameIndex(FrameIdx).addImm(0).addMemOperand(MMO);
}

void EpiphanyInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I,
    const DebugLoc &DL, unsigned DestReg,
    unsigned SrcReg, bool KillSrc) const {
  unsigned Opc = 0;

  // TODO: Should make it work for all 4 ways (i32 <-> f32)
  if (Epiphany::GPR32RegClass.contains(DestReg, SrcReg)) { // Copy between regs
    Opc = Epiphany::MOVi32rr;
  } else if (Epiphany::FPR32RegClass.contains(DestReg, SrcReg)) {
    Opc = Epiphany::MOVf32rr;
  }
  assert(Opc && "Cannot copy registers");

  BuildMI(MBB, I, DL, get(Opc), DestReg).addReg(SrcReg, getKillRegState(KillSrc));
}

//@adjustStackPtr
/// Adjusts stack pointer
void EpiphanyInstrInfo::adjustStackPtr(unsigned SP, int64_t Amount,
    MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I) const {
  DebugLoc DL = I != MBB.end() ? I->getDebugLoc() : DebugLoc();
  unsigned IP = Epiphany::IP;
  unsigned ADDri = Epiphany::ADD32ri;
  unsigned ADDrr = Epiphany::ADDrr_r32;
  unsigned MOVi32ri = Epiphany::MOVi32ri;
  unsigned MOVTi32ri = Epiphany::MOVTi32ri;

  if (isInt<11>(Amount)) {
    // add sp, sp, amount
    BuildMI(MBB, I, DL, get(ADDri), SP).addReg(SP).addImm(Amount);
  } else { // Expand immediate that doesn't fit in 11-bit.
    // Set lower 16 bits
    BuildMI(MBB, I, DL, get(MOVi32ri), IP).addImm(Amount & 0xffff);
    // Set upper 16 bits
    BuildMI(MBB, I, DL, get(MOVTi32ri), IP).addReg(IP).addImm(Amount >> 16);
    // iadd sp, sp, amount
    BuildMI(MBB, I, DL, get(ADDrr), SP).addReg(SP).addReg(IP, RegState::Kill);
  }
}

void EpiphanyInstrInfo::expandRTS(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I) const {
  BuildMI(MBB, I, I->getDebugLoc(), get(Epiphany::JR32)).addReg(Epiphany::LR);
}
// }

// Return the number of bytes of code the specified instruction may be.
unsigned EpiphanyInstrInfo::GetInstSizeInBytes(const MachineInstr &MI) const {
  switch (MI.getOpcode()) {
    default:
      return MI.getDesc().getSize();
  }
}
// }
