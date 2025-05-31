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
