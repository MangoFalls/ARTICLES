
# Оптимизируем C++ шаблоны: от инлайнинга до модулей

Мы рассмотрим, чем опасны шаблоны для проекта на C++ и как минимизировать эти риски. В оптимизации нам помогут инлайн-файлы, явные инстанциации и даже модули из C++20.

# Введение

Шаблоны в С++ опасны для начинающего пользователя не столько сложностью, сколько опосредованными, неочевидными последствиями для проекта в целом.

Среди проблем есть и замедление компиляции, и увеличение размера объектных файлов, и искажение структуры проекта из-за невозможности разделить шаблонные классы на объявления и определения также легко, как мы делаем это с обычными классами.

Далее мы рассмотрим несколько способов решить перечисленные сложности за счет более эффективной организации кода и файлов проекта.

# Подход 1: Определение шаблонов в заголовочных файлах

Уже в самом начале работы с шаблонами мы могли заметить, что они работают несколько иначе, чем обычные сущности.

Не получится объявить шаблонные функции в `.h`, а определения давать в `.cpp`. Такой код просто откажется компилироваться.

Происходит это потому, что когда мы используем шаблон с конкретными типами, например, вызываем шаблонную функцию `make_shared()`, компилятору нужно иметь доступ к полному описанию шаблона. Простого обещания, что такие классы или функции где-то существуют, ему недостаточно.

Шаблон удобно рассматривать как чертеж или схему, на основе которой компилятор создаст для нас конкретный класс, функцию, переменную...

Простейшее решение - определять шаблонные функции в заголовочном файле. Так любой `.cpp` файл, включающий этот хедер, будет иметь доступ к описанию используемых шаблонов.

При таком подходе код проекта будет выглядеть примерно так:

```cpp

//-----------------------------
// File: TemplateUnit.cpp
//-----------------------------

#include "TemplateUnit.hpp"
void SimpleClass::SimpleFunc() { puts("[SimpleClass::SimpleFunc]"); }

//-----------------------------
// File: TemplateUnit.hpp
//-----------------------------

#pragma once
#include "stdio.h"

struct SimpleClass {
    void SimpleFunc();
    template <typename T> void SimpleTemplateFunc(const T& value) {
        printf("[SimpleTemplateFunc]: %d\n", value);
    }
};

template <typename Type>
struct TemplateClass {
    void EasyFunc() { puts("[TemplateClass::EasyFunc]"); }
    template <typename T> void ComplexTemplateFunc(const T& value) {
        printf("[ComplexTemplateFunc]: %d\n", value);
    }
};

//-----------------------------
// File: Alpha.cpp
//-----------------------------

#include "Alpha.hpp"
#include "TemplateUnit.hpp"
void AlphaLogic() {
    SimpleClass().SimpleFunc();
    SimpleClass().SimpleTemplateFunc(11);
    TemplateClass<int>().ComplexTemplateFunc(11);
}

//-----------------------------
// File: Beta.cpp
//-----------------------------

#include "Beta.hpp"
#include "TemplateUnit.hpp"
void BetaLogic() {
    SimpleClass().SimpleTemplateFunc(22);
    TemplateClass<int>().ComplexTemplateFunc(22);
}

//-----------------------------
// File: Gamma.cpp
//-----------------------------

#include "Gamma.hpp"
#include "TemplateUnit.hpp"
void GammaLogic() { 
    SimpleClass().SimpleFunc();
    TemplateClass<int>().EasyFunc();
}

//-----------------------------
// File: Alpha.hpp
//-----------------------------

#pragma once
void AlphaLogic();

//-----------------------------
// File: Beta.hpp
//-----------------------------

#pragma once
void BetaLogic();

//-----------------------------
// File: Gamma.hpp
//-----------------------------

#pragma once
void GammaLogic();

//-----------------------------
// File: main.cpp
//-----------------------------

#include "Alpha.hpp"
#include "Beta.hpp"
#include "Gamma.hpp"

int main() {
    AlphaLogic();
    BetaLogic();
    GammaLogic();
}

```

Далее, в Linux-терминале выполним две операции.

Во-первых, убедимся, что код компилируется в рабочую программу.

```bash

# Step 1. Compile and run
g++ *.cpp -o main && ./main; rm main

```

GNU C++ компилятор (`g++`) соберет наш проект в исполняемый файл "main" (`-o main`) и, если компиляция прошла успешно (`&&`), мы запустим этот файл (`./main`).

Текущая оболочка развернёт `*.cpp` в список всех находящихся в директории файлов с расширением `.cpp` и в лексикографическом порядке передаст их на компиляцию. В данный момент порядок компиляции не имеет значения.

После того, как программа отработает `;` (неважно, с ошибками или без), мы удалим (`rm`) исполняемый файл, т.к. он больше нам не нужен.

Программа (в этом и всех последующих примерах) должна выводить в терминал следующее:

```bash
[SimpleClass::SimpleFunc]
[SimpleTemplateFunc]: 11
[ComplexTemplateFunc]: 11
[SimpleTemplateFunc]: 22
[ComplexTemplateFunc]: 22
[SimpleClass::SimpleFunc]
[TemplateClass::EasyFunc]
```

Во-вторых, по отдельности скомпилируем все `.cpp` нашего проекта в отдельные объектные файлы `.o` и посмотрим на их содержимое.

```bash

# Step 2. Compile translation units and look through them
g++ -c *.cpp && for f in *.o; do echo -e "\n--- $f ---"; nm "$f" | grep -E 'SimpleClass|TemplateClass' | c++filt; done; rm *.o

```

Компилятор GCC, запущенный с флагом `-c`, создаст объектные файлы для каждого `.cpp` в папке.

Затем при помощи цикла `for` для каждого объектного файла `*.o` мы выполним ряд операций.

`echo -e "\n--- $f ---"` выведет в консоль название файла. Флаг `-e` позволяет использовать escape-символы, например `\n` для переноса строки.

Далее запустим утилиту `nm`. Она вернет имена всех символов (функций, переменных), которые содержатся в текущем объектном файле `"$f"` .

Пайп-оператор `|` принимает вывод из `nm` и передаёт его следующей утилите на обработку.

Нам нужны не все символы - только функции классов `SimpleClass` и `TemplateClass`.

Утилита командной строки `grep` (Global Regular Expression Print) отфильтрует вывод, полученный от `nm`, и оставит только символы, названия которых включают любое из имен этих классов.

Флаг `-E` обеспечит поддержку расширенных регулярных выражений, чтобы мы могли использовать логический оператор `|`(OR) без раздражающей черточки (`\`) перед ним.

Несколько оставшихся после фильтрации символов будут далее переданы в утилиту `c++filt`. Она произведет **деманглинг**, то есть преобразует все переданные ей имена в человекочитаемый формат. Так, нам не придется всматриваться в конструкции типа `_ZN13TemplateClassIiE19ComplexTemplateFuncIiEEvRKT_`.

После того, как чтение файлов в цикле завершилось (`done`), мы удалим больше не нужные нам объектные файлы через `rm *.o`.

В результате терминал отобразит следующее:

```bash
--- Alpha.o ---
                 U SimpleClass::SimpleFunc()
0000000000000000 W void SimpleClass::SimpleTemplateFunc<int>(int const&)
0000000000000000 W void TemplateClass<int>::ComplexTemplateFunc<int>(int const&)

--- Beta.o ---
0000000000000000 W void SimpleClass::SimpleTemplateFunc<int>(int const&)
0000000000000000 W void TemplateClass<int>::ComplexTemplateFunc<int>(int const&)

--- Gamma.o ---
                 U SimpleClass::SimpleFunc()
0000000000000000 W TemplateClass<int>::EasyFunc()

--- TemplateUnit.o ---
0000000000000000 T SimpleClass::SimpleFunc()

--- main.o ---
```

1. Особое положение `SimpleClass::SimpleFunc()` - первое, что бросается в глаза. В отличие от всех прочих, объявление и определение этой функции мы разделили, поместив определение в отдельный файл `TemplateUnit.cpp`.

Метка `T`(text) обозначает символ с сильным определением функции. На весь собираемый проект может быть только один сильный символ (на то он и сильный).

Метка `U`(undefined) обозначает символ, который используются в данном бинарном файле, но его определение находится где-то вовне.

Так, файлы `Alpha.o` и `Gamma.o` знают о существовании `SimpleClass::SimpleFunc()`, но не о его содержании. В момент компиляции эти `.cpp` файлы имели доступ только к заголовку функции в хедере.

В дальнейшем, при создании исполняемого файла линкер будет искать определения undefined-символов в иных объектных файлах и, если не найдёт, выдаст ошибку линковки.

Длинная последовательность нулей рядом с каждым символом говорит о том, что конкретный адрес данным сущностям ещё не назначен. Мы ведь имеем дело с объектными файлами, по сути, полуфабрикатами.

2. Метка `W`(weak) говорит нам, что объектный файл содержит **слабое определение** символа.

"Слабость" определения в том, что линкер, когда начнет связывать объектные файлы воедино, будет учитывать такие определения в последнюю очередь. При наличии сильного определения линкер примет его за образец, а при отсутствии такового - возьмет любой слабый, зачастую, из обрабатываемого первым объектного файла. Все остальные слабые символы будут удалены во время линковки.

Все слабые символы в нашем проекте - это **неявные инстанциации**, функции, классы, переменные, которые компилятор определяет на основе шаблонов.

Неявные инстанциации сейчас - это прямое следствие того, что каждый `.cpp`, использующий `TemplateUnit.hpp`, имеет доступ к определениям шаблонов.

Повтор одних и тех же определений в разных объектных файлах `Alpha.o`, `Beta.o`, `Gamma.o` - прямая иллюстрация проблемы, решению которой эта статья посвящена.

> Повтор определений одного символа в разных юнитах трансляции означает, что компилятор выполнил лишнюю работу!
> Компилятор обработал один и тот же код несколько раз, произвёл один и тот же массив машинных команд.
> Линкер в итоге уничтожит все плоды этого труда, оставив лишь один из них.

В масштабах рассматриваемых примеров, время и ресурсы, потраченные компилятором зазря, ничтожны.

Но представьте проект, в котором сотни, если не тысячи, юнитов трансляции с куда более длинными и сложными шаблонными классами и функциями.

В итоге мы получаем системы, которые компилируются не за минуту, а за пять; не за пять, а за пятнадцать. Разработчик в это время простаивает, бизнес теряет деньги.

Конечно, у долгой сборки есть и иные причины. Но дублирование символов делает свой вклад, который достаточно легко устранить.

# Подход 2: Инлайн-файлы в хедере

С точки зрения оптимизации, данный подход идентичен первому, но позволяет сделать код чуть более опрятным. Эта часть также призвана познакомить с концепцией инлайн-файлов тех, кто только слышал о них.

В самом общем смысле, инлайн-файлы содержат код, который должен быть частью заголовочного файла. Но мы решили этот код хранить отдельно.

На практике инлайн-файлы используются для изоляции вообще любых частей заголовочных файлов, не обязательно только шаблонных определений.

В последующем примере мы используем `TemplateUnit.inl` для хранения определений шаблонов. Этот файл инклудится в конце `TemplateUnit.hpp`. Так определения из `.inl` становятся частью `.hpp`.

Остальные файлы проекта полностью идентичны тем, что были приведены в Подходе 1, поэтому они здесь не приводятся.

```cpp

//-----------------------------
// File: TemplateUnit.hpp
//-----------------------------

#pragma once

struct SimpleClass {
    void SimpleFunc();
    template <typename T> void SimpleTemplateFunc(const T& value);
};

template <typename Type>
struct TemplateClass {
    void EasyFunc();
    template <typename T> void ComplexTemplateFunc(const T& value);
};

#include "TemplateUnit.inl"

//-----------------------------
// File: TemplateUnit.inl
//-----------------------------

#pragma once
#include "stdio.h"

template <typename T>
void SimpleClass::SimpleTemplateFunc(const T& value) {
    printf("[SimpleTemplateFunc]: %d\n", value);
}

template <typename T>
void TemplateClass<T>::EasyFunc() { puts("[TemplateClass::EasyFunc]"); }

template<typename Type>
template<typename T>
void TemplateClass<Type>::ComplexTemplateFunc(const T& value) {
    printf("[ComplexTemplateFunc]: %d\n", value);
}

//-----------------------------
// Other files are identical to Approach 1
//-----------------------------

```

Расширение `.inl` используется просто в силу его популярности и не имеет формального значения. Работая с GCC, мы можем дать инлайн-файлу любое расширение: `.txt`, `.jpg`... В любом случае в самом начале компиляции препроцессор выгрузит содержимое `TemplateUnit.inl` в `TemplateUnit.hpp`.

Запустим в терминале те же две команды, которые использовали в первом подходе. Во-первых, проверим валидность нашего проекта, скомпилировав и запустив программу. Во-вторых, выполним ту же самую операцию анализа символов в объектных файлах, что и в прошлый раз.

Вывод программы и список символов в юнитах трансляции будут полностью идентичны тем, что мы получили в первом подходе.

Действительно, в данном случае инлайн-файл с определениями шаблонов - это просто часть хедера, пусть и аккуратно выделенная в отдельный файл. Если определения остаются доступны при компиляции в каждом `.cpp`, размещение их вне тела класса не имеет значения.

> **Вывод**:
> Включение инлайн-файла в заголовочный файл может и делает код более аккуратным, но процесс компиляции никак не оптимизирует.

# Подход 3: Инлайн-файлы в .cpp

Данный подход предполагает включение инлайн-файлов не в хедер (и оттуда во все использующие его юниты трансляции), но лишь в отдельные `.cpp` файлы.

Например, вот так:

```cpp

//-----------------------------
// File: TemplateUnit.hpp
//-----------------------------

#pragma once

struct SimpleClass {
    void SimpleFunc();
    template <typename T> void SimpleTemplateFunc(const T& value);
};

template <typename Type>
struct TemplateClass {
    void EasyFunc();
    template <typename T> void ComplexTemplateFunc(const T& value);
};

//-----------------------------
// File: TemplateUnit.inl
//-----------------------------

#pragma once
#include "stdio.h"

template <typename T>
void SimpleClass::SimpleTemplateFunc(const T& value) {
    printf("[SimpleTemplateFunc]: %d\n", value);
}

template <typename T>
void TemplateClass<T>::EasyFunc() { puts("[TemplateClass::EasyFunc]"); }

template<typename Type>
template<typename T>
void TemplateClass<Type>::ComplexTemplateFunc(const T& value) {
    printf("[ComplexTemplateFunc]: %d\n", value);
}

//-----------------------------
// File: Alpha.cpp
//-----------------------------

#include "Alpha.hpp"
#include "TemplateUnit.hpp"
#include "TemplateUnit.inl"
void AlphaLogic() {
    SimpleClass().SimpleFunc();
    SimpleClass().SimpleTemplateFunc(11);
    TemplateClass<int>().ComplexTemplateFunc(11);
}

//-----------------------------
// File: Beta.cpp
//-----------------------------

#include "Beta.hpp"
#include "TemplateUnit.hpp"

extern template void SimpleClass::SimpleTemplateFunc<int>(const int&);
extern template void TemplateClass<int>::ComplexTemplateFunc<int>(const int&);

void BetaLogic() {
    SimpleClass().SimpleTemplateFunc(22);
    TemplateClass<int>().ComplexTemplateFunc(22);
}

//-----------------------------
// File: Gamma.cpp
//-----------------------------

#include "Gamma.hpp"
#include "TemplateUnit.hpp"
#include "TemplateUnit.inl"
void GammaLogic() { 
    SimpleClass().SimpleFunc();
    TemplateClass<int>().EasyFunc();
}

//-----------------------------
// Other files are identical to Approach 2
//-----------------------------

```

Вот так выглядит таблица символов для данного проекта:

```bash
--- Alpha.o ---
                 U SimpleClass::SimpleFunc()
0000000000000000 W void SimpleClass::SimpleTemplateFunc<int>(int const&)
0000000000000000 W void TemplateClass<int>::ComplexTemplateFunc<int>(int const&)

--- Beta.o ---
                 U void SimpleClass::SimpleTemplateFunc<int>(int const&)
                 U void TemplateClass<int>::ComplexTemplateFunc<int>(int const&)

--- Gamma.o ---
                 U SimpleClass::SimpleFunc()
0000000000000000 W TemplateClass<int>::EasyFunc()

--- TemplateUnit.o ---
0000000000000000 T SimpleClass::SimpleFunc()

--- main.o ---
```

Итак, почему символы расположились именно так:

Во-первых, мы включили `TemplateUnit.inl` лишь в файлы реализации `Alpha.cpp` и `Gamma.cpp`.

В результате в момент компиляции только юниты трансляции `Alpha` и `Gamma` имели доступ к шаблонам. За счет такого доступа компилятор создал инстанциации (определения) функций, используемых в этих `.cpp` файлах.

Во-вторых, мы поместили `extern template` в `Beta.cpp` на каждую используемую в файле функцию. Что такое `extern template`, мы рассмотрим в рамках следующего подхода.

Сейчас лишь скажу, что `extern template` гарантирует, что компилятор не будет пытаться создать инстанциации для отмеченных таким образом функций в данном юните трансляции, но будет искать эти определения на этапе линковки.

В итоге мы избавились от дублирования бинарных символов в проекте. Но подход этот явно не самый удобный.

Нам нужно пристально контролировать, чтобы определение каждого шаблона присутствовало во всем проекте лишь единожды. В каждом новом `.cpp`, где используется хоть один шаблон, нам придется выполнять ментальное жонглирование. Выбор между `.inl` и `extern template` не всегда очевиден. Ситуация станет невыносимо запутанной, когда число шаблонов и юнитов трансляции в проекте перевалит за десяток.

> **Вывод**:
> Включение инлайн-файлов напрямую в `.cpp` позволяет избежать дублирования символов. Между тем, данный подход требует чрезмерных усилий для поддержания и на практике едва ли применим.

# Подход 4: Явная инстанциация шаблонов


 Мы будем рассматривать **инстанциацию шаблона** как конкретный экземпляр шаблонной функции или класса с заданными параметрами типа.

Так, применяя шаблоны в `.cpp` файлах - создавая объекты, вызывая функции - и подставляя на место абстрактной `<T>` конкретный `<int>`, мы обращаемся к инстанции шаблона для `int`.

В предыдущих подходах мы работали с **неявными инстанциациями** - мы нигде в коде отдельно не определяли, к примеру, `TemplateClass<int>::EasyFunc()`. Эта функция (инстанциация для `int`) была создана компилятором самостоятельно на основе шаблона. Это произошло в ответ на наш вызов `EasyFunc()` в `Gamma.cpp`.

Таким образом, **явная инстанциация** - это полученное на основе шаблона определение символа, которое мы прописали в коде.

К примеру:

```cpp

//-----------------------------
// File: TemplateUnit.cpp
//-----------------------------

#include "stdio.h"
#include "TemplateUnit.hpp"

void SimpleClass::SimpleFunc() { puts("[SimpleClass::SimpleFunc]"); }

template <typename T>
void SimpleClass::SimpleTemplateFunc(const T& value) {
    printf("[SimpleTemplateFunc]: %d\n", value);
}

template <typename T>
void TemplateClass<T>::EasyFunc() { puts("[NonTemplateFuncInTemplateClass]"); }

template<typename Type>
template<typename T>
void TemplateClass<Type>::ComplexTemplateFunc(const T& value) {
    printf("[ComplexTemplateFunc]: %d\n", value);
}

template void TemplateClass<int>::EasyFunc();
template void SimpleClass::SimpleTemplateFunc<int>(const int&);
template void TemplateClass<int>::ComplexTemplateFunc<int>(const int&);

//-----------------------------
// File: TemplateUnit.hpp
//-----------------------------

#pragma once

struct SimpleClass {
    void SimpleFunc();
    template <typename T> void SimpleTemplateFunc(const T& value);
};

template <typename Type>
struct TemplateClass {
    void EasyFunc();
    template <typename T> void ComplexTemplateFunc(const T& value);
};

extern template void TemplateClass<int>::EasyFunc();
extern template void SimpleClass::SimpleTemplateFunc<int>(const int&);
extern template void TemplateClass<int>::ComplexTemplateFunc<int>(const int&);

//-----------------------------
// Other files are identical to Approach 1
//-----------------------------
```

Таблица символов проекта теперь выглядит гораздо приятнее:

```bash
--- Alpha.o ---
                 U SimpleClass::SimpleFunc()
                 U void SimpleClass::SimpleTemplateFunc<int>(int const&)
                 U void TemplateClass<int>::ComplexTemplateFunc<int>(int const&)

--- Beta.o ---
                 U void SimpleClass::SimpleTemplateFunc<int>(int const&)
                 U void TemplateClass<int>::ComplexTemplateFunc<int>(int const&)

--- Gamma.o ---
                 U SimpleClass::SimpleFunc()
                 U TemplateClass<int>::EasyFunc()

--- TemplateUnit.o ---
0000000000000000 T SimpleClass::SimpleFunc()
0000000000000000 W void SimpleClass::SimpleTemplateFunc<int>(int const&)
0000000000000000 W void TemplateClass<int>::ComplexTemplateFunc<int>(int const&)
0000000000000000 W TemplateClass<int>::EasyFunc()

--- main.o ---
```

Во-первых, выражения, подобные `template void TemplateClass<int>::EasyFunc();`, в `TemplateUnit.cpp` это **определения явных инстанциаций шаблонов** (explicit instantiation definition).

Явная инстанциация сообщает компилятору, что эта функция уже определена для соответствующих типов. В результате при сборке проекта компилятор будет пользоваться этим экземпляром символа, а не создавать свои аналоги.

Использование явных инстанциаций позволяет нам централизовать все определения шаблонных сущностей в одном юните трансляции. Тем самым мы избавляемся от паутины зависимостей, которая имела место в Подходе 3.

Как демонстрирует таблица символов, теперь объектные файлы `Alpha.o`, `Beta.o`, `Gamma.o` содержат лишь упоминания наших четырех функций. Их определения компилятор обработал единожды в `TemplateUnit.o`.

Во-вторых, `extern template void TemplateClass<int>::EasyFunc();` выражения в `TemplateUnit.hpp` - это **объявления явных инстанциаций шаблонов** (explicit instantiation declaration).

С помощью `extern template` мы запрещаем компилятору самостоятельно инстанциировать шаблоны для `int` в этом юните трансляции и обещаем, что нужные инстанциации (определения) линкер найдет в ином юните трансляции.

Надо признать, что при сборке данного проекта с помощью GCC 15.1, выражения `extern template` нам не нужны. Если их удалить, таблица символов проекта не поменяется, дублирования определений все еще не будет.

Из-за этого у нас может родиться заблуждение, что объявления явных инстанциаций не нужны вовсе. Это не так!

При отсутствии `extern template` выражений мы оставляем работу с шаблонами на волю компилятора. GCC достаточно умен, чтобы компилировать код и предотвращать появление лишних символов без подсказок. Но такое поведение не стандартизировано, и у нас нет никаких гарантий, что все остальные компиляторы всегда будут поступать также.

Таким образом, комбинируя определения и объявления явных инстанциаций мы можем достаточно легко управлять большим количеством шаблонов.

Да, подход требует от нас прописывать определение для каждого нового символа, создаваемого в проекте на основе шаблона, но это все равно проще Подхода 3.

Надо также внимательно следить, чтобы `extern template` объявления точно соответствовали `template` определениям по инстанциирующим типам и сигнатуре. Несоответствие может привести к ошибкам компиляции, неявным инстанциациям и дублированию.

Убедиться в том, что мы все сделали правильно, можно следующей командой:

```bash
g++ -fno-implicit-templates *.cpp -o main && ./main; rm main

g++ -fno-implicit-templates -c *.cpp && for f in *.o; do echo -e "\n--- $f ---"; nm "$f" | grep -E 'SimpleClass|TemplateClass' | c++filt; done; rm *.o
```

Флаг `-fno-implicit-templates` запрещает компилятору создавать скрытые инстанциации, возлагая это бремя на плечи автора кода. Флаг делает проект полностью зависимым от ручного управления инстанциациями.

Если попробуем применить эту команду в проектах из предыдущих подходов, компилятор выдаст ошибку линковки, жалуясь на неопределенные символы в юнитах трансляции `Alpha`, `Beta`, `Gamma`.

> **Вывод**:
>Один из наиболее предсказуемых и масштабируемых способов избавиться от дублирования символов в проекте с шаблонами - определять явные инстанциации этих шаблонов в отдельном `.cpp`, а в заголовочном файле поддерживать симметричный набор `extern template` выражений.

# Подход 5. Использование C++20 модулей

Было бы несправедливо игнорировать C++20 модули в теме оптимизации шаблонов.

Несмотря на новизну и кажущуюся чуждость модулей, общие закономерности работы с шаблонами остаются прежними: нам все еще нужно инстанциировать шаблоны, или компилятор сделает это за нас.

Наибольшее отличие данного проекта от предыдущего заключается в том, что все, связанное с шаблонами, мы размещаем в одном файле, а не в двух.

```cpp

//-----------------------------
// File: TemplateUnit.cppm
//-----------------------------

module;
import <stdio.h>;
export module TemplateModule;
export {
    struct SimpleClass {
        void SimpleFunc();

        template <typename T>
        void SimpleTemplateFunc(const T& value) {
            printf("[SimpleTemplateFunc]: %d\n", value);
        }
    };
    
    template <typename Type>
    struct TemplateClass {
        void EasyFunc() { puts("[TemplateClass::EasyFunc]"); }

        template <typename T>
        void ComplexTemplateFunc(const T& value) {
            printf("[ComplexTemplateFunc]: %d\n", value);
        }
    };

    void SimpleClass::SimpleFunc() {
        puts("[SimpleClass::SimpleFunc]");
    }
} // export

template void SimpleClass::SimpleTemplateFunc<int>(const int&);
template void TemplateClass<int>::EasyFunc();
template void TemplateClass<int>::ComplexTemplateFunc<int>(const int&);

//-----------------------------
// File: Alpha.cpp
//-----------------------------

import TemplateModule;
#include "Alpha.hpp"
void AlphaLogic() {
    SimpleClass().SimpleFunc();
    SimpleClass().SimpleTemplateFunc(11);
    TemplateClass<int>().ComplexTemplateFunc(11);
}

//-----------------------------
// File: Beta.cpp
//-----------------------------

import TemplateModule;
#include "Beta.hpp"
void BetaLogic() {
    SimpleClass().SimpleTemplateFunc(22);
    TemplateClass<int>().ComplexTemplateFunc(22);
}

//-----------------------------
// File: Gamma.cpp
//-----------------------------

import TemplateModule;
#include "Gamma.hpp"
void GammaLogic() { 
    SimpleClass().SimpleFunc();
    TemplateClass<int>().EasyFunc();
}

//-----------------------------
// Other files are identical to Approach 1
//-----------------------------

```

Есть несколько моментов, касающихся использования **модулей**:

1. Такое расширение мы дали модульному файлу `TemplateModule.cppm`, чтобы различать его от обычных файлов реализации в ходе сборки проекта. Как в случае с `.inl`, расширение `.cppm` не имеет формального смысла и вместо него может использоваться иная комбинация символов.

2. В самом начале этого файла мы открываем глобальный фрагмент модуля (`module;`) и импортируем в него модуль стандартной библиотеки ввода-вывода (`import <stdio.h>;`), чтобы ниже иметь возможность использовать `printf()` и `puts()`.

3. С помощью `export module TemplateModule;` объявляем модуль `TemplateModule`, который будем импортировать в наши файлы реализации.

4. Строчкой `export {` открываем область символов, доступных в пространстве имен при импорте нашего модуля.

Несколько замечаний о **связи явных инстанциаций и модулей**:

1. Прежде всего, явные инстанциации все еще необходимы - без них, таблица символов проекта получится как в первом подходе.

2. Объявления явных инстанциаций (`extern template`) здесь излишни - каждый юнит трансляции, который импортирует `TemplateModule`, получает уже готовые, единожды скомпилированные определения конкретных шаблонных функций.

3. Определения явных инстанциаций (`template` выражения в конце `TemplateUnit.cppm`) можно как экспортировать, так и оставить за пределами `export {}`. **Экспорт** лишь помещает символ в пространство имен импортирующего юнита трансляции. Между тем, от определений явных инстанциаций нам нужна только **досягаемость** (*reachability*), никак не связанная с экспортом.

Далее, собираем и запускаем проект, смотрим его таблицу символов:

```bash

F="g++ -std=c++20 -fmodules-ts -fno-implicit-templates"; $F -x c++-system-header stdio.h && $F -c *.cppm && $F *.o *.cpp -o main && ./main; rm main *.o

g++ -std=c++20 -fmodules-ts -c *.cppm *.cpp && for f in *.o; do echo -e "\n--- $f ---"; nm "$f" | grep -E 'SimpleClass|TemplateClass' | c++filt; done; rm *.o

```

Прежде всего, в терминале создаем глобальную переменную `F="g++ -std=c++20 -fmodules-ts -fno-implicit-templates"`, чтобы избежать трёхкратного повторения этого фрагмента. Переменная `F` просуществует, пока мы не закроем терминал.

Флаг `-std=c++20` обеспечивает использование стандарта C++20 в последующей компиляции. В используемом компилятором по умолчанию C++17 модули недоступны.

Флаг `-fmodules-ts` активирует поддержку модулей. В данный момент, даже новейший GCC 15.1 рассматривает модули экспериментальной фичей.

Фрагмент `-x c++-system-header stdio.h` компилирует файл стандартной Си библиотеки `stdio.h` в модуль, что позволит в дальнейшем импортировать его уже в наш модуль.

Вторая команда вернет то же самое распределение символов, что было у нас в Подходе 4.

Единственное различие будет в названиях типов (`SimpleClass@TemplateModule::SimpleFunc()`) - теперь они содержат указание на модуль, в котором расположены.

> **Вывод**:
> Система C++20 модулей позволяет предотвращать дублирование символов при компиляции, пусть и с большим объемом усилий, чем традиционный подход с объявлениями и определениями явных инстанциаций.

# Заключение

Теперь мы знаем, что бездумное введение шаблонов в проект чревато замедлением компиляции из-за дублирования определений одних и тех же символов в разных юнитах трансляции. К счастью, у нас есть по крайней мере три способа организовать работу с шаблонами так, чтобы это дублирование предотвратить.

# Источники

- https://lugdunum3d.github.io/doc/guidelines.html
- https://timsong-cpp.github.io/cppwp/n4861/module.reach
- https://en.cppreference.com/w/cpp/language/class_template
- https://gcc.gnu.org/onlinedocs/gcc-15.1.0/gcc/Template-Instantiation.html
- https://stackoverflow.com/questions/1208028/significance-of-a-inl-file-in-c
- https://learn.microsoft.com/en-us/cpp/cpp/source-code-organization-cpp-templates
- https://forums.unrealengine.com/t/can-inline-files-inl-be-included-from-a-ue4-header-file/396446
- https://developercommunity.visualstudio.com/t/intellisense-if-inl-file-with-inline-method-defini/158288
- https://www.reddit.com/r/cpp_questions/comments/1bcyci3/why_is_there_need_to_use_a_inl_file_after_using/
