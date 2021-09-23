//== GlulxInstrInfo.td - Target Description for Glulx Target -*- tablegen -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Glulx implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Instruction format superclass
//===----------------------------------------------------------------------===//

// Procedure arguments
def argument : SDNode<"GlulxISD::ARGUMENT",
                      SDTypeProfile<1, 1, [SDTCisVT<1, i32>]>,
                      []>;

// Procedure return
def Glulx_ret : SDNode<"GlulxISD::Ret",
                       SDTypeProfile<0, 1, [SDTCisVT<0, i32>]>,
                       [SDNPHasChain, SDNPOptInGlue]>;

// Procedure calling
def call          : SDNode<"GlulxISD::CALL",
                           SDTypeProfile<1, -1, [SDTCisVT<0, i32>]>,
                           [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue,
                            SDNPVariadic]>;
def tailcall      : SDNode<"GlulxISD::TAILCALL",
                           SDTypeProfile<0, -1, [SDTCisVT<0, i32>]>,
                           [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue,
                            SDNPVariadic]>;
def callf         : SDNode<"GlulxISD::CALLF",
                           SDTypeProfile<1, 1, [SDTCisVT<0, i32>]>,
                           [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;
def callfi        : SDNode<"GlulxISD::CALLFI",
                           SDTypeProfile<1, 2, [SDTCisVT<0, i32>]>,
                           [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;
def callfii       : SDNode<"GlulxISD::CALLFII",
                           SDTypeProfile<1, 3, [SDTCisVT<0, i32>]>,
                           [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;
def callfiii      : SDNode<"GlulxISD::CALLFIII",
                           SDTypeProfile<1, 4, [SDTCisVT<0, i32>]>,
                           [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

// Stack manipulation for procedure calls
def push : SDNode<"GlulxISD::PUSH",
                  SDTypeProfile<0, 1, [SDTCisVT<0, i32>]>,
                  [SDNPHasChain]>;

// SELECT operations
def select_cc    : SDNode<"GlulxISD::SELECT_CC",
                          SDTypeProfile<1, 5, [SDTCisSameAs<1, 2>,
                                               SDTCisSameAs<0, 4>,
                                               SDTCisSameAs<4, 5>]>,
                          [SDNPInGlue]>;

// Global addressing
def ga_wrapper : SDNode<"GlulxISD::GA_WRAPPER",
                        SDTypeProfile<1, 1, [SDTCisSameAs<0, 1>,
                                             SDTCisPtrTy<0>]>>;

include "GlulxInstrFormats.td"

/* Arithmetic and logic */
let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in {
multiclass ALU1<bits<8> opcode, string opcodestr, SDPatternOperator OpNode> {
    def _r : GlulxInst1<opcode, opcodestr, "$rs, $rd",
                 (outs I32:$rd), (ins I32:$rs),
                 [(set I32:$rd, (OpNode I32:$rs))]>;
    def _i : GlulxInst1<opcode, opcodestr, "$rs, $rd",
                 (outs I32:$rd), (ins i32imm:$rs),
                 [(set I32:$rd, (OpNode imm:$rs))]>;
}

multiclass ALU2<bits<8> opcode, string opcodestr, SDPatternOperator OpNode> {
    def _rr : GlulxInst1<opcode, opcodestr, "$rs1, $rs2, $rd",
                 (outs I32:$rd), (ins I32:$rs1, I32:$rs2),
                 [(set I32:$rd, (OpNode I32:$rs1, I32:$rs2))]>;
    def _ri : GlulxInst1<opcode, opcodestr, "$rs1, $rs2, $rd",
                 (outs I32:$rd), (ins I32:$rs1, i32imm:$rs2),
                 [(set I32:$rd, (OpNode I32:$rs1, imm:$rs2))]>;
}
}

defm ADD  : ALU2<0x10, "add", add>;
defm SUB  : ALU2<0x11, "sub", sub>;
defm MUL  : ALU2<0x12, "mul", mul>;
defm DIV  : ALU2<0x13, "div", sdiv>;
defm MOD  : ALU2<0x14, "mod", srem>;
defm NEG  : ALU1<0x15, "neg", ineg>;
defm AND  : ALU2<0x18, "bitand", and>;
defm OR   : ALU2<0x19, "bitor", or>;
defm XOR  : ALU2<0x1a, "bitxor", xor>;
defm NOT  : ALU1<0x1b, "bitnot", not>;
defm SHL  : ALU2<0x1c, "shiftl", shl>;
defm SSHR : ALU2<0x1d, "sshiftr", sra>;
defm USHR : ALU2<0x1e, "ushiftr", srl>;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in {
multiclass FALU1<bits<16> opcode, string opcodestr, SDPatternOperator OpNode> {
    def _r : GlulxInst2<opcode, opcodestr, "$rs, $rd",
                 (outs F32:$rd), (ins F32:$rs),
                 [(set F32:$rd, (OpNode F32:$rs))]>;
    def _i : GlulxInst2<opcode, opcodestr, "$rs, $rd",
                 (outs F32:$rd), (ins f32imm:$rs),
                 [(set F32:$rd, (OpNode imm:$rs))]>;
}

multiclass FALU2<bits<16> opcode, string opcodestr, SDPatternOperator OpNode> {
    def _rr : GlulxInst2<opcode, opcodestr, "$rs1, $rs2, $rd",
                 (outs F32:$rd), (ins F32:$rs1, F32:$rs2),
                 [(set F32:$rd, (OpNode F32:$rs1, F32:$rs2))]>;
    def _ri : GlulxInst2<opcode, opcodestr, "$rs1, $rs2, $rd",
                 (outs F32:$rd), (ins F32:$rs1, f32imm:$rs2),
                 [(set F32:$rd, (OpNode F32:$rs1, imm:$rs2))]>;
}
}

def NUMTOF : GlulxInst2<0x190, "numtof", "$rs, $rd",
                        (outs F32:$rd), (ins I32:$rs),
                        [(set F32:$rd, (sint_to_fp I32:$rs))]>;
def FTONUMZ : GlulxInst2<0x191, "ftonumz", "$rs, $rd",
                        (outs I32:$rd), (ins F32:$rs),
                        [(set I32:$rd, (fp_to_sint F32:$rs))]>;
// ftonumn returns an int, but fnearbyint & fround return a float...
//def FTONUMN : GlulxInst2<0x192, "ftonumn", "$rs, $rd",
//                        (outs I32:$rd), (ins F32:$rs),
//                        [(set I32:$rd, (fnearbyint F32:$rs))]>;

defm CEIL  : FALU1<0x198, "ceil", fceil>;
defm FLOOR : FALU1<0x199, "floor", ffloor>;
defm FADD  : FALU2<0x1A0, "fadd", fadd>;
defm FSUB  : FALU2<0x1A1, "fsub", fsub>;
defm FMUL  : FALU2<0x1A2, "fmul", fmul>;
defm FDIV  : FALU2<0x1A3, "fdiv", fdiv>;
defm FMOD  : FALU2<0x1A4, "fmod", frem>;
defm SQRT  : FALU1<0x1A8, "sqrt", fsqrt>;
def fexp      : SDNode<"ISD::FEXP", SDTFPUnaryOp>;
defm EXP   : FALU1<0x1A9, "exp", fexp>;
def flog      : SDNode<"ISD::FLOG", SDTFPUnaryOp>;
defm LOG   : FALU1<0x1AA, "log", flog>;
defm POW   : FALU2<0x1AB, "pow", fpow>;
defm SIN   : FALU1<0x1B0, "sin", fsin>;
defm COS   : FALU1<0x1B1, "cos", fcos>;
// no nodes for these in SelectionDAG
//defm TAN   : FALU1<0x1B2, "tan", ...>;
//defm ASIN  : FALU1<0x1B3, "asin", ...>;
//defm ACOS  : FALU1<0x1B4, "acos", ...>;
//defm ATAN  : FALU1<0x1B5, "atan", ...>;
//defm ATAN2  : FALU2<0x1B6, "atan2", ...>;

// Moves, loads, and stores

let isReMaterializable = 1, isAsCheapAsAMove = 1 in {
let isMoveReg = 1 in
def copy_rr : GlulxInst1<0x40, "copy", "$rs, $rd",
                      (outs I32:$rd), (ins I32:$rs)>;
let isMoveImm = 1 in
def copy_ri : GlulxInst1<0x40, "copy", "$src, $rd",
                      (outs I32:$rd), (ins i32imm:$src),
                      [(set I32:$rd, imm:$src)]>;
}

let hasSideEffects = 1 in
def PUSH : GlulxInst1<0, "push", "$rs",
                      (outs), (ins I32:$rs)>;

let hasSideEffects = 1 in
def PseudoPUSH : Pseudo<(outs), (ins I32:$val), [(push I32:$val)]>,
                PseudoInstExpansion<(PUSH I32:$val)>;

def immMult4 : PatLeaf<(imm), [{
  uint32_t value = (uint32_t)N->getZExtValue();
  return value%4 == 0;
}]>;
def div4_xform : SDNodeXForm<imm, [{
  // Transformation function: imm/4
  assert(N->getZExtValue() % 4 == 0);
  return CurDAG->getTargetConstant(N->getZExtValue()/4, SDLoc(N), MVT::i32);
}]>;

let mayLoad = 1 in {
def ALOAD_rr : GlulxInst1<0x48, "aload", "$addr, $offset, $rd",
    (outs I32:$rd), (ins I32:$addr, I32:$offset)>;
def ALOAD_ri : GlulxInst1<0x48, "aload", "$addr, $offset, $rd",
    (outs I32:$rd), (ins I32:$addr, i32imm:$offset)>;

def copy_mr : GlulxInst1<0x40, "copy", "@$rs, $rd",
                      (outs I32:$rd), (ins I32:$rs)>;
}
let mayStore = 1 in {
def ASTORE_rr : GlulxInst1<0x4C, "astore", "$addr, $offset, $val",
    (outs), (ins I32:$val, I32:$addr, I32:$offset)>;
def ASTORE_ri : GlulxInst1<0x4C, "astore", "$addr, $offset, $val",
    (outs), (ins I32:$val, I32:$addr, i32imm:$offset)>;

def copy_rm : GlulxInst1<0x40, "copy", "$rs, @$rd",
                      (outs), (ins I32:$rs, I32:$rd)>;
}

def : Pat<(load (add (ga_wrapper tglobaladdr:$in), immMult4:$offset)),
          (ALOAD_ri tglobaladdr:$in, (div4_xform imm:$offset))>;
def : Pat<(load (ga_wrapper tglobaladdr:$in)),
          (copy_mr tglobaladdr:$in)>;
def : Pat<(load I32:$addr), (ALOAD_ri i32:$addr, 0)>;

def : Pat<(store i32:$val, (add (ga_wrapper tglobaladdr:$in), immMult4:$offset)),
          (ASTORE_ri i32:$val, tglobaladdr:$in, (div4_xform imm:$offset))>;
def : Pat<(store i32:$val, (ga_wrapper tglobaladdr:$addr)),
          (copy_rm i32:$val, tglobaladdr:$addr)>;
def : Pat<(store i32:$val, I32:$addr), (ASTORE_ri i32:$val, i32:$addr, 0)>;

def : Pat<(i32 (ga_wrapper tglobaladdr:$addr)),
          (copy_ri tglobaladdr:$addr)>;

// Branches

def brtarget : Operand<OtherVT>;

let isBranch = 1, isTerminator = 1 in
def JUMP : GlulxInst1<0x20, "jump", "$dst",
                      (outs), (ins brtarget:$dst),
                      [(br bb:$dst)]>;

multiclass J<int Opc, string OpcodeStr, PatFrag cond> {
    def _rr : GlulxInst1<Opc, OpcodeStr, "$v1, $v2, $dst",
                 (outs), (ins I32:$v1, I32:$v2, brtarget:$dst),
                 [(brcond (i32 (cond I32:$v1, I32:$v2)), bb:$dst)]>;
    def _ri : GlulxInst1<Opc, OpcodeStr, "$v1, $v2, $dst",
                 (outs), (ins I32:$v1, i32imm:$v2, brtarget:$dst),
                 [(brcond (i32 (cond I32:$v1, imm:$v2)), bb:$dst)]>;
}
multiclass FJ<int Opc, string OpcodeStr, PatFrag cond> {
    def _rr : GlulxInst2<Opc, OpcodeStr, "$v1, $v2, $dst",
                 (outs), (ins F32:$v1, F32:$v2, brtarget:$dst),
                 [(brcond (i32 (cond F32:$v1, F32:$v2)), bb:$dst)]>;
    def _ri : GlulxInst2<Opc, OpcodeStr, "$v1, $v2, $dst",
                 (outs), (ins F32:$v1, f32imm:$v2, brtarget:$dst),
                 [(brcond (i32 (cond F32:$v1, fpimm:$v2)), bb:$dst)]>;
}

let isBranch = 1, isTerminator = 1 in {
defm JEQ  : J<0x24, "jeq", seteq>;
defm JNE  : J<0x25, "jne", setne>;
defm JLT  : J<0x26, "jlt", setlt>;
defm JGE  : J<0x27, "jge", setge>;
defm JGT  : J<0x28, "jgt", setgt>;
defm JLE  : J<0x29, "jle", setle>;
defm JLTU : J<0x2a, "jltu", setult>;
defm JGEU : J<0x2b, "jgeu", setuge>;
defm JGTU : J<0x2c, "jgtu", setugt>;
defm JLEU : J<0x2d, "jleu", setule>;

defm JFEQ  : FJ<0x1C0, "jfeq", setoeq>;
defm JFNE  : FJ<0x1C1, "jfne", setune>;
defm JFLT  : FJ<0x1C2, "jflt", setolt>;
defm JFLE  : FJ<0x1C3, "jfle", setole>;
defm JFGT  : FJ<0x1C4, "jfgt", setogt>;
defm JFGE  : FJ<0x1C5, "jfge", setoge>;
}

// Selects

let usesCustomInserter = 1 in {
def SELECT: Pseudo<(outs I32:$dst),
                   (ins I32:$lhs, I32:$rhs, i32imm:$imm, I32:$src1, I32:$src2),
                   [(set i32:$dst, (select_cc i32:$lhs, i32:$rhs,
                                   (i32 imm:$imm), i32:$src1, i32:$src2))],
                   "# Select PSEUDO $dst = $lhs $imm $rhs ? $src1 : $src2">;
}

// Functions, calls, and returns

let isCodeGenOnly = 1 in
def MAKE_LFUNC : GlulxInst<0, "!lfunc", "$locno",
    (outs), (ins i32imm:$locno), []>;

let hasSideEffects = 1, isCodeGenOnly = 1, Defs = []<Register>,
    Uses = [ARGUMENTS] in {
def ARGUMENT_I32 : GlulxInst<0, "ARGUMENT_I32", "$argno -> $v",
    (outs I32:$v), (ins i32imm:$argno),
    [(set (i32 I32:$v), (argument timm:$argno))]>;
def ARGUMENT_F32 : GlulxInst<0, "ARGUMENT_F32", "$argno -> $v",
    (outs F32:$v), (ins i32imm:$argno),
    [(set (f32 F32:$v), (argument timm:$argno))]>;
}

let isReturn = 1, isBarrier = 1, isTerminator = 1 in {
def RET_r : GlulxInst1<0x31, "return", "$rs",
                    (outs), (ins I32:$rs),
                    [(Glulx_ret I32:$rs)]>;
def RET_i : GlulxInst1<0x31, "return", "$rs",
                    (outs), (ins i32imm:$rs),
                    [(Glulx_ret imm:$rs)]>;
}

let isCall = 1 in {
def CALL : GlulxInst1<0x30, "call", "$addr, $args, $ret",
                      (outs I32:$ret), (ins I32:$addr, I32:$args)>;

def TAILCALL : GlulxInst1<0x34, "tailcall", "$addr, $args",
                          (outs), (ins I32:$addr, I32:$args)>;

def CALLF : GlulxInst2<0x160, "callf", "$addr, $ret",
                       (outs R32:$ret), (ins R32:$addr)>;
def CALLFI : GlulxInst2<0x161, "callfi", "$addr, $arg1, $ret",
                       (outs R32:$ret), (ins R32:$addr, R32:$arg1)>;
def CALLFII : GlulxInst2<0x162, "callfii", "$addr, $arg1, $arg2, $ret",
                         (outs R32:$ret),
                         (ins R32:$addr, R32:$arg1, R32:$arg2)>;
def CALLFIII : GlulxInst2<0x163, "callfiii", "$addr, $arg1, $arg2, $arg3, $ret",
                         (outs R32:$ret),
                         (ins R32:$addr, R32:$arg1, R32:$arg2, R32:$arg3)>;
}

multiclass CallPat<SDPatternOperator OpNode, GlulxInst Inst> {
    def : Pat<(OpNode tglobaladdr:$addr, i32:$args),
              (Inst tglobaladdr:$addr, i32:$args)>;
    def : Pat<(OpNode texternalsym:$addr, i32:$args),
              (Inst texternalsym:$addr, i32:$args)>;
}
multiclass CallfiPat<ValueType opt> {
    def : Pat<(callfi tglobaladdr:$addr, opt:$arg1),
              (CALLFI tglobaladdr:$addr, opt:$arg1)>;
    def : Pat<(callfi texternalsym:$addr, opt:$arg1),
              (CALLFI texternalsym:$addr, opt:$arg1)>;
}
multiclass CallfiiPat<ValueType opt1, ValueType opt2> {
    def : Pat<(callfii tglobaladdr:$addr, opt1:$arg1, opt2:$arg2),
              (CALLFII tglobaladdr:$addr, opt1:$arg1, opt2:$arg2)>;
    def : Pat<(callfii texternalsym:$addr, opt1:$arg1, opt2:$arg2),
              (CALLFII texternalsym:$addr, opt1:$arg1, opt2:$arg2)>;
}
multiclass CallfiiiPat<ValueType opt1, ValueType opt2, ValueType opt3> {
    def : Pat<(callfiii tglobaladdr:$addr, opt1:$arg1, opt2:$arg2, opt3:$arg3),
              (CALLFIII tglobaladdr:$addr, opt1:$arg1, opt2:$arg2, opt3:$arg3)>;
    def : Pat<(callfiii texternalsym:$addr, opt1:$arg1, opt2:$arg2, opt3:$arg3),
              (CALLFIII texternalsym:$addr, opt1:$arg1, opt2:$arg2, opt3:$arg3)>;
}

defm : CallPat<call, CALL>;
defm : CallPat<tailcall, TAILCALL>;
def : Pat<(callf tglobaladdr:$addr),
          (CALLF tglobaladdr:$addr)>;
def : Pat<(callf texternalsym:$addr),
          (CALLF texternalsym:$addr)>;
defm : CallfiPat<i32>;
defm : CallfiPat<f32>;
defm : CallfiiPat<i32, i32>;
defm : CallfiiPat<f32, i32>;
defm : CallfiiPat<i32, f32>;
defm : CallfiiPat<f32, f32>;
defm : CallfiiiPat<i32, i32, i32>;
defm : CallfiiiPat<f32, i32, i32>;
defm : CallfiiiPat<i32, f32, i32>;
defm : CallfiiiPat<f32, f32, i32>;
defm : CallfiiiPat<i32, i32, f32>;
defm : CallfiiiPat<f32, i32, f32>;
defm : CallfiiiPat<i32, f32, f32>;
defm : CallfiiiPat<f32, f32, f32>;
