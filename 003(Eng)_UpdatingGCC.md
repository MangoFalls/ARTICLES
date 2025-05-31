
# Moving to C++26: How to Build and Set Up GCC 15.1 on Ubuntu

GCC 15.1 is out with C++26 features! But Ubuntu still ships GCC 13.
So let’s build 15.1 from source and start experimenting.

# Introduction

Programming languages like C and C++ move fast — but compilers and system packages don't always keep up.

With the release of GCC 15.1.0 on April 25, 2025, a bunch of shiny new C++26 features are now officially available. Naturally, I wanted to dive right in — but there’s a catch: the current Ubuntu release (`24.04.2 LTS`) still ships with GCC 13.3.0, which is starting to feel a bit old.

If we want to experiment with the latest features today, we’ll need to roll up our sleeves and build GCC 15.1 from source.

In this guide, we'll walk through how to grab the newest GCC, compile it ourself, set it up as our system's default compiler (without breaking anything!), and finally, build a simple C++26 program to make sure everything's working.

# 1. Updating the System

Fire up our terminal and run:

```bash

sudo apt update && sudo apt upgrade -y

```

This command, executed with administrative privileges (`sudo`), tells the system to fetch the latest package info (`apt update`) and then — if that goes well (`&&`) — install all available updates (`apt upgrade`) without prompting us to confirm (`-y`).

# 2. Checking our Current GCC Version

No point diving into a full install if we’ve already got the latest compiler, right?

```bash

sudo g++ --version && g++ --version

```

This checks which C++ compiler version is being used by both your regular user account and the `sudo` user. Usually, both will return something like: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0`.

If that version’s too old for us, it's time to move on...

# 3. Installing Required Dependencies

```bash

sudo apt install -y build-essential git make gawk flex bison libgmp-dev libmpfr-dev libmpc-dev python3 binutils perl libisl-dev libzstd-dev tar gzip bzip2

```

This grabs all the tools and libraries we’ll need to build GCC from source.

Some of these might already be installed — for example, `build-essential` covers core dev tools like `gcc`, `g++`, and `make`, making sure everything’s set up properly.

```bash

export CONFIG_SHELL=/bin/bash

```

This sets the `CONFIG_SHELL` environment variable so that the GCC build system uses `bash` for all its scripts, instead of whatever other shell might be active. It’s a small thing, but important for avoiding weird build issues.

# 4. Downloading the Latest GCC Sources

```bash

mkdir ~/gcc-15
cd ~/gcc-15
git clone https://gcc.gnu.org/git/gcc.git gcc-15-source
cd gcc-15-source
git checkout releases/gcc-15.1.0
./contrib/download_prerequisites

```

Here we create a working directory (`gcc-15`) and clone the GCC source repo (`https://gcc.gnu.org/git/gcc.git`) into `gcc-15-source`, along with its necessary prerequisites (`./contrib/download_prerequisites`).

We then switch the repository to the specific version we need. As of late April 2025, that is `15.1.0`.

The command `git checkout releases/gcc-15.1.0` uses Git tags, which are basically simple pointers to specific commits representing stable source versions.

Later on, newer versions will be released. To list all available Git tags, we can run `git tag -l`.

# 5. Configuring the Build

```bash

cd ~/gcc-15
mkdir gcc-15-build
cd gcc-15-build

../gcc-15-source/configure --prefix=/opt/gcc-15 --disable-multilib --enable-languages=c,c++

```

Inside our `gcc-15` directory, we create a dedicated build folder (`gcc-15-build`) and move into it. This keeps build files separate from the source — a good habit when compiling anything from source.

Then we run the `configure` script to generate the build rules (i.e., the `Makefile`), customized for our setup.

Here’s what those options mean:

- `--prefix=/opt/gcc-15` tells the build system to install GCC into `/opt/gcc-15`, so it won’t interfere with the system’s existing compiler.

- `--disable-multilib` skips building support for 32-bit targets — useful if we’re only targeting 64-bit and want a faster, simpler build.

- `--enable-languages=c,c++` limits the build to just the C and C++ compilers. GCC also supports other languages like `fortran`, `go`, and `d`, but we don’t need those right now.

# 6. Building the Compiler

```bash

make -j$(nproc)

```

This kicks off the actual build process using all available CPU cores.

The `-j` flag (full form: `--jobs`) tells `make` how many tasks to run in parallel. `$(nproc)` automatically fills in the number of CPU cores, so we’re building as fast as our machine allows.

This step takes the longest — could be 30 minutes, could be several hours, depending on our hardware.

Want to tweak things? We can hardcode the number of threads (`make -j8`) or be more creative (`make -j$(($(nproc) - 1))`).

# 7. Installing and Activating GCC

Once the build’s done, it's time to install it:

```bash

sudo make install

```

This installs our newly compiled GCC into the system under `/opt/gcc-15`, getting it ready for use.

But — if we check the compiler version now with `g++ --version`, the system will probably still show the old GCC version, like nothing happened.

That’s because we still need to _tell_ the system to use our shiny new GCC by default.

```bash

sudo update-alternatives --install /usr/bin/gcc gcc /opt/gcc-15/bin/gcc 100
sudo update-alternatives --install /usr/bin/g++ g++ /opt/gcc-15/bin/g++ 100

```

These commands register our new GCC binaries, found in `/opt/gcc-15/bin/`, with the system.

We need two separate registrations: one for `gcc` (the C compiler) and one for `g++` (the C++ compiler). Even though `gcc` can compile C++ too (with some GCC-specific extensions), it’s better practice to use `g++` for C++ code.

`100` sets the priority — higher numbers mean higher priority when the system decides which version to pick by default.

Now we can switch versions manually if needed:

```bash

sudo update-alternatives --config gcc
sudo update-alternatives --config g++

```

Each command will show a list of all registered compilers, and we just pick the one we want to use.

And that’s it!

After switching, running `g++ --version` should now show the version of the brand-new GCC we just built.

If at some point we want to go back to the old GCC, we can just re-run the `--config` commands and pick the older version.

In case the old GCC isn’t registered yet, we can add it manually like this:

```bash

sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 50
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 50

```

# 8. Enjoying the New Features

Now we’re ready to start experimenting with all the cool stuff from the C++26 standard!

First, let’s set up a test project. This creates a fresh folder and a new `.cpp` file where we can write some code using C++26 features.

```bash

mkdir gcc_test_folder
cd gcc_test_folder
touch test26.cpp

```

Add this code into the `.cpp` file:

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

This little program covers all the major C++26 features that GCC 15.1.0 currently supports, giving us a great way to test everything at once in a single buildable file.

Next, while staying inside our test directory, let’s compile the file with full C++26 support — and without any GNU extensions — by using either `-std=c++2c` or `-std=c++26`.

Then we'll run the compiled program (`./test26`):

```bash

g++ -std=c++26 test26.cpp -o test26

./test26

```

If it compiles successfully — even though the compiler spits out some planned warnings — everything is fine and works perfectly.

# References

- https://gcc.gnu.org/install/
- https://gcc.gnu.org/projects/cxx-status.html
- https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html
- https://developers.redhat.com/articles/2025/04/24/new-c-features-gcc-15

