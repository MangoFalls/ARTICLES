
#include "stdio.h"
#include "TemplateUnit.hpp"

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

template void SimpleClass::SimpleTemplateFunc<int>(const int&);
template void TemplateClass<int>::EasyFunc();
template void TemplateClass<int>::ComplexTemplateFunc<int>(const int&);
