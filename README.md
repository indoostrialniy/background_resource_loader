## resource_manager v1.6
	
## Назначение:
Библиотека *resource_manager* вкупе с интегрированной версией 1.5 библиотеки *biCycle* [https://github.com/indoostrialniy/biCycle] предоставляет возможность загрузки ресурсов в отдельном потоке, параллельно отслеживая статус завершенности этого потока и выполняя определенную функцию в основном потоке по окончании загрузки. 
<br>
Это может быть полезно при загрузке ассетов для игр, использующих OpenGL и позволит избежать блокировки основного потока на время загрузки ассета,
а также ошибок, связанных с контекстом OpenGL.

## Структура репозитория
Код распределен по нескольким файлам:<br>

*resourceManager.h* - файл самой библиотеки;

*Resource.h / Resource.cpp* - примеры отдельных файлов для объявления и определения классов-обработчиков пользовательских ресурсов.

## Способ применения:
### Любой пользовательский класс-обработчик ресурса должен наследоваться от класса *Resource*, описанного в *resourceManager.h*!

1) В файле с описанием пользовательских классов ресурсов подключить заголовочный файл **resourceManager.h** и объявить минимально-необходимое описание класса-обработчика ресурса (см. "Resources.h"),<br>
по необходимости расширить, а также создать экземпляр менеджера этого ресурса:

        #include "resourceManager.h"

        class TestResource : public Resource
        {
          public:
              TestResource( const std::string& Name );	// must-have constructor
              ~TestResource();				// must-have destructor
              void longLoading() override; 		// must-have func
              void quickConfiguring() override;		// must-have func
        };

   		extern Manager <Test> 			TestManager;	// Создать экземпляр менеджера данного ресурса:

2) В файле с определением пользовательских ресурсов (см. "Resources.cpp") задать минимальный пользовательский код:

		#include "Resources.h"
		Manager <Test> 		TestManager("Test");
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


Функции **longLoading()** и **quickConfiguring()** в базовом классе **Resource** объявлены как чисто виртуальные, поэтому избежать их определения не получится, это обязательное условие.

3) Метод TestManager.StatusTracker.Execute(); прописать для вызова в каждом цикле программы. Это обязательно!

        while (1)
        {
   			TestManager.StatusTracker.Execute();
        }

4) Менеджер предоставляет 2 способа загрузки ресурса - в основном потоке
	
 		TestManager.getAsset(const std::string& pathToResourceFile, std::shared_ptr<TestResource>& resourceInstance, false, [](){} );
   
и параллельно

	TestManager.getAsset(const std::string& pathToResourceFile, std::shared_ptr<TestResource>& resourceInstance, true, [&](){} );
    
Загрузочная функция принимает строковое имя ресурса **pathToResourceFile**, ссылку на экземпляр **resourceInstance**, который будет хранить ресурс, булев спецификатор режима загрузки (asynchro-true/false) и лямбда-коллбэк-функцию, выполняющуюся по окончании загрузки ресурса. В случае указания асинхронности (true), с помощью встроенной библиотеки biCycle v1.5 запускается вспомогательная функция, отслеживающая статус готовности ресурса и самоотвязывающаяся от исполнения при завершении загрузки ресурса.

## Актуальная версия 1.6
