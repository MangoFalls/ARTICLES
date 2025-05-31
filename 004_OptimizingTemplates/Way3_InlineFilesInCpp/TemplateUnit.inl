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
