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
// The logic is to loop over all each basic block searching for the first 
//  FPU inst. If such instruction is found - add corresponding config flag
//  in both beginning and end of the MBB.

#include "EpiphanyFpuConfigPass.h"

using namespace llvm;

#define DEBUG_TYPE "epiphany_fpu_config"

char EpiphanyFpuConfigPass::ID = 0;

bool EpiphanyFpuConfigPass::runOnMachineFunction(MachineFunction &MF) {
  DEBUG(dbgs() << "\nRunning Epiphany FPU config pass\n");
  auto &ST = MF.getSubtarget<EpiphanySubtarget>();
  const EpiphanyInstrInfo *TII = ST.getInstrInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();

  // FPU opcodes
  unsigned opcodes[] = {Epiphany::FADDrr_r16, Epiphany::FADDrr_r32, Epiphany::FSUBrr_r16, Epiphany::FSUBrr_r32,
    Epiphany::FMULrr_r16, Epiphany::FMULrr_r32, Epiphany::FMADDrr_r16, Epiphany::FMADDrr_r32,
    Epiphany::FMSUBrr_r16, Epiphany::FMSUBrr_r32};
  std::vector<MachineInstr*> setConfig;
  std::vector<MachineInstr*> resetConfig;

  // Loop over all of the basic blocks.
  for(MachineFunction::iterator it = MF.begin(); it != MF.end(); ++it) {
    MachineBasicBlock *MBB = &*it;
    bool hasFP = false;


    // Loop over all instructions.
    for(MachineBasicBlock::iterator MBBI = MBB->begin(), E = MBB->end(); MBBI != E; ++MBBI){
      MachineInstr *MI = &*MBBI;
      // Check if the opcode used is one of the FPU opcodes
      bool isFPU = std::find(std::begin(opcodes), std::end(opcodes), MI->getOpcode()) != std::end(opcodes);
      // If FPU inst found - insert config flag set
      if (isFPU) {
        DEBUG(dbgs() << "Found first FPU instruction, adding config before:\n");
        DEBUG(MI->dump());
        DebugLoc DL = MI->getDebugLoc();
        // Reg to store config
        unsigned tmpReg = MRI.createVirtualRegister(&Epiphany::GPR32RegClass);
        unsigned tmpReg2 = MRI.createVirtualRegister(&Epiphany::GPR32RegClass);
        // Add config as live-in
        MBB->addLiveIn(Epiphany::CONFIG);
        // Disable interrupts
        setConfig.push_back(BuildMI(*MBB, MBBI, DL, TII->get(Epiphany::GID)));
        // Save current config
        setConfig.push_back(BuildMI(*MBB, MBBI, DL, TII->get(Epiphany::MOVFS32rr), tmpReg).addReg(Epiphany::CONFIG));
        // Create mask with bits 19:17 set to 0
        setConfig.push_back(BuildMI(*MBB, MBBI, DL, TII->get(Epiphany::MOVi32ri), tmpReg2).addImm(0xffff));
        setConfig.push_back(BuildMI(*MBB, MBBI, DL, TII->get(Epiphany::MOVTi32ri), tmpReg2).addReg(tmpReg2).addImm(0xfff1));
        // Apply mask to tmpReg
        setConfig.push_back(BuildMI(*MBB, MBBI, DL, TII->get(Epiphany::ANDrr_r32), tmpReg).addReg(tmpReg2));
        // Push reg back to config
        setConfig.push_back(BuildMI(*MBB, MBBI, DL, TII->get(Epiphany::MOVTS32rr), Epiphany::CONFIG).addReg(tmpReg));
        // Restore interrupts
        setConfig.push_back(BuildMI(*MBB, MBBI, DL, TII->get(Epiphany::GIE)));
        // Break out of the loop
        break;
      }
    }
  }
  return true;
}

//===----------------------------------------------------------------------===//
//                         Public Constructor Functions
//===----------------------------------------------------------------------===//
FunctionPass *llvm::createEpiphanyFpuConfigPass() {
  return new EpiphanyFpuConfigPass();
}


