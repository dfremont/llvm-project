//===-- GlulxRegisterInfo.td - Glulx Register defs ---------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//  Declarations that describe the Glulx "register file"
//===----------------------------------------------------------------------===//

class GlulxReg<string n> : Register<n> {
    let Namespace = "Glulx";
}

// Special registers used as stack & frame pointers
def VRStack  : GlulxReg<"%SP">;
def VRFrame  : GlulxReg<"%FP">;

// The register allocation framework requires register classes have at least
// one register, so we define a few for the integer / floating point register
// classes since we otherwise don't need a physical register in those classes.
def I32_0 : GlulxReg<"%i32.0">;

// The incoming arguments "register". This is an opaque entity which serves to
// order the ARGUMENT instructions that are emulating live-in registers and
// must not be scheduled below other instructions.
def ARGUMENTS : GlulxReg<"ARGUMENTS">;

//===----------------------------------------------------------------------===//
//  Register classes
//===----------------------------------------------------------------------===//

class GlulxRegClass<list<ValueType> regTypes, int alignment, dag regList>
     : RegisterClass<"Glulx", regTypes, alignment, regList>;

def GPR : GlulxRegClass<[i32, f32], 8, (add I32_0, VRStack, VRFrame)>;
