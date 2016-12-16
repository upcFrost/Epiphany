
//===- EpiphanyFrameLowering.cpp - Epiphany Frame Information ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Epiphany implementation of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyFrameLowering.h"

#include "EpiphanyMachineFunction.h"
#include "EpiphanyInstrInfo.h"
#include "EpiphanySubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Target/TargetOptions.h"

using namespace llvm;

#define DEBUG_TYPE "frame-info"

//@emitPrologue {
void EpiphanyFrameLowering::emitPrologue(MachineFunction &MF,
		MachineBasicBlock &MBB) const {
	assert(&MF.front() == &MBB && "Shrink-wrapping not yet supported");
	MachineFrameInfo *MFI = MF.getFrameInfo();
	EpiphanyMachineFunctionInfo *FI = MF.getInfo<EpiphanyMachineFunctionInfo>();

	const EpiphanyInstrInfo &TII =
		*static_cast<const EpiphanyInstrInfo*>(STI.getInstrInfo());
	const EpiphanyRegisterInfo &RegInfo =
		*static_cast<const EpiphanyRegisterInfo *>(STI.getRegisterInfo());

	MachineBasicBlock::iterator MBBI = MBB.begin();
	DebugLoc dl = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc();
	EpiphanyABIInfo ABI = STI.getABI();
	unsigned SP = Epiphany::SP;
	const TargetRegisterClass *RC = &Epiphany::GPR32RegClass;

	// First, compute final stack size.
	uint64_t StackSize = MFI->getStackSize();

	// No need to allocate space on the stack.
	if (StackSize == 0 && !MFI->adjustsStack()) return;

	MachineModuleInfo &MMI = MF.getMMI();
	const MCRegisterInfo *MRI = MMI.getContext().getRegisterInfo();
	MachineLocation DstML, SrcML;

	// Adjust stack.
	TII.adjustStackPtr(SP, -StackSize, MBB, MBBI);

	// emit ".cfi_def_cfa_offset StackSize"
	unsigned CFIIndex = MMI.addFrameInst(
			MCCFIInstruction::createDefCfaOffset(nullptr, -StackSize));
	BuildMI(MBB, MBBI, dl, TII.get(TargetOpcode::CFI_INSTRUCTION))
		.addCFIIndex(CFIIndex);

	const std::vector<CalleeSavedInfo> &CSI = MFI->getCalleeSavedInfo();

	if (CSI.size()) {
		// Find the instruction past the last instruction that saves a callee-saved
		// register to the stack.
		for (unsigned i = 0; i < CSI.size(); ++i)
			++MBBI;

		// Iterate over list of callee-saved registers and emit .cfi_offset
		// directives.
		for (std::vector<CalleeSavedInfo>::const_iterator I = CSI.begin(),
				E = CSI.end(); I != E; ++I) {
			int64_t Offset = MFI->getObjectOffset(I->getFrameIdx());
			unsigned Reg = I->getReg();
			{
				// Reg is in CPURegs.
				unsigned CFIIndex = MMI.addFrameInst(MCCFIInstruction::createOffset(
							nullptr, MRI->getDwarfRegNum(Reg, 1), Offset));
				BuildMI(MBB, MBBI, dl, TII.get(TargetOpcode::CFI_INSTRUCTION))
					.addCFIIndex(CFIIndex);
			}
		}
	}
}
//}

//@emitEpilogue {
void EpiphanyFrameLowering::emitEpilogue(MachineFunction &MF,
		MachineBasicBlock &MBB) const {
	MachineBasicBlock::iterator MBBI = MBB.getLastNonDebugInstr();
	MachineFrameInfo *MFI = MF.getFrameInfo();
	EpiphanyMachineFunctionInfo *FI = MF.getInfo<EpiphanyMachineFunctionInfo>();

	const EpiphanyInstrInfo &TII =
		*static_cast<const EpiphanyInstrInfo *>(STI.getInstrInfo());
	const EpiphanyRegisterInfo &RegInfo =
		*static_cast<const EpiphanyRegisterInfo *>(STI.getRegisterInfo());

	DebugLoc dl = MBBI->getDebugLoc();
	EpiphanyABIInfo ABI = STI.getABI();
	unsigned SP = Epiphany::SP;

	// Get the number of bytes from FrameInfo
	uint64_t StackSize = MFI->getStackSize();

	if (!StackSize)
		return;

	// Adjust stack.
	TII.adjustStackPtr(SP, StackSize, MBB, MBBI);
}
//}

static void setAliasRegs(MachineFunction &MF, BitVector &SavedRegs, unsigned Reg) {
  const TargetRegisterInfo *TRI = MF.getSubtarget().getRegisterInfo();
  for (MCRegAliasIterator AI(Reg, TRI, true); AI.isValid(); ++AI)
    SavedRegs.set(*AI);
}

// This method is called immediately before PrologEpilogInserter scans the 
//  physical registers used to determine what callee saved registers should be 
//  spilled. This method is optional. 
void EpiphanyFrameLowering::determineCalleeSaves(MachineFunction &MF,
                                               BitVector &SavedRegs,
                                               RegScavenger *RS) const {
//@determineCalleeSaves-body
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);
  EpiphanyMachineFunctionInfo *FI = MF.getInfo<EpiphanyMachineFunctionInfo>();
  MachineRegisterInfo& MRI = MF.getRegInfo();

  if (MF.getFrameInfo()->hasCalls())
    setAliasRegs(MF, SavedRegs, Epiphany::LR);

  return;
}

bool
EpiphanyFrameLowering::hasReservedCallFrame(const MachineFunction &MF) const {
  const MachineFrameInfo *MFI = MF.getFrameInfo();

  // Reserve call frame if the size of the maximum call frame fits into 16-bit
  // immediate field and there are no variable sized objects on the stack.
  // Make sure the second register scavenger spill slot can be accessed with one
  // instruction.
  return isInt<16>(MFI->getMaxCallFrameSize() + getStackAlignment()) &&
    !MFI->hasVarSizedObjects();
}

// hasFP - Return true if the specified function should have a dedicated frame
// pointer register.  This is true if the function has variable sized allocas,
// if it needs dynamic stack realignment, if frame pointer elimination is
// disabled, or if the frame address is taken.
bool EpiphanyFrameLowering::hasFP(const MachineFunction &MF) const {
	const MachineFrameInfo *MFI = MF.getFrameInfo();
	const TargetRegisterInfo *TRI = STI.getRegisterInfo();

	return (MF.getTarget().Options.DisableFramePointerElim(MF) || 
			TRI->needsStackRealignment(MF) ||
			MFI->hasVarSizedObjects() ||
			MFI->isFrameAddressTaken());
}
