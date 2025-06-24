#pragma once
#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

/*
	2025-06-24  indoostrialniy  <pawel.iv.2016@yandex.ru>
	
	resource manager library v1.6 description
*/

#include <string>

#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <chrono>
using namespace std::chrono_literals;
#include <functional>
#include <future>
#include <iostream>


constexpr bool ASYNCHRO = true;		//#define ASYNCHRO true



// 1. In the name of independency, the code of library biCycle v1.5 is already included here. See repository for more information: https://github.com/indoostrialniy/biCycle
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

		~Sequencer() { sequences.clear(); }		// auto clearing when exit

		void CallSequence(const Wrapper<DataPack>& newSequence)	// функция CallSequence() лишь принимает ссылочную оберточную структуру Wrapper функции и сохраняет ее в приватном векторе sequences
		{
			sequences.push_back(std::move(newSequence));

			// так как вносим в конец вектора новую последовательность, которую подразумевается сразу же начать исполнять, то для вызова начального колбека
			// получим последний элемент вектора функцией .back(), которая и будет нашей вносимой

			if (sequences.back().startFunc)
			{
				sequences.back().startFunc(sequences.back().data);
			}		// вызов стартовой функции!!!

			if (sequences.back().start_callback)
			{
				sequences.back().start_callback();
			}

		}

		void Execute() 										// функция Execute() должна вызываться из основной части программы. Например, можно вызывать ее в каждом цикле главной петли
		{
			if (!sequences.empty())
			{
				auto it = sequences.begin();

				while (it != sequences.end())
				{
					if (it->function)
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


// 2. Struct with data to track resource status via trackIfReady()
struct ResourceTracker
{
	// in general, we often need only one string - assetNamem but for extensibility we use it via a struct
	std::string assetName;
	// perhaps some other params
};



// 3. Base class for all resources that can be managed by this resource manager
class Resource
{
public:

	biCycle_v1_5::Sequencer<ResourceTracker> ResourceTrackerSeq;	// this sequencer executes trackIfReady-func. Is called from resourceStatusTracker

	
	void initResource( bool asynchro = false )								// main initializing func. can process exceptions
	{
		try
		{
			if (asynchro)
			{
				if ( !loader.valid() )	
				{
					loader = std::async(std::launch::async, &Resource::longLoading, this);
				}

				ResourceTracker tracker;
								tracker.assetName = resourceName;

				biCycle_v1_5::Wrapper<ResourceTracker> sequenceParameters;	// shell Wrapper initialized with custom type ResourceTracker
				sequenceParameters.function = std::bind(&Resource::trackIfReady, this, tracker);	// bind tracker func. At the end specifies resourceStatus to true!

				// 2. launch tracker-func
				//ResourceLoader<ResourceTracker>.CallSequence(std::move(sequenceParameters));
				ResourceTrackerSeq.CallSequence(std::move(sequenceParameters));

				#ifdef RESOURCE_MANAGER_DEBUG
					std::cout << "\n----------------------------------\nHave you added [resource_manager::ResourceLoader<Resource::ResourceTracker>.Execute();] to main loop?\n----------------------------------" << std::endl;
				#endif
			}
			else
			{
				// по умолчанию выполняется загрузка в главном потоке. С зависанием в случае "тяжелых" ресурсов.
				longLoading();
				quickConfiguring(); // в конце выставляет resourceStatus = true!
			}
			
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
			std::string log = "Cannot configure resource " + resourceName;
			throw std::runtime_error(log);
		}
		catch (...)
		{
			std::cout << "Unknown exception by resource " << resourceName << std::endl;
		}

		// in case of exception simply do nothing
	}

	bool getStatus() { return resourceStatus; }

	std::string 		resourceName;											// имя ресурса

protected:



	

	bool 				resourceStatus = false;

	std::future<void> 	loader;

	virtual void 		longLoading() = 0;	// in case of asynchronical loading this func executes in PARALLEL thread

	virtual void 		quickConfiguring() = 0;	// this func executes in MAIN thread

	bool 				trackIfReady(ResourceTracker& tracker)		// функция, подмешиваемая во Flow для отслеживания статуса загрузки ресурса
	{
		//std::cout << "trackIfReady" << std::endl;

		if (loader.valid() && loader.wait_for(0s) == std::future_status::ready) // неблокируя ждем, пока поток выполнится
		{
			quickConfiguring();		// quick func, must be called in main thread when loadMesh is ready (meshDataStatus = true)

			loader.get();  	// нужна для корректного завершения процесса

			resourceStatus = true;	// resource is correctly loaded in parallel thread, set status to true

			return true;	// detach from sequencer
		}
		return false;	// not detach fro sequencer
	}

	virtual				~Resource() {}
};


// 4. Templated struct, which store data to help track the A-type-resource loading final
template <typename ResourceClass>
struct TrackerData
{
	std::string 					assetName;		// name of a resource
	std::shared_ptr<ResourceClass> 	ptr;			// smart pointer to custom-type resource
	/*... other params ...*/
};



// 5. Templated class, which manage resource loadings and configurations
template <typename ResourceClass>
class Manager
{
	// this manager just contains a map of weak_ptr`s for general control, and resource management is processed via shared_ptr based on the weak_ptr from the map
public:

	std::mutex mtx;

	

	biCycle_v1_5::Sequencer<TrackerData<ResourceClass>> StatusTracker;	// четко здесь! секвенсор для отслеживания завершенности загрузки. не забыть включить Execute

	

	Manager(const std::string& type)
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


	void getAsset(const std::string& assetName, std::shared_ptr<ResourceClass>& res, bool asynchro, auto funcClb = []() {std::cout << "TEST CLB\n";})
	{
		std::lock_guard<std::mutex> lock(mtx);									// blocking for multithreading

		if (resources.count(assetName) != 0 && resources[assetName].lock() != nullptr)									// if there are dublicates:
		{
			// если не проверять resources[assetName].lock() != nullptr, то выходит, что в словаре есть ключ с нужным именем, но weak_ptr ранее был автоматически очищен
			
			#ifndef RESOURCE_MANAGER_REPORTS_OFF
				std::cout << "\n\n\t getAsset: Resource [" << assetName << "] already has an instance, return shared_ptr reference." << std::endl;
			#endif

			res = resources[assetName].lock();									// noticing, that there is now one more instance of this resource
			// status of resource from map apriori is true, nothing to worry
		}
		else 																	// if there are no dublicates:
		{
			#ifndef RESOURCE_MANAGER_REPORTS_OFF
				std::cout << "\t getAsset: Create new instance of object [" << assetName << "]" << std::endl;
			#endif

			res = std::make_shared<ResourceClass>(assetName);					// fast creating new empty instance

			res->initResource(asynchro);										// initResource() can take much time and throw exceptions
			
			// если initResource выкинет исключение, то выполнение не пойдет дальше
			// res останется пустым и его resourceStatus = false;
			
			if (asynchro)
			{
				// now instead of directly pushing resource instance to map "resources[assetName] = std::weak_ptr<ResourceClass>(res);" as in getAsset(),
				// we need first to track if "resourceStatus" of res is true. 
				// For this, need to store asset name and asset smart pointer in a struct called TrackerData ...
				TrackerData<ResourceClass> 	data;
				data.assetName = assetName;
				data.ptr = res;

				// ... specify launch a biCycle-function for catching the final of loading ...
				biCycle_v1_5::Wrapper<TrackerData<ResourceClass>> 	sequenceParameters;
						sequenceParameters.function = std::bind(&Manager::resourceStatusTracker, this, data); // remember catch-func
						sequenceParameters.end_callback = funcClb;	// specify end-func of resource loading ///		end_clb;

				// ... and launch it!
				StatusTracker.CallSequence( std::move(sequenceParameters) );		//ResourceLoader<TrackerData<ResourceClass>>.CallSequence(std::move(sequenceParameters));

				#ifdef RESOURCE_MANAGER_DEBUG
					std::cout << "\n----------------------------------\nHave you added [resource_manager::ResourceLoader<resource_manager::TrackerData<ResourceClass>>.Execute();] to main loop?\n----------------------------------" << std::endl;
				#endif	

				/// don`t forget to add 
				///	resource_manager::ResourceLoader<resource_manager::TrackerData<ResourceClass>>.Execute();
				/// in main loop!
			}
			else
			{
				// если выбрана не асинхронная загрузка
				
					if (res->getStatus() == true )	//data.ptr->getStatus() == true)	// is loading is done:
					{
						resources[assetName] = std::weak_ptr<ResourceClass>(res); 	// push a weak_ptr to map "resources"

						#ifndef RESOURCE_MANAGER_REPORTS_OFF
							std::cout << "\t Resource is loaded! Weak prt created [" << assetName << "]" << std::endl;
						#endif
						
						funcClb();
						//return true;	// returning true detach func from sequencer
					}
				

			}
			// initResource запускает собственный отслеживатель! И этого достаточно для фоновой загрузки? (Не совсем)
			// не забыть только вызывать 	ResourceLoader<ResourceTracker>.Execute!


			


			// resourceStatusTracker tracks and if res->resourceStatus==true, it allow storaging this configured resource in map
		}

	}



protected:
	bool resourceStatusTracker(TrackerData<ResourceClass>& data)
	{
		// TrackerData stores an asset name and smart pointer to it, where can be found it`s resourceStatus

		// this func tracks if resource Status is true
		//std::cout << "resourceStatusTracker" << std::endl;

		// and calls the resource loader tracker
		data.ptr->ResourceTrackerSeq.Execute();

		if (data.ptr)
		{
			if (data.ptr->getStatus() == true)	// is loading is done:
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
		for (auto res : resources)
			std::cout << res.first << " " << res.second.use_count() << std::endl;
	}


	//void executeResourceTrackerSeq() // обязательно вызывать в каждом кадре!
	//{
	//	//std::cout << "\texecuteResourceTrackerSeq" << std::endl;
	//	for (auto res : resources)
	//	{

	//		// он пытается отслеживать, ориентируясь на словарь. НО словарь то пуст! до момента полной инициализации!
	//		std::cout << "name " << res.first << std::endl;
	//		res.second.lock()->ResourceTrackerSeq.Execute();	// для каждого отслеживаемого экземпляра ресурса указанного типа запустим проверку на загруженность
	//	}
	//}

private:
	std::string assetType = "undefined asset type";

	std::unordered_map <std::string, std::weak_ptr<ResourceClass> > 	resources;
};


// 6. Somewhere in the cpp code there must be defined instances of resource managers. 
// So it`s REQUIRED to call method		ResourceType_Manager.StatusTracker.Execute(); in every cycle!




#endif
