#include <iostream>
#include <iomanip>
#include <future>
using namespace std::chrono_literals;

#include "Resources.h"

int main()
{
	try
	{
		//~ auto tst = TestResources.getAsset("my test variable");
		std::shared_ptr<TestResource> tst;
		TestResources.getAsset_Async("my test", tst);
	
		int s=0;
		bool flag = false;
		while (1)
		{		
			std::cout << "\r\t\tstep= " << std::setw(8) << s << " resource ready: " << tst->getStatus() << std::flush;
			
			//~ if(s > 1000000 && !flag)
			//~ {
				//~ flag = true;
				//~ std::shared_ptr<TestResource> tst2 = TestResources.getAsset("my test");
			//~ }
			
			resource_manager::ResourceLoader<resource_manager::TrackerData<TestResource>>.Execute();
			
			resource_manager::ResourceLoader<resource_manager::Resource::ResourceTracker>.Execute();
			s++;
			
			if(s>2010000)
			{
				break;
			}
		}
	}
	catch( const std::exception& e )
	{
		std::cerr << "\nError in example program: " << e.what() << std::endl;
		
	}
	
    return 0;
}


#include "Resources.cpp"
