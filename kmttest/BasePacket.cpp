
#include <iostream>
#include <iomanip>
#include "BasePacket.hpp"
#include "KFDTestUtil.hpp"
//#include "KFDBaseComponentTest.hpp"

BasePacket::BasePacket(void): m_packetAllocation(NULL)
{
	m_FamilyId = 5;// g_baseTest->GetFamilyIdFromDefaultNode();
}

BasePacket::~BasePacket(void)
{
    if (m_packetAllocation)
        free(m_packetAllocation);
}

void BasePacket::Dump() const
{
    unsigned int size = SizeInDWords();
    const HSAuint32 *packet = (const HSAuint32 *)GetPacket();
    unsigned int i;

	std::cout << "Packet dump:" << std::hex;
    for (i = 0; i < size; i++)
		std::cout << " " << std::setw(8) << std::setfill('0') << packet[i];
	std::cout << std::endl;
}

void *BasePacket::AllocPacket(void)
{
    unsigned int size = SizeInBytes();

    //EXPECT_NE(0, size);
    if (!size)
        return NULL;

    m_packetAllocation = calloc(1, size);
    //EXPECT_NOTNULL(m_packetAllocation);

    return m_packetAllocation;
}
