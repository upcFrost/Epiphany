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

let Namespace = "Epiphany" in {
  def isub_lo  : SubRegIndex<32>;
  def isub_hi  : SubRegIndex<32, 32>;
  def subreg_overflow : SubRegIndex<1, 0>;
}

class EpiphanyReg64<bits<16> enc, string n, list<Register> subregs> : RegisterWithSubRegs<n, subregs> {
  let HWEncoding = enc;
  let Namespace = "Epiphany";
  let SubRegs = subregs;
  let SubRegIndices = [isub_hi, isub_lo];
  let CoveredBySubRegs = 1;
}

class Bank0Reg32<bits<16> enc, string n> : EpiphanyReg<enc, n>;
class Bank1Reg32<bits<16> enc, string n> : EpiphanyReg<enc, n>;
class Bank2Reg32<bits<16> enc, string n> : EpiphanyReg<enc, n>;
class Bank3Reg32<bits<16> enc, string n> : EpiphanyReg<enc, n>;

class Bank0Reg64<bits<16> enc, string n, list<Register> subregs> : EpiphanyReg64<enc, n, subregs>;

//===----------------------------------------------------------------------===//
//@Registers
//===----------------------------------------------------------------------===//

// Bank 0 GPRs
let Namespace = "Epiphany" in {
// 32 bit regs
  foreach i = 0-63 in {
    def R#i : Bank0Reg32<#i, "R"#i>,  DwarfRegNum<[#i]>;
  }

// 64 bit extension
  def D0    : Bank0Reg64<0,  "D0",  [R0, R1]>,   DwarfRegNum<[0]>;
  def D1    : Bank0Reg64<2,  "D1",  [R2, R3]>,   DwarfRegNum<[2]>;
  def D2    : Bank0Reg64<4,  "D2",  [R4, R5]>,   DwarfRegNum<[4]>;
  def D3    : Bank0Reg64<6,  "D3",  [R6, R7]>,   DwarfRegNum<[6]>;
  def D4    : Bank0Reg64<8,  "D4",  [R8, R9]>,   DwarfRegNum<[8]>;
  def D5    : Bank0Reg64<10, "D5",  [R10, R11]>, DwarfRegNum<[10]>;
  def D6    : Bank0Reg64<12, "D6",  [R12, R13]>, DwarfRegNum<[12]>;
  def D7    : Bank0Reg64<14, "D7",  [R14, R15]>, DwarfRegNum<[14]>;
  def D8    : Bank0Reg64<16, "D8",  [R16, R17]>, DwarfRegNum<[16]>;
  def D9    : Bank0Reg64<18, "D9",  [R18, R19]>, DwarfRegNum<[18]>;
  def D10   : Bank0Reg64<20, "D10", [R20, R21]>, DwarfRegNum<[20]>;
  def D11   : Bank0Reg64<22, "D11", [R22, R23]>, DwarfRegNum<[22]>;
  def D12   : Bank0Reg64<24, "D12", [R24, R25]>, DwarfRegNum<[24]>;
  def D13   : Bank0Reg64<26, "D13", [R26, R27]>, DwarfRegNum<[26]>;
  def D14   : Bank0Reg64<28, "D14", [R28, R29]>, DwarfRegNum<[28]>;
  def D15   : Bank0Reg64<30, "D15", [R30, R31]>, DwarfRegNum<[30]>;
  def D16   : Bank0Reg64<32, "D16", [R32, R33]>, DwarfRegNum<[32]>;
  def D17   : Bank0Reg64<34, "D17", [R34, R35]>, DwarfRegNum<[34]>;
  def D18   : Bank0Reg64<36, "D18", [R36, R37]>, DwarfRegNum<[36]>;
  def D19   : Bank0Reg64<38, "D19", [R38, R39]>, DwarfRegNum<[38]>;
  def D20   : Bank0Reg64<40, "D20", [R40, R41]>, DwarfRegNum<[40]>;
  def D21   : Bank0Reg64<42, "D21", [R42, R43]>, DwarfRegNum<[42]>;
  def D22   : Bank0Reg64<44, "D22", [R44, R45]>, DwarfRegNum<[44]>;
  def D23   : Bank0Reg64<46, "D23", [R46, R47]>, DwarfRegNum<[46]>;
  def D24   : Bank0Reg64<48, "D24", [R48, R49]>, DwarfRegNum<[48]>;
  def D25   : Bank0Reg64<50, "D25", [R50, R51]>, DwarfRegNum<[50]>;
  def D26   : Bank0Reg64<52, "D26", [R52, R53]>, DwarfRegNum<[52]>;
  def D27   : Bank0Reg64<54, "D27", [R54, R55]>, DwarfRegNum<[54]>;
  def D28   : Bank0Reg64<56, "D28", [R56, R57]>, DwarfRegNum<[56]>;
  def D29   : Bank0Reg64<58, "D29", [R58, R59]>, DwarfRegNum<[58]>;
  def D30   : Bank0Reg64<60, "D30", [R60, R61]>, DwarfRegNum<[60]>;
  def D31   : Bank0Reg64<62, "D31", [R62, R63]>, DwarfRegNum<[62]>;

// Aliases
  def A1    : Bank0Reg32<0,  "A1">,  DwarfRegAlias<R0>;
  def A2    : Bank0Reg32<1,  "A2">,  DwarfRegAlias<R1>;
  def A3    : Bank0Reg32<2,  "A3">,  DwarfRegAlias<R2>;
  def A4    : Bank0Reg32<3,  "A4">,  DwarfRegAlias<R3>;
  def V1    : Bank0Reg32<4,  "V1">,  DwarfRegAlias<R4>;
  def V2    : Bank0Reg32<5,  "V2">,  DwarfRegAlias<R5>;
  def V3    : Bank0Reg32<6,  "V3">,  DwarfRegAlias<R6>;
  def V4    : Bank0Reg32<7,  "V4">,  DwarfRegAlias<R7>;
  def V5    : Bank0Reg32<8,  "V5">,  DwarfRegAlias<R8>;
  def SB    : Bank0Reg32<9,  "SB">,  DwarfRegAlias<R9>;
  def SL    : Bank0Reg32<10, "SL">,  DwarfRegAlias<R10>;
  def V8    : Bank0Reg32<11, "V8">,  DwarfRegAlias<R11>;
  def IP    : Bank0Reg32<12, "IP">,  DwarfRegAlias<R12>;
  def SP    : Bank0Reg32<13, "SP">,  DwarfRegAlias<R13>;
  def LR    : Bank0Reg32<14, "LR">,  DwarfRegAlias<R14>;
  def FP    : Bank0Reg32<15, "FP">,  DwarfRegAlias<R15>;
  def ZERO  : Bank0Reg32<31, "ZERO">, DwarfRegAlias<R31>;
}

// Bank1 registers
let Namespace = "Epiphany" in {
  def CONFIG      : Bank1Reg32<0,  "CONFIG">,      DwarfRegNum<[100]>;
  def STATUS      : Bank1Reg32<1,  "STATUS">,      DwarfRegNum<[101]>;
  def PC          : Bank1Reg32<2,  "PC">,          DwarfRegNum<[102]>;
  def DEBUGSTATUS : Bank1Reg32<3,  "DEBUGSTATUS">, DwarfRegNum<[103]>;
  def LC          : Bank1Reg32<4,  "LC">,          DwarfRegNum<[104]>;
  def LS          : Bank1Reg32<5,  "LS">,          DwarfRegNum<[105]>;
  def LE          : Bank1Reg32<6,  "LE">,          DwarfRegNum<[106]>;
  def IRET        : Bank1Reg32<7,  "IRET">,        DwarfRegNum<[107]>;
  def IMASK       : Bank1Reg32<8,  "IMASK">,       DwarfRegNum<[108]>;
  def ILAT        : Bank1Reg32<9,  "ILAT">,        DwarfRegNum<[109]>;
  def ILATST      : Bank1Reg32<10, "ILATST">,      DwarfRegNum<[110]>;
  def ILATCL      : Bank1Reg32<11, "ILATCL">,      DwarfRegNum<[111]>;
  def IPEND       : Bank1Reg32<12, "IPEND">,       DwarfRegNum<[112]>;
  def CTIMER0     : Bank1Reg32<13, "CTIMER0">,     DwarfRegNum<[113]>;
  def CTIMER1     : Bank1Reg32<14, "CTIMER1">,     DwarfRegNum<[114]>;
  def FSTATUS     : Bank1Reg32<15, "FSTATUS">,     DwarfRegNum<[115]>;
  // 0xF0444 is empty
  def DEBUGCMD    : Bank1Reg32<17, "DEBUGCMD">,    DwarfRegNum<[117]>;
}

// DMA Registers
let Namespace = "Epiphany" in {
  def DMA0CONFIG  : Bank2Reg32<0, "DMA0CONFIG">,   DwarfRegNum<[200]>;
}

// 0xF07xx
let Namespace = "Epiphany" in {
  def RESETCORE   : Bank1Reg32<1, "RESETCORE">,   DwarfRegNum<[400]>;
}

//===----------------------------------------------------------------------===//
//@Register Classes
//===----------------------------------------------------------------------===//

// General purpose (first 7 only)
let AllocationPriority = 2, Size = 32, CopyCost = 1 in {
  def GPR16 : RegisterClass<"Epiphany", [i32, v4i8, v2i16], 32, (add (sequence "R%u", 0, 7))>;
  def FPR16 : RegisterClass<"Epiphany", [f32], 32, (add (sequence "R%u", 0, 7))>;
}

// General purpose 32 bit
def GPR32 : RegisterClass<"Epiphany", [i32, v4i8, v2i16], 32, (add
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
  IP,
  // Stack pointer
  SP,
  // Callee-saved link register
  LR,
  // Caller-saved GPR
  (sequence "R%u", 16, 27),
  // Reserved
  R28, R29, R30, ZERO,
  // Caller-saved GPR
  (sequence "R%u", 32, 63))> {
  
  // Reordering - putting GPR16 regs to the end to lower register pressure
  let AltOrders = [(add (sub GPR32, R0, R1, R2, R3, R4, R5, R6, R7, R8), R0, R1, R2, R3, R4, R5, R6, R7, R8)];
  let AltOrderSelect = [{ return 1; }];

  let AllocationPriority = 2;
  let Size = 32;
  let CopyCost = 1;
}

def FPR32 : RegisterClass<"Epiphany", [f32], 32, (add GPR32)>;

// 64 bit
def GPR64 : RegisterClass<"Epiphany", [i64,v2i32], 64, (add (sequence "D%u", 0, 31))> {
  let CopyCost = 4;
  let Size = 64;
}
def FPR64 : RegisterClass<"Epiphany", [f64,v2f32], 64, (add GPR64)> {
  let CopyCost = 4;
  let Size = 64;
}


// We should not be able to alloc special regs
let isAllocatable = 0 in {
// Status register
def SR : RegisterClass<"Epiphany", [i32], 32, (add STATUS)>;

// Special regs
def SPECIAL : RegisterClass<"Epiphany", [i32], 32, (add 
  CONFIG, 
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
}
