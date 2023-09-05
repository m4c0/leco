module;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

export module build;

constexpr const char *cmd =
    "clang++ -std=c++20 -o leco.exe leco.cppm -I%s/include -L%s/lib "
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
    "-lLLVMX86TargetMCA -lLLVMXRay"
#else
    "-lclang -lclang-cpp -lLLVM"
#endif
    ;

#if _WIN32
const char *clang_dir() {
  char buf[1024];
  auto f = _popen("where clang++", "r");
  if (fgets(buf, 1024, f) == nullptr)
    throw 0;

  buf[strlen(buf)] = 0;
  *strrchr(buf, '\\') = 0;
  *strrchr(buf, '\\') = 0;
  return _strdup(buf);
}
#else
const char *clang_dir() {
  char buf[1024];
  auto f = popen("brew --prefix llvm@16", "r");
  if (fgets(buf, 1024, f) == nullptr)
    throw 0;

  buf[strlen(buf)] = 0;
  return _strdup(buf);
}
#endif

extern "C" int main(int argc, char **argv) {
  auto cdir = clang_dir();
  char buf[10240];
  snprintf(buf, sizeof(buf), cmd, cdir, cdir);
  return system(buf);
}
