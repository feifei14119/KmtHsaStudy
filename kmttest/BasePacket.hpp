#ifndef __KFD_BASE_PACKET__H__
#define __KFD_BASE_PACKET__H__

/**
 * All packets profiles must be defined here
 * Every type defined here has sub-types
 */
enum PACKETTYPE 
{
    PACKETTYPE_PM4,
    PACKETTYPE_SDMA,
    PACKETTYPE_AQL
};

// @class BasePacket
class BasePacket 
{
 public:
    BasePacket(void);
    virtual ~BasePacket(void);

    // @returns Packet type
    virtual PACKETTYPE PacketType() const = 0;
    // @returns Pointer to the packet
    virtual const void *GetPacket() const = 0;
    // @returns Packet size in bytes
    virtual unsigned int SizeInBytes() const = 0;
    // @returns Packet size in dwordS
    unsigned int SizeInDWords() const { return SizeInBytes()/sizeof(unsigned int); }

    void Dump() const;

 protected:
    unsigned int m_FamilyId;
    void *m_packetAllocation;

    void *AllocPacket(void);
};

#endif  // __KFD_BASE_PACKET__H__
