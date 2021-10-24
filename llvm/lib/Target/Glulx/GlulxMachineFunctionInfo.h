// GlulxMachineFunctionInfo.h-Glulx machine function info-*- C++ -*--------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file declares Glulx-specific per-machine-function
/// information.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_GLULX_GLULXMACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_GLULX_GLULXMACHINEFUNCTIONINFO_H

#include "MCTargetDesc/GlulxMCTargetDesc.h"
#include "llvm/CodeGen/MIRYamlMapping.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/MC/MCSymbolWasm.h"

namespace llvm {

namespace yaml {
struct GlulxFunctionInfo;
}

/// This class is derived from MachineFunctionInfo and contains private
/// Glulx-specific information for each MachineFunction.
class GlulxFunctionInfo final : public MachineFunctionInfo {
  const MachineFunction &MF;

  std::vector<MVT> Params;
  std::vector<MVT> Results;
  std::vector<MVT> Locals;

  /// A mapping from CodeGen vreg index to Glulx local number.
  std::vector<unsigned> WARegs;

  // A virtual register holding the pointer to the vararg buffer for vararg
  // functions. It is created and set in TLI::LowerFormalArguments and read by
  // TLI::LowerVASTART
  unsigned VarargVreg = -1U;

  // A virtual register holding the base pointer for functions that have
  // overaligned values on the user stack.
  unsigned BasePtrVreg = -1U;
  // A virtual register holding the frame base. This is either FP or SP
  // after it has been replaced by a vreg
  unsigned FrameBaseVreg = -1U;
  // The local holding the frame base. This is either FP or SP
  // after GlulxExplicitLocals
  unsigned FrameBaseLocal = -1U;

  // Virtual registers holding computed addresses of objects in the call frame.
  DenseMap<std::pair<unsigned, const MachineBasicBlock*>,
      Register> FrameAddresses;

public:
  explicit GlulxFunctionInfo(MachineFunction &MF)
      : MF(MF) {}
  ~GlulxFunctionInfo() override;

  const MachineFunction &getMachineFunction() const { return MF; }

  void addParam(MVT VT) { Params.push_back(VT); }
  const std::vector<MVT> &getParams() const { return Params; }

  void addResult(MVT VT) { Results.push_back(VT); }
  const std::vector<MVT> &getResults() const { return Results; }

  void clearParamsAndResults() {
    Params.clear();
    Results.clear();
  }

  void setNumLocals(size_t NumLocals) { Locals.resize(NumLocals, MVT::i32); }
  void setLocal(size_t i, MVT VT) { Locals[i] = VT; }
  void addLocal(MVT VT) { Locals.push_back(VT); }
  const std::vector<MVT> &getLocals() const { return Locals; }

  unsigned getVarargBufferVreg() const {
    assert(VarargVreg != -1U && "Vararg vreg hasn't been set");
    return VarargVreg;
  }
  void setVarargBufferVreg(unsigned Reg) { VarargVreg = Reg; }

  unsigned getBasePointerVreg() const {
    assert(BasePtrVreg != -1U && "Base ptr vreg hasn't been set");
    return BasePtrVreg;
  }
  void setFrameBaseVreg(unsigned Reg) { FrameBaseVreg = Reg; }
  unsigned getFrameBaseVreg() const {
    assert(FrameBaseVreg != -1U && "Frame base vreg hasn't been set");
    return FrameBaseVreg;
  }
  void clearFrameBaseVreg() { FrameBaseVreg = -1U; }
  // Return true if the frame base physreg has been replaced by a virtual reg.
  bool isFrameBaseVirtual() const { return FrameBaseVreg != -1U; }
  void setFrameBaseLocal(unsigned Local) { FrameBaseLocal = Local; }
  unsigned getFrameBaseLocal() const {
    assert(FrameBaseLocal != -1U && "Frame base local hasn't been set");
    return FrameBaseLocal;
  }
  void setBasePointerVreg(unsigned Reg) { BasePtrVreg = Reg; }

  Register getVRegForFrameOffset(unsigned Offset,
                                 const MachineBasicBlock *MBB) {
    auto Res = FrameAddresses.find(std::make_pair(Offset, MBB));
    return Res == FrameAddresses.end() ? (Register) 0 : Res->second;
  }
  void setVRegForFrameOffset(unsigned Offset,
                             const MachineBasicBlock *MBB, Register VReg) {
    auto Key = std::make_pair(Offset, MBB);
    assert(!FrameAddresses.count(Key) && "FI VReg already set");
    FrameAddresses[Key] = VReg;
  }

  static const unsigned UnusedReg = -1u;

  void initWARegs(MachineRegisterInfo &MRI);
  void setWAReg(unsigned VReg, unsigned WAReg) {
    assert(WAReg != UnusedReg);
    auto I = Register::virtReg2Index(VReg);
    assert(I < WARegs.size());
    WARegs[I] = WAReg;
  }
  unsigned getWAReg(unsigned VReg) const {
    auto I = Register::virtReg2Index(VReg);
    assert(I < WARegs.size());
    return WARegs[I];
  }
};

namespace Glulx {
void computeLegalValueVTs(const Function &F, const TargetMachine &TM, Type *Ty,
                          SmallVectorImpl<MVT> &ValueVTs);

// Compute the signature for a given FunctionType (Ty). Note that it's not the
// signature for ContextFunc (ContextFunc is just used to get varous context)
void computeSignatureVTs(const FunctionType *Ty, const Function *TargetFunc,
                         const Function &ContextFunc, const TargetMachine &TM,
                         SmallVectorImpl<MVT> &Params,
                         SmallVectorImpl<MVT> &Results);
} // end namespace Glulx

namespace yaml {

using BBNumberMap = DenseMap<int, int>;

struct GlulxFunctionInfo final : public yaml::MachineFunctionInfo {
  std::vector<FlowStringValue> Params;
  std::vector<FlowStringValue> Results;

  GlulxFunctionInfo() = default;
  GlulxFunctionInfo(const llvm::GlulxFunctionInfo &MFI);

  void mappingImpl(yaml::IO &YamlIO) override;
  ~GlulxFunctionInfo() = default;
};

template <> struct MappingTraits<GlulxFunctionInfo> {
  static void mapping(IO &YamlIO, GlulxFunctionInfo &MFI) {
    YamlIO.mapOptional("params", MFI.Params, std::vector<FlowStringValue>());
    YamlIO.mapOptional("results", MFI.Results, std::vector<FlowStringValue>());
  }
};

} // end namespace yaml

} // end namespace llvm

#endif
