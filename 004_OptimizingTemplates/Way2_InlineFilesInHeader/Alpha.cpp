
#include "Alpha.hpp"
#include "TemplateUnit.hpp"
void AlphaLogic() {
    SimpleClass().SimpleFunc();
    SimpleClass().SimpleTemplateFunc(11);
    TemplateClass<int>().ComplexTemplateFunc(11);
}
