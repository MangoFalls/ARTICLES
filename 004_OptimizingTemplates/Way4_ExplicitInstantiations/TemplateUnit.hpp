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

extern template void SimpleClass::SimpleTemplateFunc<int>(const int&);
extern template void TemplateClass<int>::EasyFunc();
extern template void TemplateClass<int>::ComplexTemplateFunc<int>(const int&);