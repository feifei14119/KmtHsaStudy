
#include <iostream>
#include <iomanip>
#include "BasePacket.hpp"
//#include "KFDTestUtil.hpp"

void BasePacket::Dump() const 
{
    unsigned int size = SizeInDWords();
    const uint32_t *packet = (const uint32_t *)GetPacket();
    unsigned int i;

    std::cout << "Packet dump:" << std::hex;
    for (i = 0; i < size; i++)
		std::cout << " " << std::setw(8) << std::setfill('0') << packet[i];
	std::cout << std::endl;
}
