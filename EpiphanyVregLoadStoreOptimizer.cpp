//=- EpiphanyVregLoadStoreOptimizer.cpp - Epiphany load/store opt. pass -*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// 
/// \file
/// This file contains a pass that performs load / store related peephole
/// optimizations. This pass should be run after register allocation.
/// 
/// Flow:
/// * Split MachineFunction (MF) into MachineBasicBlocks (MBB)
/// * For each MBB look through instructions trying to find the next pairable one (see isPairableLoadStoreInst)
/// * If found pairable instruction, check if it has any flags preventing pairing
/// * If no such flags found, try to find matching paired instruction
///   * Take a couple of next instructions, find instruction with the same opcode, and run a couple of checks
///   * Check alignment, reg base, check if reg is not modified
///   * For real regs, try to find super-reg
///   * For real regs, check order
///   * For reg-based (not frame-based) offsets check base alignment (frame SHOULD be 8-byte aligned)
/// * If all green, try to pair regs
///   * For virtual regs, create reg sequence. If frame-based - merge based on stack growth direction and move
///     frame object into fixed local stack area
///   * For virtual regs, just swap with the super-reg
///
//===----------------------------------------------------------------------===//

#include "EpiphanyVregLoadStoreOptimizer.h"

using namespace llvm;

#define DEBUG_TYPE "epiphany_vreg_ls_opt"

STATISTIC(NumPairCreated, "Number of load/store pair instructions generated");

// The LdStLimit limits how far we search for load/store pairs.
static cl::opt<unsigned> LdStLimit("epiphany-vreg-load-store-scan-limit", cl::init(20), cl::Hidden);


char EpiphanyVregLoadStoreOptimizer::ID = 0;

/// \brief Returns true if this instruction should be considered for pairing
///
/// \param MI Machine instruction to check
///
/// \return true if this instruction should be considered for pairing
static bool isPairableLoadStoreInst(MachineInstr &MI) {
  unsigned inst[] = {
      Epiphany::STRi32_r16,
      Epiphany::STRi32_r32,
      Epiphany::STRf32,
      Epiphany::LDRi32_r16,
      Epiphany::LDRi32_r32,
      Epiphany::LDRf32
  };
  unsigned Opc = MI.getOpcode();
  return std::find(std::begin(inst), std::end(inst), Opc) != std::end(inst);
}

static unsigned int getMemScale(unsigned Opc) {
  switch (Opc) {
    default:
      llvm_unreachable("Opcode has unknown scale!");
    case Epiphany::STRi8_r16:
    case Epiphany::STRi8_r32:
    case Epiphany::LDRi8_r16:
    case Epiphany::LDRi8_r32:
      return 1;
    case Epiphany::STRi16_r16:
    case Epiphany::STRi16_r32:
    case Epiphany::LDRi16_r16:
    case Epiphany::LDRi16_r32:
      return 2;
    case Epiphany::STRi32_r16:
    case Epiphany::STRi32_r32:
    case Epiphany::LDRi32_r16:
    case Epiphany::LDRi32_r32:
    case Epiphany::STRf32:
    case Epiphany::LDRf32:
      return 4;
    case Epiphany::STRi64:
    case Epiphany::LDRi64:
    case Epiphany::STRf64:
    case Epiphany::LDRf64:
      return 8;
  }
}

static unsigned int getMemScale(MachineInstr &MI) {
  return getMemScale(MI.getOpcode());
}

/// Returns correct instruction alignment. For Epiphany it is equal to memory scale
static unsigned int getAlignment(MachineInstr &MI) {
  return getMemScale(MI);
}

static unsigned int getAlignment(unsigned Opc) {
  return getMemScale(Opc);
}

/// Return paired opcode for the provided one, e.g. STRi64_r32 for STRi32_r32
static unsigned getMatchingPairOpcode(unsigned Opc) {
  switch (Opc) {
    default:
      llvm_unreachable("Opcode has no pairwise equivalent");
      break;
    case Epiphany::STRi8_r16:
      return Epiphany::STRi16_r16;
    case Epiphany::STRi8_r32:
      return Epiphany::STRi16_r32;
    case Epiphany::STRi16_r16:
      return Epiphany::STRi32_r16;
    case Epiphany::STRi16_r32:
      return Epiphany::STRi32_r32;
    case Epiphany::STRi32_r16:
    case Epiphany::STRi32_r32:
      return Epiphany::STRi64;
    case Epiphany::LDRi8_r16:
      return Epiphany::LDRi16_r16;
    case Epiphany::LDRi8_r32:
      return Epiphany::LDRi16_r32;
    case Epiphany::LDRi16_r16:
      return Epiphany::LDRi32_r16;
    case Epiphany::LDRi16_r32:
      return Epiphany::LDRi32_r32;
    case Epiphany::LDRi32_r16:
    case Epiphany::LDRi32_r32:
      return Epiphany::LDRi64;
    case Epiphany::STRf32:
      return Epiphany::STRf64;
    case Epiphany::LDRf32:
      return Epiphany::LDRf64;
  }
}

/// Convert the byte-offset used by unscaled into an "element" offset used
/// by the scaled pair load/store instructions.
static bool inBoundsForPair(int64_t Offset) {
  // Well, in fact if the op is in bounds for any kind of store/load - it will be in bound for pairing
  return true;
}

/// Get register for the store/load machine operand
static const MachineOperand &getRegOperand(const MachineInstr &MI) {
  return MI.getOperand(0);
}

/// Get base for the store/load machine operand
static const MachineOperand &getBaseOperand(const MachineInstr &MI) {
  return MI.getOperand(1);
}

/// Get base for the store/load machine operand
static MachineOperand &getBaseOperand(MachineInstr &MI) {
  return MI.getOperand(1);
}

/// Get offset for the store/load machine operand
static const MachineOperand &getOffsetOperand(const MachineInstr &MI) {
  return MI.getOperand(2);
}

/// Get offset for the store/load machine operand
static MachineOperand &getOffsetOperand(MachineInstr &MI) {
  return MI.getOperand(2);
}

/// Returns true if we need to use offset, false if frame index should be used
static bool baseIsFrameIndex(const MachineInstr &FirstMI, const MachineInstr &SecondMI) {
  return getBaseOperand(FirstMI).isFI() && getBaseOperand(SecondMI).isFI();
}

/// Returns true if FirstMI and MI are candidates for merging or pairing.
/// Otherwise, returns false.
static bool areCandidatesToMergeOrPair(MachineInstr &FirstMI, MachineInstr &SecondMI,
                                       LoadStoreFlags &Flags, const EpiphanyInstrInfo *TII,
                                       const MachineFrameInfo *MFI) {
  // If this is volatile not a candidate.
  if (SecondMI.hasOrderedMemoryRef())
    return false;

  // We should have already checked FirstMI for pair suppression and volatility.
  assert(!FirstMI.hasOrderedMemoryRef() &&
         "FirstMI shouldn't get here if either of these checks are true.");

  unsigned OpcA = FirstMI.getOpcode();
  unsigned OpcB = SecondMI.getOpcode();

  // Opcodes match: nothing more to check.
  if (OpcA != OpcB) {
    return false;
  }

  // If using frame index - check object sizes, both should be equal to their mem scales
  Flags.setBasedOnVirtualFI(baseIsFrameIndex(FirstMI, SecondMI));
  if (Flags.isBasedOnVirtualFI()) {
    int FirstBase = getBaseOperand(FirstMI).getIndex();
    int SecondBase = getBaseOperand(SecondMI).getIndex();
    if (MFI->getObjectSize(FirstBase) != getMemScale(FirstMI)
        || MFI->getObjectSize(SecondBase) != getMemScale(SecondMI)) {
      DEBUG(dbgs() << "Object sizes not equal to their mem scales, skipping\n");
      return false;
    }
  }

  return true;
}

/// trackRegDefsUses - Remember what registers the specified instruction uses
/// and modifies.
static void trackRegDefsUses(const MachineInstr &MI, BitVector &ModifiedRegs,
                             BitVector &UsedRegs, const TargetRegisterInfo *TRI) {
  for (const MachineOperand &MO : MI.operands()) {
    if (MO.isRegMask())
      ModifiedRegs.setBitsNotInMask(MO.getRegMask());

    if (!MO.isReg())
      continue;
    unsigned Reg = MO.getReg();
    if (!Reg)
      continue;
    if (MO.isDef()) {
      // WZR/XZR are not modified even when used as a destination register.
      ModifiedRegs.set(TRI->isVirtualRegister(Reg) ? TRI->virtReg2Index(Reg) : Reg);
    } else {
      assert(MO.isUse() && "Reg operand not a def and not a use?!?");
      UsedRegs.set(TRI->isVirtualRegister(Reg) ? TRI->virtReg2Index(Reg) : Reg);
    }
  }
}

/// Keeps track on which frame indexes were used between two candidates to merge
static void trackFrameIdxs(const MachineInstr &MI, BitVector &ModifiedFrameIdxs,
                           BitVector &UsedFrameIdxs, const MachineFrameInfo *MFI) {
  for (const MachineOperand &MO : MI.operands()) {
    if (MO.isFI()) {
      if (MI.mayStore()) {
        ModifiedFrameIdxs.set(MO.getIndex());
      } else {
        UsedFrameIdxs.set(MO.getIndex());
      }
    }
  }
}

/// \brief Returns true if the alignment for specified regs and their offsets is good for pairing.
/// Only applicable when the frame is finalized
///
/// \param FirstMI First instruction to check
/// \param SecondMI Second instruction to check
/// \param UsingVirtualFI True if offset is used for alignment checks, false if frame index is used
///
/// \return true if alignment is ok for pairing
bool EpiphanyVregLoadStoreOptimizer::isAlignmentCorrect(MachineInstr &FirstMI, MachineInstr &SecondMI,
                                                    bool UsingVirtualFI) {
  // Resolve target reg class
  unsigned MainReg = getRegOperand(FirstMI).getReg();
  unsigned PairedReg = getRegOperand(SecondMI).getReg();
  int64_t MainOffset = UsingVirtualFI ? getBaseOperand(FirstMI).getIndex() : getOffsetOperand(FirstMI).getImm() ;
  int64_t PairedOffset = UsingVirtualFI ? getBaseOperand(SecondMI).getIndex() : getOffsetOperand(SecondMI).getImm();

  if (UsingVirtualFI) {
    // Check if both ops have correct alignment required
    if (MFI->getObjectAlignment(MainOffset) < getAlignment(FirstMI)
        || MFI->getObjectAlignment(PairedOffset) < getAlignment(SecondMI)) {
      return false;
    }
  } else {
    // Check that base alignment matches paired opcode alignment
    int PairedAlignment = getAlignment(getMatchingPairOpcode(FirstMI.getOpcode()));
    if (getBaseOperand(FirstMI).getReg() != Epiphany::FP) {
      // Only applicable when we are dealing with non-FP-based offset, as frame is 8-byte aligned
      MachineInstr::mmo_iterator FirstMMOI = FirstMI.memoperands_begin();
      MachineMemOperand FirstMO = **FirstMMOI;
      MachineInstr::mmo_iterator SecondMMOI = SecondMI.memoperands_begin();
      MachineMemOperand SecondMO = **SecondMMOI;
      if (FirstMO.getBaseAlignment() != PairedAlignment && SecondMO.getBaseAlignment() != PairedAlignment) {
        DEBUG(dbgs() << "Base alignment out, skipping\n");
        return false;
      }
    }

    // Check if at least one instruction is aligned to the paired opcode alignment
    if ((MainOffset % PairedAlignment != 0) && (PairedOffset % PairedAlignment) != 0) {
      DEBUG(dbgs() << "Offsets alignment out, skipping\n");
      return false;
    }
  }

  return true;
}

/// Checks if two load/store instructions have similar base, and their
/// offsets differ by some fixed stride
static bool isBaseAndOffsetCorrect(unsigned MainBase, unsigned PairBase, int64_t MainOffset,
                                   int64_t PairOffset, int OffsetStride) {
  return (MainBase == PairBase &&
          ((MainOffset == PairOffset + OffsetStride) || (MainOffset + OffsetStride == PairOffset)));
}

/// Cleans register kill flags before merge
///
/// Can have two cases based on \p MergeForward value:
/// If merging backward
/// \code
///   STRi32 %r0, ...
///   USE %r1
///   STRi32 kill %r1  ; need to clear kill flag when moving STRi32 upwards
/// \endcode
///
/// If merging forward
/// \code
///   STRi32 %r1, ...
///   USE kill %r1   ; need to clear kill flag when moving STRi32 downwards
///   STRi32 %r0
/// \endcode
void EpiphanyVregLoadStoreOptimizer::cleanKillFlags(MachineOperand RegOp0, MachineOperand RegOp1,
                                                MachineBasicBlock::iterator I, MachineBasicBlock::iterator Paired,
                                                bool MergeForward) {
  if (!MergeForward) {
    // Clear kill flags on store if moving backward
    RegOp0.setIsKill(false);
    RegOp1.setIsKill(false);
  } else {
    // Clear kill flags on store if moving forward
    unsigned Reg = getRegOperand(*I).getReg();
    for (MachineInstr &MI : make_range(std::next(I), Paired))
      MI.clearRegisterKills(Reg, TRI);
  }
}

/// \brief Merges two virtual reg-based 32-bit load/store instructions into a single 64-bit one.
///
/// \param PairedOp Wide store/load operation opcode
/// \param OffsetImm Offset to use
/// \param MainReg Reg operand from the first paired store/load
/// \param PairedReg Reg operand from the second paired store/load
/// \param I Iterator pointing at the first paired store/load
/// \param Paired Iterator pointing at the second paired store/load
/// \param Flags Store/load flags
///
/// \return Builder result
MachineInstrBuilder EpiphanyVregLoadStoreOptimizer::mergeFrameBasedInsns(unsigned PairedOp,
                                                                         MachineBasicBlock::iterator I,
                                                                         MachineBasicBlock::iterator Paired,
                                                                         const LoadStoreFlags &Flags) {
  MachineInstrBuilder MIB;
  bool MergeForward = Flags.getMergeForward();
  // Insert our new paired instruction after whichever of the paired
  // instructions MergeForward indicates.
  MachineBasicBlock::iterator InsertionPoint = MergeForward ? Paired : I;

  // Also based on MergeForward is from where we copy the base register operand
  // so we get the flags compatible with the input code.
  const MachineOperand &MainReg = getRegOperand(*I);
  const MachineOperand &PairedReg = getRegOperand(*Paired);
  const MachineOperand &MainBase = getBaseOperand(*I);
  const MachineOperand &PairedBase = getBaseOperand(*Paired);
  const int64_t &OffsetImm = getOffsetOperand(*I).getImm();

  // Resolve target reg class
  const TargetRegisterClass *RC = MRI->getRegClass(MainReg.getReg()) == &Epiphany::GPR32RegClass
                                  ? &Epiphany::GPR64RegClass
                                  : &Epiphany::FPR64RegClass;

  // Get insertion parameters
  unsigned parentReg = MRI->createVirtualRegister(RC);
  DebugLoc DL = I->getDebugLoc();
  MachineBasicBlock *MBB = I->getParent();

  // Insert reg sequence
  if (TII->get(PairedOp).mayStore()) {
    // In terms of store - create regsequence before storing
    const MCInstrDesc &RegSeq = TII->get(TargetOpcode::REG_SEQUENCE);
    MIB = BuildMI(*MBB, InsertionPoint, DL, RegSeq, parentReg)
        .addReg(MainReg.getReg())
        .addImm(Epiphany::isub_lo)
        .addReg(PairedReg.getReg())
        .addImm(Epiphany::isub_hi);
    DEBUG(dbgs() << "\t");
    DEBUG(((MachineInstr *) MIB)->print(dbgs()));
  }

  // Insert paired instruction
  unsigned flags = TII->get(PairedOp).mayLoad() ? RegState::Define : MainReg.getTargetFlags();
  MIB = BuildMI(*MBB, InsertionPoint, DL, TII->get(PairedOp))
      .addReg(parentReg, flags)
      .addOperand(MainBase)
      .addImm(OffsetImm)
      .setMemRefs(I->mergeMemRefsWith(*Paired));
  DEBUG(dbgs() << "\t");
  DEBUG(((MachineInstr *) MIB)->print(dbgs()));

  if (TII->get(PairedOp).mayLoad()) {
    // In terms of load - issue two copy instruction for vregs we had
    const MCInstrDesc &Copy = TII->get(TargetOpcode::COPY);
    MIB = BuildMI(*MBB, InsertionPoint, DL, Copy, MainReg.getReg())
        .addReg(parentReg, /* flags = */ 0, Epiphany::isub_lo);
    DEBUG(dbgs() << "\t");
    DEBUG(((MachineInstr *) MIB)->print(dbgs()));
    MIB = BuildMI(*MBB, InsertionPoint, DL, Copy, PairedReg.getReg())
        .addReg(parentReg, /* flags = */ 0, Epiphany::isub_hi);
    DEBUG(dbgs() << "\t");
    DEBUG(((MachineInstr *) MIB)->print(dbgs()));
  }

  // Erasing old instructions
  I->eraseFromParent();
  Paired->eraseFromParent();

  // Adjust alignment and update frame index and offset in all loads/stores related
  MFI->setObjectAlignment(MainBase.getIndex(), getAlignment(PairedOp));
  MFI->setObjectSize(MainBase.getIndex(), getMemScale(PairedOp));
  for (MachineBasicBlock &MBB : *MF) {
    for (MachineInstr &MI : MBB) {
      for (MachineOperand &MO : MI.operands()) {
        if (MO.isFI() && MO.getIndex() == PairedBase.getIndex()) {
          DEBUG(dbgs() << "Changing instruction\n\t";
                    MI.print(dbgs()));
          MO.setIndex(MainBase.getIndex());
          getOffsetOperand(MI).setImm(4);
          DEBUG(dbgs() << "To\n\t";
                    MI.print(dbgs()));
        }
      }
    }
  }
  MFI->RemoveStackObject(PairedBase.getIndex());

  return MIB;
}

void EpiphanyVregLoadStoreOptimizer::mergeRegBasedInsns(unsigned int PairedOp, MachineBasicBlock::iterator Paired,
                                                        MachineBasicBlock::iterator I,
                                                        const LoadStoreFlags &Flags,
                                                        int64_t OffsetImm,
                                                        MachineOperand RegOp0,
                                                        MachineOperand RegOp1) {
  MachineInstrBuilder MIB;
  bool MergeForward = Flags.getMergeForward();
  // Insert our new paired instruction after whichever of the paired
  // instructions MergeForward indicates.
  MachineBasicBlock::iterator InsertionPoint = MergeForward ? Paired : I;

  // Also based on MergeForward is from where we copy the base register operand
  // so we get the flags compatible with the input code.
  const MachineOperand &PairedBase = getBaseOperand(*Paired);
  const MachineOperand &MainBase = getBaseOperand(*I);
  // TODO: Hacky as hell, to be rewritten completely using fixed stack
  const MachineOperand &BaseRegOp = MergeForward ? PairedBase : MainBase;
  const MachineOperand &PairedBaseOp = &BaseRegOp == &MainBase ? PairedBase : MainBase;
  const std::pair<MachineInstr::mmo_iterator, int> MemRefsOrder = &BaseRegOp == &PairedBase
                                                                  ? I->mergeMemRefsWith(*Paired)
                                                                  : Paired->mergeMemRefsWith(*I);

  // Resolve target reg class
  const TargetRegisterClass *RC = MRI->getRegClass(RegOp0.getReg()) == &Epiphany::GPR32RegClass ?
                                  &Epiphany::GPR64RegClass : &Epiphany::FPR64RegClass;

  // Get insertion parameters
  unsigned parentReg = MRI->createVirtualRegister(RC);
  DebugLoc DL = I->getDebugLoc();
  MachineBasicBlock *MBB = I->getParent();

  // Insert reg sequence
  if (TII->get(PairedOp).mayStore()) {
    // In terms of store - create regsequence before storing
    const MCInstrDesc &RegSeq = TII->get(TargetOpcode::REG_SEQUENCE);
    MIB = BuildMI(*MBB, InsertionPoint, DL, RegSeq, parentReg)
        .addReg(RegOp0.getReg())
        .addImm(Epiphany::isub_lo)
        .addReg(RegOp1.getReg())
        .addImm(Epiphany::isub_hi);
    DEBUG(dbgs() << "\t");
    DEBUG(((MachineInstr *) MIB)->print(dbgs()));
  }

  // Insert paired instruction
  unsigned flags = TII->get(PairedOp).mayLoad() ? RegState::Define : RegOp0.getTargetFlags();
  MIB = BuildMI(*MBB, InsertionPoint, DL, TII->get(PairedOp))
      .addReg(parentReg, flags)
      .addOperand(BaseRegOp)
      .addImm(OffsetImm)
      .setMemRefs(MemRefsOrder);
  DEBUG(dbgs() << "\t");
  DEBUG(((MachineInstr *) MIB)->print(dbgs()));

  if (TII->get(PairedOp).mayLoad()) {
    // In terms of load - issue two copy instruction for vregs we had
    const MCInstrDesc &Copy = TII->get(TargetOpcode::COPY);
    MIB = BuildMI(*MBB, InsertionPoint, DL, Copy, RegOp0.getReg())
        .addReg(parentReg, /* flags = */ 0, Epiphany::isub_lo);
    DEBUG(dbgs() << "\t");
    DEBUG(((MachineInstr *) MIB)->print(dbgs()));
    MIB = BuildMI(*MBB, InsertionPoint, DL, Copy, RegOp1.getReg())
        .addReg(parentReg, /* flags = */ 0, Epiphany::isub_hi);
    DEBUG(dbgs() << "\t");
    DEBUG(((MachineInstr *) MIB)->print(dbgs()));
  }
}


/// Merges two n-bit load/store instructions into a single 2*n-bit one
MachineBasicBlock::iterator
EpiphanyVregLoadStoreOptimizer::mergePairedInsns(MachineBasicBlock::iterator I,
                                             MachineBasicBlock::iterator Paired, const LoadStoreFlags &Flags) {
  MachineBasicBlock::iterator NextI = I;
  ++NextI;
  // If NextI is the second of the two instructions to be merged, we need
  // to skip one further. Either way we merge will invalidate the iterator,
  // and we don't need to scan the new instruction, as it's a pairwise
  // instruction, which we're not considering for further action anyway.
  if (NextI == Paired)
    ++NextI;

  unsigned Opc = I->getOpcode();
  unsigned PairedOp = getMatchingPairOpcode(Opc);
  DEBUG(dbgs() << "Creating pair load/store. Replacing instructions:\n\t");
  DEBUG(I->print(dbgs()));
  DEBUG(dbgs() << "\t");
  DEBUG(Paired->print(dbgs()));
  DEBUG(dbgs() << "  with instruction:\n");
  if (Flags.isBasedOnVirtualFI()) {
    mergeFrameBasedInsns(PairedOp, I, Paired, Flags);
  } else {
    bool MergeForward = Flags.getMergeForward();
    // Also based on MergeForward is from where we copy the base register operand
    // so we get the flags compatible with the input code.
    const MachineOperand &BaseRegOp = MergeForward ? getBaseOperand(*Paired) : getBaseOperand(*I);
    const int64_t Offset = getOffsetOperand(*I).getImm();
    const int64_t PairedOffset = getOffsetOperand(*Paired).getImm();
    const int OffsetStride = StackGrowsDown ? getMemScale(*I) : -getMemScale(*I);

    // Which register is Rt and which is Rt2 depends on the offset order.
    MachineInstr *RtMI, *Rt2MI;
    if (Offset == PairedOffset + OffsetStride) {
      RtMI = &*Paired;
      Rt2MI = &*I;
    } else {
      RtMI = &*I;
      Rt2MI = &*Paired;
    }
    int64_t OffsetImm = getOffsetOperand(*RtMI).getImm();
    // Construct the new instruction.
    DebugLoc DL = I->getDebugLoc();
    MachineOperand RegOp0 = getRegOperand(*RtMI);
    MachineOperand RegOp1 = getRegOperand(*Rt2MI);

    // Kill flags may become invalid when moving stores for pairing.
    if (RegOp0.isUse()) {
      cleanKillFlags(RegOp0, RegOp1, I, Paired, MergeForward);
    }

    mergeRegBasedInsns(PairedOp, Paired, I, Flags, OffsetImm,
                       RegOp0, RegOp1);

    // Erase the old instructions.
    I->eraseFromParent();
    Paired->eraseFromParent();
  }
  DEBUG(dbgs() << "\n");

  return NextI;
}

/// Scan the instructions looking for a load/store that can be combined with the
/// current instruction into a wider equivalent or a load/store pair.
MachineBasicBlock::iterator
EpiphanyVregLoadStoreOptimizer::findMatchingInst(MachineBasicBlock::iterator I,
                                             LoadStoreFlags &Flags, unsigned Limit) {
  MachineBasicBlock::iterator E = I->getParent()->end();
  MachineBasicBlock::iterator MBBI = I;
  MachineInstr &FirstMI = *I;
  ++MBBI;

  bool MayLoad = FirstMI.mayLoad();
  // Get first instruction reg data
  unsigned Reg = getRegOperand(FirstMI).getReg();
  unsigned RegIdx = TRI->virtReg2Index(Reg);
  unsigned BaseReg = getBaseOperand(FirstMI).isReg() ? getBaseOperand(FirstMI).getReg() : Epiphany::FP;
  unsigned BaseRegIdx = TRI->isVirtualRegister(BaseReg) ? TRI->virtReg2Index(BaseReg) : BaseReg;

  // Track which registers have been modified and used between the first insn
  // (inclusive) and the second insn.
  ModifiedRegs.reset();
  UsedRegs.reset();
  ModifiedFrameIdxs.reset();
  UsedFrameIdxs.reset();

  // Remember any instructions that read/write memory between FirstMI and MI.
  SmallVector<MachineInstr *, 4> MemInsns;

  for (unsigned Count = 0; MBBI != E && Count < Limit; ++MBBI) {
    MachineInstr &MI = *MBBI;
    // Don't count transient instructions towards the search limit since there
    // may be different numbers of them if e.g. debug information is present.
    if (!MI.isTransient())
      ++Count;

    if (areCandidatesToMergeOrPair(FirstMI, MI, Flags, TII, MFI) &&
        getOffsetOperand(MI).isImm()) {
      assert(MI.mayLoadOrStore() && "Expected memory operation.");
      // Get second instruction reg data
      unsigned MIReg = getRegOperand(MI).getReg();
      unsigned MIRegIdx = TRI->virtReg2Index(MIReg);
      unsigned MIBaseReg = getBaseOperand(MI).isReg() ? getBaseOperand(MI).getReg() : Epiphany::FP;
      // Get offsets
      bool UsingVirtualFI = Flags.isBasedOnVirtualFI();
      int64_t Offset = UsingVirtualFI ? getBaseOperand(FirstMI).getIndex() : getOffsetOperand(FirstMI).getImm();
      int64_t MIOffset = UsingVirtualFI ? getBaseOperand(MI).getIndex() : getOffsetOperand(MI).getImm();

      // If we've found another instruction with the same opcode, check to see
      // if regs, base and offset are compatible with our starting instruction.
      // These instructions all have scaled immediate operands, so we just
      // check for +1/-1. Make sure to check the new instruction offset is
      // actually an immediate and not a symbolic reference destined for
      // a relocation.
      int OffsetStride = UsingVirtualFI ? 1 : getMemScale(FirstMI);
      if (isBaseAndOffsetCorrect(BaseReg, MIBaseReg, Offset, MIOffset, OffsetStride)) {
        DEBUG(dbgs() << "Checking instruction "; MI.dump());

        // Check if the alignment is correct
        if (!isAlignmentCorrect(FirstMI, MI, UsingVirtualFI)) {
          trackRegDefsUses(MI, ModifiedRegs, UsedRegs, TRI);
          if (UsingVirtualFI) {
            trackFrameIdxs(MI, ModifiedFrameIdxs, UsedFrameIdxs, MFI);
          }
          MemInsns.push_back(&MI);
          DEBUG(dbgs() << "Can't be paired due to alignment\n");
          continue;
        }

        // If the destination register of the loads is the same register, bail
        // and keep looking. A load-pair instruction with both destination
        // registers the same is UNPREDICTABLE and will result in an exception.
        if (MayLoad && Reg == getRegOperand(MI).getReg()) {
          trackRegDefsUses(MI, ModifiedRegs, UsedRegs, TRI);
          if (UsingVirtualFI) {
            trackFrameIdxs(MI, ModifiedFrameIdxs, UsedFrameIdxs, MFI);
          }
          MemInsns.push_back(&MI);
          DEBUG(dbgs() << "Can't merge into same reg\n");
          continue;
        }

        bool failed = false;
        for (std::pair<int, int> IdxPair : PairedIdxs) {
          if ((Offset == IdxPair.first && MIOffset != IdxPair.second) ||
              (Offset == IdxPair.second && MIOffset != IdxPair.first) ||
              (MIOffset == IdxPair.first && Offset != IdxPair.second) ||
              (MIOffset == IdxPair.second && Offset != IdxPair.first)) {
            failed = true;
            break;
          }
        }
        if (failed) {
          trackRegDefsUses(MI, ModifiedRegs, UsedRegs, TRI);
          if (UsingVirtualFI) {
            trackFrameIdxs(MI, ModifiedFrameIdxs, UsedFrameIdxs, MFI);
          }
          MemInsns.push_back(&MI);
          DEBUG(dbgs() << "Can't merge as frame idx is already paired\n");
          continue;
        }

        // If the Rt of the second instruction was not modified or used between
        // the two instructions and none of the instructions between the second
        // and first alias with the second, we can combine the second into the
        // first.
        if (!ModifiedRegs[MIRegIdx]) {
          if (!(MI.mayLoad() && UsedRegs[MIRegIdx])) {
            if (!UsingVirtualFI || !UsedFrameIdxs[MIOffset]) {
                PairedIdxs.push_back(std::make_pair(Offset, MIOffset));
              Flags.setMergeForward(false);
              return MBBI;
            }
          }
        } else {
          DEBUG(dbgs() << "Proposed paired reg was modified, will try to merge forward\n");
        }

        // Likewise, if the Rt of the first instruction is not modified or used
        // between the two instructions and none of the instructions between the
        // first and the second alias with the first, we can combine the first
        // into the second.
        if (!ModifiedRegs[RegIdx] && !(MayLoad && UsedRegs[RegIdx])) {
          if (!UsingVirtualFI || !UsedFrameIdxs[Offset]) {
              PairedIdxs.push_back(std::make_pair(Offset, MIOffset));
            Flags.setMergeForward(true);
            return MBBI;
          }
        }
        // Unable to combine these instructions due to interference in between.
        // Keep looking.
      }
    }

    // If the instruction wasn't a matching load or store.  Stop searching if we
    // encounter a call instruction that might modify memory.
    if (MI.isCall())
      return E;

    // Update modified / uses register lists.
    trackRegDefsUses(MI, ModifiedRegs, UsedRegs, TRI);
    trackFrameIdxs(MI, ModifiedFrameIdxs, UsedFrameIdxs, MFI);

    // Otherwise, if the base register is modified, we have no match, so return early.
    if (ModifiedRegs[BaseRegIdx])
      return E;

    // Update list of instructions that read/write memory.
    if (MI.mayLoadOrStore())
      MemInsns.push_back(&MI);
  }
  return E;
}

// Find loads and stores that can be merged into a single load or store pair
// instruction.
bool EpiphanyVregLoadStoreOptimizer::tryToPairLoadStoreInst(MachineBasicBlock::iterator &MBBI) {
  MachineInstr &MI = *MBBI;
  MachineBasicBlock::iterator E = MI.getParent()->end();
  DEBUG(dbgs() << "\nTrying to pair instruction: ";
            MI.print(dbgs()););

  if (!TII->isCandidateToMergeOrPair(MI)) {
    DEBUG(dbgs() << "Not a candidate for merging\n");
    return false;
  }

  // Early exit if the offset is not possible to match. (6 bits of positive
  // range, plus allow an extra one in case we find a later insn that matches
  // with Offset-1)
  int64_t Offset = getOffsetOperand(MI).getImm();
  int OffsetStride = 1;
  // Allow one more for offset.
  if (Offset > 0)
    Offset -= OffsetStride;
  if (!inBoundsForPair(Offset)) {
    DEBUG(dbgs() << "Out of bounds for pairing\n");
    return false;
  }

  // Look ahead up to LdStLimit instructions for a pairable instruction.
  LoadStoreFlags Flags;
  MachineBasicBlock::iterator Paired =
      findMatchingInst(MBBI, Flags, LdStLimit);
  if (Paired != E) {
    ++NumPairCreated;
    // Keeping the iterator straight is a pain, so we let the merge routine tell
    // us what the next instruction is after it's done mucking about.
    MBBI = mergePairedInsns(MBBI, Paired, Flags);
    return true;
  } else {
    DEBUG(dbgs() << "Unable to find matching instruction\n");
  }
  return false;
}

/// \brief Runs optimizer for the given MBB.
///
/// \param MBB Machine basic block to optimize
///
/// \return true if the block was modified
bool EpiphanyVregLoadStoreOptimizer::optimizeBlock(MachineBasicBlock &MBB) {
  bool Modified = false;

  // Find loads and stores that can be merged into a single load or store
  //    pair instruction.
  //      e.g.,
  //        str r0,  [fp]
  //        str r1,  [fp, #1]
  //        ; becomes
  //        strd r0, [fp]
  for (MachineBasicBlock::iterator MBBI = MBB.begin(), E = MBB.end();
       MBBI != E;) {
    if (isPairableLoadStoreInst(*MBBI) && tryToPairLoadStoreInst(MBBI))
      Modified = true;
    else
      ++MBBI;
  }

  return Modified;
}


INITIALIZE_PASS_BEGIN(EpiphanyVregLoadStoreOptimizer, "epiphany-vreg-ls-opt", "Epiphany Vreg Load Store Optimization", false, false)
INITIALIZE_PASS_END(EpiphanyVregLoadStoreOptimizer, "epiphany-vreg-ls-opt", "Epiphany Vreg Load Store Optimization", false, false)

bool EpiphanyVregLoadStoreOptimizer::runOnMachineFunction(MachineFunction &Fn) {
  DEBUG(dbgs() << "\nRunning Vreg Epiphany Load/Store Optimization Pass\n");
  if (skipFunction(*Fn.getFunction()))
    return false;

  Subtarget = &static_cast<const EpiphanySubtarget &>(Fn.getSubtarget());
  TII = Subtarget->getInstrInfo();
  TRI = Subtarget->getRegisterInfo();
  TFI = Subtarget->getFrameLowering();
  MFI = &Fn.getFrameInfo();
  MRI = &Fn.getRegInfo();
  MF = &Fn;

  // Get stack growth direction
  StackGrowsDown = TFI->getStackGrowthDirection() == TargetFrameLowering::StackGrowsDown;
  LastLocalBlockOffset = StackGrowsDown ? -4 : 4;

  // Resize the modified and used register bitfield trackers.  We do this once
  // per function and then clear the bitfield each time we optimize a load or
  // store.
  ModifiedRegs.resize(MRI->getNumVirtRegs() + TRI->getNumRegs());
  UsedRegs.resize(MRI->getNumVirtRegs() + TRI->getNumRegs());
  ModifiedFrameIdxs.resize(MFI->getNumObjects());
  UsedFrameIdxs.resize(MFI->getNumObjects());
  ObjectMapped.reset();
  ObjectMapped.resize(MFI->getNumObjects());

  bool Modified = false;
  for (auto &MBB : Fn) {
    Modified |= optimizeBlock(MBB);
    if (Modified) {
      MFI->setUseLocalStackAllocationBlock(true);
    }
  }

  // Adjust local frame block size
  int64_t LocalFrameSize = StackGrowsDown ? -LastLocalBlockOffset - 4 : LastLocalBlockOffset - 4;
  MFI->setLocalFrameSize(LocalFrameSize);

  return Modified;
}

/// createEpiphanyVregLoadStoreOptimizationPass - returns an instance of the
/// load / store optimization pass.
FunctionPass *llvm::createEpiphanyVregLoadStoreOptimizationPass() {
  return new EpiphanyVregLoadStoreOptimizer();
}
