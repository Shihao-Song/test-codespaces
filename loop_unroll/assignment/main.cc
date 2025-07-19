#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
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

/* Unrolling Section - Adjust Loop Control */
void adjustLoopControl(Loop *L, size_t unroll_factor, LLVMContext &Context) {
    BasicBlock *ExitingBlock = L->getExitingBlock();
    if (!ExitingBlock) {
        errs() << "Could not find exiting block. \n";
        return;
    }

    // Adjust the loop bound in the exiting block
    for (auto &I : *ExitingBlock) {
        if (auto *Cmp = dyn_cast<ICmpInst>(&I)) {
            if (auto *Bound = dyn_cast<ConstantInt>(Cmp->getOperand(1))) {
                // Adjust the loop bound
                ConstantInt *NewBound = ConstantInt::get(Context, Bound->getValue().udiv(unroll_factor));
                Cmp->setOperand(1, NewBound);
                break;
            }
        }
    }
}

void cloneLoopBody(Loop *L, size_t unroll_factor, LLVMContext &Context) {
    BasicBlock *Latch = L->getLoopLatch();
    if (!Latch) {
        errs() << "Could not find latch block.\n";
        return;
    }

    Function *ParentFunction = Latch->getParent();

    // Create a new basic block for the cloned body
    BasicBlock *ClonedBody = BasicBlock::Create(Context, 
                                                Latch->getName() + ".cloned",
                                                ParentFunction, 
                                                Latch);

    IRBuilder<> ClonedBodyBuilder(ClonedBody);

    Value *LastLoadResult = nullptr;
    Value *LastAddResult = nullptr;

    for (size_t i = 0; i < unroll_factor - 1; i++) {
        for (Instruction &Inst : *Latch) {
            Instruction *ClonedInst = Inst.clone();

            if (auto *LI = dyn_cast<LoadInst>(ClonedInst)) {
                LastLoadResult = ClonedInst;
            }

            if (auto *AddInst = dyn_cast<BinaryOperator>(ClonedInst)) {
                if (AddInst->getOpcode() == Instruction::Add) {
                    AddInst->setOperand(0, LastLoadResult);
                    LastAddResult = ClonedInst;
                }
            }

            if (auto *SI = dyn_cast<StoreInst>(ClonedInst)) {
                if (LastAddResult) {
                    SI->setOperand(0, LastAddResult);
                }
            }

            ClonedBodyBuilder.Insert(ClonedInst);

            if (isa<StoreInst>(Inst)) break;
        }
    }

    IRBuilder<> LatchBuilder(Latch);

    // Move the insertion point to the start of Latch
    LatchBuilder.SetInsertPoint(&Latch->front());

    // Move instructions from ClonedBody to Latch
    while (!ClonedBody->empty()) {
        Instruction &Inst = ClonedBody->front();
        Inst.removeFromParent();  // Detach from ClonedBody
        LatchBuilder.Insert(&Inst);   // Insert into Latch
    }

    ClonedBody->eraseFromParent();
}

void unrollLoop(Loop *L, size_t unroll_factor, LLVMContext &Context) 
{
    if (unroll_factor == 0)
        return;

    adjustLoopControl(L, unroll_factor, Context);
    cloneLoopBody(L, unroll_factor, Context);
}

void opt(Module &M, size_t unroll_factor) 
{
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


    for (Function &F : M) 
    {
        if (!F.isDeclaration()) 
        {
            // Run loop analysis
            LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);

            for (auto *L : LI) 
            {
                unrollLoop(L, unroll_factor, F.getContext());
            }
        }
    }

}


int main(int argc, char **argv) {
    if (argc < 4) {
        errs() << "Usage: " << argv[0] << " <input.bc> <output.bc> <unroll_factor>\n";
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
    opt(*M, std::stoi(argv[3]));

    errs() << "\n********************* New IR ********************** \n";
    M->print(errs(), nullptr);
    errs() << "*************************************************** \n";
    opt(*M, 0);

    // Write the (unmodified) module to the output bitcode file
    std::error_code EC;
    raw_fd_ostream OS(argv[2], EC);
    if (EC) {
        errs() << "Error opening file " << argv[2] << ": " << EC.message() << "\n";
        return 1;
    }
    WriteBitcodeToFile(*M, OS);

    return 0;
}

