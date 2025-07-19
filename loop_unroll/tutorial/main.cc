#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Passes/PassBuilder.h"
#include <memory>
#include <string>
#include <cstdlib>

using namespace llvm;

int getLoopStartVal(const Loop *L) {
    BasicBlock *Preheader = L->getLoopPreheader();
    if (!Preheader) {
        return -1;  // Sentinel value indicating no start value found
    }

    Instruction *LastInst = Preheader->getTerminator();
    if (!LastInst || !isa<BranchInst>(LastInst) || LastInst->getPrevNode() == nullptr) {
        return -1;
    }

    Instruction *PotentialInitInst = LastInst->getPrevNode();
    if (StoreInst *SI = dyn_cast<StoreInst>(PotentialInitInst)) {
        if (ConstantInt *CI = dyn_cast<ConstantInt>(SI->getValueOperand())) {
            return CI->getSExtValue();  // Return the integer value
        }
    }

    return -1;  // Start value not found or not a constant integer
}

int getLoopEndVal(const Loop *L) {
    BasicBlock *ExitingBlock = L->getExitingBlock();
    if (!ExitingBlock) {
        return -1;  // Sentinel value indicating no end value found
    }

    // Find the comparison instruction in the exiting block
    for (Instruction &I : *ExitingBlock) {
        if (CmpInst *CmpI = dyn_cast<CmpInst>(&I)) {
            // Assuming a simple comparison like 'i < EndVal'

            // Check if one of the operands is the loop induction variable
            // and the other is a constant integer
            for (int OpIdx = 0; OpIdx < 2; ++OpIdx) {
                Value *Op = CmpI->getOperand(OpIdx);
                if (auto *CI = dyn_cast<ConstantInt>(Op)) {
                    return CI->getSExtValue();  // Return the integer value
                }
            }
        }
    }

    return -1;  // End value not found or not a constant integer
}

int getLoopStepVal(const Loop *L) {
    // The step value is typically updated in the loop's latch block
    BasicBlock *LatchBlock = L->getLoopLatch();
    if (!LatchBlock) {
        return 0; // No latch block
    }

    // Look for the update to the induction variable in the latch block
    for (auto I = LatchBlock->rbegin(); I != LatchBlock->rend(); ++I) {
        if (auto *BI = dyn_cast<BinaryOperator>(&*I)) {
            if ((BI->getOpcode() == Instruction::Add || BI->getOpcode() == Instruction::Sub)) {
                if (ConstantInt *StepVal = dyn_cast<ConstantInt>(BI->getOperand(1))) {
                    return (BI->getOpcode() == Instruction::Sub) ? -StepVal->getSExtValue() 
                                                                 : StepVal->getSExtValue();
                }
            }
        }
    }

    return 0; // Step value not found or not constant
}

void printLoopInfo(Loop *L) 
{
    auto loop_start_val = getLoopStartVal(L);
    auto loop_end_val   = getLoopEndVal(L);
    auto loop_step_val   = getLoopStepVal(L);

    errs() << "Loop Info:  \n";
    errs() << "    start val - " << loop_start_val << "\n";
    errs() << "    end val - " << loop_end_val << "\n";
    errs() << "    step val - " << loop_step_val << "\n";
}

void identifyLoops(Module &M) {
    // Create the analysis managers
    FunctionAnalysisManager FAM;
    LoopAnalysisManager LAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;

    // Create the pass builder and register analysis managers
    PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    for (Function &F : M) {
        if (!F.isDeclaration()) {
            // Run loop analysis
            LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);

            errs() << "\nFunction: " << F.getName() << "\n";
            for (auto *L : LI) {
                printLoopInfo(L);
            }
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        errs() << "Usage: " << argv[0] << " <input.bc>\n";
        return 1;
    }

    LLVMContext Context;
    SMDiagnostic Err;

    // Read the input bitcode
    std::unique_ptr<Module> M = parseIRFile(argv[1], Err, Context);
    if (!M) {
        Err.print(argv[0], errs());
        return 1;
    }
    errs() << "\n******************* Original IR ******************* \n";
    M->print(errs(), nullptr);
    errs() << "*************************************************** \n";

    // Identify loops
    identifyLoops(*M);

    return 0;
}
