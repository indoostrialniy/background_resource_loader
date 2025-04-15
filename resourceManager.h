#pragma once
#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H


#include <iostream>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>

#define RESOURCE_MANAGER_REPORTS_OFF

// #define RESOURCE_MANAGER_DEBUG


// in the name of independency, the code of library biCycle v1.5 is already included here. See repository for more information: https://github.com/indoostrialniy/biCycle
namespace biCycle_v1_5
{	
	/*
		2025-04-15  indoostrialniy  <pawel.iv.2016@yandex.ru>
		
		biCycle library v1.5 description
		Называется велосипедом не просто так ¯\_(ツ)_/¯
	*/
	template <typename DataPack>							// DataPack - type of structure with data, which will be pushed as argument into "function" via Execute()
	struct Wrapper											// mixed func Wrapper
	{	
		std::function<bool(DataPack&)> 	function;			// returned bool signalize about completing
		DataPack						data;
		
		std::function<void()>			start_callback;		// empty callback
		std::function<void()>			end_callback;
		
		std::function<bool(DataPack&)>	startFunc;			// data callback
		std::function<bool(DataPack&)>	endFunc;
	};

	
	template <typename DataPack>	
	class Sequencer
	{
		std::vector<Wrapper<DataPack>> 	sequences;					// func-shell-vector
	public:
		~Sequencer()				{	sequences.clear(); 	}		// auto clearing when exit
		
		void CallSequence( const Wrapper<DataPack>& newSequence)	// функция CallSequence() лишь принимает ссылочную оберточную структуру Wrapper функции и сохраняет ее в приватном векторе sequences
		{
			sequences.push_back( std::move(newSequence) );	
			
			// так как вносим в конец вектора новую последовательность, которую подразумевается сразу же начать исполнять, то для вызова начального колбека
			// получим последний элемент вектора функцией .back(), которая и будет нашей вносимой
			
			if(sequences.back().startFunc)
			{	sequences.back().startFunc(sequences.back().data);	}		// вызов стартовой функции!!!
			
			if(sequences.back().start_callback)
			{ sequences.back().start_callback();	}
			
		}

		void Execute() 										// функция Execute() должна вызываться из основной части программы. Например, можно вызывать ее в каждом цикле главной петли
		{	
			if( !sequences.empty() )
			{
				auto it = sequences.begin();
				
				while (it != sequences.end()) 
				{
					if(it->function)
					{
						bool result = it->function(it->data);
					
						if (result) 
						{
							if (it->endFunc) 		it->endFunc(it->data);		
							if (it->end_callback) 	it->end_callback();
							
							it = sequences.erase(it); // erase returns next iterator
						} 
						else 
						{
							++it;
						}
					}
					else // если функция вдруг оказалась пустой
					{
						//std::cout << "\t\tEmpty main func!\n";
						
						if (it->endFunc) 		it->endFunc(it->data);
						if (it->end_callback) 	it->end_callback();
						
						it = sequences.erase(it); // erase returns next iterator
					}
					
				}
			}
		}

		
	};
	
}




namespace resource_manager
{
	/*
		2025-04-15  indoostrialniy  <pawel.iv.2016@yandex.ru>
		resource_manager v1.5 library definition
		To asynchronically load OpenGL resources, necessary is to include a biCycle v1.5 library, define an auxiliary struct "TrackerData" for catching loading final
		and a Manager, which organize it all.
	*/
	
	
	// 1. Templated struct, which store data to help track the A-type-resource loading final
	template <typename ResourceClass>
	struct TrackerData
	{
		std::string 					assetName;		// name of a resource
		std::shared_ptr<ResourceClass> 	ptr;			// smart pointer to custom-type resource
		/*... other params ...*/
	};
	
	
	
	
	// 2. Template means that there would be many different objects initialized by different types
	template <typename D>
	biCycle_v1_5::Sequencer<D> ResourceLoader;	// Execute()-member of this class must be called in every game cycle!
	
	
	
	class Resource	// base class for all resources that can be managed by this resource manager
	{
	public:
		bool getStatus() { return resourceStatus; }
		
		void initResource()													// главная инициализирующая функция. Вызывает configureResource()
		{
			try
			{
				configureResource();
			}
			catch( const std::exception& e )
			{
				std::cerr << e.what() << std::endl;
				std::string log = "Cannot configure resource " + resourceName;
				throw std::runtime_error( log );
			}
			catch(...)
			{
				std::cout << "Unknown exception by resource " << resourceName << std::endl;
			}
			
		}
		
		struct ResourceTracker
		{
			std::string assetName;
		};

	protected:
		std::string 		resourceName;											// имя ресурса
		
		bool 				resourceStatus 								= false;
		
		std::future<void> 	loader;
		
		virtual void 		configureResource()		// чисто виртуалка, обязательно переопределить
		{
			if ( !loader.valid()) 
			{  
				loader = std::async(  std::launch::async, &Resource::longLoading, this  ); 
			}
				
			ResourceTracker tracker;
							tracker.assetName = resourceName;
				
			biCycle_v1_5::Wrapper<ResourceTracker> sequenceParameters;
											sequenceParameters.function = std::bind( &Resource::trackIfReady, this, tracker);
							
			// 2. launch tracker-func
			ResourceLoader<ResourceTracker>.CallSequence( std::move(sequenceParameters) );	
			
			#ifdef RESOURCE_MANAGER_DEBUG
				std::cout << "\n----------------------------------\nHave you added [resource_manager::ResourceLoader<Resource::ResourceTracker>.Execute();] to main loop?\n----------------------------------" << std::endl;
			#endif	
		}
		
		
		virtual void 		longLoading() 								= 0; 
		
		virtual void 		quickConfiguring() 							= 0;
		
		virtual bool 		trackIfReady( ResourceTracker& tracker )		// функция, подмешиваемая во Flow для отслеживания статуса загрузки ресурса
		{
			if (loader.valid() && loader.wait_for(0s) == std::future_status::ready) // неблокируя ждем, пока поток выполнится
			{  
				quickConfiguring();		// quick func, must be called in main thread when loadMesh is ready (meshDataStatus = true)
				
				loader.get();  	// нужна для корректного завершения процесса
				
				resourceStatus = true;
				
				return true;
			} 
			return false; 
		}
		
		virtual ~Resource()	{}
	};


	
	
	
	// 3. Templated class, which manage resource loadings and configurations
	template <typename ResourceClass>
	class Manager
	{
		// this manager just contains a map of weak_ptr`s for general control, and resource management is processed via shared_ptr based on the weak_ptr from the map
	public:

		std::mutex mtx;
		
		Manager( const std::string& type)	
		{
			assetType = type;
				
			#ifndef RESOURCE_MANAGER_REPORTS_OFF
				std::cout << "\n Create resource manager for: " << assetType << std::endl;
			#endif	
		}

		~Manager() 
		{
			#ifndef RESOURCE_MANAGER_REPORTS_OFF
				std::cout << "\n Delete resource manager for: " << assetType << std::endl;	
			#endif
			
			resources.clear();	// map clearing automatically leads to deleting resources
		}
		
		
		// single-called func, which executes just in main thread
		std::shared_ptr<ResourceClass> getAsset( const std::string& assetName)	
		{
			std::shared_ptr<ResourceClass> res;										// creating new shared_ptr for resource storaging
			
			std::lock_guard<std::mutex> lock(mtx);									// blocking for multithreading
			
			if( resources.count(assetName) != 0 )									// if there are dublicates:
			{
				#ifndef RESOURCE_MANAGER_REPORTS_OFF
					std::cout << "\n\n\tResource [" << assetName << "] already has an instance, return shared_ptr reference." << std::endl;
				#endif
				
				res = resources[assetName].lock();										// noticing, that there is now one more instance of this resource
			}
			else 																	// if there are no dublicates:
			{
				#ifndef RESOURCE_MANAGER_REPORTS_OFF
					std::cout << "\tCreate new instance of object [" << assetName << "]" << std::endl;
				#endif
					
				res = std::make_shared<ResourceClass>(assetName);						// fast creating new instance
				
				// res must be derived from Resource
				res->initResource();													// initResource() can take much time! That leads to some freezing (because of main thread)
								
				resources[assetName] = std::weak_ptr<ResourceClass>(res); 				// push a weak_ptr to map
				
				#ifndef RESOURCE_MANAGER_REPORTS_OFF
					std::cout << "\tWeak prt created [" << assetName << "]" << std::endl;
				#endif
			}
			
			/// clear();																// clearing empty keys in map
			/// XXX there would be an error if you keep "clear()" above!
			
			return res;																// returning an initialized resource via a smart pointer
		}
		
		
		// single-called func, which allows to load specified resource "in background thread"/parallel 
		void getAsset_Async( const std::string& assetName, 	std::shared_ptr<ResourceClass>& res	)
		{		
			std::lock_guard<std::mutex> lock(mtx);									// blocking for multithreading

			if( resources.count(assetName) != 0 )									// if there are dublicates:
			{
				#ifndef RESOURCE_MANAGER_REPORTS_OFF
					std::cout << "\n\n\tResource [" << assetName << "] already has an instance, return shared_ptr reference." << std::endl;
				#endif
				
				res = resources[assetName].lock();									// noticing, that there is now one more instance of this resource
				
				// status of resource fromo map a priori is true, nothing to worry
			}
			else 																	// if there are no dublicates:
			{
				#ifndef RESOURCE_MANAGER_REPORTS_OFF
					std::cout << "\tCreate new instance of object [" << assetName << "]" << std::endl;
				#endif
			
				res = std::make_shared<ResourceClass>(assetName);					// fast creating new instance
								
				res->initResource();												// initResource() can take much time!
				
				// now instead of directly pushing resource instance to map "resources[assetName] = std::weak_ptr<ResourceClass>(res);" as in getAsset(),
				// we need first to track if "resourceStatus" of res is true. 
				// For this, need to store asset name and asset smart pointer in a struct called TrackerData ...
				TrackerData<ResourceClass> 	data;
											data.assetName 	= assetName;
											data.ptr 		= res;
				
				// ... specify launch a biCycle-function for catching the final of loading ...
				biCycle_v1_5::Wrapper<TrackerData<ResourceClass>> 	sequenceParameters;
																sequenceParameters.function = std::bind( &Manager::resourceStatusTracker, this, data); // remember catch-func
				
				// ... and launch it!
				ResourceLoader<TrackerData<ResourceClass>>.CallSequence( std::move(sequenceParameters) );	
				
				#ifdef RESOURCE_MANAGER_DEBUG
					std::cout << "\n----------------------------------\nHave you added [resource_manager::ResourceLoader<resource_manager::TrackerData<ResourceClass>>.Execute();] to main loop?\n----------------------------------" << std::endl;
				#endif	
				
				/// don`t forget to add 
				///	resource_manager::ResourceLoader<resource_manager::TrackerData<ResourceClass>>.Execute();
				/// in main loop!
				
				
				// resourceStatusTracker tracks and if res->resourceStatus==true, it allow storaging this configured resource in map
			}

		}
		

	protected:
		bool resourceStatusTracker( TrackerData<ResourceClass>& data )
		{
			// TrackerData stores an asset name and smart pointer to it, where can be found it`s resourceStatus
			
			if( data.ptr )
			{
				if( data.ptr->getStatus()==true )	// is loading is done:
				{					
					resources[data.assetName] = std::weak_ptr<ResourceClass>(data.ptr); 	// push a weak_ptr to map "resources"
					
					#ifndef RESOURCE_MANAGER_REPORTS_OFF
						std::cout << "\t Resource is loaded! Weak prt created [" << data.assetName << "]" << std::endl;
					#endif
					
					return true;	// returning true detach func from sequencer
				}
			}
			
			return false;			// by default return false - for endless looping
		}
		
	public:
		void clear()
		{
			std::lock_guard<std::mutex> lock(mtx);				// blocking for multithreading

			for (auto it = resources.begin(); it != resources.end(); ) 
			{
				if (it->second.expired()) 
				{
					#ifdef RESOURCE_MANAGER_REPORTS
						std::cout << "Object [" << it->first << "] was deleted!" << std::endl;
					#endif
					it = resources.erase(it);  // deleting an element and so getting next iterator
				} 
				else 
				{
					++it;
				}
			}
			
		}
		
		void printData()
		{
			std::cout << "\n Check counts!" << std::endl;
			for(auto res : resources)
				std::cout << res.first << " " << res.second.use_count() << std::endl;
		}

		
	private:
		std::string assetType = "undefined asset type";
		
		std::unordered_map <std::string, std::weak_ptr<ResourceClass> > 	resources;
	};

}







#endif
