//===-- GlulxISelLowering.cpp - Glulx DAG Lowering Implementation -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that Glulx uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//
#include "GlulxISelLowering.h"
#include "GlulxSubtarget.h"
#include "GlulxTargetMachine.h"
#include "GlulxMachineFunctionInfo.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/Debug.h"
#include <cassert>

using namespace llvm;

#define DEBUG_TYPE "Glulx-isellower"

GlulxTargetLowering::GlulxTargetLowering(const TargetMachine &TM,
                                         const GlulxSubtarget &STI)
    : TargetLowering(TM), Subtarget(STI)
{
  // Set up the register classes
  addRegisterClass(MVT::i32, &Glulx::GPRRegClass);
  addRegisterClass(MVT::f32, &Glulx::GPRRegClass);

  // Must, computeRegisterProperties - Once all of the register classes are
  // added, this allows us to compute derived properties we expose.
  computeRegisterProperties(Subtarget.getRegisterInfo());

  // Set scheduling preference.
  setSchedulingPreference(Sched::RegPressure);

  // Use i32 for setcc operations results (slt, sgt, ...).
  setBooleanContents(ZeroOrOneBooleanContent);
  setBooleanVectorContents(ZeroOrOneBooleanContent);

  setOperationAction(ISD::GlobalAddress, MVT::i32, Custom);
  setOperationAction(ISD::BlockAddress,  MVT::i32, Custom);
  setOperationAction(ISD::ExternalSymbol, MVT::i32, Custom);
  setOperationAction(ISD::FrameIndex, MVT::i32, Custom);

  setOperationAction(ISD::CopyToReg, MVT::Other, Custom);

  setOperationAction(ISD::ConstantFP,  MVT::f32, Legal);

  // Expand integer operations not natively supported by Glulx.
  auto BadIntOps = {
      ISD::UDIV, ISD::UREM,
      ISD::ROTL, ISD::ROTR,
      ISD::BSWAP, ISD::CTTZ, ISD::CTLZ, ISD::CTPOP,
      ISD::SMUL_LOHI, ISD::UMUL_LOHI, ISD::SDIVREM, ISD::UDIVREM,
      ISD::MULHU, ISD::MULHS,
      ISD::SHL_PARTS, ISD::SRA_PARTS, ISD::SRL_PARTS,
      ISD::UINT_TO_FP, ISD::FP_TO_UINT,
  };
  for (auto Op : BadIntOps)
    setOperationAction(Op, MVT::i32, Expand);
  setLoadExtAction(ISD::EXTLOAD, MVT::i32, MVT::i1, Promote);
  setLoadExtAction(ISD::SEXTLOAD, MVT::i32, MVT::i1, Promote);
  setLoadExtAction(ISD::ZEXTLOAD, MVT::i32, MVT::i1, Promote);
  setLoadExtAction(ISD::SEXTLOAD, MVT::i32, MVT::i8, Expand);
  setLoadExtAction(ISD::SEXTLOAD, MVT::i32, MVT::i16, Expand);

  // Expand FP operations not natively supported by Glulx.
  auto BadFloatOps = {
      ISD::FNEG, ISD::FABS, ISD::FSQRT, ISD::FSIN, ISD::FCOS,
      ISD::FSINCOS, ISD::FMA, ISD::FP16_TO_FP, ISD::FP_TO_FP16,
      ISD::FNEARBYINT,
  };
  for (auto Op : BadFloatOps)
      setOperationAction(Op, MVT::f32, Expand);
  setLoadExtAction(ISD::EXTLOAD, MVT::f32, MVT::f16, Expand);
  setTruncStoreAction(MVT::f32, MVT::f16, Expand);
  // Legalize FP operations which expand by default but are supported by Glulx.
  setOperationAction(ISD::FCEIL, MVT::f32, Legal);
  setOperationAction(ISD::FFLOOR, MVT::f32, Legal);
  setOperationAction(ISD::FEXP, MVT::f32, Legal);
  setOperationAction(ISD::FLOG, MVT::f32, Legal);
  // Custom handle some FP operations which have variants supported by Glulx.
  setOperationAction(ISD::FP_TO_SINT_SAT, MVT::f32, Custom);

  // Expand boolean operations not natively supported by Glulx.
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i1, Expand);

  // Dynamic stack allocation is not allowed.
  setOperationAction(ISD::STACKSAVE, MVT::Other, Custom);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Custom);
  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i32, Custom);

  // Convert select/setcc to select_cc, then custom handle that.
  for (auto T : {MVT::i32, MVT::f32}) {
    setOperationAction(ISD::SELECT, T, Expand);
    setOperationAction(ISD::SETCC, T, Expand);
    setOperationAction(ISD::SELECT_CC, T, Custom);
  }
  // LegalizeDAG apparently can't legalize our missing BR_CC forms for floating-
  // point comparisons, so we have to legalize them ourselves.
  setOperationAction(ISD::BR_CC, MVT::f32, Custom);

  // Expand jump tables.
  setOperationAction(ISD::JumpTable, MVT::Other, Expand);
  setOperationAction(ISD::BR_JT, MVT::Other, Expand);
  setOperationAction(ISD::BRIND, MVT::Other, Expand);

  // Take the default expansion for va_arg, va_copy, and va_end. There is no
  // default action for va_start, so we do that custom.
  setOperationAction(ISD::VASTART, MVT::Other, Custom);
  setOperationAction(ISD::VAARG, MVT::Other, Expand);
  setOperationAction(ISD::VACOPY, MVT::Other, Expand);
  setOperationAction(ISD::VAEND, MVT::Other, Expand);

  // Legalize traps (we'll emit @quit / @debugtrap).
  setOperationAction(ISD::TRAP, MVT::Other, Legal);
  setOperationAction(ISD::DEBUGTRAP, MVT::Other, Legal);

  // Always use the mcopy instruction for memcpy/memmove.
  MaxStoresPerMemcpy = MaxStoresPerMemcpyOptSize = 0;
  MaxStoresPerMemmove = MaxStoresPerMemmoveOptSize = 0;

  // Attempt to minimize selects since we don't have them.
  PredictableSelectIsExpensive = true;
}

const char *GlulxTargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  case GlulxISD::CALL: return "GlulxISD::CALL";
  case GlulxISD::TAILCALL: return "GlulxISD::TAILCALL";
  case GlulxISD::CALLF: return "GlulxISD::CALLF";
  case GlulxISD::CALLFI: return "GlulxISD::CALLFI";
  case GlulxISD::CALLFII: return "GlulxISD::CALLFII";
  case GlulxISD::CALLFIII: return "GlulxISD::CALLFIII";
  case GlulxISD::Ret: return "GlulxISD::Ret";
  case GlulxISD::PUSH: return "GlulxISD::PUSH";
  case GlulxISD::SELECT_CC: return "GlulxISD::SELECT_CC";
  case GlulxISD::GA_WRAPPER: return "GlulxISD::GA_WRAPPER";
  case GlulxISD::ARGUMENT: return "GlulxISD::ARGUMENT";
  case GlulxISD::MEMCPY: return "GlulxISD::MEMCPY";
  case GlulxISD::MEMCLR: return "GlulxISD::MEMCLR";
  case GlulxISD::JISNAN: return "GlulxISD::JISNAN";
  case GlulxISD::JORDERED: return "GlulxISD::JORDERED";
  case GlulxISD::BR_CC_FP: return "GlulxISD::BR_CC_FP";
  default:            return NULL;
  }
}

void GlulxTargetLowering::ReplaceNodeResults(SDNode *N,
                                             SmallVectorImpl<SDValue> &Results,
                                             SelectionDAG &DAG) const {
  switch (N->getOpcode()) {
  default:
    llvm_unreachable("Don't know how to custom expand this!");
  }
}

bool
GlulxTargetLowering::isLegalAddressingMode(const DataLayout &DL,
                                           const AddrMode &AM,
                                           Type *Ty, unsigned AddrSpace,
                                           Instruction *I) const {
  // No indexed addressing modes in Glulx.
  return !AM.BaseGV && !AM.BaseOffs && !AM.Scale;
}

//===----------------------------------------------------------------------===//
//@            Formal Arguments Calling Convention Implementation
//===----------------------------------------------------------------------===//

static void fail(const SDLoc &DL, SelectionDAG &DAG, const char *Msg) {
  MachineFunction &MF = DAG.getMachineFunction();
  DAG.getContext()->diagnose(
      DiagnosticInfoUnsupported(MF.getFunction(), Msg, DL.getDebugLoc()));
}

// Test whether the given calling convention is supported.
static bool callingConvSupported(CallingConv::ID CallConv) {
  // We currently support the language-independent target-independent
  // conventions. We don't yet have a way to annotate calls with properties like
  // "cold", and we don't have any call-clobbered registers, so these are mostly
  // all handled the same.
  return CallConv == CallingConv::C || CallConv == CallingConv::Fast ||
         CallConv == CallingConv::Cold ||
         CallConv == CallingConv::PreserveMost ||
         CallConv == CallingConv::PreserveAll ||
         CallConv == CallingConv::CXX_FAST_TLS;
}

/// LowerFormalArguments - transform physical registers into virtual registers
/// and generate load operations for arguments places on the stack.
SDValue GlulxTargetLowering::LowerFormalArguments(
                                    SDValue Chain,
                                    CallingConv::ID CallConv,
                                    bool IsVarArg,
                                    const SmallVectorImpl<ISD::InputArg> &Ins,
                                    const SDLoc &DL, SelectionDAG &DAG,
                                    SmallVectorImpl<SDValue> &InVals) const
{
  if (!callingConvSupported(CallConv))
    fail(DL, DAG, "unsupported CallingConv to LowerFormalArguments");

  MachineFunction &MF = DAG.getMachineFunction();
  auto *MFI = MF.getInfo<GlulxFunctionInfo>();

  // Set up the incoming ARGUMENTS value, which serves to represent the liveness
  // of the incoming values before they're represented by virtual registers.
  MF.getRegInfo().addLiveIn(Glulx::ARGUMENTS);

  for (const ISD::InputArg &In : Ins) {
    if (In.Flags.isInAlloca())
      fail(DL, DAG, "Glulx hasn't implemented inalloca arguments");
    if (In.Flags.isNest())
      fail(DL, DAG, "Glulx hasn't implemented nest arguments");
    if (In.Flags.isInConsecutiveRegs())
      fail(DL, DAG, "Glulx hasn't implemented cons regs arguments");
    if (In.Flags.isInConsecutiveRegsLast())
      fail(DL, DAG, "Glulx hasn't implemented cons regs last arguments");
    // Ignore In.getNonZeroOrigAlign() because all our arguments are passed in
    // registers.
    SDValue InVal;
    if (In.Used) {
      SDValue ArgID = DAG.getTargetConstant(InVals.size(), DL, MVT::i32);
      InVal = DAG.getNode(GlulxISD::ARGUMENT, DL, In.VT, ArgID);
    } else {
      InVal = DAG.getUNDEF(In.VT);
    }
    InVals.push_back(InVal);

    // Record the number and types of arguments.
    MFI->addParam(In.VT);
  }

  // Varargs are copied into a buffer allocated by the caller, and a pointer to
  // the buffer is passed as an argument.
  if (IsVarArg) {
    MVT PtrVT = getPointerTy(MF.getDataLayout());
    Register VarargVreg =
        MF.getRegInfo().createVirtualRegister(getRegClassFor(PtrVT));
    MFI->setVarargBufferVreg(VarargVreg);
    Chain = DAG.getCopyToReg(
        Chain, DL, VarargVreg,
        DAG.getNode(GlulxISD::ARGUMENT, DL, PtrVT,
                    DAG.getTargetConstant(Ins.size(), DL, MVT::i32)));
    MFI->addParam(PtrVT);
  }

  // Record the number and types of arguments and results.
  SmallVector<MVT, 4> Params;
  SmallVector<MVT, 4> Results;
  Glulx::computeSignatureVTs(MF.getFunction().getFunctionType(), &MF.getFunction(),
                      MF.getFunction(), DAG.getTarget(), Params, Results);
  for (MVT VT : Results)
    MFI->addResult(VT);
  assert(MFI->getParams().size() == Params.size() &&
         std::equal(MFI->getParams().begin(), MFI->getParams().end(),
                    Params.begin()));

  return Chain;
}

//===----------------------------------------------------------------------===//
//@              Return Value Calling Convention Implementation
//===----------------------------------------------------------------------===//

bool GlulxTargetLowering::CanLowerReturn(CallingConv::ID CallConv,
                                MachineFunction &MF, bool IsVarArg,
                                const SmallVectorImpl<ISD::OutputArg> &Outs,
                                LLVMContext &Context) const
{
  // Glulx can only handle returning single values.
  return Outs.size() <= 1;
}

/// LowerMemOpCallTo - Store the argument to the stack.
SDValue GlulxTargetLowering::LowerMemOpCallTo(SDValue Chain,
                                              SDValue Arg, const SDLoc &dl,
                                              SelectionDAG &DAG,
                                              const CCValAssign &VA,
                                              ISD::ArgFlagsTy Flags) const {
  llvm_unreachable("Cannot store arguments to stack");
}

SDValue GlulxTargetLowering::LowerCall(TargetLowering::CallLoweringInfo &CLI,
                                       SmallVectorImpl<SDValue> &InVals) const {
  SelectionDAG &DAG = CLI.DAG;
  SDLoc DL = CLI.DL;
  SDValue Chain = CLI.Chain;
  SDValue Callee = CLI.Callee;
  bool &IsTailCall = CLI.IsTailCall;
  bool IsVarArg = CLI.IsVarArg;
  MachineFunction &MF = DAG.getMachineFunction();
  auto Layout = MF.getDataLayout();

  CallingConv::ID CallConv = CLI.CallConv;
  if (!callingConvSupported(CallConv))
    fail(DL, DAG,
         "WebAssembly doesn't support language-specific or target-specific "
         "calling conventions yet");

  if (CLI.IsPatchPoint)
    fail(DL, DAG, "WebAssembly doesn't support patch point yet");

  // Analyze operands of the call, assigning locations to each operand.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, ArgLocs, *DAG.getContext());

  if (IsTailCall)
    IsTailCall = isEligibleForTailCallOptimization(CCInfo, CLI, MF, ArgLocs);

  if (IsTailCall) {
    auto NoTail = [&](const char *Msg) {
      if (CLI.CB && CLI.CB->isMustTailCall())
        fail(DL, DAG, Msg);
      IsTailCall = false;
    };

    // Varargs calls cannot be tail calls because the buffer is on the stack
    if (IsVarArg)
      NoTail("WebAssembly does not support varargs tail calls");

    // If pointers to local stack values are passed, we cannot tail call
    if (CLI.CB) {
      for (auto &Arg : CLI.CB->args()) {
        Value *Val = Arg.get();
        // Trace the value back through pointer operations
        while (true) {
          Value *Src = Val->stripPointerCastsAndAliases();
          if (auto *GEP = dyn_cast<GetElementPtrInst>(Src))
            Src = GEP->getPointerOperand();
          if (Val == Src)
            break;
          Val = Src;
        }
        if (isa<AllocaInst>(Val)) {
          NoTail(
              "WebAssembly does not support tail calling with stack arguments");
          break;
        }
      }
    }
  } else if (CLI.CB && CLI.CB->isMustTailCall())
    report_fatal_error("failed to perform tail call elimination on a call "
                       "site marked musttail");

  SmallVectorImpl<ISD::InputArg> &Ins = CLI.Ins;
  SmallVectorImpl<ISD::OutputArg> &Outs = CLI.Outs;
  SmallVectorImpl<SDValue> &OutVals = CLI.OutVals;

  unsigned NumFixedArgs = 0;
  for (unsigned I = 0; I < Outs.size(); ++I) {
    const ISD::OutputArg &Out = Outs[I];
    SDValue &OutVal = OutVals[I];
    if (Out.Flags.isNest())
      fail(DL, DAG, "Glulx hasn't implemented nest arguments");
    if (Out.Flags.isInAlloca())
      fail(DL, DAG, "Glulx hasn't implemented inalloca arguments");
    if (Out.Flags.isInConsecutiveRegs())
      fail(DL, DAG, "Glulx hasn't implemented cons regs arguments");
    if (Out.Flags.isInConsecutiveRegsLast())
      fail(DL, DAG, "Glulx hasn't implemented cons regs last arguments");
    if (Out.Flags.isByVal() && Out.Flags.getByValSize() != 0) {
      auto &MFI = MF.getFrameInfo();
      int FI = MFI.CreateStackObject(Out.Flags.getByValSize(),
                                     Out.Flags.getNonZeroByValAlign(),
                                     /*isSS=*/false);
      SDValue SizeNode =
          DAG.getConstant(Out.Flags.getByValSize(), DL, MVT::i32);
      SDValue FINode = DAG.getFrameIndex(FI, getPointerTy(Layout));
      Chain = DAG.getMemcpy(
          Chain, DL, FINode, OutVal, SizeNode, Out.Flags.getNonZeroByValAlign(),
          /*isVolatile*/ false, /*AlwaysInline=*/false,
          /*isTailCall*/ false, MachinePointerInfo(), MachinePointerInfo());
      OutVal = FINode;
    }
    // Count the number of fixed args *after* legalization.
    NumFixedArgs += Out.IsFixed;
  }

  if (IsVarArg) {
    // Outgoing non-fixed arguments are placed in a buffer. First
    // compute their offsets and the total amount of buffer space needed.
    for (unsigned I = NumFixedArgs; I < Outs.size(); ++I) {
      const ISD::OutputArg &Out = Outs[I];
      SDValue &Arg = OutVals[I];
      EVT VT = Arg.getValueType();
      assert(VT != MVT::iPTR && "Legalized args should be concrete");
      Type *Ty = VT.getTypeForEVT(*DAG.getContext());
      Align Alignment =
          std::max(Out.Flags.getNonZeroOrigAlign(), Layout.getABITypeAlign(Ty));
      unsigned Offset =
          CCInfo.AllocateStack(Layout.getTypeAllocSize(Ty), Alignment);
      CCInfo.addLoc(CCValAssign::getMem(ArgLocs.size(), VT.getSimpleVT(),
                                        Offset, VT.getSimpleVT(),
                                        CCValAssign::Full));
    }
  }

  unsigned NumBytes = CCInfo.getAlignedCallFrameSize();

  SDValue FINode;
  if (IsVarArg && NumBytes) {
    // For non-fixed arguments, next emit stores to store the argument values
    // to the vararg buffer at the offsets computed above.
    int FI = MF.getFrameInfo().CreateStackObject(NumBytes,
                                                 Layout.getStackAlignment(),
                                                 /*isSS=*/false);
    unsigned ValNo = 0;
    SmallVector<SDValue, 8> Chains;
    auto PtrVT = getPointerTy(Layout);
    for (SDValue Arg : drop_begin(OutVals, NumFixedArgs)) {
      assert(ArgLocs[ValNo].getValNo() == ValNo &&
             "ArgLocs should remain in order and only hold varargs args");
      unsigned Offset = ArgLocs[ValNo++].getLocMemOffset();
      FINode = DAG.getFrameIndex(FI, getPointerTy(Layout));
      SDValue Add = DAG.getNode(ISD::ADD, DL, PtrVT, FINode,
                                DAG.getConstant(Offset, DL, PtrVT));
      Chains.push_back(
          DAG.getStore(Chain, DL, Arg, Add,
                       MachinePointerInfo::getFixedStack(MF, FI, Offset)));
    }
    if (!Chains.empty())
      Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, Chains);
  } else if (IsVarArg) {
    FINode = DAG.getIntPtrConstant(0, DL);
  }

  if (Callee->getOpcode() == ISD::GlobalAddress) {
    // If the callee is a GlobalAddress node (quite common, every direct call
    // is) turn it into a TargetGlobalAddress node so that LowerGlobalAddress
    // doesn't at MO_GOT which is not needed for direct calls.
    GlobalAddressSDNode* GA = cast<GlobalAddressSDNode>(Callee);
    Callee = DAG.getTargetGlobalAddress(GA->getGlobal(), DL,
                                        getPointerTy(DAG.getDataLayout()),
                                        GA->getOffset());
  }

  unsigned NumRets = Ins.size();
  if (NumRets >= 2)
    fail(DL, DAG, "only small returns supported");

  SmallVector<EVT, 8> InTys;
  for (const auto &In : Ins) {
    assert(!In.Flags.isByVal() && "byval is not valid for return values");
    assert(!In.Flags.isNest() && "nest is not valid for return values");
    if (In.Flags.isInAlloca())
      fail(DL, DAG, "WebAssembly hasn't implemented inalloca return values");
    if (In.Flags.isInConsecutiveRegs())
      fail(DL, DAG, "WebAssembly hasn't implemented cons regs return values");
    if (In.Flags.isInConsecutiveRegsLast())
      fail(DL, DAG,
           "WebAssembly hasn't implemented cons regs last return values");
    // Ignore In.getNonZeroOrigAlign() because all our arguments are passed in
    // registers.
    InTys.push_back(In.VT);
  }

  // Construct list of actual arguments to be passed through the call, either
  // as operands of callfi, etc. or pushed on the stack.
  SmallVector<SDValue, 8> Args;
  // Add all fixed arguments.
  Args.append(OutVals.begin(),
              IsVarArg ? OutVals.begin() + NumFixedArgs : OutVals.end());
  // Add a pointer to the vararg buffer.
  if (IsVarArg)
    Args.push_back(FINode);

  unsigned NumArgs = Args.size();
  if (IsTailCall || NumArgs > 3) {
    // pass arguments on stack
    for (auto Out : llvm::reverse(Args)) {
      Chain = DAG.getNode(GlulxISD::PUSH, DL, MVT::Other, {Chain, Out});
    }
  }

  // Compute the operands for the CALLn node.
  SmallVector<SDValue, 8> Ops;
  Ops.push_back(Chain);
  Ops.push_back(Callee);

  if (IsTailCall) {
    // tailcalls do not return values to the current frame
    SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
    // add number of arguments as operand
    Ops.push_back(DAG.getTargetConstant(NumArgs, DL, MVT::i32));
    return DAG.getNode(GlulxISD::TAILCALL, DL, NodeTys, Ops);
  }

  InTys.push_back(MVT::Other);
  SDVTList InTyList = DAG.getVTList(InTys);

  if (NumArgs <= 3) {
    // Using callf: add all arguments as operands
    Ops.append(Args.begin(), Args.end());
    unsigned CallOp;
    switch (NumArgs) {
    case 0: CallOp = GlulxISD::CALLF; break;
    case 1: CallOp = GlulxISD::CALLFI; break;
    case 2: CallOp = GlulxISD::CALLFII; break;
    case 3: CallOp = GlulxISD::CALLFIII; break;
    default: llvm_unreachable("impossible # of args for callf");
    }
    Chain = DAG.getNode(CallOp, DL, InTyList, Ops);
  } else {
    // Using call: add number of arguments as operand
    Ops.push_back(DAG.getTargetConstant(NumArgs, DL, MVT::i32));
    Chain = DAG.getNode(GlulxISD::CALL, DL, InTyList, Ops);
  }

  if (NumRets == 1) {
    InVals.push_back(Chain.getValue(0));
    return Chain.getValue(1);
  }
  return Chain.getValue(0);
}

bool GlulxTargetLowering::isEligibleForTailCallOptimization(
    CCState &CCInfo, CallLoweringInfo &CLI, MachineFunction &MF,
    const SmallVector<CCValAssign, 16> &ArgLocs) const {

  auto &Callee = CLI.Callee;
  auto &Outs = CLI.Outs;
  auto &Caller = MF.getFunction();

  // Exception-handling functions need a special set of instructions to
  // indicate a return to the hardware. Tail-calling another function would
  // probably break this.
  // TODO: The "interrupt" attribute isn't currently defined by RISC-V. This
  // should be expanded as new function attributes are introduced.
  if (Caller.hasFnAttribute("interrupt"))
    return false;

  // Do not tail call opt if the stack is used to pass parameters.
  if (CCInfo.getNextStackOffset() != 0)
    return false;

  // Do not tail call opt if any parameters need to be passed indirectly.
  // Since long doubles (fp128) and i128 are larger than 2*XLEN, they are
  // passed indirectly. So the address of the value will be passed in a
  // register, or if not available, then the address is put on the stack. In
  // order to pass indirectly, space on the stack often needs to be allocated
  // in order to store the value. In this case the CCInfo.getNextStackOffset()
  // != 0 check is not enough and we need to check if any CCValAssign ArgsLocs
  // are passed CCValAssign::Indirect.
  for (auto &VA : ArgLocs)
    if (VA.getLocInfo() == CCValAssign::Indirect)
      return false;

  // Do not tail call opt if either caller or callee uses struct return
  // semantics.
  auto IsCallerStructRet = Caller.hasStructRetAttr();
  auto IsCalleeStructRet = Outs.empty() ? false : Outs[0].Flags.isSRet();
  if (IsCallerStructRet || IsCalleeStructRet)
    return false;

  // Externally-defined functions with weak linkage should not be
  // tail-called. The behaviour of branch instructions in this situation (as
  // used for tail calls) is implementation-defined, so we cannot rely on the
  // linker replacing the tail call with a return.
  if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee)) {
    const GlobalValue *GV = G->getGlobal();
    if (GV->hasExternalWeakLinkage())
      return false;
  }

  // Byval parameters hand the function a pointer directly into the stack area
  // we want to reuse during a tail call. Working around this *is* possible
  // but less efficient and uglier in LowerCall.
  for (auto &Arg : Outs)
    if (Arg.Flags.isByVal())
      return false;

  return true;
}

SDValue
GlulxTargetLowering::LowerReturn(SDValue Chain,
                                 CallingConv::ID CallConv, bool IsVarArg,
                                 const SmallVectorImpl<ISD::OutputArg> &Outs,
                                 const SmallVectorImpl<SDValue> &OutVals,
                                 const SDLoc &DL, SelectionDAG &DAG) const {
  if (Outs.size() > 1)
    fail(DL, DAG, "Glulx can only return up to one value");

  SmallVector<SDValue, 4> RetOps(1, Chain);
  RetOps.append(OutVals.begin(), OutVals.end());
  if (Outs.size() == 0)
    RetOps.push_back(DAG.getTargetConstant(0, DL, MVT::i32));
  Chain = DAG.getNode(GlulxISD::Ret, DL, MVT::Other, RetOps);

  // Record the number and types of the return values.
  for (const ISD::OutputArg &Out : Outs) {
    assert(!Out.Flags.isByVal() && "byval is not valid for return values");
    assert(!Out.Flags.isNest() && "nest is not valid for return values");
    assert(Out.IsFixed && "non-fixed return value is not valid");
    if (Out.Flags.isInAlloca())
      fail(DL, DAG, "WebAssembly hasn't implemented inalloca results");
    if (Out.Flags.isInConsecutiveRegs())
      fail(DL, DAG, "WebAssembly hasn't implemented cons regs results");
    if (Out.Flags.isInConsecutiveRegsLast())
      fail(DL, DAG, "WebAssembly hasn't implemented cons regs last results");
  }

  return Chain;
}

//===----------------------------------------------------------------------===//
//  Lower helper functions
//===----------------------------------------------------------------------===//

SDValue GlulxTargetLowering::getGlobalAddressWrapper(SDValue GA,
                                                     const GlobalValue *GV,
                                                     SelectionDAG &DAG) const {
  llvm_unreachable("Unhandled global variable");
}

//===----------------------------------------------------------------------===//
//  Misc Lower Operation implementation
//===----------------------------------------------------------------------===//

SDValue GlulxTargetLowering::
LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  const auto *GA = cast<GlobalAddressSDNode>(Op);
  EVT VT = Op.getValueType();
  assert(GA->getTargetFlags() == 0 &&
         "Unexpected target flags on generic GlobalAddressSDNode");
  if (GA->getAddressSpace() != 0)
    fail(DL, DAG, "Invalid address space for Glulx target");
  assert(!isPositionIndependent() && "position-independent code in Glulx target");
  assert(GA->getOffset() == 0 && "GlobalAddress should have offset 0 for Glulx");

  SDValue tga = DAG.getTargetGlobalAddress(GA->getGlobal(), DL, VT, 0, 0);
  return DAG.getNode(GlulxISD::GA_WRAPPER, DL, VT, tga);
}

SDValue GlulxTargetLowering::
LowerConstantPool(SDValue Op, SelectionDAG &DAG) const {
  llvm_unreachable("Unsupported constant pool");
}

SDValue GlulxTargetLowering::
LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const {
  llvm_unreachable("Glulx does not yet support computed goto");
}

SDValue
GlulxTargetLowering::LowerExternalSymbol(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  const auto *ES = cast<ExternalSymbolSDNode>(Op);
  EVT VT = Op.getValueType();
  assert(ES->getTargetFlags() == 0 &&
         "Unexpected target flags on generic ExternalSymbolSDNode");

  return DAG.getTargetExternalSymbol(ES->getSymbol(), VT);
}

SDValue GlulxTargetLowering::LowerFrameIndex(SDValue Op,
                                             SelectionDAG &DAG) const {
  int FI = cast<FrameIndexSDNode>(Op)->getIndex();
  return DAG.getTargetFrameIndex(FI, Op.getValueType());
}

SDValue GlulxTargetLowering::LowerCopyToReg(SDValue Op,
                                            SelectionDAG &DAG) const {
  SDValue Src = Op.getOperand(2);
  if (isa<FrameIndexSDNode>(Src.getNode())) {
    // CopyToReg nodes don't support FrameIndex operands. Other targets select
    // the FI to some LEA-like instruction, but since we don't have that, we
    // need to insert some kind of instruction that can take an FI operand and
    // produces a value usable by CopyToReg (i.e. in a vreg). So insert a dummy
    // copy_ri between Op and its FI operand.
    SDValue Chain = Op.getOperand(0);
    SDLoc DL(Op);
    unsigned Reg = cast<RegisterSDNode>(Op.getOperand(1))->getReg();
    EVT VT = Src.getValueType();
    SDValue Copy(DAG.getMachineNode(Glulx::copy_ri, DL, VT, Src), 0);
    return Op.getNode()->getNumValues() == 1
               ? DAG.getCopyToReg(Chain, DL, Reg, Copy)
               : DAG.getCopyToReg(Chain, DL, Reg, Copy,
                                  Op.getNumOperands() == 4 ? Op.getOperand(3)
                                                           : SDValue());
  }
  return SDValue();
}

SDValue
GlulxTargetLowering::LowerRETURNADDR(SDValue Op, SelectionDAG &DAG) const {
  return SDValue();
}

SDValue GlulxTargetLowering::LowerSELECT_CC(SDValue Op,
                                            SelectionDAG &DAG) const {
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue TrueV = Op.getOperand(2);
  SDValue FalseV = Op.getOperand(3);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(4))->get();
  SDLoc DL(Op);

  SDValue TargetCC = DAG.getConstant(CC, DL, MVT::i32);
  SDVTList VTs = DAG.getVTList(Op.getValueType(), MVT::Glue);
  SDValue Ops[] = {LHS, RHS, TargetCC, TrueV, FalseV};

  return DAG.getNode(GlulxISD::SELECT_CC, DL, VTs, Ops);
}

SDValue GlulxTargetLowering::LowerBR_CC(SDValue Op,
                                        SelectionDAG &DAG) const {
  SDLoc DL(Op);
  SDValue Chain = Op.getOperand(0);
  SDValue CCVal = Op.getOperand(1);
  SDValue LHS = Op.getOperand(2);
  SDValue RHS = Op.getOperand(3);
  SDValue TrueBB = Op.getOperand(4);

  assert(LHS.getValueType() == MVT::f32 && "unexpected BR_CC type to legalize");

  ISD::CondCode CC = cast<CondCodeSDNode>(CCVal)->get();
  switch (CC) {
  case ISD::SETO:
    // Idiosyncratic method for branching on non-NaN values.
    return DAG.getNode(GlulxISD::JORDERED, DL, MVT::Other,
                       {Chain, LHS, RHS, TrueBB});

  case ISD::SETONE:
  case ISD::SETUO:
  case ISD::SETUEQ:
  case ISD::SETUGT:
  case ISD::SETUGE:
  case ISD::SETULT:
  case ISD::SETULE:
    // Other non-natively supported cases will be handled more uniformly below.
    break;

  default:
    // This is a natively-supported conditional branch.
    return DAG.getNode(GlulxISD::BR_CC_FP, DL, MVT::Other,
                       {Chain, CCVal, LHS, RHS, TrueBB});
  }

  bool LHSNotNaN = DAG.isKnownNeverNaN(LHS);
  bool RHSNotNaN = DAG.isKnownNeverNaN(RHS);

  if (CC == ISD::SETONE && (!LHSNotNaN || !RHSNotNaN)) {
    // Test non-NaN and unequal by checking both < and >.
    Chain = DAG.getNode(GlulxISD::BR_CC_FP, DL, MVT::Other,
                        {Chain, DAG.getCondCode(ISD::SETOLT),
                         LHS, RHS, TrueBB});
    return DAG.getNode(GlulxISD::BR_CC_FP, DL, MVT::Other,
                       {Chain, DAG.getCondCode(ISD::SETOGT),
                        LHS, RHS, TrueBB});
  }

  // Compute ordered (not branching on NaN) version of condition code
  SDValue NewCC = DAG.getCondCode((ISD::CondCode) (CC & 0b0111));

  // If it's possible for an operand to be NaN, check and branch if so.
  if (!LHSNotNaN)
    Chain = DAG.getNode(GlulxISD::JISNAN, DL, MVT::Other,
                        Chain, LHS, TrueBB);
  if (!RHSNotNaN)
    Chain = DAG.getNode(GlulxISD::JISNAN, DL, MVT::Other,
                        Chain, RHS, TrueBB);
  // If we reach this point, the operands are known not to be NaN.
  if (CC != ISD::SETUO) {
    // Continue with ordered version of comparison.
    Chain = DAG.getNode(GlulxISD::BR_CC_FP, DL, MVT::Other,
                        {Chain, NewCC, LHS, RHS, TrueBB});
  }
  return Chain;
}

SDValue GlulxTargetLowering::LowerVASTART(SDValue Op,
                                          SelectionDAG &DAG) const {
  SDLoc DL(Op);
  EVT PtrVT = getPointerTy(DAG.getMachineFunction().getDataLayout());

  auto *MFI = DAG.getMachineFunction().getInfo<GlulxFunctionInfo>();
  const Value *SV = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();

  SDValue ArgN = DAG.getCopyFromReg(DAG.getEntryNode(), DL,
                                    MFI->getVarargBufferVreg(), PtrVT);
  return DAG.getStore(Op.getOperand(0), DL, ArgN, Op.getOperand(1),
                      MachinePointerInfo(SV));
}

static SDValue LowerFP_TO_SINT_SAT(SDValue Op, SelectionDAG &DAG) {
  // Glulx ftonumz saturates but doesn't produce 0 for nan. We can fix the
  // nan case with a compare and a select.
  SDValue Src = Op.getOperand(0);
  EVT DstVT = Op.getValueType();
  SDLoc DL(Op);
  SDValue FpToInt = DAG.getNode(Glulx::FTONUMZ, DL, DstVT, Src);
  SDValue ZeroInt = DAG.getConstant(0, DL, DstVT);
  return DAG.getSelectCC(DL, Src, Src, ZeroInt, FpToInt, ISD::CondCode::SETUO);
}

SDValue
GlulxTargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) const {
  switch (Op.getOpcode()) {
  case ISD::GlobalAddress:        return LowerGlobalAddress(Op, DAG);
  case ISD::BlockAddress:         return LowerBlockAddress(Op, DAG);
  case ISD::ExternalSymbol:       return LowerExternalSymbol(Op, DAG);
  case ISD::FrameIndex:           return LowerFrameIndex(Op, DAG);
  case ISD::CopyToReg:            return LowerCopyToReg(Op, DAG);
  case ISD::ConstantPool:         return LowerConstantPool(Op, DAG);
  case ISD::RETURNADDR:           return LowerRETURNADDR(Op, DAG);
  case ISD::SELECT_CC:            return LowerSELECT_CC(Op, DAG);
  case ISD::BR_CC:                return LowerBR_CC(Op, DAG);
  case ISD::VASTART:              return LowerVASTART(Op, DAG);
  case ISD::FP_TO_SINT_SAT:       return LowerFP_TO_SINT_SAT(Op, DAG);
  case ISD::STACKSAVE:
  case ISD::STACKRESTORE:
  case ISD::DYNAMIC_STACKALLOC: {
    SDLoc DL(Op);
    MachineFunction &MF = DAG.getMachineFunction();
    const char *Msg = "Glulx backend does not support dynamic stack allocation";
    DAG.getContext()->diagnose(
        DiagnosticInfoUnsupported(MF.getFunction(), Msg, DL.getDebugLoc()));
    report_fatal_error(Msg);
  }
  default: llvm_unreachable("unimplemented operand");
  }
}

static MachineBasicBlock *emitCatch(MachineInstr &MI,
                                    MachineBasicBlock *BB) {
  const TargetInstrInfo &TII = *BB->getParent()->getSubtarget().getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();

  // Create control flow handling the two execution paths for the catch

  const BasicBlock *LLVMBB = BB->getBasicBlock();
  MachineFunction::iterator I = ++BB->getIterator();

  // ThisMBB:
  // ...
  //  catch %Token, NoThrowMBB
  //  fallthrough --> ThrowMBB
  MachineBasicBlock *ThisMBB = BB;
  MachineFunction *F = BB->getParent();
  MachineBasicBlock *ThrowMBB = F->CreateMachineBasicBlock(LLVMBB);
  MachineBasicBlock *NoThrowMBB = F->CreateMachineBasicBlock(LLVMBB);

  F->insert(I, ThrowMBB);
  F->insert(I, NoThrowMBB);
  // Update machine-CFG edges by transferring all successors of the current
  // block to the new block which will contain the Phi node for the select.
  NoThrowMBB->splice(NoThrowMBB->begin(), BB,
                   std::next(MachineBasicBlock::iterator(MI)), BB->end());
  NoThrowMBB->transferSuccessorsAndUpdatePHIs(BB);
  // Next, add the throw-handling and fallthrough blocks as its successors.
  BB->addSuccessor(ThrowMBB);
  BB->addSuccessor(NoThrowMBB);

  // Prepare flag indicating no throw occurred
  Register Zero = F->getRegInfo().createVirtualRegister(&Glulx::GPRRegClass);
  BuildMI(BB, DL, TII.get(Glulx::copy_ri), Zero).addImm(0);
  // Insert catch instruction
  Register Token = F->getRegInfo().createVirtualRegister(&Glulx::GPRRegClass);
  BuildMI(BB, DL, TII.get(Glulx::CATCH), Token).addMBB(NoThrowMBB);

  // ThrowMBB:
  //  # fallthrough to NoThrowMBB
  BB = ThrowMBB;

  // Update machine-CFG edges
  BB->addSuccessor(NoThrowMBB);

  // NoThrowMBB:
  //  %Result = phi [ %Token, ThrowMBB ], [ 0, ThisMBB ]
  //  astore TokenDst, 0, %Token
  // ...
  BB = NoThrowMBB;
  Register RetVal = MI.getOperand(0).getReg();
  Register TokenDst = MI.getOperand(1).getReg();
  BuildMI(*BB, BB->begin(), DL, TII.get(Glulx::ASTORE))
      .addReg(Token)
      .addReg(TokenDst)
      .addImm(0);
  BuildMI(*BB, BB->begin(), DL, TII.get(Glulx::PHI), RetVal)
      .addReg(Token)
      .addMBB(ThrowMBB)
      .addReg(Zero)
      .addMBB(ThisMBB);

  MI.eraseFromParent(); // The pseudo instruction is gone now.
  F->getProperties().reset(MachineFunctionProperties::Property::NoPHIs);
  return BB;
}

MachineBasicBlock *
GlulxTargetLowering::EmitInstrWithCustomInserter(MachineInstr &MI,
                                                 MachineBasicBlock *BB) const {
  unsigned Opc = MI.getOpcode();

  switch (Opc) {
  case Glulx::CATCH_INT:
    return emitCatch(MI, BB);
  case Glulx::SELECT:
    break;
  default:
    llvm_unreachable("unexpected instr type with custom inserter");
  }

  const TargetInstrInfo &TII = *BB->getParent()->getSubtarget().getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();

  // To "insert" a SELECT instruction, we actually have to insert the diamond
  // control-flow pattern.  The incoming instruction knows the destination vreg
  // to set, the condition code register to branch on, the true/false values to
  // select between, and a branch opcode to use.
  const BasicBlock *LLVMBB = BB->getBasicBlock();
  MachineFunction::iterator I = ++BB->getIterator();

  // ThisMBB:
  // ...
  //  TrueVal = ...
  //  jmp_XX r1, r2 goto Copy1MBB
  //  fallthrough --> Copy0MBB
  MachineBasicBlock *ThisMBB = BB;
  MachineFunction *F = BB->getParent();
  MachineBasicBlock *Copy0MBB = F->CreateMachineBasicBlock(LLVMBB);
  MachineBasicBlock *Copy1MBB = F->CreateMachineBasicBlock(LLVMBB);

  F->insert(I, Copy0MBB);
  F->insert(I, Copy1MBB);
  // Update machine-CFG edges by transferring all successors of the current
  // block to the new block which will contain the Phi node for the select.
  Copy1MBB->splice(Copy1MBB->begin(), BB,
                   std::next(MachineBasicBlock::iterator(MI)), BB->end());
  Copy1MBB->transferSuccessorsAndUpdatePHIs(BB);
  // Next, add the true and fallthrough blocks as its successors.
  BB->addSuccessor(Copy0MBB);
  BB->addSuccessor(Copy1MBB);

  // Insert Branch if Flag
  int CC = MI.getOperand(3).getImm();
  int NewCC;
  switch (CC) {
#define SET_NEWCC(X, Y) \
  case ISD::X: \
    NewCC = Glulx::Y; \
    break
    SET_NEWCC(SETGT, JGT);
    SET_NEWCC(SETUGT, JGTU);
    SET_NEWCC(SETGE, JGE);
    SET_NEWCC(SETUGE, JGEU);
    SET_NEWCC(SETEQ, JEQ);
    SET_NEWCC(SETNE, JNE);
    SET_NEWCC(SETLT, JLT);
    SET_NEWCC(SETULT, JLTU);
    SET_NEWCC(SETLE, JLE);
    SET_NEWCC(SETULE, JLEU);
    SET_NEWCC(SETOEQ, JFEQ);
    SET_NEWCC(SETUNE, JFNE);
    SET_NEWCC(SETOLT, JFLT);
    SET_NEWCC(SETOLE, JFLE);
    SET_NEWCC(SETOGT, JFGT);
    SET_NEWCC(SETOGE, JFGE);
  default:
    report_fatal_error("unimplemented select CondCode " + Twine(CC));
  }

  const MachineOperand &LHS = MI.getOperand(1);
  const MachineOperand &RHS = MI.getOperand(2);
  BuildMI(BB, DL, TII.get(NewCC)).add(LHS).add(RHS).addMBB(Copy1MBB);

  // Copy0MBB:
  //  %FalseValue = ...
  //  # fallthrough to Copy1MBB
  BB = Copy0MBB;

  // Update machine-CFG edges
  BB->addSuccessor(Copy1MBB);

  // Copy1MBB:
  //  %Result = phi [ %FalseValue, Copy0MBB ], [ %TrueValue, ThisMBB ]
  // ...
  BB = Copy1MBB;
  BuildMI(*BB, BB->begin(), DL, TII.get(Glulx::PHI), MI.getOperand(0).getReg())
      .addReg(MI.getOperand(5).getReg())
      .addMBB(Copy0MBB)
      .addReg(MI.getOperand(4).getReg())
      .addMBB(ThisMBB);

  MI.eraseFromParent(); // The pseudo instruction is gone now.
  F->getProperties().reset(MachineFunctionProperties::Property::NoPHIs);
  return BB;
}
