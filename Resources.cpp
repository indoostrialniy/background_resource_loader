#include "Resources.h"

Manager <Test> 					TestManager("Test");



Test::Test( const std::string& meshName )
{	
	resourceName = meshName;
}

Test::~Test()
{

}

void Test::longLoading()	// some heavy func, processed in background thread
{	
	
}
	
void Test::quickConfiguring() // some quick func
{

	resourceStatus = true;
}



