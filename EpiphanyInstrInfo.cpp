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
		if (I->getOpcode() == Epiphany::BL32)
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

void EpiphanyInstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
		MachineBasicBlock::iterator MI, unsigned SrcReg, bool KillSrc, int FrameIdx,
		const TargetRegisterClass *Rd, const TargetRegisterInfo *TRI) const {
	DebugLoc DL;
	// Get instruction, only STRi32 for now
	unsigned STRi32_r16 = Epiphany::STRi32_r16;
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
	if (Rd == &Epiphany::GPR16RegClass) {
		BuildMI(MBB, MI, DL, get(STRi32_r16)).addReg(SrcReg, getKillRegState(KillSrc))
			.addFrameIndex(FrameIdx).addImm(0).addMemOperand(MMO);
	} else if (Rd == &Epiphany::GPR32RegClass) {
		BuildMI(MBB, MI, DL, get(STRi32_r32)).addReg(SrcReg, getKillRegState(KillSrc))
			.addFrameIndex(FrameIdx).addImm(0).addMemOperand(MMO);
	} else {
		llvm_unreachable("Cannot store this register to stack slot!");
	}
}

void EpiphanyInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
		MachineBasicBlock::iterator MI, unsigned DestReg, int FrameIdx,
		const TargetRegisterClass *Rd, const TargetRegisterInfo *TRI) const {
	DebugLoc DL;
	// Get instruction, only LDRi32 for now
	unsigned LDRi32_r16 = Epiphany::LDRi32_r16;
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
	if (Rd == &Epiphany::GPR16RegClass) {
		BuildMI(MBB, MI, DL, get(LDRi32_r16), DestReg).addFrameIndex(FrameIdx).addImm(0).addMemOperand(MMO);
	} else if (Rd == &Epiphany::GPR32RegClass) {
		BuildMI(MBB, MI, DL, get(LDRi32_r32), DestReg).addFrameIndex(FrameIdx).addImm(0).addMemOperand(MMO);
	} else {
		llvm_unreachable("Cannot store this register to stack slot!");
	}
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
	unsigned A1 = Epiphany::A1;
	unsigned ADD = Epiphany::ADD32ri;
	unsigned IADD = Epiphany::ADDrr_i32_r32;
	unsigned MOVi32ri = Epiphany::MOVi32ri;
	unsigned MOVTi32ri = Epiphany::MOVTi32ri;

	if (isInt<11>(Amount)) {
		// add sp, sp, amount
		BuildMI(MBB, I, DL, get(ADD), SP).addReg(SP).addImm(Amount);
	} else { // Expand immediate that doesn't fit in 11-bit.
		// Set lower 16 bits
		BuildMI(MBB, I, DL, get(MOVi32ri), A1).addImm(Amount & 0xffff);
		// Set upper 16 bits
		BuildMI(MBB, I, DL, get(MOVTi32ri), A1).addReg(A1).addImm(Amount >> 16);
		// iadd sp, sp, amount
		BuildMI(MBB, I, DL, get(IADD), SP).addReg(SP).addReg(A1, RegState::Kill);
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
