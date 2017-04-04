//=- EpiphanyLoadStoreOptimizer.cpp - Epiphany load/store opt. pass -*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that performs load / store related peephole
// optimizations. This pass should be run after register allocation.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyLoadStoreOptimizer.h"

using namespace llvm;

#define DEBUG_TYPE "epiphany_ls_opt"

STATISTIC(NumPairCreated, "Number of load/store pair instructions generated");

// The LdStLimit limits how far we search for load/store pairs.
static cl::opt<unsigned> LdStLimit("epiphany-load-store-scan-limit", cl::init(20), cl::Hidden);


char EpiphanyLoadStoreOptimizer::ID = 0;

// Check if the instruction is on the promotable list
static bool isPairableLoadStoreInst(MachineInstr &MI) {
  unsigned inst[] = {
    Epiphany::STRi16_r16,
    Epiphany::STRi16_r32,
    Epiphany::STRi32_r16,
    Epiphany::STRi32_r32,
    Epiphany::LDRi32_r16,
    Epiphany::LDRi32_r32
  };
  unsigned Opc = MI.getOpcode();
  return std::find(std::begin(inst), std::end(inst), Opc) != std::end(inst);
}

static int getMemScale(MachineInstr &MI) {
  switch (MI.getOpcode()) {
    default:
      llvm_unreachable("Opcode has unknown scale!");
    case Epiphany::STRi16_r16:
    case Epiphany::STRi16_r32:
      return 2;
    case Epiphany::STRi32_r16:
    case Epiphany::STRi32_r32:
    case Epiphany::LDRi32_r16:
    case Epiphany::LDRi32_r32:
      return 4;
  }
}

// Pair opcodes, e.g. STRi64_r32 for STRi32_r32
static unsigned getMatchingPairOpcode(unsigned Opc) {
  switch(Opc) {
    default:
      llvm_unreachable("Opcode has no pairwise equivalent");
      break;
    case Epiphany::STRi16_r16:
      return Epiphany::STRi32_r16;
    case Epiphany::STRi16_r32:
      return Epiphany::STRi32_r32;
    case Epiphany::STRi32_r16:
    case Epiphany::STRi32_r32:
      return Epiphany::STRi64;
    case Epiphany::LDRi32_r16:
    case Epiphany::LDRi32_r32:
      return Epiphany::LDRi64;
  }
}

// Convert the byte-offset used by unscaled into an "element" offset used
// by the scaled pair load/store instructions.
static bool inBoundsForPair(int Offset) {
  // Well, in fact if the op is in bounds for any kind of store/load - it will be in bound for pairing
  return true;
  //return Offset <= 256 && Offset >= -256;
}

// Get register for the store
static const MachineOperand &getRegOperand(const MachineInstr &MI) {
  return MI.getOperand(0);
}

// Get base for the store
static const MachineOperand &getBaseOperand(const MachineInstr &MI) {
  return MI.getOperand(1);
}

// Get offset for the store
static const MachineOperand &getOffsetOperand(const MachineInstr &MI) {
  return MI.getOperand(2);
}

// Returns true if FirstMI and MI are candidates for merging or pairing.
// Otherwise, returns false.
static bool areCandidatesToMergeOrPair(MachineInstr &FirstMI, MachineInstr &MI,
    LoadStoreFlags &Flags, const EpiphanyInstrInfo *TII) {
  // If this is volatile not a candidate.
  if (MI.hasOrderedMemoryRef())
    return false;

  // We should have already checked FirstMI for pair suppression and volatility.
  assert(!FirstMI.hasOrderedMemoryRef() &&
      "FirstMI shouldn't get here if either of these checks are true.");

  unsigned OpcA = FirstMI.getOpcode();
  unsigned OpcB = MI.getOpcode();

  // Opcodes match: nothing more to check.
  return OpcA == OpcB;
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
      for (MCRegAliasIterator AI(Reg, TRI, true); AI.isValid(); ++AI)
        ModifiedRegs.set(*AI);
    } else {
      assert(MO.isUse() && "Reg operand not a def and not a use?!?");
      for (MCRegAliasIterator AI(Reg, TRI, true); AI.isValid(); ++AI)
        UsedRegs.set(*AI);
    }
  }
}

MachineBasicBlock::iterator
EpiphanyLoadStoreOptimizer::mergePairedInsns(MachineBasicBlock::iterator I,
    MachineBasicBlock::iterator Paired,
    const LoadStoreFlags &Flags) {
  MachineBasicBlock::iterator NextI = I;
  ++NextI;
  // If NextI is the second of the two instructions to be merged, we need
  // to skip one further. Either way we merge will invalidate the iterator,
  // and we don't need to scan the new instruction, as it's a pairwise
  // instruction, which we're not considering for further action anyway.
  if (NextI == Paired)
    ++NextI;

  unsigned Opc = I->getOpcode();
  int OffsetStride = getMemScale(*I);

  bool MergeForward = Flags.getMergeForward();
  // Insert our new paired instruction after whichever of the paired
  // instructions MergeForward indicates.
  MachineBasicBlock::iterator InsertionPoint = MergeForward ? Paired : I;
  // Also based on MergeForward is from where we copy the base register operand
  // so we get the flags compatible with the input code.
  const MachineOperand &BaseRegOp =
    MergeForward ? getBaseOperand(*Paired) : getBaseOperand(*I);
  int Offset = getOffsetOperand(*I).getImm();
  int PairedOffset = getOffsetOperand(*Paired).getImm();

  // Which register is Rt and which is Rt2 depends on the offset order.
  MachineInstr *RtMI, *Rt2MI;
  if (Offset == PairedOffset + OffsetStride) {
    RtMI = &*Paired;
    Rt2MI = &*I;
  } else {
    RtMI = &*I;
    Rt2MI = &*Paired;
  }
  int OffsetImm = getOffsetOperand(*RtMI).getImm();
  // Construct the new instruction.
  MachineInstrBuilder MIB;
  DebugLoc DL = I->getDebugLoc();
  MachineBasicBlock *MBB = I->getParent();
  MachineOperand RegOp0 = getRegOperand(*RtMI);
  MachineOperand RegOp1 = getRegOperand(*Rt2MI);
  // Kill flags may become invalid when moving stores for pairing.
  if (RegOp0.isUse()) {
    if (!MergeForward) {
      // Clear kill flags on store if moving upwards. Example:
      //   STRi32 %r0, ...
      //   USE %r1
      //   STRi32 kill %r1  ; need to clear kill flag when moving STRi32 upwards
      RegOp0.setIsKill(false);
      RegOp1.setIsKill(false);
    } else {
      // Clear kill flags of the first stores register. Example:
      //   STRi32 %r1, ...
      //   USE kill %r1   ; need to clear kill flag when moving STRi32 downwards
      //   STRi32 %r0
      unsigned Reg = getRegOperand(*I).getReg();
      for (MachineInstr &MI : make_range(std::next(I), Paired))
        MI.clearRegisterKills(Reg, TRI);
    }
  }
  unsigned PairedOp = getMatchingPairOpcode(Opc);
  if (PairedOp == Epiphany::STRi64 || PairedOp == Epiphany::LDRi64) {
    // Reg with subs
    unsigned PairedReg = TRI->getMatchingSuperReg(RegOp0.getReg(), Epiphany::isub_lo, &Epiphany::GPR64RegClass);
    if (!PairedReg) {
      PairedReg = TRI->getMatchingSuperReg(RegOp1.getReg(), Epiphany::isub_lo, &Epiphany::GPR64RegClass);
    }
    MIB = BuildMI(*MBB, InsertionPoint, DL, TII->get(PairedOp))
      .addReg(PairedReg)
      .addOperand(BaseRegOp)
      .addImm(OffsetImm)
      .setMemRefs(I->mergeMemRefsWith(*Paired));
  } else {
    // Standard 32-bit reg
    MIB = BuildMI(*MBB, InsertionPoint, DL, TII->get(PairedOp))
      .addOperand(RegOp0)
      .addOperand(BaseRegOp)
      .addImm(OffsetImm)
      .setMemRefs(I->mergeMemRefsWith(*Paired));
  }

  (void)MIB;

  DEBUG(dbgs() << "Creating pair load/store. Replacing instructions:\n    ");
  DEBUG(I->print(dbgs()));
  DEBUG(dbgs() << "    ");
  DEBUG(Paired->print(dbgs()));
  DEBUG(dbgs() << "  with instruction:\n    ");
  DEBUG(((MachineInstr *)MIB)->print(dbgs()));
  DEBUG(dbgs() << "\n");

  // Erase the old instructions.
  I->eraseFromParent();
  Paired->eraseFromParent();

  return NextI;
}

/// Scan the instructions looking for a load/store that can be combined with the
/// current instruction into a wider equivalent or a load/store pair.
MachineBasicBlock::iterator
EpiphanyLoadStoreOptimizer::findMatchingInst(MachineBasicBlock::iterator I, 
    LoadStoreFlags &Flags, unsigned Limit) {
  MachineBasicBlock::iterator E = I->getParent()->end();
  MachineBasicBlock::iterator MBBI = I;
  MachineInstr &FirstMI = *I;
  ++MBBI;

  bool MayLoad = FirstMI.mayLoad();
  unsigned Reg = getRegOperand(FirstMI).getReg();
  unsigned BaseReg = getBaseOperand(FirstMI).getReg();
  int Offset = getOffsetOperand(FirstMI).getImm();
  int OffsetStride = getMemScale(FirstMI);

  // Track which registers have been modified and used between the first insn
  // (inclusive) and the second insn.
  ModifiedRegs.reset();
  UsedRegs.reset();

  // Remember any instructions that read/write memory between FirstMI and MI.
  SmallVector<MachineInstr *, 4> MemInsns;

  for (unsigned Count = 0; MBBI != E && Count < Limit; ++MBBI) {
    MachineInstr &MI = *MBBI;

    // Don't count transient instructions towards the search limit since there
    // may be different numbers of them if e.g. debug information is present.
    if (!MI.isTransient())
      ++Count;

    if (areCandidatesToMergeOrPair(FirstMI, MI, Flags, TII) &&
        getOffsetOperand(MI).isImm()) {
      assert(MI.mayLoadOrStore() && "Expected memory operation.");
      // If we've found another instruction with the same opcode, check to see
      // if regs, base and offset are compatible with our starting instruction.
      // These instructions all have scaled immediate operands, so we just
      // check for +1/-1. Make sure to check the new instruction offset is
      // actually an immediate and not a symbolic reference destined for
      // a relocation.
      unsigned MIBaseReg = getBaseOperand(MI).getReg();
      int MIOffset = getOffsetOperand(MI).getImm();
      if (BaseReg == MIBaseReg && ((Offset == MIOffset + OffsetStride) ||
            (Offset + OffsetStride == MIOffset))) {
        // First, check pairity for 64-bit pairing
        unsigned Opc = MI.getOpcode();
        if (Opc == Epiphany::LDRi32_r32 || Opc == Epiphany::LDRi32_r32 || 
            Opc == Epiphany::STRi32_r32 || Opc == Epiphany::STRi32_r32) {
          unsigned MIReg = getRegOperand(MI).getReg();
          unsigned sra = TRI->getMatchingSuperReg (Reg, Epiphany::isub_lo, &Epiphany::GPR64RegClass);
          unsigned srb = TRI->getMatchingSuperReg (MIReg, Epiphany::isub_hi, &Epiphany::GPR64RegClass);
          if( (!sra || !srb) || sra != srb){
            sra = TRI->getMatchingSuperReg (MIReg, Epiphany::isub_lo, &Epiphany::GPR64RegClass);
            srb = TRI->getMatchingSuperReg (Reg, Epiphany::isub_hi, &Epiphany::GPR64RegClass);
          }
          if (!(sra && srb)) {
            trackRegDefsUses(MI, ModifiedRegs, UsedRegs, TRI);
            MemInsns.push_back(&MI);
            continue;
          }
        }

        // Get the left-lowest offset
        int MinOffset = Offset < MIOffset ? Offset : MIOffset;
        // If the resultant immediate offset of merging these 
        // instructions is out of range for
        // a pairwise instruction, bail and keep looking.
        if (!inBoundsForPair(MinOffset)) {
          trackRegDefsUses(MI, ModifiedRegs, UsedRegs, TRI);
          MemInsns.push_back(&MI);
          continue;
        }
        // If the destination register of the loads is the same register, bail
        // and keep looking. A load-pair instruction with both destination
        // registers the same is UNPREDICTABLE and will result in an exception.
        if (MayLoad && Reg == getRegOperand(MI).getReg()) {
          trackRegDefsUses(MI, ModifiedRegs, UsedRegs, TRI);
          MemInsns.push_back(&MI);
          continue;
        }

        // If the Rt of the second instruction was not modified or used between
        // the two instructions and none of the instructions between the second
        // and first alias with the second, we can combine the second into the
        // first.
        if (!ModifiedRegs[getRegOperand(MI).getReg()] &&
            !(MI.mayLoad() && UsedRegs[getRegOperand(MI).getReg()])) {
          Flags.setMergeForward(false);
          return MBBI;
        }

        // Likewise, if the Rt of the first instruction is not modified or used
        // between the two instructions and none of the instructions between the
        // first and the second alias with the first, we can combine the first
        // into the second.
        if (!ModifiedRegs[getRegOperand(FirstMI).getReg()] &&
            !(MayLoad && UsedRegs[getRegOperand(FirstMI).getReg()])) {
          Flags.setMergeForward(true);
          return MBBI;
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

    // Otherwise, if the base register is modified, we have no match, so
    // return early.
    if (ModifiedRegs[BaseReg])
      return E;

    // Update list of instructions that read/write memory.
    if (MI.mayLoadOrStore())
      MemInsns.push_back(&MI);
  }
  return E;
}

// Find loads and stores that can be merged into a single load or store pair
// instruction.
bool EpiphanyLoadStoreOptimizer::tryToPairLoadStoreInst(MachineBasicBlock::iterator &MBBI) {
  MachineInstr &MI = *MBBI;
  MachineBasicBlock::iterator E = MI.getParent()->end();
  DEBUG(dbgs() << "\nTrying to pair instruction: "; MI.print(dbgs()););

  if (!TII->isCandidateToMergeOrPair(MI))
    return false;

  // Early exit if the offset is not possible to match. (6 bits of positive
  // range, plus allow an extra one in case we find a later insn that matches
  // with Offset-1)
  int Offset = getOffsetOperand(MI).getImm();
  int OffsetStride = 1;
  // Allow one more for offset.
  if (Offset > 0)
    Offset -= OffsetStride;
  if (!inBoundsForPair(Offset))
    return false;

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
  }
  return false;
}



bool EpiphanyLoadStoreOptimizer::optimizeBlock(MachineBasicBlock &MBB) {
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


INITIALIZE_PASS_BEGIN(EpiphanyLoadStoreOptimizer, "epiphany-ls-opt", "Epiphany Load Store Optimization", false, false);
INITIALIZE_PASS_END(EpiphanyLoadStoreOptimizer, "epiphany-ls-opt", "Epiphany Load Store Optimization", false, false);

bool EpiphanyLoadStoreOptimizer::runOnMachineFunction(MachineFunction &Fn) {
  DEBUG(dbgs() << "\nRunning Epiphany Load/Store Optimization Pass\n");
  if (skipFunction(*Fn.getFunction()))
    return false;

  Subtarget = &static_cast<const EpiphanySubtarget &>(Fn.getSubtarget());
  TII = static_cast<const EpiphanyInstrInfo *>(Subtarget->getInstrInfo());
  TRI = Subtarget->getRegisterInfo();

  // Resize the modified and used register bitfield trackers.  We do this once
  // per function and then clear the bitfield each time we optimize a load or
  // store.
  ModifiedRegs.resize(TRI->getNumRegs());
  UsedRegs.resize(TRI->getNumRegs());

  bool Modified = false;
  for (auto &MBB : Fn)
    Modified |= optimizeBlock(MBB);

  return Modified;
}

/// createEpiphanyLoadStoreOptimizationPass - returns an instance of the
/// load / store optimization pass.
FunctionPass *llvm::createEpiphanyLoadStoreOptimizationPass() {
  return new EpiphanyLoadStoreOptimizer();
}
