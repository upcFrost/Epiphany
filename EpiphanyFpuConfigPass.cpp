//===---------------------EpiphanyFpuConfigPass.cpp -----------------------===//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass adds correct FPU/IALU2 flags to CONFIG register.
//
//  Run through all instruction of the first block and its successors, find
//  first FPU instruction in each branch, set config flag on the top-level MBB and restore it
//  at the end.
//

#include "EpiphanyFpuConfigPass.h"

using namespace llvm;

#define DEBUG_TYPE "epiphany_fpu_config"


char EpiphanyFpuConfigPass::ID = 0;

bool EpiphanyFpuConfigPass::runOnMachineFunction(MachineFunction &MF) {
  DEBUG(dbgs() << "\nRunning Epiphany FPU config pass\n");
  auto &ST = MF.getSubtarget<EpiphanySubtarget>();
  const EpiphanyInstrInfo *TII = ST.getInstrInfo();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const TargetRegisterClass *RC = &Epiphany::GPR32RegClass;

  // FPU opcodes
  unsigned opcodesFPU[] = {Epiphany::FADDrr_r16, Epiphany::FADDrr_r32, Epiphany::FSUBrr_r16, Epiphany::FSUBrr_r32,
    Epiphany::FMULrr_r16, Epiphany::FMULrr_r32, Epiphany::FMADDrr_r16, Epiphany::FMADDrr_r32,
    Epiphany::FMSUBrr_r16, Epiphany::FMSUBrr_r32};

  // Prepare binary flag and regs
  bool hasFPU;
  unsigned frameIdx;

  // Step 1: Loop over all of the basic blocks to find the first FPU instruction
  for(MachineFunction::iterator it = MF.begin(), E = MF.end(); it != E; ++it) {
    MachineBasicBlock *MBB = &*it;
    // Loop over all instructions search for FPU instructions
    for(MachineBasicBlock::iterator MBBI = MBB->begin(), MBBE = MBB->end(); MBBI != MBBE; ++MBBI) {
      MachineInstr *MI = &*MBBI;
      // Check if the opcode used is one of the FPU opcodesFPU
      bool isFPU = std::find(std::begin(opcodesFPU), std::end(opcodesFPU), MI->getOpcode()) != std::end(opcodesFPU);
      // If FPU inst found - break and insert config
      if (isFPU) {
        hasFPU = true;
        break;
      }
    }
    // Break if the first instruction was already found
    if (hasFPU) {
      break;
    }
  }

  // Step 2: Insert config
  if (hasFPU) {
    MachineBasicBlock *MBB = &MF.front();
    MachineInstr *insertPos = &MBB->front();
    MBB->addLiveIn(Epiphany::CONFIG);
    DebugLoc DL = insertPos->getDebugLoc();
    // Disable interrupts
    BuildMI(*MBB, insertPos, DL, TII->get(Epiphany::GID));
    // Get current config and save it to stack
    unsigned configTmpReg = MRI.createVirtualRegister(RC);
    BuildMI(*MBB, insertPos, DL, TII->get(Epiphany::MOVFS32rr), configTmpReg).addReg(Epiphany::CONFIG);
    frameIdx = MFI->CreateStackObject(RC->getSize(), /* Alignment = */ RC->getSize(), /* isSS = */ false);
    TII->storeRegToStackSlot(*MBB, insertPos, configTmpReg, /* killReg = */ false, frameIdx, RC, ST.getRegisterInfo());
    // Create mask with bits 19:17 set to 0
    unsigned lowTmpReg = MRI.createVirtualRegister(RC);
    BuildMI(*MBB, insertPos, DL, TII->get(Epiphany::MOVi32ri), lowTmpReg).addImm(0xffff);
    unsigned tmpReg = MRI.createVirtualRegister(RC);
    BuildMI(*MBB, insertPos, DL, TII->get(Epiphany::MOVTi32ri), tmpReg).addReg(lowTmpReg).addImm(0xfff1);
    // Apply mask to configTmpReg
    unsigned maskReg = MRI.createVirtualRegister(RC);
    BuildMI(*MBB, insertPos, DL, TII->get(Epiphany::ANDrr_r32), maskReg).addReg(configTmpReg, RegState::Kill).addReg(tmpReg, RegState::Kill);
    // Push reg back to config
    BuildMI(*MBB, insertPos, DL, TII->get(Epiphany::MOVTS32rr)).addReg(Epiphany::CONFIG).addReg(maskReg, RegState::Kill);
    // Restore interrupts
    BuildMI(*MBB, insertPos, DL, TII->get(Epiphany::GIE));
  }


  // Step 3 - find the last FPU instruction of the last block and insert flag reset 
  if (hasFPU) {
    MachineBasicBlock *MBB = &MF.back();
    MachineBasicBlock::iterator MBBI = MBB->end();
    for (MachineBasicBlock::iterator MBBE = MBB->begin(); MBBI != MBBE; --MBBI) {
      // Check if the opcode used is one of the FPU opcodesFPU
      MachineInstr *MI = &*MBBI;
      bool isFPU = std::find(std::begin(opcodesFPU), std::end(opcodesFPU), MI->getOpcode()) != std::end(opcodesFPU);
      // If FPU inst found - insert config flag
      if (isFPU) {
        break;
      }
    }
    MachineInstr *MI = &*MBBI;
    DebugLoc DL = MI->getDebugLoc();
    unsigned configTmpReg = MRI.createVirtualRegister(RC);
    // Reload old config value 
    TII->loadRegFromStackSlot(*MBB, MBBI, configTmpReg, frameIdx, RC, ST.getRegisterInfo());
    // Disable interrupts
    BuildMI(*MBB, MBBI, DL, TII->get(Epiphany::GID));
    // Upload config value to the core
    BuildMI(*MBB, MBBI, DL, TII->get(Epiphany::MOVTS32rr), Epiphany::CONFIG).addReg(configTmpReg, RegState::Kill);
    // Restore interrupts
    BuildMI(*MBB, MBBI, DL, TII->get(Epiphany::GIE));
  }

  return true;
}

//===----------------------------------------------------------------------===//
//                         Public Constructor Functions
//===----------------------------------------------------------------------===//
FunctionPass *llvm::createEpiphanyFpuConfigPass() {
  return new EpiphanyFpuConfigPass();
}


