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
                  SDTypeProfile<0, 1, []>,
                  [SDNPHasChain]>;

// SELECT operations
def select_cc    : SDNode<"GlulxISD::SELECT_CC",
                          SDTypeProfile<1, 5, [SDTCisSameAs<1, 2>,
                                               SDTCisSameAs<0, 4>,
                                               SDTCisSameAs<4, 5>]>,
                          [SDNPInGlue]>;

// Branches
def jisnan : SDNode<"GlulxISD::JISNAN",
                    SDTypeProfile<0, 2, [SDTCisVT<0, f32>,
                                         SDTCisVT<1, OtherVT>]>,
                    [SDNPHasChain]>;
def jordered : SDNode<"GlulxISD::JORDERED",
                      SDTypeProfile<0, 3, [SDTCisVT<0, f32>,
                                           SDTCisVT<1, f32>,
                                           SDTCisVT<2, OtherVT>]>,
                      [SDNPHasChain]>;
def brcc_fp : SDNode<"GlulxISD::BR_CC_FP", SDTBrCC, [SDNPHasChain]>;

// Global addressing
def ga_wrapper : SDNode<"GlulxISD::GA_WRAPPER",
                        SDTypeProfile<1, 1, [SDTCisSameAs<0, 1>,
                                             SDTCisPtrTy<0>]>>;

// Memory block copy and clear
def memcpy : SDNode<"GlulxISD::MEMCPY",
                    SDTypeProfile<0, 3, [SDTCisVT<0, i32>,
                                         SDTCisPtrTy<1>,
                                         SDTCisPtrTy<2>]>,
                    [SDNPHasChain, SDNPMayStore, SDNPMayLoad]>;
def memclr : SDNode<"GlulxISD::MEMCLR",
                    SDTypeProfile<0, 2, [SDTCisVT<0, i32>,
                                         SDTCisPtrTy<1>]>,
                    [SDNPHasChain, SDNPMayStore]>;

include "GlulxInstrFormats.td"

def I32 : PatLeaf<(i32 GPR:$R)>;
def F32 : PatLeaf<(f32 GPR:$R)>;

def anyi32 : ComplexPattern<i32, 1, "SelectAny", []>;
def anyf32 : ComplexPattern<f32, 1, "SelectAny", []>;
def addr : ComplexPattern<iPTR, 1, "SelectAny", []>;

/* Arithmetic and logic */
let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class ALU1<bits<8> opcode, string opcodestr, SDPatternOperator OpNode>
    : GlulxInst1<opcode, opcodestr, "$rs, $rd",
                 (outs GPR:$rd), (ins GPR:$rs),
                 [(set I32:$rd, (OpNode anyi32:$rs))]>;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class ALU2<bits<8> opcode, string opcodestr, SDPatternOperator OpNode>
    : GlulxInst1<opcode, opcodestr, "$rs1, $rs2, $rd",
                 (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                 [(set I32:$rd, (OpNode anyi32:$rs1, anyi32:$rs2))]>;

let isCommutable = 1 in
def ADD  : ALU2<0x10, "add", add>;
def SUB  : ALU2<0x11, "sub", sub>;
let isCommutable = 1 in
def MUL  : ALU2<0x12, "mul", mul>;
def DIV  : ALU2<0x13, "div", sdiv>;
def MOD  : ALU2<0x14, "mod", srem>;
def NEG  : ALU1<0x15, "neg", ineg>;
let isCommutable = 1 in {
def AND  : ALU2<0x18, "bitand", and>;
def OR   : ALU2<0x19, "bitor", or>;
def XOR  : ALU2<0x1a, "bitxor", xor>;
}
def NOT  : ALU1<0x1b, "bitnot", not>;
def SHL  : ALU2<0x1c, "shiftl", shl>;
def SSHR : ALU2<0x1d, "sshiftr", sra>;
def USHR : ALU2<0x1e, "ushiftr", srl>;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in {
class FALU1<bits<16> opcode, string opcodestr, SDPatternOperator OpNode>
    : GlulxInst2<opcode, opcodestr, "$rs, $rd",
                 (outs GPR:$rd), (ins GPR:$rs),
                 [(set F32:$rd, (OpNode anyf32:$rs))]>;

class FALU2<bits<16> opcode, string opcodestr, SDPatternOperator OpNode>
    : GlulxInst2<opcode, opcodestr, "$rs1, $rs2, $rd",
                 (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                 [(set F32:$rd, (OpNode anyf32:$rs1, anyf32:$rs2))]>;
}

def NUMTOF : GlulxInst2<0x190, "numtof", "$rs, $rd",
                        (outs GPR:$rd), (ins GPR:$rs),
                        [(set F32:$rd, (sint_to_fp I32:$rs))]>;
def FTONUMZ : GlulxInst2<0x191, "ftonumz", "$rs, $rd",
                        (outs GPR:$rd), (ins GPR:$rs),
                        [(set I32:$rd, (fp_to_sint F32:$rs))]>;
// ftonumn returns an int, but fnearbyint & fround return a float...
//def FTONUMN : GlulxInst2<0x192, "ftonumn", "$rs, $rd",
//                        (outs GPR:$rd), (ins GPR:$rs),
//                        [(set I32:$rd, (fnearbyint F32:$rs))]>;

def CEIL  : FALU1<0x198, "ceil", fceil>;
def FLOOR : FALU1<0x199, "floor", ffloor>;
let isCommutable = 1 in
def FADD  : FALU2<0x1A0, "fadd", fadd>;
def FSUB  : FALU2<0x1A1, "fsub", fsub>;
let isCommutable = 1 in
def FMUL  : FALU2<0x1A2, "fmul", fmul>;
def FDIV  : FALU2<0x1A3, "fdiv", fdiv>;
def FMOD  : FALU2<0x1A4, "fmod", frem>;
def SQRT  : FALU1<0x1A8, "sqrt", fsqrt>;
def fexp      : SDNode<"ISD::FEXP", SDTFPUnaryOp>;
def EXP   : FALU1<0x1A9, "exp", fexp>;
def flog      : SDNode<"ISD::FLOG", SDTFPUnaryOp>;
def LOG   : FALU1<0x1AA, "log", flog>;
def POW   : FALU2<0x1AB, "pow", fpow>;
def SIN   : FALU1<0x1B0, "sin", fsin>;
def COS   : FALU1<0x1B1, "cos", fcos>;
// no nodes for these in SelectionDAG
def TAN   : FALU1<0x1B2, "tan", int_glulx_tan>;
def ASIN  : FALU1<0x1B3, "asin", int_glulx_asin>;
def ACOS  : FALU1<0x1B4, "acos", int_glulx_acos>;
def ATAN  : FALU1<0x1B5, "atan", int_glulx_atan>;
def ATAN2  : FALU2<0x1B6, "atan2", int_glulx_atan2>;

// Moves, loads, and stores

let isReMaterializable = 1, isAsCheapAsAMove = 1 in {
let isMoveReg = 1 in
def copy_rr : GlulxInst1<0x40, "copy", "$rs, $rd",
                      (outs GPR:$rd), (ins GPR:$rs)>;
let isMoveImm = 1 in {
def copy_ri : GlulxInst1<0x40, "copy", "$src, $rd",
                      (outs GPR:$rd), (ins i32imm:$src),
                      [(set I32:$rd, imm:$src)]>;
def copy_rf : GlulxInst1<0x40, "copy", "$src, $rd",
                      (outs GPR:$rd), (ins f32imm:$src),
                      [(set F32:$rd, fpimm:$src)]>;
}
}

def : Pat<(i32 (bitconvert F32:$v)), (I32:$v)>;
def : Pat<(f32 (bitconvert I32:$v)), (F32:$v)>;

// Stack manipulation

let hasSideEffects = 1 in
def PUSH : GlulxInst1<0, "push", "$rs",
                      (outs), (ins GPR:$rs)>;

foreach ot = [anyi32, anyf32] in {
def : Pat<(push ot:$val), (PUSH ot:$val)>;
def : Pat<(int_glulx_push ot:$val), (PUSH ot:$val)>;
}

let hasSideEffects = 1 in
def POP : GlulxInst1<0x40, "pop", "$rd",
                     (outs GPR:$rd), (ins),
                     [(set I32:$rd, (int_glulx_pop))]>;

// Utilities for working with address offsets

def offset : Operand<i32>;

def immMult4 : PatLeaf<(imm), [{
  uint32_t value = (uint32_t)N->getZExtValue();
  return value%4 == 0;
}]>;
def immMult2 : PatLeaf<(imm), [{
  uint32_t value = (uint32_t)N->getZExtValue();
  return value%2 == 0;
}]>;
def div4_xform : SDNodeXForm<imm, [{
  // Transformation function: imm/4
  assert(N->getZExtValue() % 4 == 0);
  return CurDAG->getTargetConstant(N->getZExtValue()/4, SDLoc(N), MVT::i32);
}]>;
def div2_xform : SDNodeXForm<imm, [{
  // Transformation function: imm/2
  assert(N->getZExtValue() % 2 == 0);
  return CurDAG->getTargetConstant(N->getZExtValue()/2, SDLoc(N), MVT::i32);
}]>;
def nop_xform : SDNodeXForm<imm, [{(void) N; return V;}]>;

// Loads

let mayLoad = 1, canFoldAsLoad = 1 in
def copy_mr : GlulxInst1<0x40, "copy", "@$rs, $rd",
                         (outs GPR:$rd), (ins GPR:$rs)>;
// No copys_mr and copyb_mr since copys and copyb cannot reliably load
// into 4-byte locals in Glulx (see Glulx Spec section 2.3).
foreach VT = [i32, f32] in
def : Pat<(VT (load (ga_wrapper tglobaladdr:$in))),
          (VT (copy_mr tglobaladdr:$in))>;

multiclass ALOADPats<int opc, string opcodestr, list<SDPatternOperator> loadTys,
                     SDPatternOperator evenImm, SDNodeXForm immXform,
                     int shift, GlulxInst inst, ValueType VT> {
    foreach loadTy = loadTys in {
    def : Pat<(VT (loadTy addr:$addr)),
              (VT (inst addr:$addr, 0))>;
    def : Pat<(VT (loadTy (add anyi32:$in, evenImm:$offset))),
              (VT (inst anyi32:$in, (immXform imm:$offset)))>;
    def : Pat<(VT (loadTy (add anyi32:$in, (shl anyi32:$index, shift)))),
              (VT (inst anyi32:$in, anyi32:$index))>;
    def : Pat<(VT (loadTy (add anyi32:$in, (mul anyi32:$index, !shl(1, shift))))),
              (VT (inst anyi32:$in, anyi32:$index))>;
    }
}

multiclass ALOAD_C<int opc, string opcodestr, list<SDPatternOperator> loadTys,
                   SDPatternOperator evenImm, SDNodeXForm immXform,
                   int shift> {
    let mayLoad = 1, UseNamedOperandTable = 1 in
    def NAME : GlulxInst<opc, opcodestr, "$addr, $offset, $rd",
                         (outs GPR:$rd), (ins GPR:$addr, offset:$offset)>;
    defvar inst = !cast<GlulxInst>(NAME);
    defm : ALOADPats<opc, opcodestr, loadTys, evenImm, immXform, shift, inst, i32>;
    defm : ALOADPats<opc, opcodestr, loadTys, evenImm, immXform, shift, inst, f32>;
}

defm ALOAD : ALOAD_C<0x48, "aload", [load], immMult4, div4_xform, 2>;
defm ALOADS : ALOAD_C<0x49, "aloads", [extloadi16, zextloadi16], immMult2, div2_xform, 1>;
defm ALOADB : ALOAD_C<0x4A, "aloadb", [extloadi8, zextloadi8], imm, nop_xform, 0>;

// Exceptional pattern for ALOADB, which doesn't have a scale factor for offsets.
foreach VT = [i32, f32] in {
foreach loadTy = [extloadi8, zextloadi8] in
def : Pat<(VT (loadTy (add anyi32:$addr, anyi32:$offset))),
          (VT (ALOADB anyi32:$addr, anyi32:$offset))>;
}

// Stores

let mayStore = 1 in {
def copy_rm : GlulxInst1<0x40, "copy", "$rs, @$rd",
                      (outs), (ins GPR:$rs, GPR:$rd)>;
// No copys_rm and copyb_rm since copys and copyb cannot reliably store
// into 4-byte locals in Glulx (see Glulx Spec section 2.3).
def copys_im : GlulxInst<0x41, "copys", "$rs, @$rd",
                         (outs), (ins i32imm:$rs, GPR:$rd)>;
def copyb_im : GlulxInst<0x42, "copyb", "$rs, @$rd",
                         (outs), (ins i32imm:$rs, GPR:$rd)>;
}

foreach ot = [anyi32, anyf32] in
def : Pat<(store ot:$val, (ga_wrapper tglobaladdr:$addr)),
          (copy_rm ot:$val, addr:$addr)>;
foreach ot = [imm, fpimm] in {
def : Pat<(truncstorei16 ot:$val, (ga_wrapper tglobaladdr:$addr)),
          (copys_im ot:$val, addr:$addr)>;
def : Pat<(truncstorei8 ot:$val, (ga_wrapper tglobaladdr:$addr)),
          (copyb_im ot:$val, addr:$addr)>;
}

multiclass ASTOREPats<int opc, string opcodestr, SDPatternOperator storeTy,
                       SDPatternOperator evenImm, SDNodeXForm immXform,
                       int shift,
                       GlulxInst inst, ComplexPattern valTy> {
    def : Pat<(storeTy valTy:$val, addr:$addr),
              (inst valTy:$val, addr:$addr, 0)>;
    def : Pat<(storeTy valTy:$val, (add anyi32:$in, evenImm:$offset)),
              (inst valTy:$val, anyi32:$in, (immXform imm:$offset))>;
    def : Pat<(storeTy valTy:$val, (add anyi32:$in, (shl anyi32:$index, shift))),
              (inst valTy:$val, anyi32:$in, anyi32:$index)>;
    def : Pat<(storeTy valTy:$val, (add anyi32:$in, (mul anyi32:$index, !shl(1, shift)))),
              (inst valTy:$val, anyi32:$in, anyi32:$index)>;
}

multiclass ASTORE_C<int opc, string opcodestr, SDPatternOperator storeTy,
                    SDPatternOperator evenImm, SDNodeXForm immXform,
                    int shift> {
    let mayStore = 1, UseNamedOperandTable = 1 in
    def NAME : GlulxInst<opc, opcodestr, "$addr, $offset, $val",
                        (outs), (ins GPR:$val, GPR:$addr, offset:$offset)>;
    defvar inst = !cast<GlulxInst>(NAME);
    defm : ASTOREPats<opc, opcodestr, storeTy, evenImm, immXform, shift, inst, anyi32>;
    defm : ASTOREPats<opc, opcodestr, storeTy, evenImm, immXform, shift, inst, anyf32>;
}

defm ASTORE : ASTORE_C<0x4C, "astore", store, immMult4, div4_xform, 2>;
defm ASTORES : ASTORE_C<0x4D, "astores", truncstorei16, immMult2, div2_xform, 1>;
defm ASTOREB : ASTORE_C<0x4E, "astoreb", truncstorei8, imm, nop_xform, 0>;

// Exceptional pattern for ASTOREB, which doesn't have a scale factor for offsets.
foreach valTy = [anyi32, anyf32] in
def : Pat<(truncstorei8 valTy:$val, (add anyi32:$addr, anyi32:$offset)),
          (ASTOREB valTy:$val, anyi32:$addr, anyi32:$offset)>;

def SEXS : GlulxInst1<0x44, "sexs", "$val, $res",
    (outs GPR:$res), (ins GPR:$val),
    [(set I32:$res, (sext_inreg I32:$val, i16))]>;
def SEXB : GlulxInst1<0x45, "sexb", "$val, $res",
    (outs GPR:$res), (ins GPR:$val),
    [(set I32:$res, (sext_inreg I32:$val, i8))]>;

def : Pat<(i32 (ga_wrapper tglobaladdr:$addr)),
          (copy_ri tglobaladdr:$addr)>;

// Branches

def brtarget : Operand<OtherVT>;

let isBranch = 1, isTerminator = 1, isBarrier = 1 in {
def JUMP : GlulxInst1<0x20, "jump", "$dst",
                      (outs), (ins brtarget:$dst),
                      [(br bb:$dst)]>;
let isIndirectBranch = 1 in
def JUMPABS : GlulxInst<0x104, "jumpabs", "$dst",
                        (outs), (ins GPR:$dst),
                        [(brind i32:$dst)]>;
}

// Conditional code predicates - used for pattern matching for jump instructions
def CC_EQ  : PatLeaf<(cond), [{return (N->get() == ISD::SETEQ);}]>;
def CC_NE  : PatLeaf<(cond), [{return (N->get() == ISD::SETNE);}]>;
def CC_GE  : PatLeaf<(cond), [{return (N->get() == ISD::SETGE);}]>;
def CC_GT  : PatLeaf<(cond), [{return (N->get() == ISD::SETGT);}]>;
def CC_UGT : PatLeaf<(cond), [{return (N->get() == ISD::SETUGT);}]>;
def CC_UGE : PatLeaf<(cond), [{return (N->get() == ISD::SETUGE);}]>;
def CC_LE  : PatLeaf<(cond), [{return (N->get() == ISD::SETLE);}]>;
def CC_LT  : PatLeaf<(cond), [{return (N->get() == ISD::SETLT);}]>;
def CC_ULT : PatLeaf<(cond), [{return (N->get() == ISD::SETULT);}]>;
def CC_ULE : PatLeaf<(cond), [{return (N->get() == ISD::SETULE);}]>;
def CC_OEQ : PatLeaf<(cond), [{return (N->get() == ISD::SETOEQ);}]>;
def CC_UNE : PatLeaf<(cond), [{return (N->get() == ISD::SETUNE);}]>;
def CC_OLT : PatLeaf<(cond), [{return (N->get() == ISD::SETOLT);}]>;
def CC_OLE : PatLeaf<(cond), [{return (N->get() == ISD::SETOLE);}]>;
def CC_OGT : PatLeaf<(cond), [{return (N->get() == ISD::SETOGT);}]>;
def CC_OGE : PatLeaf<(cond), [{return (N->get() == ISD::SETOGE);}]>;

let isBranch = 1, isTerminator = 1 in {
def JZ : GlulxInst<0x22, "jz", "$val, $dst",
                   (outs), (ins GPR:$val, brtarget:$dst)>;
def JNZ : GlulxInst<0x23, "jnz", "$val, $dst",
                    (outs), (ins GPR:$val, brtarget:$dst),
                    [(brcond I32:$val, bb:$dst)]>;
}

let AddedComplexity = 1 in {    // take priority over generic patterns below
def : Pat<(brcc CC_EQ, anyi32:$val, 0, bb:$dst),
          (JZ anyi32:$val, bb:$dst)>;
def : Pat<(brcc CC_NE, anyi32:$val, 0, bb:$dst),
          (JNZ anyi32:$val, bb:$dst)>;
}

class J<int Opc, string OpcodeStr, PatFrag cond>
    : GlulxInst1<Opc, OpcodeStr, "$v1, $v2, $dst",
                 (outs), (ins GPR:$v1, GPR:$v2, brtarget:$dst),
                 [(brcc cond, anyi32:$v1, anyi32:$v2, bb:$dst)]>;

class FJ<int Opc, string OpcodeStr, PatFrag cond,
              string operandstr = "$v1, $v2, $dst">
   : GlulxInst2<Opc, OpcodeStr, operandstr,
                 (outs), (ins GPR:$v1, GPR:$v2, brtarget:$dst),
                 [(brcc_fp cond, anyf32:$v1, anyf32:$v2, bb:$dst)]>;

let isBranch = 1, isTerminator = 1 in {
def JEQ  : J<0x24, "jeq", CC_EQ>;
def JNE  : J<0x25, "jne", CC_NE>;
def JLT  : J<0x26, "jlt", CC_LT>;
def JGE  : J<0x27, "jge", CC_GE>;
def JGT  : J<0x28, "jgt", CC_GT>;
def JLE  : J<0x29, "jle", CC_LE>;
def JLTU : J<0x2a, "jltu", CC_ULT>;
def JGEU : J<0x2b, "jgeu", CC_UGE>;
def JGTU : J<0x2c, "jgtu", CC_UGT>;
def JLEU : J<0x2d, "jleu", CC_ULE>;

def JFEQ  : FJ<0x1C0, "jfeq", CC_OEQ, "$v1, $v2, 0, $dst">;
def JFNE  : FJ<0x1C1, "jfne", CC_UNE, "$v1, $v2, 0, $dst">;
def JFLT  : FJ<0x1C2, "jflt", CC_OLT>;
def JFLE  : FJ<0x1C3, "jfle", CC_OLE>;
def JFGT  : FJ<0x1C4, "jfgt", CC_OGT>;
def JFGE  : FJ<0x1C5, "jfge", CC_OGE>;

def JISNAN : GlulxInst<0x1C8, "jisnan", "$val, $dst",
                       (outs), (ins GPR:$val, brtarget:$dst),
                       [(jisnan F32:$val, bb:$dst)]>;
// TODO expose all operands in JFEQ and replace this
def JORDERED : GlulxInst<0x1C0, "jfeq", "$v1, $v1, $v2, $dst",
                       (outs), (ins GPR:$v1, GPR:$v2, brtarget:$dst),
                       [(jordered anyf32:$v1, anyf32:$v2, bb:$dst)]>;
}

// Selects

let usesCustomInserter = 1 in
def SELECT : Pseudo<(outs GPR:$dst),
                    (ins GPR:$lhs, GPR:$rhs, i32imm:$ccode, GPR:$src1, GPR:$src2)>;

foreach compTy = [anyi32, anyf32] in {
// You can't have immediates in PHI instructions, apparently, so we only
// allow registers for the values to select among.
foreach srcTy = [I32, F32] in
def : Pat<(select_cc compTy:$lhs, compTy:$rhs,
            (i32 imm:$ccode), srcTy:$src1, srcTy:$src2),
          (SELECT compTy:$lhs, compTy:$rhs,
            (i32 imm:$ccode), srcTy:$src1, srcTy:$src2)>;
}

// Functions, calls, and returns

let isCodeGenOnly = 1 in
def MAKE_LFUNC : GlulxInst<0, "!lfunc", "$locno",
    (outs), (ins i32imm:$locno), []>;

let hasSideEffects = 1, isCodeGenOnly = 1, Defs = []<Register>,
    Uses = [ARGUMENTS] in {
def ARGUMENT_I32 : GlulxInst<0, "ARGUMENT_I32", "$argno -> $v",
    (outs GPR:$v), (ins i32imm:$argno),
    [(set (I32:$v), (argument timm:$argno))]>;
def ARGUMENT_F32 : GlulxInst<0, "ARGUMENT_F32", "$argno -> $v",
    (outs GPR:$v), (ins i32imm:$argno),
    [(set (F32:$v), (argument timm:$argno))]>;
}

let isReturn = 1, isBarrier = 1, isTerminator = 1 in
def RET : GlulxInst1<0x31, "return", "$rs",
                    (outs), (ins GPR:$rs)>;
foreach ot = [i32, f32, imm, fpimm] in
def : Pat<(Glulx_ret ot:$val), (RET ot:$val)>;

let isCall = 1 in {
def CALL : GlulxInst1<0x30, "call", "$addr, $args, $ret",
                      (outs GPR:$ret), (ins GPR:$addr, GPR:$args)>;

let isTerminator = 1, isReturn = 1, isBarrier = 1 in
def TAILCALL : GlulxInst1<0x34, "tailcall", "$addr, $args",
                          (outs), (ins GPR:$addr, GPR:$args)>;
}

multiclass CallPat<SDPatternOperator OpNode, GlulxInst Inst> {
    foreach at = [tglobaladdr, texternalsym, i32] in
    def : Pat<(OpNode at:$addr, i32:$args),
              (Inst at:$addr, i32:$args)>;
}

defm : CallPat<call, CALL>;
defm : CallPat<tailcall, TAILCALL>;

let isCall = 1 in
def CALLF : GlulxInst2<0x160, "callf", "$addr, $ret",
                       (outs GPR:$ret), (ins GPR:$addr)>;
def : Pat<(callf addr:$addr), (CALLF addr:$addr)>;

// loop(s) necessary to create call patterns for all possible
// combinations of register/immediate operands
let isCall = 1 in
def CALLFI : GlulxInst2<0x161, "callfi", "$addr, $arg1, $ret",
                    (outs GPR:$ret), (ins GPR:$addr, GPR:$arg1)>;
foreach ty1 = [anyi32, anyf32] in {
    def : Pat<(callfi addr:$addr, ty1:$arg1),
              (CALLFI addr:$addr, ty1:$arg1)>;
}

let isCall = 1 in
def CALLFII : GlulxInst2<0x162, "callfii", "$addr, $arg1, $arg2, $ret",
                    (outs GPR:$ret), (ins GPR:$addr, GPR:$arg1, GPR:$arg2)>;
foreach ty1 = [anyi32, anyf32] in {
foreach ty2 = [anyi32, anyf32] in {
    def : Pat<(callfii addr:$addr, ty1:$arg1, ty2:$arg2),
              (CALLFII addr:$addr, ty1:$arg1, ty2:$arg2)>;
}
}

let isCall = 1 in
def CALLFIII : GlulxInst2<0x163, "callfiii", "$addr, $arg1, $arg2, $arg3, $ret",
                    (outs GPR:$ret), (ins GPR:$addr, GPR:$arg1, GPR:$arg2, GPR:$arg3)>;
foreach ty1 = [anyi32, anyf32] in {
foreach ty2 = [anyi32, anyf32] in {
foreach ty3 = [anyi32, anyf32] in {
    def : Pat<(callfiii addr:$addr, ty1:$arg1, ty2:$arg2, ty3:$arg3),
              (CALLFIII addr:$addr, ty1:$arg1, ty2:$arg2, ty3:$arg3)>;
}
}
}

// Memory block copy and clear

let mayLoad = 1, mayStore = 1 in
def MCOPY : GlulxInst<0x171, "mcopy", "$size, $src, $dst",
                      (outs), (ins GPR:$size, GPR:$src, GPR:$dst),
                      [(int_glulx_mcopy anyi32:$size, addr:$src, addr:$dst)]>;
def : Pat<(memcpy anyi32:$size, addr:$src, addr:$dst),
          (MCOPY anyi32:$size, addr:$src, addr:$dst)>;

let mayStore = 1 in
def MZERO : GlulxInst<0x170, "mzero", "$size, $dst",
                      (outs), (ins GPR:$size, GPR:$dst),
                      [(int_glulx_mzero anyi32:$size, addr:$dst)]>;
def : Pat<(memclr anyi32:$size, addr:$dst),
          (MZERO anyi32:$size, addr:$dst)>;

// Random number generator

let hasSideEffects = 1 in
def RANDOM : GlulxInst<0x110, "random", "$range, $res",
    (outs GPR:$res), (ins GPR:$range),
    [(set GPR:$res, (int_glulx_random anyi32:$range))]>;

let hasSideEffects = 1 in
def SETRANDOM : GlulxInst<0x111, "setrandom", "$seed",
    (outs), (ins GPR:$seed),
    [(int_glulx_setrandom anyi32:$seed)]>;

// Searching

let mayLoad = 1 in {
def LINEARSEARCH : GlulxInst<0x150, "linearsearch",
    "$key, $ksize, $start, $ssize, $num, $keyoffset, $options, $res",
    (outs GPR:$res), (ins GPR:$key, GPR:$ksize, GPR:$start, GPR:$ssize,
                          GPR:$num, GPR:$keyoffset, GPR:$options),
    [(set GPR:$res, (int_glulx_linearsearch anyi32:$key, anyi32:$ksize, addr:$start,
        anyi32:$ssize, anyi32:$num, anyi32:$keyoffset, anyi32:$options))]>;

def BINARYSEARCH : GlulxInst<0x151, "binarysearch",
    "$key, $ksize, $start, $ssize, $num, $keyoffset, $options, $res",
    (outs GPR:$res), (ins GPR:$key, GPR:$ksize, GPR:$start, GPR:$ssize,
                          GPR:$num, GPR:$keyoffset, GPR:$options),
    [(set GPR:$res, (int_glulx_binarysearch anyi32:$key, anyi32:$ksize, addr:$start,
        anyi32:$ssize, anyi32:$num, anyi32:$keyoffset, anyi32:$options))]>;

def LINKEDSEARCH : GlulxInst<0x152, "linkedsearch",
    "$key, $ksize, $start, $keyoffset, $nextoffset, $options, $res",
    (outs GPR:$res), (ins GPR:$key, GPR:$ksize, GPR:$start, GPR:$keyoffset,
                          GPR:$nextoffset, GPR:$options),
    [(set GPR:$res, (int_glulx_linkedsearch anyi32:$key, anyi32:$ksize, addr:$start,
        anyi32:$keyoffset, anyi32:$nextoffset, anyi32:$options))]>;
}

// Accelerated functions

let hasSideEffects = 1 in {
def ACCELFUNC : GlulxInst<0x180, "accelfunc", "$num, $func",
    (outs), (ins GPR:$num, GPR:$func),
    [(int_glulx_accelfunc anyi32:$num, addr:$func)]>;

def ACCELPARAM : GlulxInst<0x181, "accelparam", "$num, $val",
    (outs), (ins GPR:$num, GPR:$val),
    [(int_glulx_accelparam anyi32:$num, anyi32:$val)]>;
}

// Multiclasses for intrinsics

multiclass Int00<int Opc, string OpcodeStr, Intrinsic intrinsic> {
    def : GlulxInst<Opc, OpcodeStr, "", (outs), (ins), [(intrinsic)]>;
}

multiclass Int01<int Opc, string OpcodeStr, Intrinsic intrinsic> {
    def _r : GlulxInst<Opc, OpcodeStr, "$in",
                       (outs), (ins GPR:$in),
                       [(intrinsic I32:$in)]>;
    def _i : GlulxInst<Opc, OpcodeStr, "$in",
                       (outs), (ins i32imm:$in),
                       [(intrinsic imm:$in)]>;
}

multiclass Int11<int Opc, string OpcodeStr, Intrinsic intrinsic,
                 string OperandStr = "$in, $out"> {
    def _r : GlulxInst<Opc, OpcodeStr, OperandStr,
                       (outs GPR:$out), (ins GPR:$in),
                       [(set I32:$out, (intrinsic I32:$in))]>;
    def _i : GlulxInst<Opc, OpcodeStr, OperandStr,
                       (outs GPR:$out), (ins i32imm:$in),
                       [(set I32:$out, (intrinsic imm:$in))]>;
}

multiclass Int02<int Opc, string OpcodeStr, Intrinsic intrinsic> {
    def _rr : GlulxInst<Opc, OpcodeStr, "$in1, $in2",
                       (outs), (ins GPR:$in1, GPR:$in2),
                       [(intrinsic I32:$in1, I32:$in2)]>;
    def _ri : GlulxInst<Opc, OpcodeStr, "$in1, $in2",
                        (outs), (ins GPR:$in1, i32imm:$in2),
                        [(intrinsic I32:$in1, imm:$in2)]>;
    def _ir : GlulxInst<Opc, OpcodeStr, "$in1, $in2",
                        (outs), (ins i32imm:$in1, GPR:$in2),
                        [(intrinsic imm:$in1, I32:$in2)]>;
    def _ii : GlulxInst<Opc, OpcodeStr, "$in1, $in2",
                        (outs), (ins i32imm:$in1, i32imm:$in2),
                        [(intrinsic imm:$in1, imm:$in2)]>;
}

multiclass Int12<int Opc, string OpcodeStr, Intrinsic intrinsic> {
    def _rr : GlulxInst<Opc, OpcodeStr, "$in1, $in2, $out",
                       (outs GPR:$out), (ins GPR:$in1, GPR:$in2),
                       [(set I32:$out, (intrinsic I32:$in1, I32:$in2))]>;
    def _ri : GlulxInst<Opc, OpcodeStr, "$in1, $in2, $out",
                        (outs GPR:$out), (ins GPR:$in1, i32imm:$in2),
                        [(set I32:$out, (intrinsic I32:$in1, imm:$in2))]>;
    def _ir : GlulxInst<Opc, OpcodeStr, "$in1, $in2, $out",
                        (outs GPR:$out), (ins i32imm:$in1, GPR:$in2),
                        [(set I32:$out, (intrinsic imm:$in1, I32:$in2))]>;
    def _ii : GlulxInst<Opc, OpcodeStr, "$in1, $in2, $out",
                        (outs GPR:$out), (ins i32imm:$in1, i32imm:$in2),
                        [(set I32:$out, (intrinsic imm:$in1, imm:$in2))]>;
}

// Integer math

def : Pat<(int_glulx_div I32:$v1, I32:$v2), (DIV I32:$v1, I32:$v2)>;
def : Pat<(int_glulx_mod I32:$v1, I32:$v2), (MOD I32:$v1, I32:$v2)>;

// Continuations

let hasSideEffects = 1, isTerminator = 1 in
def CATCH : GlulxInst<0x32, "catch", "$out, $dst",
                      (outs GPR:$out), (ins brtarget:$dst)>;
let isCodeGenOnly = 1, usesCustomInserter = 1, mayStore = 1,
    hasSideEffects = 1 in
def CATCH_INT : GlulxInst<0, "catch_intrinsic", "$out, $in",
                          (outs GPR:$out), (ins GPR:$in),
                          [(set I32:$out, (int_glulx_catch I32:$in))]>;
let isBarrier = 1, isTerminator = 1, hasSideEffects = 1 in
defm THROW : Int02<0x33, "throw", int_glulx_throw>;

// Memory allocation heap

defm MALLOC : Int11<0x178, "malloc", int_glulx_malloc>;
defm MFREE : Int01<0x179, "mfree", int_glulx_mfree>;

// Output

defm SETIOSYS : Int02<0x149, "setiosys", int_glulx_setiosys>;

defm STREAMCHAR : Int01<0x70, "streamchar", int_glulx_streamchar>;
defm STREAMNUM : Int01<0x71, "streamnum", int_glulx_streamnum>;
defm STREAMSTR : Int01<0x72, "streamstr", int_glulx_streamstr>;
defm STREAMUNICHAR : Int01<0x73, "streamunichar", int_glulx_streamunichar>;

def GETSTRINGTBL: GlulxInst<0x140, "getstringtbl", "$res",
    (outs GPR:$res), (ins),
    [(set GPR:$res, (int_glulx_getstringtbl))]>;

let hasSideEffects = 1 in
def SETSTRINGTBL : GlulxInst<0x141, "setstringtbl", "$tbl",
    (outs), (ins GPR:$tbl),
    [(int_glulx_setstringtbl addr:$tbl)]>;

// Game state

let isBarrier = 1, isTerminator = 1, hasSideEffects = 1 in {
defm QUIT : Int00<0x120, "quit", int_glulx_quit>;

def RESTART : GlulxInst<0x122, "restart", "", (outs), (ins),
    [(int_glulx_restart)]>;
}

let mayLoad = 1, mayStore = 1, hasSideEffects = 1 in {
def SAVE : GlulxInst<0x123, "save", "$stream, $res",
    (outs GPR:$res), (ins GPR:$stream),
    [(set GPR:$res, (int_glulx_save anyi32:$stream))]>;

def SAVEUNDO : GlulxInst<0x125, "saveundo", "$res",
    (outs GPR:$res), (ins),
    [(set GPR:$res, (int_glulx_saveundo))]>;
}

let hasSideEffects = 1 in {
def RESTORE : GlulxInst<0x124, "restore", "$stream, $res",
    (outs GPR:$res), (ins GPR:$stream),
    [(set GPR:$res, (int_glulx_restore anyi32:$stream))]>;

def RESTOREUNDO : GlulxInst<0x126, "restoreundo", "$res",
    (outs GPR:$res), (ins),
    [(set GPR:$res, (int_glulx_restoreundo))]>;
}

def HASUNDO : GlulxInst<0x128, "hasundo", "$res",
    (outs GPR:$res), (ins),
    [(set GPR:$res, (int_glulx_hasundo))]>;

let hasSideEffects = 1 in
def DISCARDUNDO : GlulxInst<0x129, "discardundo", "",
    (outs), (ins), [(int_glulx_discardundo)]>;

let hasSideEffects = 1 in
def PROTECT : GlulxInst<0x127, "protect", "$start, $len",
    (outs), (ins GPR:$start, GPR:$len),
    [(int_glulx_protect addr:$start, anyi32:$len)]>;

def VERIFY : GlulxInst<0x121, "verify", "$res",
    (outs GPR:$res), (ins),
    [(set GPR:$res, (int_glulx_verify))]>;

// Miscellaneous

defm GESTALT : Int12<0x100, "gestalt", int_glulx_gestalt>;
defm DEBUGTRAP : Int01<0x101, "debugtrap", int_glulx_debugtrap>;
defm GLK : Int12<0x130, "glk", int_glulx_glk>;

// Floating-point math (not already covered above)

def : Pat<(int_glulx_ceil anyf32:$v1), (CEIL anyf32:$v1)>;
def : Pat<(int_glulx_floor anyf32:$v1), (FLOOR anyf32:$v1)>;
def : Pat<(int_glulx_sqrt anyf32:$v1), (SQRT anyf32:$v1)>;
def : Pat<(int_glulx_exp anyf32:$v1), (EXP anyf32:$v1)>;
def : Pat<(int_glulx_log anyf32:$v1), (LOG anyf32:$v1)>;
def : Pat<(int_glulx_pow anyf32:$v1, anyf32:$v2), (POW anyf32:$v1, anyf32:$v2)>;
def : Pat<(int_glulx_sin anyf32:$v1), (SIN anyf32:$v1)>;
def : Pat<(int_glulx_cos anyf32:$v1), (COS anyf32:$v1)>;
