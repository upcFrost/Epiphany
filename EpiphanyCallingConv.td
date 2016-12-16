//==-- EpiphanyCallingConv.td - Calling Conventions for ARM ----*- tblgen -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This describes the calling conventions for Epiphany architecture.
//===----------------------------------------------------------------------===//


//===----------------------------------------------------------------------===//
// Epiphany Argument Calling Conventions
//===----------------------------------------------------------------------===//
def CC_Epiphany_AssignStack : CallingConv<[
  // Put ByVal arguments directly on the stack. 
  CCIfByVal<CCPassByVal<4, 4>>,

  CCIfType<[i8, i16], CCPromoteToType<i32>>,
  CCIfType<[i32,f32], CCAssignToReg<[R0, R1, R2, R3]>>,
  CCIfType<[i32,f32], CCAssignToStack<4, 4>>
]>;

//===----------------------------------------------------------------------===//
// Epiphany Return Value Calling Convention
//===----------------------------------------------------------------------===//
def RetCC_Epiphany : CallingConv<[
  CCIfType<[i8, i16], CCPromoteToType<i32>>,
  CCIfType<[i32,f32], CCAssignToReg<[R0, R1, R2, R3]>>
]>;

def CSR32 : CalleeSavedRegs<(add R4, R5, R6, R7, R8, SB, SL, FP, LR, R15,
                            (sequence "R%u", 32, 43))>;
