#include <iostream>
#include "List.h"

int main()
{
    BiList list;
    list.add(new BiListNode());
    std::cout << list.size() << std::endl;
    return 0;
}
