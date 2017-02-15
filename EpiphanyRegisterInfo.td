//===- EpiphanyRegisterInfo.td - ARM Register defs ----------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file contains declarations that describe the Epiphany register file
//
//===----------------------------------------------------------------------===//

// Registers are identified with 5-bit ID numbers.
class EpiphanyReg<bits<16> enc, string n> : Register<n> {
  let HWEncoding = enc;
  let Namespace = "Epiphany";
}

class Bank0Reg<bits<16> enc, string n> : EpiphanyReg<enc, n>;
class Bank1Reg<bits<16> enc, string n> : EpiphanyReg<enc, n>;
class Bank2Reg<bits<16> enc, string n> : EpiphanyReg<enc, n>;
class Bank3Reg<bits<16> enc, string n> : EpiphanyReg<enc, n>;

//===----------------------------------------------------------------------===//
//@Registers
//===----------------------------------------------------------------------===//

// Bank 0 GPRs
let Namespace = "Epiphany" in {
  foreach i = 0-63 in {
    def R#i : Bank0Reg<#i, "R"#i>,  DwarfRegNum<[#i]>;
  }
  
// Aliases
  def A1    : Bank0Reg<0,  "A1">,  DwarfRegAlias<R0>;
  def A2    : Bank0Reg<1,  "A2">,  DwarfRegAlias<R1>;
  def A3    : Bank0Reg<2,  "A3">,  DwarfRegAlias<R2>;
  def A4    : Bank0Reg<3,  "A4">,  DwarfRegAlias<R3>;
  def V1    : Bank0Reg<4,  "V1">,  DwarfRegAlias<R4>;
  def V2    : Bank0Reg<5,  "V2">,  DwarfRegAlias<R5>;
  def V3    : Bank0Reg<6,  "V3">,  DwarfRegAlias<R6>;
  def V4    : Bank0Reg<7,  "V4">,  DwarfRegAlias<R7>;
  def V5    : Bank0Reg<8,  "V5">,  DwarfRegAlias<R8>;
  def SB    : Bank0Reg<9,  "SB">,  DwarfRegAlias<R9>;
  def SL    : Bank0Reg<10, "SL">,  DwarfRegAlias<R10>;
  def V8    : Bank0Reg<11, "V8">,  DwarfRegAlias<R11>;
  def SP    : Bank0Reg<13, "SP">,  DwarfRegAlias<R13>;
  def LR    : Bank0Reg<14, "LR">,  DwarfRegAlias<R14>;
  def FP    : Bank0Reg<15, "FP">,  DwarfRegAlias<R15>;
  def ZERO  : Bank0Reg<31, "ZERO">, DwarfRegAlias<R31>;
}

// Bank1 registers
let Namespace = "Epiphany" in {
  def CONFIG      : Bank1Reg<0,  "CONFIG">,      DwarfRegNum<[100]>;
  def STATUS      : Bank1Reg<1,  "STATUS">,      DwarfRegNum<[101]>;
  def PC          : Bank1Reg<2,  "PC">,          DwarfRegNum<[102]>;
  def DEBUGSTATUS : Bank1Reg<3,  "DEBUGSTATUS">, DwarfRegNum<[103]>;
  def LC          : Bank1Reg<4,  "LC">,          DwarfRegNum<[104]>;
  def LS          : Bank1Reg<5,  "LS">,          DwarfRegNum<[105]>;
  def LE          : Bank1Reg<6,  "LE">,          DwarfRegNum<[106]>;
  def IRET        : Bank1Reg<7,  "IRET">,        DwarfRegNum<[107]>;
  def IMASK       : Bank1Reg<8,  "IMASK">,       DwarfRegNum<[108]>;
  def ILAT        : Bank1Reg<9,  "ILAT">,        DwarfRegNum<[109]>;
  def ILATST      : Bank1Reg<10, "ILATST">,      DwarfRegNum<[110]>;
  def ILATCL      : Bank1Reg<11, "ILATCL">,      DwarfRegNum<[111]>;
  def IPEND       : Bank1Reg<12, "IPEND">,       DwarfRegNum<[112]>;
  def CTIMER0     : Bank1Reg<13, "CTIMER0">,     DwarfRegNum<[113]>;
  def CTIMER1     : Bank1Reg<14, "CTIMER1">,     DwarfRegNum<[114]>;
  def FSTATUS     : Bank1Reg<15, "FSTATUS">,     DwarfRegNum<[115]>;
  // 0xF0444 is empty
  def DEBUGCMD    : Bank1Reg<17, "DEBUGCMD">,    DwarfRegNum<[117]>;
}

// DMA Registers
let Namespace = "Epiphany" in {
  def DMA0CONFIG  : Bank2Reg<0, "DMA0CONFIG">,   DwarfRegNum<[200]>;
}

// 0xF07xx
let Namespace = "Epiphany" in {
  def RESETCORE   : Bank1Reg<1, "RESETCORE">,   DwarfRegNum<[400]>;
}

//===----------------------------------------------------------------------===//
//@Register Classes
//===----------------------------------------------------------------------===//

// General purpose (first 7 only)
// It should be first 7, but i'd prefer not to use R4-R7 as they're callee-saved
// And this code size benefit will be discarded by the store/load
def GPR16 : RegisterClass<"Epiphany", [i32], 32, (add (sequence "R%u", 0, 3))>;

// General purpose (all)
def GPR32 : RegisterClass<"Epiphany", [i32], 32, (add
  // Arg/result/scratch caller-saved regs
  R0, R1, R2, R3,
  // Variable callee-saved
  R4, R5, R6, R7, R8,
  // Static base
  SB,
  // Stack limit
  SL,
  // Frame pointer
  FP,
  // Intra-procedure call scratch reg
  R12,
  // Stack pointer
  SP,
  // Callee-saved link register
  LR,
  // Callee-saved GPR
  R15,
  // Caller-saved GPR
  (sequence "R%u", 16, 27),
  // Reserved
  R28, R29, R30, ZERO,
  // Caller-saved GPR
  (sequence "R%u", 32, 63))> {
  
  // Reordering - putting callee-saved regs to the end of the list
  let AltOrders = [(add (sub GPR32, R4, R5, R6, R7, R8), R4, R5, R6, R7, R8)];
  let AltOrderSelect = [{ return 1; }];
}

def FPR32 : RegisterClass<"Epiphany", [f32], 32, (add GPR32)>;

// Status register
def SR : RegisterClass<"Epiphany", [i32], 32, (add STATUS)>;

// Special regs
def SPECIAL : RegisterClass<"Epiphany", [i32], 32, (add CONFIG, 
  STATUS, 
  PC, 
  DEBUGSTATUS,
  LC,
  LS,
  LE,
  IRET,
  IMASK,
  ILAT,
  ILATST,
  ILATCL,
  IPEND,
  CTIMER0,
  CTIMER1,
  FSTATUS,
  DEBUGCMD)>;
