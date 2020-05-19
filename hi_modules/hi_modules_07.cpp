

#include "hi_modules.h"
#include "nodes/examples/hello_world.h"
#include "nodes/examples/property_demo.h"
#if 0
#include "nodes/examples/combined.h"
#include "nodes/examples/event_wrapped.h"

#endif

namespace scriptnode
{
using namespace hise;
using namespace juce;


namespace examples
{
Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
	registerNode<hello_world>({});
	//registerNode<combined>({});
	//registerNode<event_processor>({});
	registerNode<property_demo>({});
}
}


}
