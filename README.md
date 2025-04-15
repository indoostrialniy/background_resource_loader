## resource_manager v1.5
	
## Назначение:
Header-only библиотека *resource_manager* вкупе с интегрированной версией 1.5 библиотеки *biCycle* [https://github.com/indoostrialniy/biCycle] предоставляет возможность обработки данных в отдельном потоке, параллельно отслеживая
статус завершенности этого потока и выполняя определенную функцию в основном потоке. 
<br>
Это может быть полезно при загрузке ассетов для игр, использующих OpenGL и позволит избежать блокировки основного потока на время загрузки ассета,
а также ошибок, связанных с контекстом OpenGL.

<br>В данном репозитории представлена демонстрационная программа, имитирующая фоновую загрузку модели. Код распределен по нескольким файлам:

*resourceManager.h* - файл с описанием самой библиотеки;

*resourceExample.cpp* - файл с телом программы;

*Resource.h / Resource.cpp* - отдельные файлы для объявления и определения классов-обработчиков пользовательских ресурсов.

### Любой пользовательский класс-обработчик ресурса должен наследоваться от класса *resource_manager::Resource*, описанного в *resourceManager.h*!
  

## Способ применения:
1) В файле **Resource.h** подключить заголовочный файл **resourceManager.h** и объявить минимально-необходимое описание класса-обработчика:

        #include "resourceManager.h"

        class TestResource : public resource_manager::Resource
        {
          public:
              TestResource( const std::string& Name );	// must-have constructor
              ~TestResource();							        // must-have destructor
          private:
              void longLoading() override; 				  // must-have func
              void quickConfiguring() override;			// must-have func
        };

Функции **longLoading()** и **quickConfiguring()** в базовом классе **Resource** объявлены как чисто виртуальные, поэтому избежать их определения не получится, это обязательное условие.

2) В нужном месте, например - в главном цикле, прописать вызовы метода Execute() используемых секвенсоров:

        while (1)
        {
           resource_manager::ResourceLoader<resource_manager::Resource::ResourceTracker>.Execute();  // for manager
           resource_manager::ResourceLoader<resource_manager::TrackerData<TestResource>>.Execute();  // for resource
        }

3) Менеджер предоставляет 2 способа загрузки ресурса - в основном потоке

  **.getAsset(const std::string& assetName)**
   
и параллельно

  **.getAsset_Async(const std::string& assetName, std::shared_ptr<ResourceClass>& res)**
    
Во втором случае функция не возвращает значений, но с помощью библиотеки biCycle v1.5 запускает вспомогательную функцию, отслеживающую статус готовности ресурса и самоотвязывающуюся от исполнения при завершении загрузки ресурса.

## Актуальная версия 1.5
