
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
