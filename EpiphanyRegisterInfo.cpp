//===- EpiphanyRegisterInfo.cpp - Epiphany Register Information -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Epiphany implementation of the TargetRegisterInfo
// class.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyRegisterInfo.h"

#include "MCTargetDesc/EpiphanyAddressingModes.h"
#include "Epiphany.h"
#include "EpiphanySubtarget.h"
#include "EpiphanyMachineFunction.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "epiphany-reg-info"

#define GET_REGINFO_TARGET_DESC
#include "EpiphanyGenRegisterInfo.inc"

EpiphanyRegisterInfo::EpiphanyRegisterInfo(const EpiphanySubtarget &ST)
	: EpiphanyGenRegisterInfo(Epiphany::LR), Subtarget(ST) {}

	//===----------------------------------------------------------------------===//
	// Callee Saved Registers methods
	//===----------------------------------------------------------------------===//
	/// Epiphany Callee Saved Registers
	// In EpiphanyCallConv.td,
	// def CSR32 : CalleeSavedRegs<(add V1, V2, V3, V4, V5, SB, SL, FP, LR, R15)>;
	// llc create CSR32_SaveList and CSR32_RegMask from above defined.
	const MCPhysReg *
	EpiphanyRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
		return CSR32_SaveList;
	}

const uint32_t*
EpiphanyRegisterInfo::getCallPreservedMask(const MachineFunction &MF,
		CallingConv::ID) const {
	return CSR32_RegMask;
}

// pure virtual method
BitVector EpiphanyRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
	BitVector Reserved(getNumRegs());
	// Stack base, limit and pointer
	Reserved.set(Epiphany::SB);
	Reserved.set(Epiphany::SL);
	Reserved.set(Epiphany::SP);
	// Frame pointer
	Reserved.set(Epiphany::FP);
	// Link register
	Reserved.set(Epiphany::LR);
	// Constants
	Reserved.set(Epiphany::R28);
	Reserved.set(Epiphany::R29);
	Reserved.set(Epiphany::R30);
	Reserved.set(Epiphany::ZERO);

	Reserved.set(Epiphany::STATUS);

	return Reserved;
}


//- If no eliminateFrameIndex(), it will hang on run. 
// pure virtual method
// FrameIndex represent objects inside a abstract stack.
// We must replace FrameIndex with an stack/frame pointer
// direct reference.
void EpiphanyRegisterInfo::
eliminateFrameIndex(MachineBasicBlock::iterator MBBI, int SPAdj,
		unsigned FIOperandNum, RegScavenger *RS) const {
	MachineInstr &MI = *MBBI;
	MachineFunction &MF = *MI.getParent()->getParent();
	MachineFrameInfo *MFI = MF.getFrameInfo();
	EpiphanyMachineFunctionInfo *FI = MF.getInfo<EpiphanyMachineFunctionInfo>();

	unsigned i = 0;
	while (!MI.getOperand(i).isFI()) {
		++i;
		assert(i < MI.getNumOperands() &&
				"Instr doesn't have FrameIndex operand!");
	}

	DEBUG(errs() << "\nFunction : " << MF.getFunction()->getName() << "\n";
			errs() << "<--------->\n" << MI);

	int FrameIndex = MI.getOperand(i).getIndex();
	uint64_t stackSize = MF.getFrameInfo()->getStackSize();
	int64_t spOffset = MF.getFrameInfo()->getObjectOffset(FrameIndex);

	DEBUG(errs() << "FrameIndex : " << FrameIndex << "\n"
			<< "spOffset   : " << spOffset << "\n"
			<< "stackSize  : " << stackSize << "\n");

	const std::vector<CalleeSavedInfo> &CSI = MFI->getCalleeSavedInfo();
	int MinCSFI = 0;
	int MaxCSFI = -1;

	if (CSI.size()) {
		MinCSFI = CSI[0].getFrameIdx();
		MaxCSFI = CSI[CSI.size() - 1].getFrameIdx();
	}

	// The following stack frame objects are always referenced relative to $sp:
	//  1. Outgoing arguments.
	//  2. Pointer to dynamically allocated stack space.
	//  3. Locations for callee-saved registers.
	// Everything else is referenced relative to whatever register
	// getFrameRegister() returns.
	unsigned FrameReg = getFrameRegister(MF);
  if (FrameIndex >= 0) {
    if (hasBasePointer(MF)) {
      FrameReg = getBaseRegister();
    } else if (needsStackRealignment(MF)) {
	    FrameReg = Epiphany::SP;
    }
  }

	// Calculate final offset.
	// - There is no need to change the offset if the frame object is one of the
	//   following: an outgoing argument, pointer to a dynamically allocated
	//   stack space or a $gp restore location,
	// - If the frame object is any of the following, its offset must be adjusted
	//   by adding the size of the stack:
	//   incoming argument, callee-saved register location or local variable.
	int64_t Offset;
	Offset = spOffset + (int64_t)stackSize;
	Offset += MI.getOperand(i+1).getImm();
	DEBUG(errs() << "Offset     : " << Offset << "\n" << "<--------->\n");

	// If MI is not a debug value, make sure Offset fits in the 16-bit immediate
	// field.
	if (!MI.isDebugValue() && !isInt<16>(Offset)) {
		assert("(!MI.isDebugValue() && !isInt<16>(Offset))");
	}

	MI.getOperand(i).ChangeToRegister(FrameReg, false);
	MI.getOperand(i+1).ChangeToImmediate(Offset);
}

bool
EpiphanyRegisterInfo::requiresRegisterScavenging(const MachineFunction &MF) const {
	return true;
}

bool
EpiphanyRegisterInfo::trackLivenessAfterRegAlloc(const MachineFunction &MF) const {
	return true;
}

bool EpiphanyRegisterInfo::hasBasePointer(const MachineFunction &MF) const {
  const MachineFrameInfo *MFI = MF.getFrameInfo();
  // When we need stack realignment and there are dynamic allocas, we can't
  // reference off of the stack pointer, so we reserve a base pointer.
  if (needsStackRealignment(MF) && MFI->hasVarSizedObjects()) {
    return true;
  }
  return false;
}

// Returns stack base register
unsigned EpiphanyRegisterInfo::getBaseRegister() const { 
  return Epiphany::SB; 
}


// Returns current frame register: FP or SP depending if FramePointer is set
unsigned EpiphanyRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
	const TargetFrameLowering *TFI = MF.getSubtarget().getFrameLowering();
	return TFI->hasFP(MF) ? (Epiphany::FP) : (Epiphany::SP);
}

const TargetRegisterClass *
EpiphanyRegisterInfo::GPR32(unsigned Size) const {
	return &Epiphany::GPR32RegClass;
}

const TargetRegisterClass *
EpiphanyRegisterInfo::GPR16(unsigned Size) const {
	return &Epiphany::GPR16RegClass;
}

