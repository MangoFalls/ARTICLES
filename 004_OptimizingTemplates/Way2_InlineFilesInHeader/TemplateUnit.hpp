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
