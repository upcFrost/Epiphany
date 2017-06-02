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
//  For now we do not handle cases where both FPU and IALU2 instructions can be present
//

#include "EpiphanyFpuConfigPass.h"

using namespace llvm;

#define DEBUG_TYPE "epiphany_fpu_config"


char EpiphanyFpuConfigPass::ID = 0;

INITIALIZE_PASS_BEGIN(EpiphanyFpuConfigPass, "epiphany_fpu_config", "Epiphany FPU/IALU2 Config", false, false);
INITIALIZE_PASS_END(EpiphanyFpuConfigPass, "epiphany_fpu_config", "Epiphany FPU/IALU2 Config", false, false);

void EpiphanyFpuConfigPass::insertConfigInst(MachineBasicBlock *MBB, MachineBasicBlock::iterator MBBI, 
    MachineRegisterInfo &MRI, const EpiphanySubtarget &ST, unsigned frameIdx) {

  const TargetRegisterClass *RC = &Epiphany::GPR32RegClass;

  DebugLoc DL = DebugLoc();
  unsigned Reg = MRI.createVirtualRegister(RC);
  TII->loadRegFromStackSlot(*MBB, MBBI, Reg, frameIdx, RC, ST.getRegisterInfo());
  BuildMI(*MBB, MBBI, DL, TII->get(Epiphany::GID)).addReg(Epiphany::CONFIG, RegState::ImplicitDefine);
  BuildMI(*MBB, MBBI, DL, TII->get(Epiphany::MOVTS32_core), Epiphany::CONFIG).addReg(Reg, RegState::Kill);
  BuildMI(*MBB, MBBI, DL, TII->get(Epiphany::GIE)).addReg(Epiphany::CONFIG, RegState::ImplicitDefine);
}

bool EpiphanyFpuConfigPass::runOnMachineFunction(MachineFunction &MF) {
  DEBUG(dbgs() << "\nRunning Epiphany FPU/IALU2 config pass\n");
  auto &ST = MF.getSubtarget<EpiphanySubtarget>();
  TII = ST.getInstrInfo();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const TargetRegisterClass *RC = &Epiphany::GPR32RegClass;

  // FPU opcodes
  unsigned opcodesFPU[] = {Epiphany::FADDrr_r16, Epiphany::FADDrr_r32, Epiphany::FSUBrr_r16, Epiphany::FSUBrr_r32,
    Epiphany::FMULrr_r16, Epiphany::FMULrr_r32, Epiphany::FMADDrr_r16, Epiphany::FMADDrr_r32,
    Epiphany::FMSUBrr_r16, Epiphany::FMSUBrr_r32};
  // IALU2 opcodes
  unsigned opcodesIALU2[] = {Epiphany::IADDrr_r16, Epiphany::IADDrr_r32, Epiphany::ISUBrr_r16, Epiphany::ISUBrr_r32,
    Epiphany::IMULrr_r16, Epiphany::IMULrr_r32, Epiphany::IMADDrr_r16, Epiphany::IMADDrr_r32,
    Epiphany::IMSUBrr_r16, Epiphany::IMSUBrr_r32};

  // Prepare binary flag and regs
  bool hasFPU;
  bool hasIALU2;

  // Step 1: Loop over all of the basic blocks to find the first FPU instruction
  for(MachineFunction::iterator it = MF.begin(), E = MF.end(); it != E; ++it) {
    MachineBasicBlock *MBB = &*it;
    // Loop over all instructions search for FPU instructions
    for(MachineBasicBlock::iterator MBBI = MBB->begin(), MBBE = MBB->end(); MBBI != MBBE; ++MBBI) {
      MachineInstr *MI = &*MBBI;
      // Check if the opcode used is one of the FPU opcodes
      bool isFPU = std::find(std::begin(opcodesFPU), std::end(opcodesFPU), MI->getOpcode()) != std::end(opcodesFPU);
      if (isFPU) {
        hasFPU = true;
      }
      // Check if the opcode used is one of the IALU2 opcodes
      bool isIALU2 = std::find(std::begin(opcodesIALU2), std::end(opcodesIALU2), MI->getOpcode()) != std::end(opcodesIALU2);
      if (isIALU2) {
        hasIALU2 = true;
      }
    }
    // Break if we already know that we have both
    if (hasFPU && hasIALU2) {
      break;
    }
  }

  // Step 2: Insert config
  if (hasFPU || hasIALU2) {
    // Create and insert new basic block for the config reg switch
    MachineBasicBlock *Front = &MF.front();
    MachineBasicBlock *MBB = MF.CreateMachineBasicBlock();
    MachineBasicBlock::iterator insertPos = MBB->begin();
    DebugLoc DL = DebugLoc();
    MF.insert(MF.begin(), MBB);
    MBB->addSuccessor(Front);
    // Add function live-ins so that they'll be defined in every path
    for (MachineRegisterInfo::livein_iterator LB = MRI.livein_begin(), LE = MRI.livein_end(); LB != LE; ++LB) {
      MBB->addLiveIn(LB->first);
    }
    // Create config regs for both cases
    unsigned FpuConfigReg   = MRI.createVirtualRegister(RC);
    unsigned IaluConfigReg  = MRI.createVirtualRegister(RC);
    unsigned OriginalConfigReg = MRI.createVirtualRegister(RC);
    // Disable interrupts
    BuildMI(*MBB, insertPos, DL, TII->get(Epiphany::GID)).addReg(Epiphany::CONFIG, RegState::ImplicitDefine);
    // Gather reg values
    BuildMI(*MBB, insertPos, DL, TII->get(Epiphany::MOVFS32_core), OriginalConfigReg).addReg(Epiphany::CONFIG, RegState::Kill);
    originalFrameIdx = MFI.CreateStackObject(RC->getSize(), /* Alignment = */ RC->getSize(), /* isSS = */ false);
    TII->storeRegToStackSlot(*MBB, insertPos, OriginalConfigReg, /* killReg = */ false, originalFrameIdx, RC, ST.getRegisterInfo());
    // Calculate FPU config reg value
    if (hasFPU) {
      // Create mask with bits 19:17 set to 0
      unsigned lowTmpReg = MRI.createVirtualRegister(RC);
      BuildMI(*MBB, insertPos, DL, TII->get(Epiphany::MOVi32ri), lowTmpReg).addImm(0xffff);
      unsigned tmpReg = MRI.createVirtualRegister(RC);
      BuildMI(*MBB, insertPos, DL, TII->get(Epiphany::MOVTi32ri), tmpReg).addReg(lowTmpReg).addImm(0xfff1);
      // Apply mask to OriginalConfigReg
      BuildMI(*MBB, insertPos, DL, TII->get(Epiphany::ANDrr_r32), FpuConfigReg).addReg(OriginalConfigReg, RegState::Kill).addReg(tmpReg, RegState::Kill);
      // Store fpu config reg to stack
      fpuFrameIdx = MFI.CreateStackObject(RC->getSize(), /* Alignment = */ RC->getSize(), /* isSS = */ false);
      TII->storeRegToStackSlot(*MBB, insertPos, FpuConfigReg, /* killReg = */ false, fpuFrameIdx, RC, ST.getRegisterInfo());
    }
    // Calculate IALU2 conf reg value
    if (hasIALU2) {
      // Set bits 16-32 to 0b0000000001001000 = 0x48 (all other bits are reserved/not recommended
      // TODO: bit 22 may have 2 values, though value 1 is recommended
      // TODO: bit 26 may have 2 values, though the second one is avail only on Epiphany-IV
      BuildMI(*MBB, insertPos, DL, TII->get(Epiphany::MOVTi32ri), IaluConfigReg).addReg(OriginalConfigReg, RegState::Kill).addImm(0x48);
      // Store ialu config reg to stack
      ialuFrameIdx = MFI.CreateStackObject(RC->getSize(), /* Alignment = */ RC->getSize(), /* isSS = */ false);
      TII->storeRegToStackSlot(*MBB, insertPos, IaluConfigReg, /* killReg = */ false, ialuFrameIdx, RC, ST.getRegisterInfo());
    }
    // If we only have one mode - push reg back to config right now
    if (hasFPU && !hasIALU2) {
      BuildMI(*MBB, insertPos, DL, TII->get(Epiphany::MOVTS32_core), Epiphany::CONFIG).addReg(FpuConfigReg, RegState::Kill);
    } else if (hasIALU2 && !hasFPU) {
      BuildMI(*MBB, insertPos, DL, TII->get(Epiphany::MOVTS32_core), Epiphany::CONFIG).addReg(IaluConfigReg, RegState::Kill);
    }
    // Restore interrupts
    BuildMI(*MBB, insertPos, DL, TII->get(Epiphany::GIE)).addReg(Epiphany::CONFIG, RegState::ImplicitKill);
  }

  // Step 3 - if we have both FPU and IALU2 instructions, run through the whole routine and insert config
  // FIXME: config based on on successors and first use
  if (hasFPU && hasIALU2) {
    bool lastFPU = false;
    for(MachineFunction::iterator it = MF.begin(), E = MF.end(); it != E; ++it) {
      MachineBasicBlock *MBB = &*it;
      // Loop over all instructions search for FPU instructions
      for(MachineBasicBlock::iterator MBBI = MBB->begin(), MBBE = MBB->end(); MBBI != MBBE; ++MBBI) {
        MachineInstr *MI = &*MBBI;
        // Check if the opcode used is one of the FPU opcodes
        bool isFPU = std::find(std::begin(opcodesFPU), std::end(opcodesFPU), MI->getOpcode()) != std::end(opcodesFPU);
        if (isFPU && !lastFPU) {
          insertConfigInst(MBB, MBBI, MRI, ST, fpuFrameIdx);
          lastFPU = true;
          continue;
        }
        // Check if the opcode used is one of the IALU2 opcodes
        bool isIALU2 = std::find(std::begin(opcodesIALU2), std::end(opcodesIALU2), MI->getOpcode()) != std::end(opcodesIALU2);
        if (isIALU2 && lastFPU) {
          insertConfigInst(MBB, MBBI, MRI, ST, ialuFrameIdx);
          lastFPU = false;
          continue;
        }
      }
    }
  }

  // Step 4 - find the last FPU/IALU2 instruction of the last block and restore the config flags
  if (hasFPU || hasIALU2) {
    MachineBasicBlock *MBB = &MF.back();
    MachineBasicBlock::iterator MBBI = MBB->end();
    MBBI--;
    for (MachineBasicBlock::iterator MBBE = MBB->begin(); MBBI != MBBE; --MBBI) {
      MachineInstr *MI = &*MBBI;
      if (MI->isTerminator()) {
        break;
      }
      // Check if the opcode used is one of the FPU opcodesFPU
      //bool isFPU = std::find(std::begin(opcodesFPU), std::end(opcodesFPU), MI->getOpcode()) != std::end(opcodesFPU);
      // If FPU inst found - insert config flag
      //if (isFPU) {
      //break;
      //}
    }
    DebugLoc DL = DebugLoc();
    unsigned OriginalConfigReg = MRI.createVirtualRegister(RC);
    // Reload old config value 
    TII->loadRegFromStackSlot(*MBB, MBBI, OriginalConfigReg, originalFrameIdx, RC, ST.getRegisterInfo());
    // Disable interrupts
    BuildMI(*MBB, MBBI, DL, TII->get(Epiphany::GID));
    // Upload config value to the core
    BuildMI(*MBB, MBBI, DL, TII->get(Epiphany::MOVTS32_core), Epiphany::CONFIG).addReg(OriginalConfigReg, RegState::Kill);
    // Restore interrupts
    BuildMI(*MBB, MBBI, DL, TII->get(Epiphany::GIE)).addReg(Epiphany::CONFIG, RegState::ImplicitKill);
  }

  return true;
}

//===----------------------------------------------------------------------===//
//                         Public Constructor Functions
//===----------------------------------------------------------------------===//
FunctionPass *llvm::createEpiphanyFpuConfigPass() {
  return new EpiphanyFpuConfigPass();
}


