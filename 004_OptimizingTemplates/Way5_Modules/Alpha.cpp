
import TemplateModule;
#include "Alpha.hpp"
void AlphaLogic() {
    SimpleClass().SimpleFunc();
    SimpleClass().SimpleTemplateFunc(11);
    TemplateClass<int>().ComplexTemplateFunc(11);
}
