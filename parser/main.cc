#include "lexer/lexer.hh"
#include "parser/parser.hh"

#include <iomanip>
#include <iostream>

using namespace Frontend;

int main(int argc, char* argv[])
{
    // Parser
    Parser parser(argv[1]);
    parser.printStatements();
}
