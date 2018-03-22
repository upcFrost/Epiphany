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

char EpiphanyHardwareLoops::ID = 0;

INITIALIZE_PASS_BEGIN(EpiphanyHardwareLoops, "epiphany_hw_loops", "Epiphany Hardware Loops", false, false)
  INITIALIZE_PASS_DEPENDENCY(MachineDominatorTree)
  INITIALIZE_PASS_DEPENDENCY(MachineLoopInfo)
INITIALIZE_PASS_END(EpiphanyHardwareLoops, "epiphany_hw_loops", "Epiphany Hardware Loops", false, false)

bool optimizeBlock(MachineBasicBlock *MBB) {
  return false;
}

bool EpiphanyHardwareLoops::runOnMachineFunction(MachineFunction &MF) {
  Subtarget = &static_cast<const EpiphanySubtarget &>(MF.getSubtarget());
  TII = Subtarget->getInstrInfo();
  TRI = Subtarget->getRegisterInfo();
  MFI = &MF.getFrameInfo();
  MRI = &MF.getRegInfo();
  MLI = &getAnalysis<MachineLoopInfo>();

  bool Changed = false;
  for (auto &L : *MLI)
    // Find innermost loop
    if (L->getSubLoops().size() == 0) {
      // Don't generate hw loop if the loop has more than one exit.
      MachineBasicBlock *MBB = L->findLoopControlBlock();
      if (!MBB) {
        DEBUG(dbgs() << "Cannot convert to hw_loop, has more than one exit\n");
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
      if (!BumpMI->getOperand(2).isImm() || !CmpMI->getOperand(2).isImm()) {
        DEBUG(dbgs() << "HW Loops defined with regs are not implemented yet\n");
        continue;
      }
      int64_t TripCount = CmpMI->getOperand(2).getImm() / BumpMI->getOperand(2).getImm();

      // Determine the loop start.
      MachineBasicBlock *TopBlock = L->getTopBlock();
      MachineBasicBlock *ExitingBlock = L->findLoopControlBlock();
      MachineBasicBlock *LoopStart = nullptr;
      if (ExitingBlock != L->getLoopLatch()) {
        MachineBasicBlock *TB = nullptr, *FB = nullptr;
        SmallVector<MachineOperand, 2> Cond;
        if (TII->analyzeBranch(*ExitingBlock, TB, FB, Cond, false)) {
          continue;
        }

        if (L->contains(TB)) {
          LoopStart = TB;
        } else if (L->contains(FB)) {
          LoopStart = FB;
        } else {
          continue;
        }
      } else {
        LoopStart = TopBlock;
      }

      // Convert the loop to a hardware loop.
      DEBUG(dbgs() << "Change to hardware loop at ";
                L->dump());
      DebugLoc DL;
      if (InsertPos != Preheader->end()) {
        DL = InsertPos->getDebugLoc();
      }

      unsigned TempReg = BumpMI->getOperand(0).getReg();
      CmpMI->removeFromParent();
      BranchMI->removeFromParent();

      // Allocate two new blocks and align first one to dword to meet condition
      MachineBasicBlock *AlignBlock = MF.CreateMachineBasicBlock(ExitingBlock->getBasicBlock());
      MF.insert(++ExitingBlock->getIterator(), AlignBlock);
      MF.addToMBBNumbering(AlignBlock);
      AlignBlock->setAlignment(3);
      // Split two instructions from the end
      auto SplitBegin = ExitingBlock->begin();
      auto SplitEnd = ExitingBlock->begin();
      (SplitEnd++)++;
      while (++SplitEnd != ExitingBlock->end()) {
        SplitBegin++;
      }
      AlignBlock->splice(AlignBlock->begin(), ExitingBlock, SplitBegin, SplitEnd);
      AlignBlock->transferSuccessorsAndUpdatePHIs(ExitingBlock);

      // We must create new Basic block to get new global address
      BasicBlock *NewExitBlock = BasicBlock::Create(AlignBlock->getBasicBlock()->getContext(),
                                                    AlignBlock->getBasicBlock()->getName() + ".end",
                                                    const_cast<Function *>(AlignBlock->getBasicBlock()->getParent()));
      MachineBasicBlock *NewExitMBB = MF.CreateMachineBasicBlock(NewExitBlock);
      MF.insert(++AlignBlock->getIterator(), NewExitMBB);
      MF.addToMBBNumbering(NewExitMBB);
      NewExitMBB->splice(NewExitMBB->begin(), AlignBlock, --AlignBlock->end());
      NewExitMBB->transferSuccessorsAndUpdatePHIs(AlignBlock);

      // Get addresses
      LoopStart->setHasAddressTaken();
      BlockAddress *StartAddress = BlockAddress::get(const_cast<BasicBlock *>(LoopStart->getBasicBlock()));
      NewExitMBB->setHasAddressTaken();
      BlockAddress *ExitAddress = BlockAddress::get(const_cast<BasicBlock *>(NewExitMBB->getBasicBlock()));

      // Add instructions
      BuildMI(*Preheader, InsertPos, DL, TII->get(Epiphany::MOVi32ri), TempReg)
          .addBlockAddress(StartAddress, /* Offset = */ 0, EpiphanyII::MO_LOW);
      BuildMI(*Preheader, InsertPos, DL, TII->get(Epiphany::MOVTi32ri), TempReg)
          .addReg(TempReg)
          .addBlockAddress(StartAddress, /* Offset = */ 0, EpiphanyII::MO_HIGH);
      BuildMI(*Preheader, InsertPos, DL, TII->get(Epiphany::MOVTS32_core), Epiphany::LS).addReg(TempReg);
      // Offset 4 as it Exiting block was split two instructions back, not one
      BuildMI(*Preheader, InsertPos, DL, TII->get(Epiphany::MOVi32ri), TempReg)
          .addBlockAddress(ExitAddress, /* Offset = */ 0, EpiphanyII::MO_LOW);
      BuildMI(*Preheader, InsertPos, DL, TII->get(Epiphany::MOVTi32ri), TempReg)
          .addReg(TempReg)
          .addBlockAddress(ExitAddress, /* Offset = */ 0, EpiphanyII::MO_HIGH);
      BuildMI(*Preheader, InsertPos, DL, TII->get(Epiphany::MOVTS32_core), Epiphany::LE).addReg(TempReg);
      BuildMI(*Preheader, InsertPos, DL, TII->get(Epiphany::MOVi32ri), TempReg).addImm(TripCount);
      BuildMI(*Preheader, InsertPos, DL, TII->get(Epiphany::MOVTS32_core), Epiphany::LC).addReg(TempReg);
      BuildMI(*Preheader, InsertPos, DL, TII->get(Epiphany::GID));
      LoopStart->setAlignment(3);
      Changed = true;
      DEBUG(dbgs() << "Swapped one loop with HW Loop\n");
      DEBUG(Preheader->getParent()->dump());
    }

  return Changed;
}

bool EpiphanyHardwareLoops::containsInvalidInstruction(MachineLoop *L) {
  const std::vector<MachineBasicBlock *> &Blocks = L->getBlocks();
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
  if (TII->getInstSizeInBytes(MI) != 4)
    return true;

  return false;
}

bool EpiphanyHardwareLoops::lessThanEightCalls(MachineLoop *L) {
  const std::vector<MachineBasicBlock *> &Blocks = L->getBlocks();
  int size = 0;
  std::for_each(Blocks.begin(), Blocks.end(), [&size](MachineBasicBlock *MBB) { size += MBB->size(); });
  return size < 8;
}

void EpiphanyHardwareLoops::findControlInstruction(MachineLoop *L, MachineInstr *&BranchMI, MachineInstr *&CmpMI,
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
    if ((&*I)->isBranch()) {
      BranchMI = &*I;
      break;
    }
    I++;
  }

  // Find last compare
  while (++I != E) {
    if ((&*I)->isCompare()) {
      CmpMI = &*I;
      break;
    }
  }

  // Find bump
  unsigned CmpReg = CmpMI->getOperand(1).getReg();
  while (++I != E) {
    if (I->getNumExplicitOperands() > 0 && I->definesRegister(CmpReg, TRI)) {
      BumpMI = &*I;
      break;
    }
  }
}

bool EpiphanyHardwareLoops::isLoopEligible(MachineLoop *L) {
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

/// createEpiphanyLoadStoreOptimizationPass - returns an instance of the
/// load / store optimization pass.
FunctionPass *llvm::createEpiphanyHardwareLoopsPass() {
  return new EpiphanyHardwareLoops();

}
