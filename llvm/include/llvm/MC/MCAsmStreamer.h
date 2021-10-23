//===- lib/MC/MCAsmStreamer.h - Text Assembly Output ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//


#ifndef LLVM_MC_MCASMSTREAMER_H
#define LLVM_MC_MCASMSTREAMER_H

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDirectives.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/MD5.h"

namespace llvm {

class MCContext;
class Twine;

class MCAsmStreamer : public MCStreamer {
protected:
  std::unique_ptr<formatted_raw_ostream> OSOwner;
  formatted_raw_ostream &OS;
  const MCAsmInfo *MAI;
  std::unique_ptr<MCInstPrinter> InstPrinter;
  std::unique_ptr<MCAssembler> Assembler;

  SmallString<128> ExplicitCommentToEmit;
  SmallString<128> CommentToEmit;
  raw_svector_ostream CommentStream;
  raw_null_ostream NullStream;

  unsigned IsVerboseAsm : 1;
  unsigned ShowInst : 1;
  unsigned UseDwarfDirectory : 1;

  void EmitRegisterName(int64_t Register);
  void PrintQuotedString(StringRef Data, raw_ostream &OS) const;
  void printDwarfFileDirective(unsigned FileNo, StringRef Directory,
                               StringRef Filename,
                               Optional<MD5::MD5Result> Checksum,
                               Optional<StringRef> Source,
                               bool UseDwarfDirectory,
                               raw_svector_ostream &OS) const;
  void emitCFIStartProcImpl(MCDwarfFrameInfo &Frame) override;
  void emitCFIEndProcImpl(MCDwarfFrameInfo &Frame) override;

public:
  MCAsmStreamer(MCContext &Context, std::unique_ptr<formatted_raw_ostream> os,
                bool isVerboseAsm, bool useDwarfDirectory,
                MCInstPrinter *printer, std::unique_ptr<MCCodeEmitter> emitter,
                std::unique_ptr<MCAsmBackend> asmbackend, bool showInst)
      : MCStreamer(Context), OSOwner(std::move(os)), OS(*OSOwner),
        MAI(Context.getAsmInfo()), InstPrinter(printer),
        Assembler(std::make_unique<MCAssembler>(
            Context, std::move(asmbackend), std::move(emitter),
            (asmbackend) ? asmbackend->createObjectWriter(NullStream)
                         : nullptr)),
        CommentStream(CommentToEmit), IsVerboseAsm(isVerboseAsm),
        ShowInst(showInst), UseDwarfDirectory(useDwarfDirectory) {
    assert(InstPrinter);
    if (IsVerboseAsm)
      InstPrinter->setCommentStream(CommentStream);
    if (Assembler->getBackendPtr())
      setAllowAutoPadding(Assembler->getBackend().allowAutoPadding());

    Context.setUseNamesOnTempLabels(true);
  }

  MCAssembler &getAssembler() { return *Assembler; }
  MCAssembler *getAssemblerPtr() override { return nullptr; }

  inline void EmitEOL() {
    // Dump Explicit Comments here.
    emitExplicitComments();
    // If we don't have any comments, just emit a \n.
    if (!IsVerboseAsm) {
      OS << '\n';
      return;
    }
    EmitCommentsAndEOL();
  }

  void emitSyntaxDirective() override;

  void EmitCommentsAndEOL();

  /// Return true if this streamer supports verbose assembly at all.
  bool isVerboseAsm() const override { return IsVerboseAsm; }

  /// Do we support EmitRawText?
  bool hasRawTextSupport() const override { return true; }

  /// Add a comment that can be emitted to the generated .s file to make the
  /// output of the compiler more readable. This only affects the MCAsmStreamer
  /// and only when verbose assembly output is enabled.
  void AddComment(const Twine &T, bool EOL = true) override;

  /// Add a comment showing the encoding of an instruction.
  void AddEncodingComment(const MCInst &Inst, const MCSubtargetInfo &);

  /// Return a raw_ostream that comments can be written to.
  /// Unlike AddComment, you are required to terminate comments with \n if you
  /// use this method.
  raw_ostream &GetCommentOS() override {
    if (!IsVerboseAsm)
      return nulls();  // Discard comments unless in verbose asm mode.
    return CommentStream;
  }

  void emitRawComment(const Twine &T, bool TabPrefix = true) override;

  void addExplicitComment(const Twine &T) override;
  void emitExplicitComments() override;

  /// Emit a blank line to a .s file to pretty it up.
  void AddBlankLine() override {
    EmitEOL();
  }

  /// @name MCStreamer Interface
  /// @{

  void changeSection(MCSection *Section, const MCExpr *Subsection) override;

  void emitELFSymverDirective(const MCSymbol *OriginalSym, StringRef Name,
                              bool KeepOriginalSym) override;

  void emitLOHDirective(MCLOHType Kind, const MCLOHArgs &Args) override;

  void emitGNUAttribute(unsigned Tag, unsigned Value) override;

  StringRef getMnemonic(MCInst &MI) override {
    return InstPrinter->getMnemonic(&MI).first;
  }

  void emitLabel(MCSymbol *Symbol, SMLoc Loc = SMLoc()) override;

  void emitAssemblerFlag(MCAssemblerFlag Flag) override;
  void emitLinkerOptions(ArrayRef<std::string> Options) override;
  void emitDataRegion(MCDataRegionType Kind) override;
  void emitVersionMin(MCVersionMinType Kind, unsigned Major, unsigned Minor,
                      unsigned Update, VersionTuple SDKVersion) override;
  void emitBuildVersion(unsigned Platform, unsigned Major, unsigned Minor,
                        unsigned Update, VersionTuple SDKVersion) override;
  void emitThumbFunc(MCSymbol *Func) override;

  void emitAssignment(MCSymbol *Symbol, const MCExpr *Value) override;
  void emitWeakReference(MCSymbol *Alias, const MCSymbol *Symbol) override;
  bool emitSymbolAttribute(MCSymbol *Symbol, MCSymbolAttr Attribute) override;

  void emitSymbolDesc(MCSymbol *Symbol, unsigned DescValue) override;
  void BeginCOFFSymbolDef(const MCSymbol *Symbol) override;
  void EmitCOFFSymbolStorageClass(int StorageClass) override;
  void EmitCOFFSymbolType(int Type) override;
  void EndCOFFSymbolDef() override;
  void EmitCOFFSafeSEH(MCSymbol const *Symbol) override;
  void EmitCOFFSymbolIndex(MCSymbol const *Symbol) override;
  void EmitCOFFSectionIndex(MCSymbol const *Symbol) override;
  void EmitCOFFSecRel32(MCSymbol const *Symbol, uint64_t Offset) override;
  void EmitCOFFImgRel32(MCSymbol const *Symbol, int64_t Offset) override;
  void emitXCOFFLocalCommonSymbol(MCSymbol *LabelSym, uint64_t Size,
                                  MCSymbol *CsectSym,
                                  unsigned ByteAlign) override;
  void emitXCOFFSymbolLinkageWithVisibility(MCSymbol *Symbol,
                                            MCSymbolAttr Linakge,
                                            MCSymbolAttr Visibility) override;
  void emitXCOFFRenameDirective(const MCSymbol *Name,
                                StringRef Rename) override;

  void emitELFSize(MCSymbol *Symbol, const MCExpr *Value) override;
  void emitCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                        unsigned ByteAlignment) override;

  /// Emit a local common (.lcomm) symbol.
  ///
  /// @param Symbol - The common symbol to emit.
  /// @param Size - The size of the common symbol.
  /// @param ByteAlignment - The alignment of the common symbol in bytes.
  void emitLocalCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                             unsigned ByteAlignment) override;

  void emitZerofill(MCSection *Section, MCSymbol *Symbol = nullptr,
                    uint64_t Size = 0, unsigned ByteAlignment = 0,
                    SMLoc Loc = SMLoc()) override;

  void emitTBSSSymbol(MCSection *Section, MCSymbol *Symbol, uint64_t Size,
                      unsigned ByteAlignment = 0) override;

  void emitBinaryData(StringRef Data) override;

  void emitBytes(StringRef Data) override;

  void emitValueImpl(const MCExpr *Value, unsigned Size,
                     SMLoc Loc = SMLoc()) override;
  void emitIntValue(uint64_t Value, unsigned Size) override;
  void emitIntValueInHex(uint64_t Value, unsigned Size) override;
  void emitIntValueInHexWithPadding(uint64_t Value, unsigned Size) override;

  void emitULEB128Value(const MCExpr *Value) override;

  void emitSLEB128Value(const MCExpr *Value) override;

  void emitDTPRel32Value(const MCExpr *Value) override;
  void emitDTPRel64Value(const MCExpr *Value) override;
  void emitTPRel32Value(const MCExpr *Value) override;
  void emitTPRel64Value(const MCExpr *Value) override;

  void emitGPRel64Value(const MCExpr *Value) override;

  void emitGPRel32Value(const MCExpr *Value) override;

  void emitFill(const MCExpr &NumBytes, uint64_t FillValue,
                SMLoc Loc = SMLoc()) override;

  void emitFill(const MCExpr &NumValues, int64_t Size, int64_t Expr,
                SMLoc Loc = SMLoc()) override;

  void emitValueToAlignment(unsigned ByteAlignment, int64_t Value = 0,
                            unsigned ValueSize = 1,
                            unsigned MaxBytesToEmit = 0) override;

  void emitCodeAlignment(unsigned ByteAlignment,
                         const MCSubtargetInfo *STI,
                         unsigned MaxBytesToEmit = 0) override;

  void emitValueToOffset(const MCExpr *Offset,
                         unsigned char Value,
                         SMLoc Loc) override;

  void emitFileDirective(StringRef Filename) override;
  void emitFileDirective(StringRef Filename, StringRef CompilerVerion,
                         StringRef TimeStamp, StringRef Description) override;
  Expected<unsigned> tryEmitDwarfFileDirective(unsigned FileNo,
                                               StringRef Directory,
                                               StringRef Filename,
                                               Optional<MD5::MD5Result> Checksum = None,
                                               Optional<StringRef> Source = None,
                                               unsigned CUID = 0) override;
  void emitDwarfFile0Directive(StringRef Directory, StringRef Filename,
                               Optional<MD5::MD5Result> Checksum,
                               Optional<StringRef> Source,
                               unsigned CUID = 0) override;
  void emitDwarfLocDirective(unsigned FileNo, unsigned Line, unsigned Column,
                             unsigned Flags, unsigned Isa,
                             unsigned Discriminator,
                             StringRef FileName) override;
  MCSymbol *getDwarfLineTableSymbol(unsigned CUID) override;

  bool EmitCVFileDirective(unsigned FileNo, StringRef Filename,
                           ArrayRef<uint8_t> Checksum,
                           unsigned ChecksumKind) override;
  bool EmitCVFuncIdDirective(unsigned FuncId) override;
  bool EmitCVInlineSiteIdDirective(unsigned FunctionId, unsigned IAFunc,
                                   unsigned IAFile, unsigned IALine,
                                   unsigned IACol, SMLoc Loc) override;
  void emitCVLocDirective(unsigned FunctionId, unsigned FileNo, unsigned Line,
                          unsigned Column, bool PrologueEnd, bool IsStmt,
                          StringRef FileName, SMLoc Loc) override;
  void emitCVLinetableDirective(unsigned FunctionId, const MCSymbol *FnStart,
                                const MCSymbol *FnEnd) override;
  void emitCVInlineLinetableDirective(unsigned PrimaryFunctionId,
                                      unsigned SourceFileId,
                                      unsigned SourceLineNum,
                                      const MCSymbol *FnStartSym,
                                      const MCSymbol *FnEndSym) override;

  void PrintCVDefRangePrefix(
      ArrayRef<std::pair<const MCSymbol *, const MCSymbol *>> Ranges);

  void emitCVDefRangeDirective(
      ArrayRef<std::pair<const MCSymbol *, const MCSymbol *>> Ranges,
      codeview::DefRangeRegisterRelHeader DRHdr) override;

  void emitCVDefRangeDirective(
      ArrayRef<std::pair<const MCSymbol *, const MCSymbol *>> Ranges,
      codeview::DefRangeSubfieldRegisterHeader DRHdr) override;

  void emitCVDefRangeDirective(
      ArrayRef<std::pair<const MCSymbol *, const MCSymbol *>> Ranges,
      codeview::DefRangeRegisterHeader DRHdr) override;

  void emitCVDefRangeDirective(
      ArrayRef<std::pair<const MCSymbol *, const MCSymbol *>> Ranges,
      codeview::DefRangeFramePointerRelHeader DRHdr) override;

  void emitCVStringTableDirective() override;
  void emitCVFileChecksumsDirective() override;
  void emitCVFileChecksumOffsetDirective(unsigned FileNo) override;
  void EmitCVFPOData(const MCSymbol *ProcSym, SMLoc L) override;

  void emitIdent(StringRef IdentString) override;
  void emitCFIBKeyFrame() override;
  void emitCFISections(bool EH, bool Debug) override;
  void emitCFIDefCfa(int64_t Register, int64_t Offset) override;
  void emitCFIDefCfaOffset(int64_t Offset) override;
  void emitCFIDefCfaRegister(int64_t Register) override;
  void emitCFILLVMDefAspaceCfa(int64_t Register, int64_t Offset,
                               int64_t AddressSpace) override;
  void emitCFIOffset(int64_t Register, int64_t Offset) override;
  void emitCFIPersonality(const MCSymbol *Sym, unsigned Encoding) override;
  void emitCFILsda(const MCSymbol *Sym, unsigned Encoding) override;
  void emitCFIRememberState() override;
  void emitCFIRestoreState() override;
  void emitCFIRestore(int64_t Register) override;
  void emitCFISameValue(int64_t Register) override;
  void emitCFIRelOffset(int64_t Register, int64_t Offset) override;
  void emitCFIAdjustCfaOffset(int64_t Adjustment) override;
  void emitCFIEscape(StringRef Values) override;
  void emitCFIGnuArgsSize(int64_t Size) override;
  void emitCFISignalFrame() override;
  void emitCFIUndefined(int64_t Register) override;
  void emitCFIRegister(int64_t Register1, int64_t Register2) override;
  void emitCFIWindowSave() override;
  void emitCFINegateRAState() override;
  void emitCFIReturnColumn(int64_t Register) override;

  void EmitWinCFIStartProc(const MCSymbol *Symbol, SMLoc Loc) override;
  void EmitWinCFIEndProc(SMLoc Loc) override;
  void EmitWinCFIFuncletOrFuncEnd(SMLoc Loc) override;
  void EmitWinCFIStartChained(SMLoc Loc) override;
  void EmitWinCFIEndChained(SMLoc Loc) override;
  void EmitWinCFIPushReg(MCRegister Register, SMLoc Loc) override;
  void EmitWinCFISetFrame(MCRegister Register, unsigned Offset,
                          SMLoc Loc) override;
  void EmitWinCFIAllocStack(unsigned Size, SMLoc Loc) override;
  void EmitWinCFISaveReg(MCRegister Register, unsigned Offset,
                         SMLoc Loc) override;
  void EmitWinCFISaveXMM(MCRegister Register, unsigned Offset,
                         SMLoc Loc) override;
  void EmitWinCFIPushFrame(bool Code, SMLoc Loc) override;
  void EmitWinCFIEndProlog(SMLoc Loc) override;

  void EmitWinEHHandler(const MCSymbol *Sym, bool Unwind, bool Except,
                        SMLoc Loc) override;
  void EmitWinEHHandlerData(SMLoc Loc) override;

  void emitCGProfileEntry(const MCSymbolRefExpr *From,
                          const MCSymbolRefExpr *To, uint64_t Count) override;

  void emitInstruction(const MCInst &Inst, const MCSubtargetInfo &STI) override;

  void emitPseudoProbe(uint64_t Guid, uint64_t Index, uint64_t Type,
                       uint64_t Attr,
                       const MCPseudoProbeInlineStack &InlineStack) override;

  void emitBundleAlignMode(unsigned AlignPow2) override;
  void emitBundleLock(bool AlignToEnd) override;
  void emitBundleUnlock() override;

  Optional<std::pair<bool, std::string>>
  emitRelocDirective(const MCExpr &Offset, StringRef Name, const MCExpr *Expr,
                     SMLoc Loc, const MCSubtargetInfo &STI) override;

  void emitAddrsig() override;
  void emitAddrsigSym(const MCSymbol *Sym) override;

  /// If this file is backed by an assembly streamer, this dumps the specified
  /// string in the output .s file. This capability is indicated by the
  /// hasRawTextSupport() predicate.
  void emitRawTextImpl(StringRef String) override;

  void finishImpl() override;

  void emitDwarfUnitLength(uint64_t Length, const Twine &Comment) override;

  MCSymbol *emitDwarfUnitLength(const Twine &Prefix,
                                const Twine &Comment) override;

  void emitDwarfLineStartLabel(MCSymbol *StartSym) override;

  void emitDwarfLineEndEntry(MCSection *Section, MCSymbol *LastLabel) override;

  void emitDwarfAdvanceLineAddr(int64_t LineDelta, const MCSymbol *LastLabel,
                                const MCSymbol *Label,
                                unsigned PointerSize) override;

  void doFinalizationAtSectionEnd(MCSection *Section) override;
};

} // end namespace llvm.

#endif // LLVM_MC_MCASMSTREAMER_H
