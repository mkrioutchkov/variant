// Testing/example code
#include "variant.h"   
#include <string>

int main()
{
    variant v = 9;
    std::cout << v << std::endl;
    struct S{S(std::string x) : g(std::move(x)) {} std::string g;};
    v = S{"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" };
    auto ptr = static_cast<S*>(v)->g.c_str();
    std::cout << ptr << std::endl;
    v = S{"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"};
    std::cout << v << std::endl; // prints typeid as best it can do
    std::cout << static_cast<S*>(v)->g << std::endl; // << ' ' << ptr; // ptr is junk
    std::string s = "cccccccccccccccccccccccccccccccccccccccccccccccccc";
    v = std::move(s);
    std::cout << v << std::endl;
    std::cout << s << std::endl;
    
    v = S{""};
    std::cout << v << std::endl;
    v = std::string("ddddddddddddddddddddddddddddddddd");
    variant v2 = v;
    std::cout << v << std::endl;
    std::cout << v2 << std::endl;
    variant v3 = std::move(v);
    std::cout << v << std::endl;
    std::cout << v3 << std::endl;
    *static_cast<std::string*>(v) = "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
    std::cout << v << std::endl;
    
    struct S2 : std::string {};
    
    if(static_cast<S2*>(v))
    {
        std::cout << "Cast should fail!" << std::endl;
        return 1;
    }
    
    v = S2{{"fffffffffffffffffffffffffffffffffffffffffffffffffffff"}};
    if(static_cast<std::string*>(v))
    {
        std::cout << "Cast should fail (limitation)!" << std::endl;
        return 1;
    }
    
    std::cout << v << std::endl; // should still print base type
    
    if(static_cast<S2*>(v) && static_cast<const S2*>(v))
        std::cout << "Both should work" << std::endl;
}
