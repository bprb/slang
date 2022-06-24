// slang-ir-diff-call.cpp
#include "slang-ir-diff-call.h"

#include "slang-ir.h"
#include "slang-ir-insts.h"

namespace Slang
{

struct DerivativeCallProcessContext
{
    // This type passes over the module and replaces
    // derivative calls with the processed derivative 
    // function.
    //
    IRModule*                       module;

    bool processModule()
    {
        // Run through all the global-level instructions, 
        // looking for callable blocks.
        for (auto inst : module->getGlobalInsts())
        {
            // If the instr is a callable, get all the basic blocks
            if (auto callable = as<IRGlobalValueWithCode>(inst))
            {
                // Iterate over each block in the callable
                for (auto block : callable->getBlocks())
                {
                    // Iterate over each child instruction.
                    auto child = block->getFirstInst();
                    if (!child) continue;

                    do 
                    {
                        auto nextChild = child->getNextInst();
                        // Look for IRJVPDerivativeOf
                        if (auto derivOf = as<IRJVPDerivativeOf>(child))
                        {
                            processDerivativeOf(derivOf);
                        }
                        child = nextChild;
                    } 
                    while (child);
                }
            }
        }
        return true;
    }

    // Perform forward-mode automatic differentiation on 
    // the intstructions.
    void processDerivativeOf(IRJVPDerivativeOf* derivOfInst)
    {
        IRFunc* jvpFunc = nullptr;

        // Resolve the derivative function.
        //
        // Check for the 'JVPDerivativeReference' decorator on the
        // base function.
        if (auto jvpRefDecorator = derivOfInst->base.get()->findDecoration<IRJVPDerivativeReferenceDecoration>())
        {
            jvpFunc = jvpRefDecorator->getJVPFunc();
        }
        
        // Substitute all uses of the 'derivativeOf' operation 
        // with the resolved derivative function.
        while (auto use = derivOfInst->firstUse)
        {
            use->set(jvpFunc);
        }

        // Remove the 'derivativeOf'
        derivOfInst->removeAndDeallocate();
    }
};

// Set up context and call main process method.
// 
bool processDerivativeCalls(
        IRModule* module, 
        IRDerivativeCallProcessOptions const&)
{
    DerivativeCallProcessContext context;
    context.module = module;

    return context.processModule();
}

}