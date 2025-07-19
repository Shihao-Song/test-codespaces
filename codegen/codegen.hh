#ifndef __CODEGEN_HH__
#define __CODEGEN_HH__

#include "parser/parser.hh"

// LLVM IR codegen libraries
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Bitcode/BitcodeWriter.h"

using namespace llvm;

namespace Frontend
{
class Codegen
{
  protected:
/*

 Context vs Module

 Module -  

     * The Module is a container for the functions, global variables, and other 
       top-level constructs that make up your program or a part of your program.
     
     * It represents a single compilation unit or a collection of code that is 
       compiled together. In many ways, it's akin to a file in C or C++.

     * Each Module object is associated with a LLVMContext. All the IR constructs 
       (like functions, instructions, and types) that you add to a Module must be 
       created in the context associated with that module.

 Context - 

     * The LLVMContext represents the environment or context in which your LLVM 
       IR is constructed and exists. It holds LLVM's internal state and ensures 
       consistency across the IR that you generate.
     
     * It encapsulates things like unique identifier tables (for types, constants, 
       global values, etc.), which helps in managing the overall state and data of 
       your IR generation process.
     
     * A single LLVMContext can be used to create multiple modules, but all entities 
       within a module (like functions, types, and values) must be created within the 
       same context. This ensures that the IR in a module is self-consistent.

  Context examples - 
     LLVM pools constants like integers, floating-point values, and strings. If you 
     create a constant value (e.g., an integer with value 42), and later create the 
     same constant again, LLVM will reuse the existing constant. This pooling helps 
     in reducing memory usage and also in simplifying the optimization process, as the 
     optimizer can easily recognize and operate on unique constant values.

 * */

    std::unique_ptr<LLVMContext> context;
    std::unique_ptr<Module> module;
    std::unique_ptr<IRBuilder<>> builder;

    std::string mod_name;
    std::string out_fn;

    Parser* parser;

    size_t num_loops_per_func = 0;

  public:

    Codegen(const char* _mod_name,
            const char* _out_fn)
        : mod_name(_mod_name)
        , out_fn(_out_fn)
    {}

    void setParser(Parser *_parser)
    {
        parser = _parser;
    }

    void gen();

    void print();

  protected:
    std::vector<std::unordered_map<std::string,
                                   ValueType::Type>*> local_vars_ref;
    std::vector<std::unordered_map<std::string,Value*>> local_vars_tracker;

    void recordLocalVar(std::string& var_name, Value* reg)
    {
        auto &tracker = local_vars_tracker.back();
        tracker.insert({var_name, reg});
    }

    ValueType::Type getValType(std::string& _var_name)
    {
        for (int i = local_vars_ref.size() - 1;
                 i >= 0;
                 i--)
        {
            auto &ref = local_vars_ref[i];

            if (auto iter = ref->find(_var_name);
                    iter != ref->end())
            {
                return iter->second;
            }
        }
    }
    
    std::pair<bool,Value*> getReg(std::string& _var_name)
    {
        for (int i = local_vars_tracker.size() - 1;
                 i >= 0;
                 i--)
        {
            auto &tracker = local_vars_tracker[i];

            if (auto iter = tracker.find(_var_name);
                    iter != tracker.end())
            {
                return std::make_pair(true,iter->second);
            }
        }
        return std::make_pair(false,nullptr);
    }

    void statementGen(std::string&, Statement*);

    void funcGen(Statement *);
    void assnGen(Statement *);
    void builtinGen(Statement *);
    void callGen(Statement *);
    void retGen(std::string &,Statement *);

    Value* condGen(Condition*);
    void ifGen(std::string&,Statement *);
    void forGen(std::string&,Statement *);

    Value* allocaForIden(std::string&,
                         ValueType::Type&,
                         Expression*,
                         ArrayExpression*);
   
    Value* exprGen(ValueType::Type,Expression*);

    void arrayExprGen(ValueType::Type,
                      Value*,
                      ArrayExpression*);

    Value* arithExprGen(ValueType::Type,ArithExpression*);

    Value* literalExprGen(ValueType::Type, LiteralExpression*);

    Value* indexExprGen(ValueType::Type, IndexExpression*);

    Value* callExprGen(CallExpression*);
};
}

#endif
