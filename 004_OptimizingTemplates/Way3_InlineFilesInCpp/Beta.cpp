
#include "Beta.hpp"
#include "TemplateUnit.hpp"

extern template void SimpleClass::SimpleTemplateFunc<int>(const int&);
extern template void TemplateClass<int>::ComplexTemplateFunc<int>(const int&);

void BetaLogic() {
    SimpleClass().SimpleTemplateFunc(22);
    TemplateClass<int>().ComplexTemplateFunc(22);
}
