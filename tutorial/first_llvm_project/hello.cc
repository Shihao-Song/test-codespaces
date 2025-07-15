#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

/*
 * The purpose of 'using namespace llvm;' is illustrated in later comments.
 * */
using namespace llvm;

/*
 * 1. 'static':
 *     * The 'static' keyword specifies that 'FileName' has static duration, 
 *       meaning it is allocated when the program starts and deallocated when 
 *       the program ends. This is typical for command line options which need 
 *       to be accessible throughout the lifetime of the program.
 *
 * 2. 'cl::opt<std::string>':
 *     * 'cl::opt' is a template class from the LLVM command line parsing library 
 *       (cl stands for command line). This class is used to define a command line option.
 *
 *     * Definition of 'namespace cl' can be found in 
 *       ~/Documents/llvm/llvm-project/llvm/include/llvm/Support/CommandLine.h, line 53.
 *
 *     * Definition of 'class opt' can be found in 
 *       ~/Documents/llvm/llvm-project/llvm/include/llvm/Support/CommandLine.h, line 1412.
 *       As seen, 'class opt' belongs to 'namespace cl' and 'namespace cl' belongs to 
 *       'namespace llvm'. 
 *
 *     * If we didn't have 'using namespace llvm;' in the first place, we would have to do
 *       'llvm::cl::opt<std::string> FileName...'
 *
 *     * The template parameter '<std::string>' specifies that this command line option 
 *       will hold a value of type std::string.
 *
 * 3. 'cl::Positional':
 *     * This argument specifies that FileName is a positional option. Positional options 
 *       are those that are specified without an accompanying flag on the command line 
 *       (e.g., ./program input_file as opposed to ./program --input-file=input_file).
 *
 * 4. 'cl::desc("Bitcode file")':
 *     * 'cl::desc' is a descriptive string that explains what the option is for. 
 *       This description is used when generating help text for the command line 
 *       interface. In this case, the description indicates that this option is expected 
 *       to be the name of a bitcode file.
 *
 * 5. 'cl::Required':
 *     * This argument specifies that FileName is a required option. The program 
 *       will report an error and exit if this option is not provided on the command line.
 * */
static cl::opt<std::string> FileName(cl::Positional,
                                     cl::desc("Bitcode file"), 
                                     cl::Required);

// TODO - uncomment for your assignment
// void printInstructionList(Function &F) {
//     for (/**/) { // Fill in this section to iterate through each basic block in the function
//         for (/**/) { // Fill in this section to iterate through each instruction in the basic block
               // Do something with each instruction I, e.g., print it:
//             I->print(errs());
//             errs() << "\n";
//         }
//     }
// }


/*
 * 1. 'argc':
 *     This is the count of command-line arguments passed to the program. 
 *     It is a traditional parameter passed to the main() function in C++.
 *
 * 2. 'argv':
 *     This is an array of char pointers representing the actual command-line 
 *     arguments passed to the program. Like argc, it is also a traditional 
 *     parameter passed to the main() function.
 * */
int main(int argc, char** argv) 
{
    /*
     * cl::ParseCommandLineOptions is a function provided by LLVM for parsing command-line 
     * arguments.
     *
     * Purpose - 
     * * When you declare a command-line option using the cl::opt template class, like in line 
     *       static cl::opt<std::string> FileName(cl::Positional, 
     *                                            cl::desc("Bitcode file"), 
     *                                            cl::Required);, 
     *   the 'FileName' option gets registered GLOBALLY with LLVM's command-line parsing library. 
     *   This registration is done behind the scenes by the cl::opt class's constructor.
     *
     * * When you later call cl::ParseCommandLineOptions(argc, argv, "LLVM hello world\n");, 
     *   the function goes through the command-line arguments provided in argv, 
     *   and matches them against the list of globally registered command-line options.
     *
     * * When a match is found between a command-line argument and a registered option, 
     *   the value of that argument is assigned to the corresponding variable—
     *   in this case, FileName. This assignment is done automatically by the 
     *   cl::opt class's assignment operator.
     * 
     * */
    cl::ParseCommandLineOptions(argc, argv, 
                                "LLVM hello world\n");

    /*
     * Read the bitcode file (FileName) into memory buffer
     *
     * 1. 'MemoryBuffer::getFile(FileName)' is a static method call to the LLVM MemoryBuffer 
     *    class to create a memory buffer to store the content of the file specified by FileName.
     *
     * 2. 'std::unique_ptr<MemoryBuffer>' is a smart pointer that will hold the memory buffer. 
     *    This type of pointer will automatically delete the memory buffer when it's no longer 
     *    needed, helping prevent memory leaks.
     *
     * 3. 'ErrorOr<std::unique_ptr<MemoryBuffer>> mb' declares a variable mb of type ErrorOr, 
     *    parameterized with std::unique_ptr<MemoryBuffer>. ErrorOr is an LLVM type that can 
     *    hold either a value (in this case, a std::unique_ptr<MemoryBuffer>) or an error code. 
     *    This is a way of signaling whether the operation succeeded or failed.
     *
     * 4. If an error occurred, the block of code inside the if statement is executed.
     *    * 'errs() << ec.message();' prints the error message to the standard error output. 
     *      'errs()' is an LLVM function that returns a reference to a raw_ostream for standard 
     *      error, and 'ec.message()' retrieves a human-readable error message from the error code 'ec'.
     *
     *    * 'return -1;' exits the program with a return code of -1, indicating an error occurred.
     *
     * */

    ErrorOr<std::unique_ptr<MemoryBuffer>> mb =
        MemoryBuffer::getFile(FileName);
    if (std::error_code ec = mb.getError())
    {
        errs() << ec.message();
        return -1;
    }

    /*
     * 'LLVMContext' is a class from the LLVM library that holds all of the LLVM-related data 
     * for a single compilation. It's a necessary part of working with LLVM, as many other 
     * LLVM classes and functions require a LLVMContext as an argument. 
     *
     * 'LLVMContext context;' creates a new instance of LLVMContext, named context.
     *
     * */
    LLVMContext context;

    /*
     * 1. 'parseBitcodeFile' is a function that parses an LLVM bitcode file and returns a 
     *    module representing the contents of that file.
     *
     * 2. 'mb->get()->getMemBufferRef()' is calling methods to get a reference to the memory 
     *    buffer holding the bitcode file data. 'mb' is a 'std::unique_ptr' to a 'MemoryBuffer' 
     *    (from the previous code snippet), 'get()' gets the MemoryBuffer pointer from the 
     *    'std::unique_ptr', and 'getMemBufferRef()' gets a reference to the memory buffer 
     *    from the MemoryBuffer pointer.
     *
     * 3. 'context' is the 'LLVMContext' created earlier, and is passed to 'parseBitcodeFile' 
     *    to associate the parsed module with this context.
     *
     * 4. 'Expected<std::unique_ptr<Module>> m' declares a variable m to hold the result of 
     *    parsing the bitcode file. Expected is an LLVM template class similar to ErrorOr, 
     *    which can hold either a value or an error. In this case, it will hold a std::unique_ptr 
     *    to a Module if parsing succeeds, or an error if parsing fails.
     *
     * */
    Expected<std::unique_ptr<Module>> m = 
        parseBitcodeFile(mb->get()->getMemBufferRef(), context);

    /*
     * 1. 'm.get()->getFunctionList().begin()' and 'm.get()->getFunctionList().end()' are obtaining 
     *    iterators to the beginning and end of the function list in the LLVM module.
     *
     * 2. 'Module::const_iterator i' and 'e' are iterators used to traverse the list of functions 
     *    in the module. 'i' is initialized to the beginning of the function list, and 'e'
     *    is set to the end of the function list.
     *
     * 3. The 'for' loop will continue as long as 'i' is not equal to 'e', incrementing 'i' at the 
     *    end of each iteration with '++i'.
     *
     * */
    for (Module::const_iterator i = m.get()->getFunctionList().begin(), 
         e = m.get()->getFunctionList().end(); i != e; ++i) 
    {
        /*
	 * 1. 'i->isDeclaration()' checks whether the function pointed to by 'i' is a declaration 
	 *    or a definition. In LLVM, a declaration is a function that is declared but not defined 
	 *    within the current module (i.e., it doesn't have a body), whereas a definition is a 
	 *    function that has a body.
	 *
	 * 2. The '!' operator negates the result, so '!i->isDeclaration()' is 'true' if the function 
	 *    is a definition, and 'false' if it's a declaration.
	 * */
        if (!i->isDeclaration())
        {
            /* 'i->getName()' obtains the name of the function pointed to by 'i'. */
            outs() << "Function name - " << i->getName() << "\n";

            // TODO - uncomment for your assignment
            // outs() << "Instructions - " << "\n";
            // printInstructionList(const_cast<Function&>(*i));
            // outs() << "\n";
        }
    }

    return 0;
}

