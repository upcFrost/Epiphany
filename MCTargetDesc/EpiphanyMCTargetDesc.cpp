//===-- EpiphanyMCTargetDesc.cpp - Epiphany Target Descriptions -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
//===----------------------------------------------------------------------===//
//
// This file provides Epiphany specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyMCTargetDesc.h"

#include "InstPrinter/EpiphanyInstPrinter.h"
#include "EpiphanyMCAsmInfo.h"
#include "EpiphanyTargetStreamer.h"
#include "llvm/MC/MachineLocation.h"
#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#include "EpiphanyGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "EpiphanyGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "EpiphanyGenRegisterInfo.inc"


/// Select the Epiphany Architecture Feature for the given triple and cpu name.
/// The function will be called at command 'llvm-objdump -d' for Epiphany elf input.
static StringRef selectEpiphanyArchFeature(const Triple &TT, StringRef CPU) {
  std::string EpiphanyArchFeature;
  if (CPU.empty() || CPU == "generic") {
    if (TT.getArch() == Triple::epiphany) {
      if (CPU.empty() || CPU == "E16") {
        EpiphanyArchFeature = "+E16";
      }
    }
  }
  return EpiphanyArchFeature;
}

static MCInstrInfo *createEpiphanyMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitEpiphanyMCInstrInfo(X); // defined in EpiphanyGenInstrInfo.inc
  return X;
}

static MCRegisterInfo *createEpiphanyMCRegisterInfo(const Triple &Triple) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitEpiphanyMCRegisterInfo(X, Epiphany::STATUS); // defined in EpiphanyGenRegisterInfo.inc
  return X;
}

static MCSubtargetInfo *createEpiphanyMCSubtargetInfo(const Triple &TT,
    StringRef CPU,
    StringRef FS) {
  std::string ArchFS = selectEpiphanyArchFeature(TT,CPU);
  if (!FS.empty()) {
    if (!ArchFS.empty())
      ArchFS = ArchFS + "," + FS.str();
    else
      ArchFS = FS;
  }
  // createEpiphanyMCSubtargetInfoImpl defined in EpiphanyGenSubtargetInfo.inc
  return createEpiphanyMCSubtargetInfoImpl(TT,CPU,FS);
}

static MCAsmInfo *createEpiphanyMCAsmInfo(const MCRegisterInfo &MRI,
    const Triple &TT) {
  MCAsmInfo *MAI = new EpiphanyELFMCAsmInfo(TT);

  unsigned SP = MRI.getDwarfRegNum(Epiphany::SP, true);
  MCCFIInstruction Inst = MCCFIInstruction::createDefCfa(nullptr, SP, 0);
  MAI->addInitialFrameState(Inst);

  return MAI;
}

namespace {

  class EpiphanyMCInstrAnalysis : public MCInstrAnalysis {
    public:
      EpiphanyMCInstrAnalysis(const MCInstrInfo *Info) : MCInstrAnalysis(Info) {}
  };
}

static MCInstrAnalysis *createEpiphanyMCInstrAnalysis(const MCInstrInfo *Info) {
  return new EpiphanyMCInstrAnalysis(Info);
}

static MCInstPrinter *createEpiphanyMCInstPrinter(const Triple &T,
    unsigned SyntaxVariant,
    const MCAsmInfo &MAI,
    const MCInstrInfo &MII,
    const MCRegisterInfo &MRI) {
  return new EpiphanyInstPrinter(MAI, MII, MRI);
}

static MCStreamer *createMCStreamer(const Triple &TT, MCContext &Context, 
    MCAsmBackend &MAB, raw_pwrite_stream &OS, 
    MCCodeEmitter *Emitter, bool RelaxAll) {
  return createELFStreamer(Context, MAB, OS, Emitter, RelaxAll);
}

static MCTargetStreamer *createEpiphanyAsmTargetStreamer(MCStreamer &S,
    formatted_raw_ostream &OS,
    MCInstPrinter *InstPrint,
    bool isVerboseAsm) {
  return new EpiphanyTargetAsmStreamer(S, OS);
}

extern "C" void LLVMInitializeEpiphanyTargetMC() {
  // Register the MC asm info.
  RegisterMCAsmInfoFn X(TheEpiphanyTarget, createEpiphanyMCAsmInfo);

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
  TargetRegistry::RegisterMCInstrAnalysis(TheEpiphanyTarget, createEpiphanyMCInstrAnalysis);

  // Register the MCInstPrinter.
  TargetRegistry::RegisterMCInstPrinter(TheEpiphanyTarget,
      createEpiphanyMCInstPrinter);

  // Register the elf streamer.
  TargetRegistry::RegisterELFStreamer(TheEpiphanyTarget, createMCStreamer);

  // Register the asm target streamer.
  TargetRegistry::RegisterAsmTargetStreamer(TheEpiphanyTarget, createEpiphanyAsmTargetStreamer);

  // Register the MC Code Emitter
  TargetRegistry::RegisterMCCodeEmitter(TheEpiphanyTarget,
      createEpiphanyMCCodeEmitterEL);

  // Register the asm backend.
  TargetRegistry::RegisterMCAsmBackend(TheEpiphanyTarget,
      createEpiphanyAsmBackendEL32);


}
