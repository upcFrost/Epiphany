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
  DEBUG(dbgs()<< "\n<----------------->";);
  DEBUG(dbgs()<< "\nAnalyzing block " << MBB.getNumber() << "\n";);
  DEBUG(MBB.dump(););
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
    if (!I->isBranch()) {
      return true;
    }

    // Indirect branches with links are not handled
    if (I->getOpcode() == Epiphany::BL32 || I->getOpcode() == Epiphany::JR32) {
      return true;
    }

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
        DEBUG(dbgs()<< "\n<----------------->";);
        DEBUG(dbgs()<< "\nErasing the jump to successor block " << MBB.getNumber() << "\n";);
        TBB = nullptr;
        I->eraseFromParent();
        I = MBB.end();
        UnCondBrIter = MBB.end();
        DEBUG(MBB.getParent()->dump(););
        continue;
      }

      // TBB is used to indicate the unconditional destination.
      TBB = I->getOperand(0).getMBB();
      continue;
    }

    // Handle conditional branches.
    if (I->getOpcode() != Epiphany::BCC) {
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

// removeBranch - helper function for branch analysis
// Used with IfConversion pass
unsigned EpiphanyInstrInfo::removeBranch(MachineBasicBlock &MBB, int *BytesRemoved) const {
  assert(!BytesRemoved && "code size not handled");

  // Branches to handle
  DEBUG(dbgs()<< "\n<----------------->";);
  DEBUG(dbgs() << "\nRemoving branches out of BB#" << MBB.getNumber() << "\n");
  unsigned uncond[] = {Epiphany::BNONE32, Epiphany::BL32, Epiphany::BCC};
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

  DEBUG(MBB.getParent()->dump(););
  return Count;
}

unsigned EpiphanyInstrInfo::insertBranch(MachineBasicBlock &MBB,
    MachineBasicBlock *TBB, MachineBasicBlock *FBB,	ArrayRef<MachineOperand> Cond,
    const DebugLoc &DL, int *BytesAdded) const {
  DEBUG(dbgs()<< "\n<----------------->";);
  DEBUG(dbgs() << "\nInserting branch into BB#" << MBB.getNumber() << "\n");

  // Shouldn't be a fall through.
  assert(TBB && "InsertBranch must not be told to insert a fallthrough");
  assert((Cond.size() == 1 || Cond.size() == 0) &&
      "Branch conditions have one component!");
  assert(!BytesAdded && "code size not handled");

  if (Cond.empty()) {
    // Unconditional branch?
    assert(!FBB && "Unconditional branch with multiple successors!");
    BuildMI(&MBB, DL, get(Epiphany::BNONE32)).addMBB(TBB);
    return 1;
  }

  // Conditional branch.
  unsigned Count = 0;
  BuildMI(&MBB, DL, get(Epiphany::BCC)).addMBB(TBB).addImm(Cond[0].getImm());
  ++Count;

  if (FBB) {
    // Two-way Conditional branch. Insert the second branch.
    BuildMI(&MBB, DL, get(Epiphany::BNONE32)).addMBB(FBB);
    ++Count;
  }
  DEBUG(MBB.getParent()->dump(););
  return Count;
}

bool EpiphanyInstrInfo::reverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const {
  assert(Cond.size() == 1 && "More than 1 condition");
  EpiphanyCC::CondCodes CC = static_cast<EpiphanyCC::CondCodes>(Cond[0].getImm());
  switch(CC) {
    default:
      llvm_unreachable("Wrong branch condition code!");
      break;
    case EpiphanyCC::COND_BLT:
    case EpiphanyCC::COND_BLTE:
      // can't be reversed
      return true;
      break;
    case EpiphanyCC::COND_NONE:
    case EpiphanyCC::COND_L:
      llvm_unreachable("Unconditional branch cant be reversed");
      break;
    case EpiphanyCC::COND_BEQ:
      CC = EpiphanyCC::COND_BNE;
      break;
    case EpiphanyCC::COND_BNE:
      CC = EpiphanyCC::COND_BEQ;
      break;
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
// Is this a candidate for ld/st merging or pairing?  For example, we don't
// touch volatiles or load/stores that have a hint to avoid pair formation.
bool EpiphanyInstrInfo::isCandidateToMergeOrPair(MachineInstr &MI) const {
  // If this is a volatile load/store, don't mess with it.
  if (MI.hasOrderedMemoryRef()) {
    DEBUG(dbgs() << "Volatile load/store, skipping\n");
    return false;
  }

  // Make sure this is a reg+imm (as opposed to an address reloc).
  assert((MI.getOperand(1).isReg() || MI.getOperand(1).isFI()) && "Expected a reg operand.");
  if (!MI.getOperand(2).isImm())
    return false;

  // Can't merge/pair if the instruction modifies the base register.
  // e.g., ldr r0, [r0]
  unsigned BaseReg = MI.getOperand(1).isReg() ? MI.getOperand(1).getReg() : Epiphany::FP;
  const TargetRegisterInfo *TRI = &getRegisterInfo();
  if (MI.modifiesRegister(BaseReg, TRI))
    return false;

  return true;
}

/// isLoadFromStackSlot - If the specified machine instruction is a direct
/// load from a stack slot, return the virtual or physical register number of
/// the destination along with the FrameIndex of the loaded stack slot.  If
/// not, return 0.  This predicate must return 0 if the instruction has
/// any side effects other than loading from the stack slot.
unsigned EpiphanyInstrInfo::isLoadFromStackSlot(const MachineInstr &MI,
    int &FrameIndex) const {
  // Load instructions
  unsigned inst[] = {
    Epiphany::LDRi16_r16,
    Epiphany::LDRi16_r32,
    Epiphany::LDRi16_idx_add_r16,
    Epiphany::LDRi16_idx_add_r32,
    Epiphany::LDRi16_idx_sub_r32,
    Epiphany::LDRi16_pm_add_r16,
    Epiphany::LDRi16_pm_add_r32,
    Epiphany::LDRi16_pm_sub_r32,
    Epiphany::LDRi16_pmd_r32,
    Epiphany::LDRi32_r16,
    Epiphany::LDRi32_r32,
    Epiphany::LDRi32_idx_add_r16,
    Epiphany::LDRi32_idx_add_r32,
    Epiphany::LDRi32_idx_sub_r32,
    Epiphany::LDRi32_pm_add_r16,
    Epiphany::LDRi32_pm_add_r32,
    Epiphany::LDRi32_pm_sub_r32,
    Epiphany::LDRi32_pmd_r32,
    Epiphany::LDRi64,
    Epiphany::LDRf64
  };
  DEBUG(dbgs() << "\nisLoadToStackSlot for "; MI.print(dbgs()));
  // Check if current opcode is one of those
  bool found = (std::find(std::begin(inst), std::end(inst), MI.getOpcode()) != std::end(inst));
  // If true, check operands
  if (found) {
    if (MI.getOperand(1).isFI() && MI.getOperand(2).isImm() && MI.getOperand(2).getImm() == 0) {
      FrameIndex = MI.getOperand(1).getIndex();
      DEBUG(dbgs() << "\nFound load op for "; MI.print(dbgs()));
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
    Epiphany::STRi16_r16,
    Epiphany::STRi16_r32,
    Epiphany::STRi16_idx_add_r16,
    Epiphany::STRi16_idx_add_r32,
    Epiphany::STRi16_idx_sub_r32,
    Epiphany::STRi16_pm_add_r16,
    Epiphany::STRi16_pm_add_r32,
    Epiphany::STRi16_pm_sub_r32,
    Epiphany::STRi16_pmd_r32,
    Epiphany::STRi32_r16,
    Epiphany::STRi32_r32,
    Epiphany::STRi32_idx_add_r16,
    Epiphany::STRi32_idx_add_r32,
    Epiphany::STRi32_idx_sub_r32,
    Epiphany::STRi32_pm_add_r16,
    Epiphany::STRi32_pm_add_r32,
    Epiphany::STRi32_pm_sub_r32,
    Epiphany::STRi32_pmd_r32,
    Epiphany::STRi64,
    Epiphany::STRf64
  };
  DEBUG(dbgs() << "\nisStoreToStackSlot for "; MI.print(dbgs()));
  // Check if current opcode is one of those
  bool found = (std::find(std::begin(inst), std::end(inst), MI.getOpcode()) != std::end(inst));
  // If true, check operands
  if (found) {
    if (MI.getOperand(0).isFI() && MI.getOperand(1).isImm() && MI.getOperand(1).getImm() == 0) {
      FrameIndex = MI.getOperand(0).getIndex();
      DEBUG(dbgs() << "\nFound store op for "; MI.print(dbgs()));
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
  unsigned Opc;
  // Choose instruction
  if (Epiphany::GPR16RegClass.hasSubClassEq(Rd)) {
    Opc = Epiphany::STRi32_r32;
  } else if (Epiphany::GPR32RegClass.hasSubClassEq(Rd)) {
    Opc = Epiphany::STRi32_r32;
  } else if (Epiphany::FPR32RegClass.hasSubClassEq(Rd)) {
    Opc = Epiphany::STRf32;
  } else if (Epiphany::GPR64RegClass.hasSubClassEq(Rd)) {
    Opc = Epiphany::STRi64;
  } else if (Epiphany::FPR64RegClass.hasSubClassEq(Rd)) {
    Opc = Epiphany::STRf64;
  }

  if (!Opc) {
    DEBUG(dbgs() << "\nFail ahead!\n";);
  }
  assert(Opc && "Can't load reg, unknown reg class");

  // Get function and frame info
  if (MI != MBB.end()) DL = MI->getDebugLoc();
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();

  // Get mem operand where to store
  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIdx),
      MachineMemOperand::MOStore, 
      MFI.getObjectSize(FrameIdx),
      MFI.getObjectAlignment(FrameIdx));

  // Build instruction
  BuildMI(MBB, MI, DL, get(Opc)).addReg(SrcReg, getKillRegState(KillSrc))
    .addFrameIndex(FrameIdx).addImm(0).addMemOperand(MMO);
}

void EpiphanyInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MI, unsigned DestReg, int FrameIdx,
    const TargetRegisterClass *Rd, const TargetRegisterInfo *TRI) const {
  DebugLoc DL;
  // Choose instruction
  unsigned Opc;
  if (Epiphany::GPR16RegClass.hasSubClassEq(Rd)) {
    Opc = Epiphany::LDRi32_r32;
  } else if (Epiphany::GPR32RegClass.hasSubClassEq(Rd)) {
    Opc = Epiphany::LDRi32_r32;
  } else if (Epiphany::FPR32RegClass.hasSubClassEq(Rd)) {
    Opc = Epiphany::LDRf32;
  } else if (Epiphany::GPR64RegClass.hasSubClassEq(Rd)) {
    Opc = Epiphany::LDRi64;
  } else if (Epiphany::FPR64RegClass.hasSubClassEq(Rd)) {
    Opc = Epiphany::LDRf64;
  }

  if (!Opc) {
    DEBUG(dbgs() << "\nFail ahead!\n";);
  }
  assert(Opc && "Can't load reg, unknown reg class");

  // Get function and frame info
  if (MI != MBB.end()) DL = MI->getDebugLoc();
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();

  // Get mem operand from where to load
  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIdx),
      MachineMemOperand::MOLoad, MFI.getObjectSize(FrameIdx),
      MFI.getObjectAlignment(FrameIdx));

  // Build instruction
  BuildMI(MBB, MI, DL, get(Opc), DestReg).addFrameIndex(FrameIdx).addImm(0).addMemOperand(MMO);
}

static MachineMemOperand* extractMemOp(MachineSDNode *Load) {
  MachineMemOperand **IMemOp = Load->memoperands_begin();
  MachineMemOperand* MMO = nullptr;

  if(IMemOp) {
    MMO = *IMemOp;
    assert(++IMemOp == Load->memoperands_end() &&
        "Expect a single memory operand in a load");
  }

  return MMO;
}

// gets the chain operand from an SDNode.
static SDValue getChainOperand(SDNode *Node) {
  // Loop past any glue nodes.
  assert(Node->getNumOperands() &&
         "Expect non-zero operand count on SDNode in chain");
  unsigned OpIndex  = Node->getNumOperands() - 1;
  while(OpIndex && Node->getOperand(OpIndex).getValueType() == MVT::Glue) {
    --OpIndex;
  }

  // OpIndex is now the index of the last non-glue operand.
  SDValue ChainOp = Node->getOperand(OpIndex);
  assert(ChainOp.getValueType() == MVT::Other &&
         "Expected Chain Operand on mayLoad MachineSDNode!");
  return ChainOp;
}

bool EpiphanyInstrInfo::areLoadsFromSameBasePtr(SDNode *Load1, SDNode *Load2,
    int64_t &Offset1, int64_t &Offset2) const {

  // Only interested in MachineSDNodes
  if(!Load1->isMachineOpcode() || !Load2->isMachineOpcode()) {
    return false;
  }

  const MCInstrDesc &MCIDesc1 = get(Load1->getMachineOpcode());
  const MCInstrDesc &MCIDesc2 = get(Load2->getMachineOpcode());
  // Only interested in 'real' loads.
  if (MCIDesc1.isPseudo() || !MCIDesc1.mayLoad() || MCIDesc2.isPseudo() || !MCIDesc2.mayLoad()) {
    return false;
  }

  // only interested in Loads in the same chain.
  if(getChainOperand(Load1) != getChainOperand(Load2)) {
    return false;
  }

  // Get the memory operands
  MachineSDNode *MachineLoad1 = dyn_cast<MachineSDNode>(Load1);
  MachineSDNode *MachineLoad2 = dyn_cast<MachineSDNode>(Load2);
  assert(MachineLoad1 && MachineLoad1);
  MachineMemOperand *MemOp1 = extractMemOp(MachineLoad1);
  MachineMemOperand *MemOp2 = extractMemOp(MachineLoad2);

  // Not every load will have its MMO properly set. For example the loads
  // created from intrinsic calls may not have them set.
  if(!MemOp1 || !MemOp2)
    return false;

  // Check that the memory ops use the same base value
  if(MemOp1->getValue() == MemOp2->getValue()) {
    Offset1 = MemOp1->getOffset();
    Offset2 = MemOp2->getOffset();
    return true;
  }

  // Loads are off different base values.
  return false;
}

void EpiphanyInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I,
    const DebugLoc &DL, unsigned DestReg,
    unsigned SrcReg, bool KillSrc) const {
  unsigned Opc = 0;
  unsigned BeginIdx = 0;
  unsigned SubRegs = 0;

  // Do not copy Special regs
  if (Epiphany::SpecialRegClass.contains(DestReg) || Epiphany::SpecialRegClass.contains(SrcReg))
    return;

  if (Epiphany::GPR32RegClass.contains(DestReg, SrcReg)) { // Copy between regs
    Opc = Epiphany::MOVi32rr;
  } else if (Epiphany::FPR32RegClass.contains(DestReg, SrcReg)) {
    Opc = Epiphany::MOVf32rr;
  } else if (Epiphany::GPR64RegClass.contains(DestReg, SrcReg)) { // Copy between regs
    Opc = Epiphany::MOVi32rr;
    SubRegs = 2;
    BeginIdx = Epiphany::isub_hi;
  } else if (Epiphany::FPR64RegClass.contains(DestReg, SrcReg)) {
    Opc = Epiphany::MOVf32rr;
    SubRegs = 2;
    BeginIdx = Epiphany::isub_hi;
  }

  assert(Opc && "Cannot copy registers");

  if (SubRegs == 0) {
    // 32-bit reg copy
    DEBUG(dbgs() << "Expanding 32-bit copy\n";);
    BuildMI(MBB, I, DL, get(Opc), DestReg).addReg(SrcReg, getKillRegState(KillSrc));
  } else {
    // 64-bit reg copy
    const TargetRegisterInfo *TRI = &getRegisterInfo();
    MachineInstrBuilder Mov;
    DEBUG(dbgs() << "Expanding 64-bit copy\n";);
    for (unsigned i = 0; i != SubRegs; ++i) {
      // Get subregs
      DEBUG(dbgs() << "Expanding subreg " << i << "\n";);
      unsigned DstSub = TRI->getSubReg(DestReg, BeginIdx + i);
      unsigned SrcSub = TRI->getSubReg(SrcReg, BeginIdx + i);
      assert(DstSub && SrcSub && "Bad sub-register");
      // Build op
      Mov = BuildMI(MBB, I, DL, get(Opc), DstSub).addReg(SrcSub, getKillRegState(KillSrc));
    }
    // Add implicit super-register defs and kills to the last instruction.
    Mov->addRegisterDefined(DestReg, TRI);
    if (KillSrc) {
      Mov->addRegisterKilled(SrcReg, TRI);
    }
  }

}

//@adjustStackPtr
/// Adjusts stack pointer
void EpiphanyInstrInfo::adjustStackPtr(unsigned SP, int64_t Amount,
    MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I) const {
  DebugLoc DL = I != MBB.end() ? I->getDebugLoc() : DebugLoc();
  unsigned IP = Epiphany::IP;
  unsigned ADDri = Epiphany::ADDri_r32;
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
