//===-- EpiphanyHardwareLoops.cpp - Identify and generate hardware loops --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass identifies loops where we can generate the Hexagon hardware
// loop instruction.  The hardware loop can perform loop branches with a
// zero-cycle overhead.
//
// Criteria for loops:
//  - All interrupts must be disabled while inside a hardware loop.
//  - The start of the loop must be aligned on a double word boundary.
//  - The next-to-last instruction must be aligned on a double word boundary.
//  - All instructions in the loop set as 32 bit instructions using “.l” assembly suffix
//  - The minimum loop length is 8 instructions.
//
// Criteria for chosing the loop:
//  - Countable loops (w/ ind. var for a trip count)
//  - Try inner-most loops first
//  - No function calls in loops.
//
// This passed is based on HexagonHardwareLoops.cpp pass
//
//===----------------------------------------------------------------------===//

#include <MCTargetDesc/EpiphanyBaseInfo.h>
#include "EpiphanyHardwareLoops.h"

using namespace llvm;

#define DEBUG_TYPE "epiphany_hw_loop"

char EpiphanyHardwareLoopsPre::ID = 0;
char EpiphanyHardwareLoopsPost::ID = 0;

INITIALIZE_PASS_BEGIN(EpiphanyHardwareLoopsPre, "epiphany_hw_loops_pre", "Epiphany Hardware Loops Pre-RA", false, false)
  INITIALIZE_PASS_DEPENDENCY(MachineDominatorTree)
  INITIALIZE_PASS_DEPENDENCY(MachineLoopInfo)
INITIALIZE_PASS_END(EpiphanyHardwareLoopsPre, "epiphany_hw_loops_pre", "Epiphany Hardware Loops Pre-RA", false, false)

INITIALIZE_PASS_BEGIN(EpiphanyHardwareLoopsPost, "epiphany_hw_loops_post", "Epiphany Hardware Loops Post-RA", false,
                      false)
  INITIALIZE_PASS_DEPENDENCY(MachineDominatorTree)
  INITIALIZE_PASS_DEPENDENCY(MachineLoopInfo)
  INITIALIZE_PASS_DEPENDENCY(EpiphanyHardwareLoopsPre)
INITIALIZE_PASS_END(EpiphanyHardwareLoopsPost, "epiphany_hw_loops_post", "Epiphany Hardware Loops Post-RA", false,
                    false)


//===----------------------------------------------------------------------===//
// General HW loops methods
//===----------------------------------------------------------------------===//

bool EpiphanyHardwareLoops::containsInvalidInstruction(MachineLoop *L) {
  for (MachineBasicBlock *MBB : L->getBlocks()) {
    for (MachineInstr &MI : *MBB) {
      if (isInvalidLoopOperation(MI)) {
        DEBUG(dbgs() << "Cannot convert to hw_loop, illegal instruction found\n";
                  MI.dump(););
        return true;
      }
    }
  }
  return false;
}

bool EpiphanyHardwareLoops::isInvalidLoopOperation(MachineInstr &MI) {
  // Call is not allowed because the callee may use a hardware loop
  if (MI.getDesc().isCall())
    return true;

  // Must use 4-byte long instructions
  // TODO: We can actually convert between them
  if (TII->getInstSizeInBytes(MI) != 4 && !MI.isTransient())
    return true;

  return false;
}

bool EpiphanyHardwareLoops::lessThanEightCalls(MachineLoop *L) {
  const std::vector<MachineBasicBlock *> &Blocks = L->getBlocks();
  int size = 0;
  std::for_each(Blocks.begin(), Blocks.end(), [&size](MachineBasicBlock *MBB) { size += MBB->size(); });
  return size < 8;
}

bool EpiphanyHardwareLoops::isLoopEligible(MachineLoop *L) {
  // Don't generate hw loop if the loop has more than one exit.
  MachineBasicBlock *MBB = L->findLoopControlBlock();
  if (!MBB) {
    DEBUG(dbgs() << "Cannot convert to hw_loop, has more than one exit\n");
    return false;
  }

  // Does the loop contain any invalid instructions?
  if (containsInvalidInstruction(L))
    return false;

  // Loop must be longer than 8 instructions
  if (lessThanEightCalls(L)) {
    DEBUG(dbgs() << "Cannot convert to hw_loop, loop is less than 8 instructions long\n");
    return false;
  }

  MachineBasicBlock *TopMBB = L->getTopBlock();
  MachineBasicBlock::pred_iterator PI = TopMBB->pred_begin();
  assert(PI != TopMBB->pred_end() &&
         "Loop must have more than one incoming edge!");
  MachineBasicBlock *Backedge = *PI++;
  if (PI == TopMBB->pred_end())  // dead loop?
    return false;
  MachineBasicBlock *Incoming = *PI++;
  if (PI != TopMBB->pred_end())  // multiple backedges?
    return false;

  // Make sure there is one incoming and one backedge and determine which
  // is which.
  if (L->contains(Incoming)) {
    if (L->contains(Backedge))
      return false;
    std::swap(Incoming, Backedge);
  } else if (!L->contains(Backedge)) {
    return false;
  }

  return true;
}

//===----------------------------------------------------------------------===//
// Pre-RA HW loops methods
//===----------------------------------------------------------------------===//

bool EpiphanyHardwareLoopsPre::runOnMachineFunction(MachineFunction &MF) {
  Subtarget = &static_cast<const EpiphanySubtarget &>(MF.getSubtarget());
  TII = Subtarget->getInstrInfo();
  TRI = Subtarget->getRegisterInfo();
  MFI = &MF.getFrameInfo();
  MRI = &MF.getRegInfo();
  MLI = &getAnalysis<MachineLoopInfo>();

  bool Changed = false;
  for (auto &L : *MLI) {
    // Find innermost loop
    if (!L->getSubLoops().empty()) {
      continue;
    }

    // Ensure the loop has a preheader: the loop instruction will be
    // placed there.
    MachineBasicBlock *Preheader = MLI->findLoopPreheader(L, /* Speculative preheader = */ false);
    if (!Preheader) {
      DEBUG(dbgs() << "Cannot convert to hw_loop, no preheader\n");
      continue;
    }
    MachineBasicBlock::iterator InsertPos = Preheader->getFirstTerminator();

    if (!isLoopEligible(L)) {
      continue;
    }

    // Try to find trip count
    MachineInstr *BranchMI = nullptr, *CmpMI = nullptr, *BumpMI = nullptr;
    findControlInstruction(L, BranchMI, CmpMI, BumpMI);
    if (!BranchMI || !CmpMI || !BumpMI) {
      DEBUG(dbgs() << "Unable to find control instructions\n");
      continue;
    }

    // Check if all conditions are met
    // TODO: This is the simples case with numeric bump and compare
    if (!BumpMI->getOperand(2).isImm()) {
      DEBUG(dbgs() << "Bump is defined with reg, not implemented yet\n");
      continue;
    }

    int64_t BumpValue = BumpMI->getOperand(2).getImm();
    int64_t CmpValue;
    if (!findCmpValue(CmpMI, CmpValue)) {
      DEBUG(dbgs() << "Unable to find CMP value\n");
      continue;
    }
    if (CmpValue % BumpValue != 0) {
      DEBUG(dbgs() << "Unable to determine exact loop count, CMP value is not a multiple of loop bump\n");
      return false;
    };
    int64_t TripCount = CmpValue / BumpValue;

    // Determine the loop start.
    MachineBasicBlock *TopBlock = L->getTopBlock();
    MachineBasicBlock *ExitingBlock = L->findLoopControlBlock();
    MachineBasicBlock *LoopStart = findLoopStart(ExitingBlock, TopBlock, L);
    if (TopBlock == nullptr || ExitingBlock == nullptr || LoopStart == nullptr) {
      DEBUG(dbgs() << "Unable to determine main loop blocks\n");
      continue;
    }

    // Convert the loop to a hardware loop.
    DEBUG(dbgs() << "Change to hardware loop at ";
              L->dump());
    DebugLoc DL;
    if (InsertPos != Preheader->end()) {
      DL = InsertPos->getDebugLoc();
    }

    // Get addresses
    MachineBasicBlock *ExitMBB = L->getExitBlock();
    LoopStart->setHasAddressTaken();
    BlockAddress *StartAddress = BlockAddress::get(const_cast<BasicBlock *>(LoopStart->getBasicBlock()));
    ExitMBB->setHasAddressTaken();
    BlockAddress *ExitAddress = BlockAddress::get(const_cast<BasicBlock *>(ExitMBB->getBasicBlock()));
    addLoopSetInstructions(*Preheader, InsertPos, TripCount, DL, StartAddress, ExitAddress);

    // Enable interrupts
    BuildMI(*ExitMBB, ExitMBB->begin(), DL, TII->get(Epiphany::GIE));
    LoopStart->setAlignment(3);
    Changed = true;
  }

  return Changed;
}

MachineBasicBlock *EpiphanyHardwareLoopsPre::findLoopStart(MachineBasicBlock *ExitingBlock,
                                                           MachineBasicBlock *TopBlock,
                                                           MachineLoop *L) {
  if (ExitingBlock != L->getLoopLatch()) {
    MachineBasicBlock *TB = nullptr, *FB = nullptr;
    SmallVector<MachineOperand, 2> Cond;
    if (TII->analyzeBranch(*ExitingBlock, TB, FB, Cond, false)) {
      return nullptr;
    }

    if (L->contains(TB)) {
      return TB;
    } else if (L->contains(FB)) {
      return FB;
    }
  } else {
    return TopBlock;
  }

  return nullptr;
}

void EpiphanyHardwareLoopsPre::addLoopSetInstructions(MachineBasicBlock &Preheader,
                                                      MachineBasicBlock::iterator &InsertPos,
                                                      int64_t TripCount, const DebugLoc &DL,
                                                      const BlockAddress *StartAddress,
                                                      const BlockAddress *ExitAddress) const {
  // Start
  unsigned LowReg = MRI->createVirtualRegister(&Epiphany::GPR32RegClass);
  BuildMI(Preheader, InsertPos, DL, TII->get(Epiphany::MOVi32ri), LowReg)
      .addBlockAddress(StartAddress, /* Offset = */ 0, EpiphanyII::MO_LOW);
  unsigned Reg = MRI->createVirtualRegister(&Epiphany::GPR32RegClass);
  BuildMI(Preheader, InsertPos, DL, TII->get(Epiphany::MOVTi32ri), Reg)
      .addReg(LowReg)
      .addBlockAddress(StartAddress, /* Offset = */ 0, EpiphanyII::MO_HIGH);
  BuildMI(Preheader, InsertPos, DL, TII->get(Epiphany::MOVTS32_core), Epiphany::LS).addReg(Reg);
  // End
  LowReg = MRI->createVirtualRegister(&Epiphany::GPR32RegClass);
  BuildMI(Preheader, InsertPos, DL, TII->get(Epiphany::MOVi32ri), LowReg)
      .addBlockAddress(ExitAddress, /* Offset = */ 0, EpiphanyII::MO_LOW);
  Reg = MRI->createVirtualRegister(&Epiphany::GPR32RegClass);
  BuildMI(Preheader, InsertPos, DL, TII->get(Epiphany::MOVTi32ri), Reg)
      .addReg(LowReg)
      .addBlockAddress(ExitAddress, /* Offset = */ 0, EpiphanyII::MO_HIGH);
  BuildMI(Preheader, InsertPos, DL, TII->get(Epiphany::MOVTS32_core), Epiphany::LE).addReg(Reg);
  // Count
  Reg = MRI->createVirtualRegister(&Epiphany::GPR32RegClass);
  BuildMI(Preheader, InsertPos, DL, TII->get(Epiphany::MOVi32ri), Reg).addImm(TripCount & 0xffff);
  if (!isIntN(16, TripCount)) {
    LowReg = Reg;
    Reg = MRI->createVirtualRegister(&Epiphany::GPR32RegClass);
    BuildMI(Preheader, InsertPos, DL, TII->get(Epiphany::MOVTi32ri), Reg).addReg(LowReg).addImm(TripCount & 0xffff0000);
  }
  BuildMI(Preheader, InsertPos, DL, TII->get(Epiphany::MOVTS32_core), Epiphany::LC).addReg(Reg);
  // Disable interrupts
  BuildMI(Preheader, InsertPos, DL, TII->get(Epiphany::GID));
}

void EpiphanyHardwareLoopsPre::findControlInstruction(MachineLoop *L, MachineInstr *&BranchMI, MachineInstr *&CmpMI,
                                                      MachineInstr *&BumpMI) {
  // Look for the cmp instruction to determine if we can get a useful trip
  // count.  The trip count can be either a register or an immediate.  The
  // location of the value depends upon the type (reg or imm).
  MachineBasicBlock *ExitingBlock = L->findLoopControlBlock();
  if (!ExitingBlock)
    return;

  // Check if branch is analyzable
  SmallVector<MachineOperand, 2> Cond;
  MachineBasicBlock *TB = nullptr, *FB = nullptr;
  bool NotAnalyzed = TII->analyzeBranch(*ExitingBlock, TB, FB, Cond, false);
  if (NotAnalyzed)
    return;

  MachineBasicBlock::reverse_instr_iterator I = ExitingBlock->instr_rbegin();
  MachineBasicBlock::reverse_instr_iterator E = ExitingBlock->instr_rend();
  // Find branch
  while (I != E) {
    if (I->isConditionalBranch()) {
      BranchMI = &*I;
      break;
    }
    if (!I->isBranch()) {
      DEBUG(dbgs() << "Non-branch between branches, not implemented yet, exiting\n");
      break;
    }
    I++;
  }

  // Find last compare
  while (++I != E) {
    if (I->isCompare()) {
      CmpMI = &*I;
      break;
    }
    if (I->definesRegister(Epiphany::STATUS, TRI)) {
      DEBUG(dbgs() << "Status flag rewritten without compare, exiting\n");
      break;
    }
  }

  // Find bump
  unsigned CmpReg = CmpMI->getOperand(1).getReg();
  BumpMI = MRI->getVRegDef(CmpReg);
}

bool EpiphanyHardwareLoopsPre::findCmpValueInner(MachineInstr *CmpMI, int64_t &CmpValue) {
  switch (CmpMI->getOpcode()) {
    case Epiphany::MOVTi32ri:
      CmpValue += CmpMI->getOperand(2).getImm() << 16;
      return findCmpValueInner(MRI->getVRegDef(CmpMI->getOperand(1).getReg()), CmpValue);
    case Epiphany::MOVi32ri:
    case Epiphany::MOVi16ri:
      CmpValue += CmpMI->getOperand(1).getImm();
      return true;
    default:
      return false;
  }
}

bool EpiphanyHardwareLoopsPre::findCmpValue(MachineInstr *CmpMI, int64_t &CmpValue) {
  if (CmpMI->getOperand(2).isImm()) {
    CmpValue = CmpMI->getOperand(2).getImm();
    return true;
  }

  if (!CmpMI->getOperand(2).isReg()) {
    DEBUG(dbgs() << "Comparing with neither reg nor imm, don't know how to proceed\n");
    return false;
  }

  CmpMI = MRI->getVRegDef(CmpMI->getOperand(2).getReg());
  CmpValue = 0;
  return findCmpValueInner(CmpMI, CmpValue);
}

//===----------------------------------------------------------------------===//
// Post-RA HW loops methods
//===----------------------------------------------------------------------===//

bool EpiphanyHardwareLoopsPost::runOnMachineFunction(MachineFunction &MF) {
  Subtarget = &static_cast<const EpiphanySubtarget &>(MF.getSubtarget());
  TII = Subtarget->getInstrInfo();
  TRI = Subtarget->getRegisterInfo();
  MFI = &MF.getFrameInfo();
  MRI = &MF.getRegInfo();
  MLI = &getAnalysis<MachineLoopInfo>();

  bool Changed = false;
  for (auto &L : *MLI) {
    // Find loop preheader and check if it has previously added HW loop instructions
    MachineBasicBlock *Preheader = MLI->findLoopPreheader(L, /* Speculative preheader = */ false);
    MachineBasicBlock *StartBlock = L->getTopBlock();
    MachineBasicBlock *ExitingBlock = L->getExitingBlock();
    if (!Preheader) {
      continue;
    }

    bool HasHwLoop = false;
    for (MachineInstr &MI : reverse(*Preheader)) {
      if (MI.getOpcode() == Epiphany::MOVTS32_core
          && MI.getOperand(0).isReg()
          && MI.getOperand(0).getReg() == Epiphany::LE) {
        HasHwLoop = true;
        break;
      }
    }
    if (!HasHwLoop) {
      continue;
    }
    DEBUG(dbgs() << "Found HW loop pre-header");

    if (!isLoopEligible(L)) {
      DEBUG(dbgs() << "Loop is not eligible anymore, removing HW loop");
      removeHardwareLoop(Preheader);
      continue;
    }

    cleanUpBranch(StartBlock, ExitingBlock);
    createExitMBB(MF, L);
    Changed = true;
  }

  return Changed;
}

void EpiphanyHardwareLoopsPost::cleanUpBranch(const MachineBasicBlock *StartBlock,
                                              MachineBasicBlock *ExitingBlock) const {
  bool IsConditionalBranch = false;
  auto E = ExitingBlock->instr_rend();

  // Remove last branch pointing to the start block
  for (auto MI = ExitingBlock->instr_rbegin(); MI != E; ++MI) {
    if (MI->isBranch() && MI->getNumOperands() > 0 && MI->getOperand(0).getMBB() == StartBlock) {
      IsConditionalBranch = MI->isConditionalBranch();
      MI->eraseFromParent();
      break;
    }
  }

  // In case of conditional branch - remove last CMP
  if (IsConditionalBranch) {
    for (auto MI = ExitingBlock->instr_rbegin(); MI != E; ++MI) {
      if (MI->isCompare()) {
        MI->eraseFromParent();
        break;
      }
    }
  }
}

void EpiphanyHardwareLoopsPost::removeHardwareLoop(MachineBasicBlock *Preheader) {
  for (MachineInstr &MI : *Preheader) {
    if (MI.getOpcode() == Epiphany::MOVTS32_core && MI.getOperand(0).isReg()) {
      unsigned Reg = MI.getOperand(0).getReg();
      if (Reg == Epiphany::LE || Reg == Epiphany::LS || Reg == Epiphany::LC) {
        MI.eraseFromParent();
      }
    }
  }
}

// TODO: Store `movts LE` instruction pointer and create new basic block, as there might be smth else pointing to it.
/// Create HW loop exit blocks matching requirements.
/// There are two main requirements:
///  - The instruction before last should have dword alignment
///  - The instruction pointed by LE should be the last one. After Pre-RA, LE points at the exit block begin
/// The easiest way to satisfy both is to first move all instructions from the Exit block to the new block,
/// making a clean exit block, and transfer one last instruction of the loop into this block. It will allow to skip
/// dealing with basic blocks and memory addresses.
/// Next, move one more instruction from the loop to the new dword-aligned block
///
/// \param MF Machine function
/// \param L Currently processed loop
void EpiphanyHardwareLoopsPost::createExitMBB(MachineFunction &MF, MachineLoop *L) const {
  MachineBasicBlock *ExitingBlock = L->getExitingBlock();
  MachineBasicBlock *ExitBlock = L->getExitBlock();
  MachineBasicBlock *NewExitBlock = MF.CreateMachineBasicBlock(ExitBlock->getBasicBlock());
  MachineBasicBlock *AlignedBlock = MF.CreateMachineBasicBlock(ExitingBlock->getBasicBlock());

  // Move all instruction from the exit block to new exit block
  MF.insert(++ExitBlock->getIterator(), NewExitBlock);
  MF.addToMBBNumbering(NewExitBlock);
  NewExitBlock->splice(NewExitBlock->begin(), ExitBlock, ExitBlock->begin(), ExitBlock->end());
  NewExitBlock->transferSuccessorsAndUpdatePHIs(ExitBlock);

  // Move last instruction of the exiting block to now-empty exit block
  // LE-pointed instruction will still be handled
  ExitBlock->splice(ExitBlock->begin(), ExitingBlock, (--ExitingBlock->end()));
  ExitBlock->transferSuccessorsAndUpdatePHIs(ExitingBlock);

  // Move all instruction from the exit block to new exit block
  MF.insert(++ExitingBlock->getIterator(), AlignedBlock);
  MF.addToMBBNumbering(AlignedBlock);
  AlignedBlock->splice(AlignedBlock->begin(), ExitingBlock, (--ExitingBlock->end()));
  AlignedBlock->setAlignment(3);
}

/// createEpiphanyLoadStoreOptimizationPass - returns an instance of the
/// pre-RA hardware loops optimization pass part.
FunctionPass *llvm::createEpiphanyHardwareLoopsPrePass() {
  return new EpiphanyHardwareLoopsPre();
}

/// createEpiphanyLoadStoreOptimizationPass - returns an instance of the
/// post-RA hardware loops optimization pass part.
FunctionPass *llvm::createEpiphanyHardwareLoopsPostPass() {
  return new EpiphanyHardwareLoopsPost();
}


