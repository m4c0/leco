#include "clang_dir.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

constexpr const char *cmd =
    "clang++ -g -std=c++20 -o leco.exe -I%s/include -L%s/lib "
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
    " leco.o"
    " compile.o"
    " evoker.o";

#if _WIN32
#define stat _stat
#endif

bool compile(const char *stem) {
  auto cdir = clang_dir();
  char buf[1024];
  snprintf(buf, sizeof(buf),
           "clang++ -g -std=c++20 -I%s/include -c %s.cpp -o %s.o", cdir, stem,
           stem);
  return 0 == system(buf);
}
bool link() {
  auto cdir = clang_dir();
  char buf[1024];
  snprintf(buf, sizeof(buf), cmd, cdir, cdir);
  return 0 == system(buf);
}

int main(int argc, char **argv) {
  return compile("leco") && compile("compile") && compile("evoker") && link()
             ? 0
             : 1;
}
