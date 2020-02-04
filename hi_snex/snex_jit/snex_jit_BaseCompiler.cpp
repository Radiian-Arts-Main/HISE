/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


namespace snex {
namespace jit {
using namespace juce;
using namespace asmjit;

    
    
    void BaseCompiler::executeOptimization(ReferenceCountedObject* statement, BaseScope* scope)
    {
        if(currentOptimization == nullptr)
            return;
        
        Operations::Statement::Ptr ptr(dynamic_cast<Operations::Statement*>(statement));
        
        dynamic_cast<OptimizationPass*>(currentOptimization)->processStatementInternal(this, scope, ptr);
    }
    
	

	void BaseCompiler::executePass(Pass p, BaseScope* scope, SyntaxTree* statements)
    {
        if (isOptimizationPass(p) && passes.isEmpty())
            return;
        
        setCurrentPass(p);
        
        for (auto s : *statements)
        {
            try
            {
                for (auto o : passes)
                    o->reset();
                
                if(isOptimizationPass(p))
                {
                    Operations::Statement::Ptr ptr(s);
                    
                    for(auto o: passes)
                    {
                        
                        currentOptimization = o;
                        ptr->process(this, scope);
                    }
                    
                    ptr = nullptr;
                }
                else
                    s->process(this, scope);
            }
            catch (DeadCodeException& d)
            {
                auto lineNumber = d.location.getLineNumber(d.location.program, d.location.location);
                
				juce::String m;
                m << "Skipping removed expression at Line " << lineNumber;
                s->logOptimisationMessage(m);
            }
        }
    }
    
}
}
