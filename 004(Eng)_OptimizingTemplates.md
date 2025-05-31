
# Optimizing C++ Templates: From Inlining to Modules

We’ll explore the potential pitfalls of using templates in a C++ project and how to minimize these risks.
To optimize template usage, we’ll rely on inline files, explicit instantiations, and even C++20 modules.

# Introduction

Templates in C++ can be tricky for beginners, not because of their inherent complexity, but because of the subtle, often hidden consequences they can have on a project.

Some of the challenges include compile-time slowdown, binary code bloat, and the fact that templates can’t be split into declarations and definitions as easily as we usually do with regular classes.

Below, we’ll look at a few ways to tackle these issues by organizing our code and project files efficiently.

# Approach 1: Defining Templates in Header Files

Templates behave a bit differently from regular entities.

We can’t declare template functions in a `.h` file and then provide their definitions in a `.cpp` file. This kind of code won’t compile.

This happens because when we use a template with specific types - like when we call a template function such as `make_shared()` - the compiler needs full access to the complete description of the template. A mere promise that these classes or functions exist somewhere isn’t enough.

It’s helpful to think of a template as a blueprint, which the compiler uses to create actual classes, functions, or variables for us.

The simplest solution is to define template functions directly in the header file. This way, any `.cpp` file that includes this header will have direct access to the definitions of the templates it uses.

When we use this approach, the project’s code will look something like this:

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

Next, let's perform two operations in the Linux terminal.

First, we will verify that the code compiles into a functional program.

```bash
# Step 1. Compile and run
g++ *.cpp -o main && ./main; rm main
```

The GNU C++ compiler (`g++`) builds our project into an executable file named `main` (`-o main`) and, if the compilation is successful (`&&`), we launch this file (`./main`).

The current shell expands `*.cpp` into a list of all `.cpp` files present in the directory and passes them in lexicographical order to the compiler. At this stage, the order of compilation is not relevant.

After the program completes execution, the `;` operator (regardless of whether execution finishes with or without errors) will trigger the removal (`rm`) of the executable file, as we no longer need it.

The program (in this and all subsequent examples) should output the following to the terminal:

```bash
[SimpleClass::SimpleFunc]
[SimpleTemplateFunc]: 11
[ComplexTemplateFunc]: 11
[SimpleTemplateFunc]: 22
[ComplexTemplateFunc]: 22
[SimpleClass::SimpleFunc]
[TemplateClass::EasyFunc]
```

Secondly, we compile all `.cpp` files of our project individually into separate `.o` object files and examine their contents.

```bash
# Step 2. Compile translation units and look through them
g++ -c *.cpp && for f in *.o; do echo -e "\n--- $f ---"; nm "$f" | grep -E 'SimpleClass|TemplateClass' | c++filt; done; rm *.o
```

The GCC compiler, invoked with the `-c` flag, creates object files for each `.cpp` file in the directory.

Then, using a `for` loop, we perform a series of operations on each object file `*.o`.

`echo -e "\n--- $f ---"` prints the file name to the console. The `-e` flag allows us to use escape characters, such as `\n` for a newline.

Next, we run the `nm` utility. It returns the names of all symbols (functions, variables) contained in the current object file `"$f"`.

The pipe operator `|` takes the output from `nm` and passes it to the next utility for further processing.

We don’t need all the symbols. The `grep` command-line tool (Global Regular Expression Print) filters the output received from `nm`, keeping only the symbols whose names include either the `SimpleClass` or `TemplateClass` class names.

The `-E` flag enables extended regular expressions, so we can use the logical operator `|` (OR) without that backslash (`\`) before it.

A few symbols that survive this filtering are piped to the `c++filt` utility. It performs **demangling**, transforming all those cryptic names into a human-readable format. This way, we don’t have to squint at monstrous constructs like `_ZN13TemplateClassIiE19ComplexTemplateFuncIiEEvRKT_`.

After the loop finishes reading the files (`done`), we remove the now-useless object files with `rm *.o`.

As a result, the terminal displays the following:

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

1. `SimpleClass::SimpleFunc()` is the first thing that catches the eye. Unlike all the others, we’ve separated this function’s declaration from its definition, placing the definition in a separate file: `TemplateUnit.cpp`.

The `T` (text) label marks a symbol with a strong definition. There can be only one strong symbol for the entire project being built - that’s why it’s called strong.

The `U` (undefined) label marks a symbol that’s used in this binary file but its definition exists somewhere else. So, the files `Alpha.o` and `Gamma.o` know about the existence of `SimpleClass::SimpleFunc()`, but not about its contents. At the compilation stage, these `.cpp` files only had access to the function’s declaration in the header file.

Later, when the executable is created, the linker will look for definitions of these undefined symbols in other object files and, if it can’t find them, it’ll throw a linkage error.

That long sequence of zeroes next to each symbol shows that no concrete address has been assigned to them yet. After all, we’re dealing with object files here - basically, unfinished products.

2. The `W` (weak) label tells us that the object file contains a **weak definition** of a symbol.

The "weakness" of this definition is that, when the linker starts tying all these object files together, it’ll consider such definitions only as a last resort. If there’s a strong definition available, the linker will use that as the canonical one. If no strong definition exists, it’ll grab any of the weak ones - usually whichever it processes first. All other weak symbols will be discarded during linking.

All the weak symbols in our project come from **implicit instantiations** - functions, classes, and variables that the compiler creates based on templates.

These implicit instantiations are a direct consequence of each `.cpp` file that includes `TemplateUnit.hpp` having access to the template definitions.

The repetition of the same definitions across several object files - `Alpha.o`, `Beta.o`, `Gamma.o` - perfectly illustrates the problem this article is dedicated to solve.

> **Warning**:
> When the same symbol is defined in multiple translation units, it means the compiler did extra work!
> The compiler processed the same code multiple times and produced duplicate blocks of machine instructions.
> In the end, the linker discards all of these duplicates except one.

In the small-scale examples we’re dealing with here, the time and resources wasted by the compiler are trivial.

But imagine a project with hundreds, if not thousands, of translation units, each with much larger and more complex template classes and functions.

Suddenly, you’re dealing with build systems that take not one minute, but five - or not five, but fifteen. Developers sit idle, and businesses lose money.

Of course, long build times have other causes as well. But duplicated symbols certainly play a role - a role we can actually do something about.

# Approach 2: Inline Files in the Header

From an optimization standpoint, this approach is identical to the first one, but it lets us keep the code just a bit tidier. This section is also meant to illustrate the idea of inline files for those who might not be familiar with it.

In the broadest sense, inline files contain code that should be part of the header file, but we’ve chosen to store it separately.

In practice, inline files can be used to isolate any parts of the header, not just template definitions.

In the following example, we use `TemplateUnit.inl` to store template definitions. This file gets included at the end of `TemplateUnit.hpp`, so the definitions from `.inl` become part of `.hpp`.

The rest of the project files are exactly the same as those shown in Approach 1, so we’re not reproducing them here.

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
// Other files are the same as in Approach 1
//-----------------------------

```

The `.inl` extension is used simply because it’s popular - it doesn’t have any formal significance. When we’re working with GCC, we can give the inline file any extension we want: `.txt`, `.jpg` - it doesn’t matter. At the very beginning of compilation, the preprocessor will dump the contents of `TemplateUnit.inl` into `TemplateUnit.hpp` anyway.

Let’s run the same two commands in the terminal that we used in the first approach. First, we’ll check the validity of our project by compiling and running the program. Second, we’ll perform the exact same symbol analysis operation in the object files as before.

The program’s output and the list of symbols in the translation units will be completely identical to what we got in the first approach.

That’s because, in this case, the inline file with the template definitions is simply a part of the header file - even if it’s been neatly separated into its own file. As long as the definitions remain accessible during compilation in each `.cpp`, placing them outside the class body doesn’t matter.

> **Conclusion**:
> Including an inline file in the header might make the code look a bit tidier, but it doesn’t do anything to optimize the compilation process.

# Approach 3: Inline Files in `.cpp`

This approach involves including the inline files not in the header file (and thus in every translation unit that uses it) but only in specific `.cpp` files.

For example, like this:

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
// Other files are the same as in Approach 2
//-----------------------------

```

Here’s what the symbol table looks like for this project:

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

So, why are the symbols arranged this way?

First, we included `TemplateUnit.inl` only in the implementation files `Alpha.cpp` and `Gamma.cpp`.

As a result, during compilation, only the `Alpha` and `Gamma` translation units had access to the templates. Because of this access, the compiler created instantiations (definitions) for the functions used in these `.cpp` files.

Second, we placed `extern template` in `Beta.cpp` for each function used in that file. We’ll discuss what `extern template` actually is in the next approach. For now, let’s just say that `extern template` guarantees that the compiler won’t attempt to create instantiations for these marked functions in this particular translation unit - it’ll look for their definitions during the linking stage instead.

To sum up, we eliminated the duplication of binary symbols across the project. But this approach is clearly not the most convenient.

We have to watch carefully to make sure each template definition is present in the project only once. In every new `.cpp` file that uses even a single template, we’ll need to do some mental juggling. Choosing between including the `.inl` file and using `extern template` isn’t always obvious. This situation becomes unbearably convoluted when the number of templates and translation units in the project grows beyond a handful.

> **Conclusion**:
> Including inline files directly in `.cpp` files helps avoid duplicate symbols. However, this approach requires excessive effort to maintain and is hardly practical in real-world projects.

# Approach 4: Explicit Template Instantiation

We’ll treat a **template instantiation** as a specific instance of a template function or class with given type parameters.

So, when we use templates in `.cpp` files - creating objects, calling functions - and substituting the abstract `<T>` with a concrete `<int>`, we’re dealing with an instantiation of the template for `int`.

In the previous approaches, we worked with **implicit instantiations** - we never explicitly defined, for example, `TemplateClass<int>::EasyFunc()`. That function (the instantiation for `int`) was generated by the compiler on its own based on the template. This happened in response to our call to `EasyFunc()` in `Gamma.cpp`.

Thus, **explicit instantiation** is an actual definition of a symbol based on a template that we explicitly write in the code.

For example:

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
// Other files are the same as in Approach 1
//-----------------------------
```

The project’s symbol table now looks much cleaner:

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

First, expressions like `template void TemplateClass<int>::EasyFunc();` in `TemplateUnit.cpp` are **explicit instantiation definitions**.

An explicit instantiation tells the compiler that this function is already defined for the given types. As a result, during the build process, the compiler will use this specific instance of the symbol instead of creating its copies.

Explicit instantiations allow us to centralize all template definitions in a single translation unit. This gets rid of the tangled web of dependencies that was present in Approach 3.

As the symbol table shows, the object files `Alpha.o`, `Beta.o`, and `Gamma.o` now contain only references to our four functions. Their definitions were compiled once in `TemplateUnit.o`.

Second, the expressions like `extern template void TemplateClass<int>::EasyFunc();` in `TemplateUnit.hpp` are **explicit instantiation declarations**.

By using `extern template`, we tell the compiler not to create template instantiations for `int` in this translation unit. Instead, we’re promising that the linker will find the needed instantiations elsewhere.

It’s worth noting that when building this project with GCC 15.1, these `extern template` expressions aren’t strictly required. If we remove them, the project’s symbol table won’t change, and there still won’t be any duplicated definitions.

This might lead us to mistakenly believe that explicit instantiation declarations aren’t needed at all. But that’s not true!

If we leave out the `extern template` declarations, we’re letting the compiler decide how to handle templates on its own. GCC is smart enough to compile the code and avoid redundant symbols without any hints. But this behavior isn’t standardized, and there’s no guarantee that other compilers will always do the same.

So, by combining explicit instantiation definitions and declarations, we can manage a large number of templates with relative ease.

Sure, this approach does require us to explicitly define each new symbol in the project that’s based on a template. But it’s still a lot simpler than Approach 3.

We also need to be careful to make sure the `extern template` declarations exactly match the `template` definitions in terms of instantiation types and signatures. Any mismatch can lead to compilation errors, unintended implicit instantiations, and duplicated symbols.

We can verify that we’ve done everything correctly with the following command:

```bash
g++ -fno-implicit-templates *.cpp -o main && ./main; rm main

g++ -fno-implicit-templates -c *.cpp && for f in *.o; do echo -e "\n--- $f ---"; nm "$f" | grep -E 'SimpleClass|TemplateClass' | c++filt; done; rm *.o
```

The `-fno-implicit-templates` flag prevents the compiler from creating hidden instantiations, putting that burden squarely on the shoulders of the developer. This flag makes the project entirely dependent on manual instantiation management.

If we try to apply this flag to the projects from the previous approaches, the compiler will throw a linkage error, complaining about undefined symbols in the `Alpha`, `Beta`, and `Gamma` translation units.

> **Conclusion**:
> One of the most predictable and scalable ways to eliminate symbol duplication in a project that uses templates is to define explicit instantiations of those templates in a separate `.cpp` file and maintain a matching set of `extern template` declarations in the header file.

# Approach 5: Using C++20 Modules

It’d be unfair to leave out C++20 modules when talking about template optimization.

Despite their novelty and apparent unfamiliarity, the fundamental principles of working with templates remain the same: we still need to instantiate templates ourselves, or let the compiler handle it.

The biggest difference in this project compared to the previous approaches is that we place everything related to templates in a single file, rather than splitting it across two.

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
// Other files are the same as in Approach 1
//-----------------------------

```

There are a few points to keep in mind when working with **modules**:

1. We’ve given the module file the extension `.cppm` to distinguish it from regular implementation files during the build process. Just like with `.inl` files, the `.cppm` extension doesn’t have any formal meaning - any combination of characters would work.

2. At the very beginning of this file, we open the global module fragment (`module;`) and import the standard input/output module (`import <stdio.h>;`) so we can use `printf()` and `puts()` later on.

3. With `export module TemplateModule;`, we declare the module `TemplateModule`, which we’ll import into our implementation files.

4. The line `export {` opens a block of symbols that will be available in the namespace of any translation unit that imports our module.

A few notes on the **relationship between explicit instantiations and modules**:

1. First off, explicit instantiations are still necessary - without them, the project’s symbol table would look just like it did in Approach 1.

2. Explicit instantiation declarations (`extern template`) are redundant here - every translation unit that imports `TemplateModule` already receives fully compiled definitions of the specific template functions.

3. The explicit instantiation definitions (`template` expressions at the end of `TemplateUnit.cppm`) can either be exported or left outside the `export {}` block. **Exporting** just places the symbol in the namespace of the importing translation unit. But for explicit instantiation definitions, all we need is **reachability**, which isn’t tied to whether the symbol is exported.

Next, we’ll build and run the project and take a look at its symbol table:

```bash

F="g++ -std=c++20 -fmodules-ts -fno-implicit-templates"; $F -x c++-system-header stdio.h && $F -c *.cppm && $F *.o *.cpp -o main && ./main; rm main *.o

g++ -std=c++20 -fmodules-ts -c *.cppm *.cpp && for f in *.o; do echo -e "\n--- $f ---"; nm "$f" | grep -E 'SimpleClass|TemplateClass' | c++filt; done; rm *.o

```

First, let’s create a global variable in the terminal: `F="g++ -std=c++20 -fmodules-ts -fno-implicit-templates"`. This way, we won’t have to repeat this fragment three times. The variable `F` will persist until we close the terminal.

The `-std=c++20` flag ensures we’re using the C++20 standard for subsequent compilations. The default standard for the compiler is C++17 now, which doesn’t support modules.

The `-fmodules-ts` flag activates module support. At this point, even the latest GCC 15.1 still treats modules as an experimental feature.

The fragment `-x c++-system-header stdio.h` compiles the standard C library file `stdio.h` into a module, so that later we can import it directly into our own module.

The second command will give us the exact same symbol distribution that we saw in Approach 4.

The only difference will be in the type names (`SimpleClass@TemplateModule::SimpleFunc()`) - they’ll now include the module’s name where they’re located.

> **Conclusion**:
> The C++20 module system does let us avoid symbol duplication during compilation, though it demands considerably more effort than the traditional approach with explicit instantiation declarations and definitions.

# Conclusion

We now know that carelessly introducing templates into a project can lead to significant compile-time slowdowns because of duplicate symbol definitions in different translation units. Fortunately, we have at least three ways to organize our use of templates to avoid this duplication.

All the files for the projects presented in this article are available at the following link: https://github.com/MangoFalls/ARTICLES/tree/main/004_OptimizingTemplates

# References

- https://lugdunum3d.github.io/doc/guidelines.html
- https://timsong-cpp.github.io/cppwp/n4861/module.reach
- https://en.cppreference.com/w/cpp/language/class_template
- https://gcc.gnu.org/onlinedocs/gcc-15.1.0/gcc/Template-Instantiation.html
- https://stackoverflow.com/questions/1208028/significance-of-a-inl-file-in-c
- https://learn.microsoft.com/en-us/cpp/cpp/source-code-organization-cpp-templates
- https://forums.unrealengine.com/t/can-inline-files-inl-be-included-from-a-ue4-header-file/396446
- https://developercommunity.visualstudio.com/t/intellisense-if-inl-file-with-inline-method-defini/158288
- https://www.reddit.com/r/cpp_questions/comments/1bcyci3/why_is_there_need_to_use_a_inl_file_after_using/
