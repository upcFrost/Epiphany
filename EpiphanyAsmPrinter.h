//===-- EpiphanyAsmPrinter.h - Print machine code to an Epiphany .s file --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to GAS-format Epiphany assembly language.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EPIPHANY_EPIPHANYASMPRINTER_H
#define LLVM_LIB_TARGET_EPIPHANY_EPIPHANYASMPRINTER_H

#include "EpiphanyConfig.h"

#include "EpiphanyMachineFunction.h"
#include "EpiphanyMCInstLower.h"
#include "EpiphanySubtarget.h"
#include "EpiphanyTargetMachine.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class MCStreamer;
class MachineInstr;
class MachineBasicBlock;
class Module;
class raw_ostream;

class LLVM_LIBRARY_VISIBILITY EpiphanyAsmPrinter : public AsmPrinter {

  void EmitInstrWithMacroNoAT(const MachineInstr *MI);

private:
  bool lowerOperand(const MachineOperand &MO, MCOperand &MCOp);

public:

  const EpiphanySubtarget *Subtarget;
  const EpiphanyMachineFunctionInfo *EpiphanyFI;
  EpiphanyMCInstLower MCInstLowering;

  explicit EpiphanyAsmPrinter(TargetMachine &TM, 
                              std::unique_ptr<MCStreamer> Streamer)
    : AsmPrinter(TM, std::move(Streamer)), 
      MCInstLowering(*this) {
    Subtarget = static_cast<EpiphanyTargetMachine &>(TM).getSubtargetImpl();
  }

  virtual const char *getPassName() const override {
    return "Epiphany Assembly Printer";
  }

  virtual bool runOnMachineFunction(MachineFunction &MF) override;

  //- EmitInstruction() must exists or will have run time error.
  void EmitInstruction(const MachineInstr *MI) override;
  void printSavedRegsBitmask(raw_ostream &O);
  void printHex32(unsigned int Value, raw_ostream &O);
  void emitFrameDirective();
  const char *getCurrentABIString() const;
  void EmitFunctionEntryLabel() override;
  void EmitFunctionBodyStart() override;
  void EmitFunctionBodyEnd() override;
  void EmitStartOfAsmFile(Module &M) override;
  void PrintDebugValueComment(const MachineInstr *MI, raw_ostream &OS);
};

} // End of namespace llvm

#endif
