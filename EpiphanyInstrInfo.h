//===- EpiphanyInstrInfo.h - Epiphany Instruction Information -----*- C++ -*-===//
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

#ifndef LLVM_TARGET_EPIPHANYINSTRINFO_H
#define LLVM_TARGET_EPIPHANYINSTRINFO_H

#include "EpiphanyConfig.h"

#include "Epiphany.h"
#include "EpiphanyRegisterInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Target/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "EpiphanyGenInstrInfo.inc"

namespace llvm {

  class EpiphanyInstrInfo : public EpiphanyGenInstrInfo {
    virtual void anchor();
    protected:
    const EpiphanySubtarget &Subtarget;
    const EpiphanyRegisterInfo RI;
    public:
    explicit EpiphanyInstrInfo(const EpiphanySubtarget &STI);

    static const EpiphanyInstrInfo *create(EpiphanySubtarget &STI);

    /// getRegisterInfo - TargetInstrInfo is a superset of MRegister info.  As
    /// such, whenever a client has an instance of instruction info, it should
    /// always be able to get register info as well (through this method).
    ///
    const EpiphanyRegisterInfo &getRegisterInfo() const;

    /// Return the number of bytes of code the specified instruction may be.
    unsigned GetInstSizeInBytes(const MachineInstr &MI) const;

    bool expandPostRAPseudo(MachineInstr &MI) const override;

    void adjustStackPtr(unsigned SP, int64_t Amount, MachineBasicBlock &MBB,
        MachineBasicBlock::iterator I) const;

    // Is this a candidate for ld/st merging or pairing?  For example, we don't
    // touch volatiles or load/stores that have a hint to avoid pair formation.
    bool isCandidateToMergeOrPair(MachineInstr &MI) const;

    /// isLoadFromStackSlot - If the specified machine instruction is a direct
    /// load from a stack slot, return the virtual or physical register number of
    /// the destination along with the FrameIndex of the loaded stack slot.  If
    /// not, return 0.  This predicate must return 0 if the instruction has
    /// any side effects other than loading from the stack slot.
    unsigned isLoadFromStackSlot(const MachineInstr &MI,
        int &FrameIndex) const override;

    /// isStoreToStackSlot - If the specified machine instruction is a direct
    /// store to a stack slot, return the virtual or physical register number of
    /// the source reg along with the FrameIndex of the loaded stack slot.  If
    /// not, return 0.  This predicate must return 0 if the instruction has
    /// any side effects other than storing to the stack slot.
    unsigned isStoreToStackSlot(const MachineInstr &MI,
        int &FrameIndex) const override;

    /// This is used by the pre-regalloc scheduler to determine if two loads are
    ///  loading from the same base address. It should only return true if the base
    /// pointers are the same and the only differences between the two addresses
    /// are the offset. It also returns the offsets by reference.
    bool areLoadsFromSameBasePtr(SDNode *Load1, SDNode *Load2,
        int64_t &Offset1, int64_t &Offset2) const override;

    void storeRegToStackSlot(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
        unsigned SrcReg, bool KillSrc, int FrameIdx, const TargetRegisterClass *Rd,
        const TargetRegisterInfo *TRI) const override;

    void loadRegFromStackSlot(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
        unsigned DestReg, int FrameIdx, const TargetRegisterClass *Rd,
        const TargetRegisterInfo *TRI) const override;

    void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
        const DebugLoc &DL, unsigned DestReg, unsigned SrcReg,
        bool KillSrc) const override;

    //==---
    // Branch analysis.
    //==---
    bool isUnpredicatedTerminator(const MachineInstr &MI) const override;
    bool analyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
        MachineBasicBlock *&FBB, SmallVectorImpl<MachineOperand> &Cond,
        bool AllowModify) const override;

    /// Remove the branching code at the end of the specific MBB.
    /// This is only invoked in cases where AnalyzeBranch returns success. It
    /// returns the number of instructions that were removed.
    unsigned removeBranch(MachineBasicBlock &MBB, int *BytesRemoved = nullptr) const override;

    /// Insert branch code into the end of the specified MachineBasicBlock.
    /// The operands to this method are the same as those
    /// returned by AnalyzeBranch.  This is only invoked in cases where
    /// AnalyzeBranch returns success. It returns the number of instructions
    /// inserted.
    ///
    /// It is also invoked by tail merging to add unconditional branches in
    /// cases where AnalyzeBranch doesn't apply because there was no original
    /// branch to analyze.  At least this much must be implemented, else tail
    /// merging needs to be disabled.
    unsigned insertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
        MachineBasicBlock *FBB, ArrayRef<MachineOperand> Cond,
        const DebugLoc &DL, int *BytesAdded = nullptr) const override;
    bool reverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const override;

    // Misc
    void insertNoop(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI) const override;
    /// Test if the given instruction should be considered a scheduling boundary.
    /// This primarily includes labels and terminators.
    bool isSchedulingBoundary(const MachineInstr &MI,
        const MachineBasicBlock *MBB, const MachineFunction &MF) const override;

    private:
    void expandRTS(MachineBasicBlock &MBB, MachineBasicBlock::iterator I) const;

  };

} // End of namespace llvm
#endif
