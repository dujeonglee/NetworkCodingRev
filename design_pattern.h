#ifndef DESIGN_PATTERN
#define DESIGN_PATTERN

#define SINGLETON_PATTERN_DECLARATION_H(class_type) \
private: \
static class_type* _instance; \
void init();\
class_type(){\
    init();\
} \
~class_type(){} \
public:\
static inline class_type* instance(){\
    return _instance;\
}

#define SINGLETON_PATTERN_INITIALIZATION_CPP(class_type)\
class_type* class_type::_instance = new class_type;\
void class_type::init()
#endif
