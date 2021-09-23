//=== GlulxInstrFormats.td - Glulx Instruction Formats -----*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

class GlulxInst<int sz, string opcodestr, string argstr,
                dag outs, dag ins, list<dag> pattern>
    : Instruction {
  field bits<32> Inst;
  // SoftFail is a field the disassembler can use to provide a way for
  // instructions to not match without killing the whole decode process. It is
  // mainly used for ARM, but Tablegen expects this field to exist or it fails
  // to build the decode table.
  field bits<32> SoftFail = 0;
  let Size = sz;

  bits<16> Opcode = 0;

  let Inst{15-0} = Opcode;

  let Namespace = "Glulx";

  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = opcodestr # "\t" # argstr;
  let Pattern = pattern;

  let Defs = [ARGUMENTS];

  //let TSFlags{4-0} = format.Value;
}

// Pseudo instructions
class Pseudo<dag outs, dag ins, list<dag> pattern, string opcodestr = "",
             string argstr = "">
    : GlulxInst<1, opcodestr, argstr, outs, ins, pattern>,
      Sched<[]> {
  let isPseudo = 1;
  let isCodeGenOnly = 1;
}

class GlulxInst1<bits<8> opcode, string opcodestr, string argstr,
                 dag outs, dag ins, list<dag> pattern = []>
    : GlulxInst<1, opcodestr, argstr, outs, ins, pattern> {
    let Opcode{7-0} = opcode;
}

class GlulxInst2<bits<16> opcode, string opcodestr, string argstr,
                 dag outs, dag ins, list<dag> pattern = []>
    : GlulxInst<2, opcodestr, argstr, outs, ins, pattern> {
    let Opcode = opcode;
}
