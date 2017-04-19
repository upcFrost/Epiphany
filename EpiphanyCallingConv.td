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
def CC_Epiphany_Assign : CallingConv<[
  // Put ByVal arguments directly on the stack. 
  CCIfByVal<CCPassByVal<4, 4>>,

  // Promote all ints to natural i32
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,
  CCIfType<[f16], CCPromoteToType<f32>>,
  // First try to assign to the first 8 regs so that we will be able to use 16-bit instr
  CCIfType<[i32,f32], CCAssignToReg<[R0, R1, R2, R3]>>,

  // If still no luck - save to stack
  CCIfType<[i32,f32], CCAssignToStack<4, 4>>,

  // Pass 64-bit within 2 first regs or in the stack
  CCIfType<[i64,f64], CCAssignToReg<[D0, D1]>>,
  CCIfType<[i64,f64], CCAssignToStack<8, 8>>,

  CCAssignToStack<8, 8>
]>;

//===----------------------------------------------------------------------===//
// Epiphany Return Value Calling Convention
//===----------------------------------------------------------------------===//
def RetCC_Epiphany : CallingConv<[
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,
  CCIfType<[f16], CCPromoteToType<f32>>,
  // Result to be returned in first 4 regs
  CCIfType<[i32,f32], CCAssignToReg<[R0, R1, R2, R3]>>,
  // Alternatively, they are assigned to the stack in 4-byte aligned units.
  CCIfType<[i32,f32], CCAssignToStack<4, 4>>,

  // Pass 64-bit within 2 first regs or in the stack
  CCIfType<[i64,f64], CCAssignToReg<[D0, D1]>>,
  CCIfType<[i64,f64], CCAssignToStack<8, 8>>,

  CCAssignToStack<8, 8>
]>;

def CSR32 : CalleeSavedRegs<(add R4, R5, R6, R7, R8, SB, SL, FP, LR, R15)>;
