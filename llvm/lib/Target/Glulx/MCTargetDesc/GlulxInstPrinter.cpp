//===-- GlulxInstPrinter.cpp - Convert Glulx MCInst to assembly syntax ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints a Glulx MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

#include "GlulxInstPrinter.h"
#include "GlulxMachineFunctionInfo.h"
#include "GlulxInstrInfo.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#define DEBUG_TYPE "Glulx-isel"

#define PRINT_ALIAS_INSTR
#include "GlulxGenAsmWriter.inc"

void GlulxInstPrinter::printRegName(raw_ostream &OS, unsigned RegNo) const {
  assert(RegNo != GlulxFunctionInfo::UnusedReg);
  if (RegNo == 0) {   // fake register 0 used to indicate discarding stores
    OS << "0";    // glasm notation for discarding stores
  } else {
    llvm_unreachable("Glulx target does not currently use physical registers");
  }
}

void GlulxInstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                 StringRef Annot, const MCSubtargetInfo &STI,
                                 raw_ostream &O) {
  // Try to print any aliases first.
  if (!printAliasInstr(MI, Address, O)) {
    printInstruction(MI, Address, O);
  }
  printAnnotation(O, Annot);
}

void GlulxInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                    raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg()) {
    unsigned WAReg = Op.getReg();
    if (int(WAReg) >= 0)
      printRegName(O, WAReg);
    else
      O << "$" << ((unsigned) WAReg & INT32_MAX);
  } else if (Op.isImm()) {
    int64_t Val = Op.getImm();
    assert(isInt<32>(Val) && "Glulx integer immediate out of range");
    O << Val;
  } else if (Op.isSFPImm()) {
    O << static_cast<int32_t>(Op.getSFPImm());
  } else {
    assert(Op.isExpr() && "unknown operand kind in printOperand");
    Op.getExpr()->print(O, &MAI, true);
    O << MAI.getLabelSuffix();
  }
}
