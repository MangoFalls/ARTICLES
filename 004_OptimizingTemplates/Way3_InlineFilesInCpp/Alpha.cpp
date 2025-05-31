
#include "Alpha.hpp"
#include "TemplateUnit.hpp"
#include "TemplateUnit.inl"
void AlphaLogic() {
    SimpleClass().SimpleFunc();
    SimpleClass().SimpleTemplateFunc(11);
    TemplateClass<int>().ComplexTemplateFunc(11);
}
