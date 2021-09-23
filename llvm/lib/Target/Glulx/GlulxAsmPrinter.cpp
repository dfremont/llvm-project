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
#include "TargetInfo/GlulxTargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCContext.h"
#include "llvm/Support/TargetRegistry.h"

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

  if (!MO.isJTI() && !MO.isMBB() && MO.getOffset())
    Expr = MCBinaryExpr::createAdd(
        Expr, MCConstantExpr::create(MO.getOffset(), Ctx), Ctx);

  return MCOperand::createExpr(Expr);
}

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeGlulxAsmPrinter() {
  RegisterAsmPrinter<GlulxAsmPrinter> X(getTheGlulxTarget());
}
