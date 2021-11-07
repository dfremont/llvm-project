//===- IntrinsicsGlulx.td - Defines Glulx intrinsics --*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file defines all of the Glulx-specific intrinsics.
///
//===----------------------------------------------------------------------===//

let TargetPrefix = "glulx" in {  // All intrinsics start with "llvm.glulx.".

// Integer math

def int_glulx_div : GCCBuiltin<"__div">,
    Intrinsic<[llvm_i32_ty], [llvm_i32_ty, llvm_i32_ty],
              [IntrNoMem]>;     // ignore side effect of division by zero

def int_glulx_mod : GCCBuiltin<"__mod">,
    Intrinsic<[llvm_i32_ty], [llvm_i32_ty, llvm_i32_ty],
              [IntrNoMem]>;     // ignore side effect of division by zero

// Continuations

def int_glulx_catch : GCCBuiltin<"__catch">,
    Intrinsic<[llvm_i32_ty], [llvm_ptr_ty],
              [IntrArgMemOnly, WriteOnly<ArgIndex<0>>]>;

def int_glulx_throw : GCCBuiltin<"__throw">,
    Intrinsic<[], [llvm_i32_ty, llvm_i32_ty],
              [IntrNoReturn]>;

// Memory allocation heap

def int_glulx_malloc : GCCBuiltin<"__malloc">,
    Intrinsic<[llvm_ptr_ty], [llvm_i32_ty], [IntrInaccessibleMemOnly]>;

def int_glulx_mfree : GCCBuiltin<"__mfree">,
    Intrinsic<[], [llvm_ptr_ty],
              [IntrInaccessibleMemOrArgMemOnly, NoCapture<ArgIndex<0>>]>;

// Output

def int_glulx_setiosys : GCCBuiltin<"__setiosys">,
    Intrinsic<[], [llvm_i32_ty, llvm_i32_ty],
              [IntrNoMem, IntrHasSideEffects]>;

def int_glulx_streamchar : GCCBuiltin<"__streamchar">,
    Intrinsic<[], [llvm_i32_ty], []>;

def int_glulx_streamnum : GCCBuiltin<"__streamnum">,
    Intrinsic<[], [llvm_i32_ty], []>;

def int_glulx_streamstr : GCCBuiltin<"__streamstr">,
    Intrinsic<[], [llvm_anyptr_ty], []>;

def int_glulx_streamunichar : GCCBuiltin<"__streamunichar">,
    Intrinsic<[], [llvm_i32_ty], []>;

// Game state

def int_glulx_quit : GCCBuiltin<"__quit">,
    Intrinsic<[], [], [IntrNoReturn]>;

// Miscellaneous

def int_glulx_gestalt : GCCBuiltin<"__gestalt">,
    Intrinsic<[llvm_i32_ty], [llvm_i32_ty, llvm_i32_ty],
              [IntrReadMem]>;

def int_glulx_debugtrap : GCCBuiltin<"__debugtrap">,
    Intrinsic<[], [llvm_i32_ty], []>;

def int_glulx_glk : GCCBuiltin<"__glk">,
    Intrinsic<[llvm_i32_ty], [llvm_i32_ty, llvm_i32_ty], []>;

// Floating-point math

def int_glulx_ceil : GCCBuiltin<"__ceil">,
    Intrinsic<[llvm_float_ty], [llvm_float_ty],
              [IntrNoMem]>;

def int_glulx_floor : GCCBuiltin<"__floor">,
    Intrinsic<[llvm_float_ty], [llvm_float_ty],
              [IntrNoMem]>;

def int_glulx_sqrt : GCCBuiltin<"__sqrt">,
    Intrinsic<[llvm_float_ty], [llvm_float_ty],
              [IntrNoMem]>;

def int_glulx_exp : GCCBuiltin<"__exp">,
    Intrinsic<[llvm_float_ty], [llvm_float_ty],
              [IntrNoMem]>;

def int_glulx_log : GCCBuiltin<"__log">,
    Intrinsic<[llvm_float_ty], [llvm_float_ty],
              [IntrNoMem]>;

def int_glulx_pow : GCCBuiltin<"__pow">,
    Intrinsic<[llvm_float_ty], [llvm_float_ty, llvm_float_ty],
              [IntrNoMem]>;

def int_glulx_sin : GCCBuiltin<"__sin">,
    Intrinsic<[llvm_float_ty], [llvm_float_ty],
              [IntrNoMem]>;

def int_glulx_cos : GCCBuiltin<"__cos">,
    Intrinsic<[llvm_float_ty], [llvm_float_ty],
              [IntrNoMem]>;

def int_glulx_tan : GCCBuiltin<"__tan">,
    Intrinsic<[llvm_float_ty], [llvm_float_ty],
              [IntrNoMem]>;

def int_glulx_asin : GCCBuiltin<"__asin">,
    Intrinsic<[llvm_float_ty], [llvm_float_ty],
              [IntrNoMem]>;

def int_glulx_acos : GCCBuiltin<"__acos">,
    Intrinsic<[llvm_float_ty], [llvm_float_ty],
              [IntrNoMem]>;

def int_glulx_atan : GCCBuiltin<"__atan">,
    Intrinsic<[llvm_float_ty], [llvm_float_ty],
              [IntrNoMem]>;

def int_glulx_atan2 : GCCBuiltin<"__atan2">,
    Intrinsic<[llvm_float_ty], [llvm_float_ty, llvm_float_ty],
              [IntrNoMem]>;

// Block copy and clear

def int_glulx_mzero : GCCBuiltin<"__mzero">,
    Intrinsic<[], [llvm_i32_ty, llvm_ptr_ty],
              [IntrWriteMem, IntrArgMemOnly,
               NoCapture<ArgIndex<1>>, WriteOnly<ArgIndex<1>>]>;

def int_glulx_mcopy : GCCBuiltin<"__mcopy">,
    Intrinsic<[], [llvm_i32_ty, llvm_ptr_ty, llvm_ptr_ty],
              [IntrArgMemOnly,
               NoCapture<ArgIndex<1>>, ReadOnly<ArgIndex<1>>,
               NoCapture<ArgIndex<2>>, WriteOnly<ArgIndex<2>>]>;

// Searching

def int_glulx_linearsearch : GCCBuiltin<"__linearsearch">,
    Intrinsic<[llvm_ptr_ty],
              [llvm_i32_ty, llvm_i32_ty, llvm_ptr_ty, llvm_i32_ty,
               llvm_i32_ty, llvm_i32_ty, llvm_i32_ty],
              [IntrReadMem,
               NoCapture<ArgIndex<2>>, ReadOnly<ArgIndex<2>>]>;

def int_glulx_binarysearch : GCCBuiltin<"__binarysearch">,
    Intrinsic<[llvm_ptr_ty],
              [llvm_i32_ty, llvm_i32_ty, llvm_ptr_ty, llvm_i32_ty,
               llvm_i32_ty, llvm_i32_ty, llvm_i32_ty],
              [IntrReadMem,
               NoCapture<ArgIndex<2>>, ReadOnly<ArgIndex<2>>]>;

def int_glulx_linkedsearch : GCCBuiltin<"__linkedsearch">,
    Intrinsic<[llvm_ptr_ty],
              [llvm_i32_ty, llvm_i32_ty, llvm_ptr_ty,
               llvm_i32_ty, llvm_i32_ty, llvm_i32_ty],
              [IntrReadMem,
               NoCapture<ArgIndex<2>>, ReadOnly<ArgIndex<2>>]>;

// Convenient intrinsics not corresponding exactly to opcodes

def int_glulx_push : GCCBuiltin<"__push">,
    Intrinsic<[], [llvm_i32_ty], [IntrNoMem, IntrHasSideEffects]>;

def int_glulx_pop : GCCBuiltin<"__pop">,
    Intrinsic<[llvm_i32_ty], [], [IntrNoMem, IntrHasSideEffects]>;

} // TargetPrefix = "glulx"
