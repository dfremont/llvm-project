//=== Glulx.td - Describe the Glulx Target Machine ---------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Top tablegen file that pulls in code from the LLVM framework along with
// code specific to this target.
//
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// Calling Conv, Instruction Descriptions, Register information, etc
//===----------------------------------------------------------------------===//

include "GlulxSchedule.td"
include "GlulxRegisterInfo.td"
include "GlulxInstrInfo.td"

def GlulxInstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// Glulx processors supported
//===----------------------------------------------------------------------===//

def : ProcessorModel<"generic",  NoSchedModel, []>;

//===----------------------------------------------------------------------===//
// Glulx Target declaration
//===----------------------------------------------------------------------===//

def Glulx : Target {
  let InstructionSet = GlulxInstrInfo;
}
