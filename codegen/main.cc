#include "parser/parser.hh"
#include "codegen/codegen.hh"

#include <iomanip>
#include <iostream>

using namespace Frontend;

int main(int argc, char* argv[])
{
    // Parser
    Parser parser(argv[1]);

    // LLVM IR generation
    Codegen codegen(argv[1], argv[2]);
    codegen.setParser(&parser);
    codegen.gen();
    codegen.print();
}
