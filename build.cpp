#define MTIME_IMPLEMENTATION
#include "../mtime/mtime.h"
#include "clang_dir.hpp"
#include "sim.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(x, _) _mkdir(x)
#endif

static constexpr const char *files[]{
    "actool",  "bouncer", "cl",     "cleaner",    "compile",
    "context", "dag",     "dag_io", "dag_plugin", "droid_path",
    "evoker",  "link",    "plist",  "pragma",     "target_defs",
};

constexpr const char *cmd =
    "clang++ -std=c++20 -I%s/include -L%s/lib "
#if _WIN32
    "-fms-runtime-lib=dll -nostdlib -nostdlib++ -lVersion "
    "-lclangAnalysis -lclangAnalysisFlowSensitive "
    "-lclangAnalysisFlowSensitiveModels -lclangAPINotes -lclangARCMigrate "
    "-lclangAST -lclangASTMatchers -lclangBasic -lclangCodeGen -lclangCrossTU "
    "-lclangDependencyScanning -lclangDirectoryWatcher -lclangDriver "
    "-lclangDynamicASTMatchers -lclangEdit -lclangExtractAPI -lclangFormat "
    "-lclangFrontend -lclangFrontendTool -lclangHandleCXX -lclangHandleLLVM "
    "-lclangIndex -lclangIndexSerialization -lclangInterpreter -lclangLex "
    "-lclangParse -lclangRewrite -lclangRewriteFrontend -lclangSema "
    "-lclangSerialization -lclangStaticAnalyzerCheckers "
    "-lclangStaticAnalyzerCore -lclangStaticAnalyzerFrontend -lclangSupport "
    "-lclangTooling -lclangToolingASTDiff -lclangToolingCore "
    "-lclangToolingInclusions -lclangToolingInclusionsStdlib "
    "-lclangToolingRefactoring -lclangToolingSyntax -lclangTransformer "
    "-lLLVMAggressiveInstCombine -lLLVMAnalysis -lLLVMAsmParser "
    "-lLLVMAsmPrinter -lLLVMBinaryFormat -lLLVMBitReader -lLLVMBitstreamReader "
    "-lLLVMBitWriter -lLLVMCFGuard -lLLVMCFIVerify -lLLVMCodeGen "
    "-lLLVMCodeGenTypes -lLLVMCore -lLLVMCoroutines -lLLVMCoverage "
    "-lLLVMDebugInfoCodeView -lLLVMDebuginfod -lLLVMDebugInfoDWARF "
    "-lLLVMDebugInfoGSYM -lLLVMDebugInfoLogicalView -lLLVMDebugInfoMSF "
    "-lLLVMDebugInfoPDB -lLLVMDemangle -lLLVMDiff -lLLVMDlltoolDriver "
    "-lLLVMDWARFLinker -lLLVMDWARFLinkerParallel -lLLVMDWP "
    "-lLLVMExecutionEngine -lLLVMExegesis -lLLVMExegesisX86 -lLLVMExtensions "
    "-lLLVMFileCheck -lLLVMFrontendHLSL -lLLVMFrontendOpenACC "
    "-lLLVMFrontendOpenMP -lLLVMFuzzerCLI -lLLVMFuzzMutate -lLLVMGlobalISel "
    "-lLLVMInstCombine -lLLVMInstrumentation -lLLVMInterfaceStub "
    "-lLLVMInterpreter -lLLVMipo -lLLVMIRPrinter -lLLVMIRReader -lLLVMJITLink "
    "-lLLVMLibDriver -lLLVMLineEditor -lLLVMLinker -lLLVMLTO -lLLVMMC "
    "-lLLVMMCA -lLLVMMCDisassembler -lLLVMMCJIT -lLLVMMCParser -lLLVMMIRParser "
    "-lLLVMObjCARCOpts -lLLVMObjCopy -lLLVMObject -lLLVMObjectYAML "
    "-lLLVMOption -lLLVMOrcJIT -lLLVMOrcShared -lLLVMOrcTargetProcess "
    "-lLLVMPasses -lLLVMProfileData -lLLVMRemarks -lLLVMRuntimeDyld "
    "-lLLVMScalarOpts -lLLVMSelectionDAG -lLLVMSupport -lLLVMSymbolize "
    "-lLLVMTableGen -lLLVMTableGenCommon -lLLVMTableGenGlobalISel -lLLVMTarget "
    "-lLLVMTargetParser -lLLVMTextAPI -lLLVMTransformUtils -lLLVMVectorize "
    "-lLLVMWindowsDriver -lLLVMWindowsManifest -lLLVMX86AsmParser "
    "-lLLVMX86CodeGen -lLLVMX86Desc -lLLVMX86Disassembler -lLLVMX86Info "
    "-lLLVMX86TargetMCA -lLLVMXRay";
#else
    "-lclang -lclang-cpp -lLLVM";
#endif

bool compile(const char *stem) {
  sim_sbt in{32};
  sim_sbt out{32};
  sim_sb_printf(&in, "%s.cpp", stem);
  sim_sb_printf(&out, "out/%s.o", stem);

  if (mtime_of(in.buffer) < mtime_of(out.buffer))
    return true;

  auto cdir = clang_dir();
  sim_sbt buf{1024};
  sim_sb_copy(&buf, "clang++ -std=c++20 ");
#ifdef _WIN32
  sim_sb_copy(&buf, "-D_CRT_SECURE_NO_WARNINGS -fms-runtime-lib=dll ");
#endif
  sim_sb_printf(&buf, "-I%s/include -c %s -o %s", cdir, in.buffer, out.buffer);
  return 0 == system(buf.buffer);
}
bool link(const char *outf) {
  auto cdir = clang_dir();
  sim_sbt buf{10240};
  sim_sb_printf(&buf, cmd, cdir, cdir);
  for (auto f : files) {
    sim_sb_printf(&buf, " out/%s.o ", f);
  }
  sim_sb_concat(&buf, outf);
  return 0 == system(buf.buffer);
}

int main(int argc, char **argv) {
  mkdir("out", 0777);
  for (auto f : files) {
    if (!compile(f))
      return 1;
  }
  if (!compile("leco"))
    return 1;
  if (!link("out/leco.o -o leco.exe"))
    return 1;
  if (!compile("null_pragma"))
    return 1;
  if (!link("out/null_pragma.o -o null_pragma.dll -shared"))
    return 1;
  return 0;
}
