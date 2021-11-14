//===-- GlulxAsmPrinter.cpp - Glulx LLVM Assembly Printer -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to Glulx assembly language.
//
//===----------------------------------------------------------------------===//

#include "GlulxInstrInfo.h"
#include "GlulxTargetMachine.h"
#include "MCTargetDesc/GlulxInstPrinter.h"
#include "MCTargetDesc/GlulxMCExpr.h"
#include "TargetInfo/GlulxTargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Timer.h"
#include "llvm/Target/TargetLoweringObjectFile.h"

using namespace llvm;

#define DEBUG_TYPE "Glulx-asm-printer"

namespace llvm {
class GlulxAsmPrinter : public AsmPrinter {
public:
  explicit GlulxAsmPrinter(TargetMachine &TM,
                           std::unique_ptr<MCStreamer> Streamer)
    : AsmPrinter(TM, std::move(Streamer)) {}

  virtual StringRef getPassName() const override {
    return "Glulx Assembly Printer";
  }

  void emitInstruction(const MachineInstr *MI) override;
  void emitGlobalVariable(const GlobalVariable *GV) override;

  // This function must be present as it is internally used by the
  // auto-generated function emitPseudoExpansionLowering to expand pseudo
  // instruction
  void EmitToStreamer(MCStreamer &S, const MCInst &Inst);
  // Auto-generated function in GlulxGenMCPseudoLowering.inc
  bool emitPseudoExpansionLowering(MCStreamer &OutStreamer,
                                   const MachineInstr *MI);

private:
  void LowerInstruction(const MachineInstr *MI, MCInst &OutMI) const;
  bool lowerOperand(const MachineOperand &MO, MCOperand &MCOp) const;
  MCOperand LowerSymbolOperand(const MachineOperand &MO, MCSymbol *Sym) const;
};
}

// Simple pseudo-instructions have their lowering (with expansion to real
// instructions) auto-generated.
#include "GlulxGenMCPseudoLowering.inc"
void GlulxAsmPrinter::EmitToStreamer(MCStreamer &S, const MCInst &Inst) {
  AsmPrinter::EmitToStreamer(*OutStreamer, Inst);
}

void GlulxAsmPrinter::emitInstruction(const MachineInstr *MI) {
  // Do any auto-generated pseudo lowerings.
  if (emitPseudoExpansionLowering(*OutStreamer, MI))
    return;

  MCInst TmpInst;
  LowerInstruction(MI, TmpInst);
  EmitToStreamer(*OutStreamer, TmpInst);
}

void GlulxAsmPrinter::emitGlobalVariable(const GlobalVariable *GV) {
  if (GV->hasInitializer()) {
    // Check to see if this is a special global used by LLVM, if so, emit it.
    if (emitSpecialLLVMGlobal(GV))
      return;

    // Skip the emission of global equivalents. The symbol can be emitted later
    // on by emitGlobalGOTEquivs in case it turns out to be needed.
    if (GlobalGOTEquivs.count(getSymbol(GV)))
      return;
  }

  MCSymbol *GVSym = getSymbol(GV);
  MCSymbol *EmittedSym = GVSym;

  // getOrCreateEmuTLSControlSym only creates the symbol with name and default
  // attributes.
  // GV's or GVSym's attributes will be used for the EmittedSym.
  emitVisibility(EmittedSym, GV->getVisibility(), !GV->isDeclaration());

  if (!GV->hasInitializer())   // External globals require no extra code.
    return;

  GVSym->redefineIfPossible();
  if (GVSym->isDefined() || GVSym->isVariable())
    OutContext.reportError(SMLoc(), "symbol '" + Twine(GVSym->getName()) +
                                        "' is already defined");

  SectionKind GVKind = TargetLoweringObjectFile::getKindForGlobal(GV, TM);

  const DataLayout &DL = GV->getParent()->getDataLayout();
  uint64_t Size = DL.getTypeAllocSize(GV->getValueType());

  // If the alignment is specified, we *must* obey it.  Overaligning a global
  // with a specified alignment is a prompt way to break globals emitted to
  // sections and expected to be contiguous (e.g. ObjC metadata).
  const Align Alignment = getGVAlignment(GV, DL);

  for (const HandlerInfo &HI : Handlers) {
    NamedRegionTimer T(HI.TimerName, HI.TimerDescription,
                       HI.TimerGroupName, HI.TimerGroupDescription,
                       TimePassesIsEnabled);
    HI.Handler->setSymbolSize(GVSym, Size);
  }

  // Handle common symbols
  if (GVKind.isCommon()) {
    if (Size == 0) Size = 1;   // .comm Foo, 0 is undefined, avoid it.
    // .comm _foo, 42, 4
    const bool SupportsAlignment =
        getObjFileLowering().getCommDirectiveSupportsAlignment();
    OutStreamer->emitCommonSymbol(GVSym, Size,
                                  SupportsAlignment ? Alignment.value() : 0);
    return;
  }

  // Determine to which section this global should be emitted.
  MCSection *TheSection = getObjFileLowering().SectionForGlobal(GV, GVKind, TM);

  MCSymbol *EmittedInitSym = GVSym;

  OutStreamer->SwitchSection(TheSection);

  // Handle BSS globals.
  if (GVKind.isBSS() &&
      getObjFileLowering().getBSSSection() == TheSection) {
    if (Size == 0)
      Size = 1; // .comm Foo, 0 is undefined, avoid it.

    // Use .lcomm only if it supports user-specified alignment.
    // Otherwise, while it would still be correct to use .lcomm in some
    // cases (e.g. when Align == 1), the external assembler might enfore
    // some -unknown- default alignment behavior, which could cause
    // spurious differences between external and integrated assembler.
    // Prefer to simply fall back to .local / .comm in this case.
    if (MAI->getLCOMMDirectiveAlignmentType() != LCOMM::NoAlignment) {
      // .lcomm _foo, 42
      OutStreamer->emitLocalCommonSymbol(GVSym, Size, Alignment.value());
      return;
    }

    // .local _foo
    OutStreamer->emitSymbolAttribute(GVSym, MCSA_Local);
    // .comm _foo, 42, 4
    const bool SupportsAlignment =
        getObjFileLowering().getCommDirectiveSupportsAlignment();
    OutStreamer->emitCommonSymbol(GVSym, Size,
                                  SupportsAlignment ? Alignment.value() : 0);
    return;
  }

  emitLinkage(GV, EmittedInitSym);
  emitAlignment(Alignment, GV);

  OutStreamer->emitLabel(EmittedInitSym);
  MCSymbol *LocalAlias = getSymbolPreferLocal(*GV);
  if (LocalAlias != EmittedInitSym)
    OutStreamer->emitLabel(LocalAlias);

  emitGlobalConstant(GV->getParent()->getDataLayout(), GV->getInitializer());

  OutStreamer->AddBlankLine();
}

void GlulxAsmPrinter::LowerInstruction(const MachineInstr *MI,
                                       MCInst &OutMI) const {
  OutMI.setOpcode(MI->getOpcode());

  for (const MachineOperand &MO : MI->operands()) {
    MCOperand MCOp;
    lowerOperand(MO, MCOp);
    if (MCOp.isValid())
      OutMI.addOperand(MCOp);
  }
}

bool GlulxAsmPrinter::lowerOperand(const MachineOperand &MO,
                                   MCOperand &MCOp) const {
  switch (MO.getType()) {
  case MachineOperand::MO_Register:
    // Ignore all implicit register operands.
    if (MO.isImplicit()) {
      break;
    }
    MCOp = MCOperand::createReg(MO.getReg());
    break;

  case MachineOperand::MO_Immediate:
    MCOp = MCOperand::createImm(MO.getImm());
    break;

  case MachineOperand::MO_FPImmediate: {
    const uint64_t BitPattern =
        MO.getFPImm()->getValueAPF().bitcastToAPInt().getZExtValue();
    MCOp = MCOperand::createSFPImm(static_cast<uint32_t>(BitPattern));
    break;
  }

  case MachineOperand::MO_MachineBasicBlock:
    MCOp = LowerSymbolOperand(MO, MO.getMBB()->getSymbol());
    break;

  case MachineOperand::MO_GlobalAddress:
    MCOp = LowerSymbolOperand(MO, getSymbol(MO.getGlobal()));
    break;

  case MachineOperand::MO_BlockAddress:
    MCOp = LowerSymbolOperand(MO, GetBlockAddressSymbol(MO.getBlockAddress()));
    break;

  case MachineOperand::MO_JumpTableIndex:
    MCOp = LowerSymbolOperand(MO, GetJTISymbol(MO.getIndex()));
    break;

  case MachineOperand::MO_ExternalSymbol:
    MCOp = LowerSymbolOperand(MO, GetExternalSymbolSymbol(MO.getSymbolName()));
    break;

  case MachineOperand::MO_ConstantPoolIndex:
    MCOp = LowerSymbolOperand(MO, GetCPISymbol(MO.getIndex()));
    break;

  case MachineOperand::MO_RegisterMask:
    return false;

  default:
    report_fatal_error("unknown operand type");
 }

  return true;
}

MCOperand GlulxAsmPrinter::LowerSymbolOperand(const MachineOperand &MO,
                                              MCSymbol *Sym) const {
  MCContext &Ctx = OutContext;

  const MCExpr *Expr =
    MCSymbolRefExpr::create(Sym, MCSymbolRefExpr::VK_None, Ctx);
  if (MO.getTargetFlags() == GlulxII::MO_DEREFERENCE)
    Expr = GlulxMCExpr::create(GlulxMCExpr::VK_GLULX_DEREFERENCE, Expr,
                               OutContext);

  if (!MO.isJTI() && !MO.isMBB() && MO.getOffset())
    Expr = MCBinaryExpr::createAdd(
        Expr, MCConstantExpr::create(MO.getOffset(), Ctx), Ctx);

  return MCOperand::createExpr(Expr);
}

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeGlulxAsmPrinter() {
  RegisterAsmPrinter<GlulxAsmPrinter> X(getTheGlulxTarget());
}
