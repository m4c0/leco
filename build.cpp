#include "clang_dir.hpp"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static constexpr const char *files[]{
    "leco",    "bouncer", "cl",         "cleaner",     "compile",
    "context", "dag",     "droid_path", "evoker",      "instance",
    "link",    "plist",   "pragma",     "target_defs",
};

constexpr const char *cmd =
    "clang++ -std=c++20 -o leco.exe -I%s/include -L%s/lib "
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

uint64_t mtime(const char *stem, const char *ext) {
  char buf[128];
  snprintf(buf, sizeof(buf), "%s.%s", stem, ext);

#if _WIN32
  struct __stat64 s {};
  _stat64(buf, &s);
  return s.st_mtime;
#else
  struct stat s {};
  stat(buf, &s);
  auto mtime = s.st_mtimespec;
  return static_cast<uint64_t>(mtime.tv_sec) * 1000000000ul +
         static_cast<uint64_t>(mtime.tv_nsec);
#endif
}

bool compile(const char *stem) {
  if (mtime(stem, "cpp") < mtime(stem, "o"))
    return true;

  auto cdir = clang_dir();
  char buf[1024];
  snprintf(buf, sizeof(buf),
           "clang++ -std=c++20 "
#ifdef _WIN32
           "-D_CRT_SECURE_NO_WARNINGS -fms-runtime-lib=dll "
#endif
           "-I%s/include -c %s.cpp -o %s.o",
           cdir, stem, stem);
  return 0 == system(buf);
}
#ifdef _WIN32
#define strncat strncat_s
#endif
bool link() {
  auto cdir = clang_dir();
  char buf[10240];
  snprintf(buf, sizeof(buf), cmd, cdir, cdir);
  for (auto f : files) {
    strncat(buf, " ", sizeof(buf) - 1);
    strncat(buf, f, sizeof(buf) - 1);
    strncat(buf, ".o", sizeof(buf) - 1);
  }
  return 0 == system(buf);
}

int main(int argc, char **argv) {
  for (auto f : files) {
    if (!compile(f))
      return 1;
  }
  return link() ? 0 : 1;
}
