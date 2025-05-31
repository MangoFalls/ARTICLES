
# Переходим на C++26: как собрать и настроить GCC 15.1 в Ubuntu

# Введение

Стандарты языков C и C++ и их компиляторы довольно быстро развиваются, но далеко не все новинки становятся нам доступны сразу.

Эту статью я решил подготовить по случаю недавнего (25 апреля 2025) релиза компилятора GCC 15.1.0. В нем стали доступны для использования некоторые фичи из C++26.

К сожалению, текущая версия Ubuntu (`24.04.2 LTS`) изначально содержит достаточно старый GCC 13.3.0. Чтобы использовать что-то поновее, нужен "ручной" подход.

Дальнейший наш план таков. Мы установим новейшую версию GCC из исходников, настроим ее работу в системе по умолчанию и на пробу скомпилируем С++ код, использующий элементы стандарта C++26.

# 1. Проводим общее обновление системы

Запускаем в терминале:

```bash

sudo apt update && sudo apt upgrade -y

```

Этой командой мы, действуя как администратор (`sudo`), даем знать системе о последних доступных через пакетный менеджер версиях компонентов (`apt update`).

Затем, если все прошло ок (`&&`), устанавливаем доступные обновления `apt upgrade`, заранее на это соглашаясь (`-y`).

# 2. Проверяем нашу версию GCC

Нет смысла продолжать, если у нас уже установлена последняя версия компилятора, правда? 😉

```bash

sudo g++ --version && g++ --version

```

Этой командой мы узнаем, какая версия С++ компилятора используется у нас на обычном пользовательском аккаунте и на аккаунте администратора.

В ответ, как правило, терминал выдаст два идентичных ответа, где будет указано что-то вроде `g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0`.

Если эта версия компилятора слишком стара для нас, идем дальше...

# 3. Устанавливаем необходимые зависимости

```bash

sudo apt install -y build-essential git make gawk flex bison libgmp-dev libmpfr-dev libmpc-dev python3 binutils perl libisl-dev libzstd-dev tar gzip bzip2

```

Это набор программ, который нам потребуется при сборке новейшего GCC на нашей системе.

Некоторые из этих компонентов уже предустановлены в Ubuntu. К примеру, `build-essential`, среди прочего, проверит и гарантирует наличие изначально присутствующих в системе `gcc, g++, make`.

```bash

export CONFIG_SHELL=/bin/bash

```

Данная команда устанавливает переменную среды `CONFIG_SHELL` в состояние `/bin/bash`. В результате скрипты, которые будут запускаться при последующей сборке GCC, будут обрабатываться именно через требуемый самими скриптами `bash`, а не через какую-то иную оболочку.

# 4. Скачиваем компоненты новейшего GCC

```bash

mkdir ~/gcc-15
cd ~/gcc-15
git clone https://gcc.gnu.org/git/gcc.git gcc-15-source
cd gcc-15-source
git checkout releases/gcc-15.1.0
./contrib/download_prerequisites

```

На этой стадии мы создаем папки (`gcc-15`, `gcc-15-source`) и закачиваем туда исходники компилятора из репозитория (`https://gcc.gnu.org/git/gcc.git`) с необходимыми зависимостями (`./contrib/download_prerequisites`).

Мы также переключаем состояние скачанного репозитория на нужную нам версию компилятора. На конец апреля 2025 - это версия `15.1.0`.

Команда `git checkout releases/gcc-15.1.0` использует теги, которые, по сути, простые указатели на определенные коммиты со стабильными версиями исходников.

В дальнейшем выйдут новые версии. С полным списком доступных гит-тегов мы можем ознакомиться, введя команду `git tag -l`.

# 5. Настраиваем будущий билд

```bash

cd ~/gcc-15
mkdir gcc-15-build
cd gcc-15-build

../gcc-15-source/configure --prefix=/opt/gcc-15 --disable-multilib --enable-languages=c,c++

```

Далее в директории `gcc-15` мы создаем папку сборки `gcc-15-build`, в которой будет создан рабочий билд нашего компилятора, и переходим в нее.

Оттуда мы отправляем команду создания Make-файла с правилами, по которым будет проводиться сборка проекта.

Опция `--prefix=/opt/gcc-15` определяет, что GCC установится в папку `/opt/gcc-15`. Так новая версия будет существовать отдельно от уже установленного у нас GCC старой версии.

Опция `--disable-multilib` предполагает, что собрана будет лишь версия компилятора, которая полностью совместима в текущей системой (к примеру, на x64 системе не будет создан x32 компилятор). Такой подход ускорит сборку и исключит возможные ошибки.

В `--enable-languages=c,c++` мы перечисляем языки, которые будет поддерживать наш компилятор. GCC, к слову, может еще работать с иными языками, вроде `fortran`, `go`, `d`...

# 6. Собираем компилятор

```bash

make -j$(nproc)

```

Команда запускает сборку по правилам, которые мы определили в ранее созданном Make-файле.

Опция `-j` (полная форма `--jobs`) требует проводить компиляцию многопоточно для более быстрой сборки. В целом, эта стадия самая долгая и займет от получаса до нескольких часов, в зависимости от ресурсов машины.

Подстановка `$(nproc)` достанет из системы общее число ядер процессора. Соответственно, все они будут задействованы сборке GCC. При желании можно указать конкретное число (`make -j8`) или подойти чуть более творчески (`make -j$(($(nproc) - 1))`).

# 7. Инсталлируем и подключаем GCC

Команда `sudo make install` запустит инсталляцию собранного компилятора и сделает его готовым к работе.

Между тем, на данный момент запрос `g++ --version` выдаст нам номер старой версии GCC, а система в целом будет вести себя так, будто ничего не произошло.

Потому последним шагом нам следует объяснить системе, какой из наличных GCC отныне считается главным.

```bash

sudo update-alternatives --install /usr/bin/gcc gcc /opt/gcc-15/bin/gcc 100
sudo update-alternatives --install /usr/bin/g++ g++ /opt/gcc-15/bin/g++ 100

```

Эти команды зарегистрируют в системе бинарные файлы нового компилятора, расположенные в `/opt/gcc-15/bin/`.

Мы используем две команды, потому что `g++` это обычный C++ компилятор, работающий по ISO стандартам, тогда как `gcc` - это C компилятор с поддержкой C++ и собственными GCC-расширениями.

Параметр `100` означает приоритет. Бинарники с большим приоритетом выбираются по умолчанию, если мы не предусмотрим иного.

```bash

sudo update-alternatives --config gcc
sudo update-alternatives --config g++

```

Каждая из этих команд выведет в консоль нумерованный список зарегистрированных бинарников и предложит выбрать тот, который будет вызываться при обращении к `g++` или `gcc`.

На этом все. Теперь команда `g++ --version` должна возвращать номер той самой последней версии компилятора GCC, которую мы все это время устанавливали.

В дальнейшем, если мы решим вернуться к использованию изначальной "системной" версии, нам помогут те же команды.

Если старый компилятор у нас на тот момент будет не зарегистрирован, нужно будет провести его регистрацию:

```bash

sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 50
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 50

```

# 8. Наслаждаемся новыми фичами

GCC 15.1 открыл нам доступ к целому ряду особенностей, запланированных для стандарта C++26. Теперь мы можем на практике ознакомиться с будущими новациями языка.

Для этого создадим отдельную директорию, перейдем туда и сделаем там `.cpp` файл.

```bash

mkdir gcc_test_folder
cd gcc_test_folder
touch test26.cpp

```

Запишем в файл следующий код:

```cpp

#include <iostream>
#include <format>

// ===================================================
// Feature 1: Pack Indexing
// ===================================================

template<typename... Types>
void PrintFirstArg(Types... Args)
{
    std::cout << std::format("{}\n\n", Args...[0]);
}

void TestPackIndexing()
{
    puts("=== Feature 1: Pack Indexing ===");
    PrintFirstArg('a', "two", 33);
}

// ===================================================
// Feature 2: Attributes for Structured Bindings
// ===================================================

void TestAttributesForStructuredBindings()
{
    puts("=== Feature 2: Attributes for Structured Bindings ===");
    struct StructS1 { int A = 10, B = 20; } S;
    auto [A [[maybe_unused]], B [[deprecated]]] = S;
    std::cout << std::format("A = {}, B = {}\n\n", A, B);
}

// ===================================================
// Feature 3: Variadic Friends
// ===================================================

template<typename... Ts>
class Passkey
{
    friend Ts...;
    Passkey() { }
};

class ClassA;
class ClassB;

struct Widget
{
    void Secret(Passkey<ClassA, ClassB>) const
    {
        puts("Secret accessed!");
    }
};

struct ClassA
{
    void Access(const Widget& W) const { W.Secret({}); }	
};

struct ClassB
{
    void Access(const Widget& W) const { W.Secret({}); }
};

struct ClassD
{
    // Compilation error: ClassD is not a friend
    // void Access(Widget& W) { W.Secret({}); }
};

void TestVariadicFriends()
{
    puts("=== Feature 3: Variadic Friends ===");
    Widget W;
    ClassA A, B;
    A.Access(W);
    B.Access(W);
    puts("");
}

// ===================================================
// Feature 4: Constexpr Placement New
// ===================================================

constexpr int ConstexprPlacementNew()
{
    std::allocator<int> Allocator;
    int* P = Allocator.allocate(16);
    new (P) int(33);
    int Value = *P;
    Allocator.deallocate(P, 16);
    return Value;
}

void TestConstexprPlacementNew()
{
    puts("=== Feature 4: Constexpr Placement New ===");
    constexpr int Result = ConstexprPlacementNew();
    std::cout << std::format("Value constructed with placement new: {}\n\n", Result);
}

// ===================================================
// Feature 5: Structured Binding as Condition
// ===================================================

struct StructS2
{
    int A, B;
    explicit operator bool() const noexcept
    {
        return A != B;
    }
};

void Use(int A, int B)
{
    std::cout << std::format("Using A = {}, B = {}\n", A, B);
}

void TestStructuredBindingAsCondition()
{
    puts("=== Feature 5: Structured Binding as Condition ===");
    StructS2 S{10, 20};
    if (auto [A, B] = S)
    {
        Use(A, B);
    }
    puts("");
}

// ===================================================
// Feature 6: Deleted Function with Reason
// ===================================================

void OldFunction(char*) = delete("unsafe, use NewFunction instead");
void NewFunction(char*) { }

void TestDeletedFunctionWithReason()
{
    // error: use of deleted function ‘void OldFunction(char*)’: unsafe, use NewFunction instead
    // OldFunction("test");
}

// ===================================================
// Feature 7: Deleting Pointer to Incomplete Type
// ===================================================

struct IncompleteType;

void TestDeletingIncompleteType(IncompleteType* Pointer)
{
    // delete Pointer; // Compilation error in C++26
}

// ===================================================
// Feature 8: Oxford Variadic Comma
// ===================================================

void DeprecatedFunction(auto......) { } // omission of ‘,’ before varargs ‘...’ is deprecated in C++26
void OkayFunction(auto..., ...) { }

void DeprecatedG(int...) { } // Deprecated in C++26
void OkayG(int, ...) { }

void DeprecatedH(int X...) { } // Deprecated in C++26
void OkayH(int X, ...) { }

// ===================================================
// Main Function
// ===================================================

int main()
{
    TestPackIndexing();
    TestAttributesForStructuredBindings();
    TestVariadicFriends();
    TestConstexprPlacementNew();
    TestStructuredBindingAsCondition();
    TestDeletedFunctionWithReason();
    TestDeletingIncompleteType(nullptr);
}

```

Этот код содержит все основные поддерживаемые GCC 15.1.0 фичи стандарта С++26, которые мы можем испытать в рамках одного компилируемого кода.

Далее, пребывая в той же испытательной директории, проведем компиляцию нашего файла без GNU расширений, но с поддержкой C++26 (`-std=c++2c` или `-std=c++26`) и запустим полученную в результате программу (`./test26`):

```bash

g++ -std=c++26 test26.cpp -o test26

./test26

```

Если файл успешно скомпилировался, в процессе выдав нам несколько запланированных предупреждений, у нас все получилось.

# Источники

- https://gcc.gnu.org/install/
- https://gcc.gnu.org/projects/cxx-status.html
- https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html
- https://developers.redhat.com/articles/2025/04/24/new-c-features-gcc-15#defect_report_resolutions
