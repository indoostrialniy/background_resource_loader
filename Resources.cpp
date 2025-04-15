#include "Resources.h"

#include <thread>

//---------------------------------------------------------------------- 
//---------------------------------------------------------------------- 
//---------------------------------------------------------------------- 
// TestResource
//---------------------------------------------------------------------- 
//---------------------------------------------------------------------- 
//---------------------------------------------------------------------- 
TestResource::TestResource( const std::string& meshName )
{	
	resourceName = meshName;
}


TestResource::~TestResource()
{
	
}


void TestResource::longLoading()								// runs in background thread
{
	std::this_thread::sleep_for(3000ms);
}
	

void TestResource::quickConfiguring()							// runs in main thread
{
	std::cout << "\n\n\t quickConfiguring is called from main thread!\n\n" << std::endl;
}

