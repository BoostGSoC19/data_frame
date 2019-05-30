#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <any>
#include <memory>
class AnyBase
{
public:
    virtual ~AnyBase() = 0;
};

template<class T>
class Any : public AnyBase
{
public:
    typedef T Type;
    explicit Any(const Type& data) : data(data) {}
    Any() {}
    Type data;
};

int main() {
    std::map<std::string, AnyBase*> anymap;
    anymap["hello"] = new Any<int>(3);
    anymap["text"] = new Any<std::string>("5");
}
