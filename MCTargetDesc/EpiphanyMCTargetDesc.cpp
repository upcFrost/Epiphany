//===-- EpiphanyMCTargetDesc.cpp - Epiphany Target Descriptions -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides Epiphany specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyMCTargetDesc.h"
//#include "EpiphanyELFStreamer.h"
#include "EpiphanyMCAsmInfo.h"
#include "InstPrinter/EpiphanyInstPrinter.h"
#include "llvm/ADT/APInt.h"
#include "llvm/MC/MCCodeGenInfo.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"

#define GET_REGINFO_MC_DESC
#include "EpiphanyGenRegisterInfo.inc"

#define GET_INSTRINFO_MC_DESC
#include "EpiphanyGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "EpiphanyGenSubtargetInfo.inc"

using namespace llvm;

static MCSubtargetInfo *createEpiphanyMCSubtargetInfo(const Triple &TT,
                                                          StringRef CPU,
                                                          StringRef FS) {
  return createEpiphanyMCSubtargetInfoImpl(TT,CPU,FS);
}


static MCInstrInfo *createEpiphanyMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitEpiphanyMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createEpiphanyMCRegisterInfo(const Triple &Triple) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitEpiphanyMCRegisterInfo(X, Epiphany::LR);
  return X;
}

static MCAsmInfo *createEpiphanyMCAsmInfo(const MCRegisterInfo &MRI,
                                          const Triple &TT) {
  MCAsmInfo *MAI = new EpiphanyELFMCAsmInfo();
  unsigned Reg = MRI.getDwarfRegNum(Epiphany::SP, true);
  MCCFIInstruction Inst = MCCFIInstruction::createDefCfa(nullptr, Reg, 0);
  MAI->addInitialFrameState(Inst);

  return MAI;
}

static MCCodeGenInfo *createEpiphanyMCCodeGenInfo(const Triple &TT, 
                                                 Reloc::Model RM,
                                                 CodeModel::Model CM,
                                                 CodeGenOpt::Level OL) {
  MCCodeGenInfo *X = new MCCodeGenInfo();
  RM = Reloc::Static;
  CM = CodeModel::Small;
  X->initMCCodeGenInfo(RM, CM, OL);
  return X;
}


static MCInstPrinter *createEpiphanyMCInstPrinter(const Triple &T,
                                                  unsigned SyntaxVariant,
                                                  const MCAsmInfo &MAI,
                                                  const MCInstrInfo &MII,
                                                  const MCRegisterInfo &MRI) {
  if (SyntaxVariant == 0)
    return new EpiphanyInstPrinter(MAI, MII, MRI);
  return nullptr;
}

namespace {

class EpiphanyMCInstrAnalysis : public MCInstrAnalysis {
public:
  EpiphanyMCInstrAnalysis(const MCInstrInfo *Info) : MCInstrAnalysis(Info) {}

  virtual bool isUnconditionalBranch(const MCInst &Inst) const {
    if (Inst.getOpcode() == Epiphany::Bcc
        && Inst.getOperand(0).getImm() == EpiphanyCC::AL)
      return true;
    return MCInstrAnalysis::isUnconditionalBranch(Inst);
  }

  virtual bool isConditionalBranch(const MCInst &Inst) const {
    if (Inst.getOpcode() == Epiphany::Bcc
        && Inst.getOperand(0).getImm() == EpiphanyCC::AL)
      return false;
    return MCInstrAnalysis::isConditionalBranch(Inst);
  }

  uint64_t evaluateBranch(const MCInst &Inst, uint64_t Addr,
                          uint64_t Size) const {
    unsigned LblOperand = Inst.getOpcode() == Epiphany::Bcc ? 1 : 0;
    // FIXME: We only handle PCRel branches for now.
    if (Info->get(Inst.getOpcode()).OpInfo[LblOperand].OperandType
        != MCOI::OPERAND_PCREL)
      return -1ULL;

    int64_t Imm = Inst.getOperand(LblOperand).getImm();

    return Addr + Imm;
  }
};

}

static MCInstrAnalysis *createEpiphanyMCInstrAnalysis(const MCInstrInfo *Info) {
  return new EpiphanyMCInstrAnalysis(Info);
}



extern "C" void LLVMInitializeEpiphanyTargetMC() {
  // Register the MC asm info.
  RegisterMCAsmInfoFn X(TheEpiphanyTarget, createEpiphanyMCAsmInfo);

  // Register the MC codegen info.
  TargetRegistry::RegisterMCCodeGenInfo(TheEpiphanyTarget,
                                        createEpiphanyMCCodeGenInfo);

  // Register the MC instruction info.
  TargetRegistry::RegisterMCInstrInfo(TheEpiphanyTarget,
                                      createEpiphanyMCInstrInfo);

  // Register the MC register info.
  TargetRegistry::RegisterMCRegInfo(TheEpiphanyTarget,
                                    createEpiphanyMCRegisterInfo);

  // Register the MC subtarget info.
  TargetRegistry::RegisterMCSubtargetInfo(TheEpiphanyTarget,
                                          createEpiphanyMCSubtargetInfo);

  // Register the MC instruction analyzer.
  TargetRegistry::RegisterMCInstrAnalysis(TheEpiphanyTarget,
                                          createEpiphanyMCInstrAnalysis);

  //// Register the MC Code Emitter
  //TargetRegistry::RegisterMCCodeEmitter(TheEpiphanyTarget,
  //                                      createEpiphanyMCCodeEmitter);

  //// Register the asm backend.
  //TargetRegistry::RegisterMCAsmBackend(TheEpiphanyTarget,
  //                                     createEpiphanyAsmBackend);

  //// Register the object streamer.
  //TargetRegistry::RegisterMCObjectStreamer(TheEpiphanyTarget,
  //                                         createMCStreamer);

  // Register the MCInstPrinter.
  TargetRegistry::RegisterMCInstPrinter(TheEpiphanyTarget,
                                        createEpiphanyMCInstPrinter);
}
