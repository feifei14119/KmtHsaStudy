
#ifndef __KFD_SDMA_PACKET__H__
#define __KFD_SDMA_PACKET__H__

#include "BasePacket.hpp"
#include "./include/sdma_pkt_struct.h"

// @class SDMAPacket: Marks a group of all SDMA packets
class SDMAPacket : public BasePacket 
{
 public:
        SDMAPacket(void) {}
        virtual ~SDMAPacket(void) {}

        virtual PACKETTYPE PacketType() const { return PACKETTYPE_SDMA; }
};

class SDMAWriteDataPacket : public SDMAPacket
{
 public:
    // This contructor will also init the packet, no need for additional calls
    SDMAWriteDataPacket(unsigned int familyId, void* destAddr, unsigned int data);
    SDMAWriteDataPacket(unsigned int familyId, void* destAddr, unsigned int ndw, void *data);

    virtual ~SDMAWriteDataPacket(void) {}

    // @returns Pointer to the packet
    virtual const void *GetPacket() const  { return packetData; }
    // @breif Initialise the packet
    void InitPacket(void* destAddr, unsigned int ndw, void *data);
    // @returns Packet size in bytes
    virtual unsigned int SizeInBytes() const { return packetSize; }

 protected:
    // SDMA_PKT_WRITE_UNTILED struct contains all the packet's data
    SDMA_PKT_WRITE_UNTILED *packetData;
    unsigned int packetSize;
};

class SDMACopyDataPacket : public SDMAPacket
{
 public:
    // This contructor will also init the packet, no need for additional calls
    SDMACopyDataPacket(unsigned int familyId, void *dest, void *src, unsigned int size);
    SDMACopyDataPacket(unsigned int familyId, void *const dst[], void *src, int n, unsigned int surfsize);

    virtual ~SDMACopyDataPacket(void) {}

    // @returns Pointer to the packet
    virtual const void *GetPacket() const  { return packetData; }

    // @returns Packet size in bytes
    virtual unsigned int SizeInBytes() const { return packetSize; }

 protected:
    // SDMA_PKT_COPY_LINEAR struct contains all the packet's data
    SDMA_PKT_COPY_LINEAR  *packetData;

    unsigned int packetSize;
};

class SDMAFillDataPacket : public SDMAPacket 
{
 public:
    // This contructor will also init the packet, no need for additional calls
    SDMAFillDataPacket(unsigned int familyId, void *dest, unsigned int data, unsigned int size);

    virtual ~SDMAFillDataPacket(void) {}

    // @returns Pointer to the packet
    virtual const void *GetPacket() const  { return m_PacketData; }

    // @returns Packet size in bytes
    virtual unsigned int SizeInBytes() const { return m_PacketSize; }

 protected:
    // SDMA_PKT_CONSTANT_FILL struct contains all the packet's data
    SDMA_PKT_CONSTANT_FILL  *m_PacketData;

    unsigned int m_PacketSize;
};

class SDMAFencePacket : public SDMAPacket 
{
 public:
    // Empty constructor, before using the packet call the init func
    SDMAFencePacket(void);
    // This contructor will also init the packet, no need for additional calls
    SDMAFencePacket(unsigned int familyId, void* destAddr, unsigned int data);

    virtual ~SDMAFencePacket(void);

    // @returns Pointer to the packet
    virtual const void *GetPacket() const  { return &packetData; }
    // @brief Initialise the packet
    void InitPacketCI(void* destAddr, unsigned int data);
    void InitPacketNV(void* destAddr, unsigned int data);

    // @returns Packet size in bytes
    virtual unsigned int SizeInBytes() const { return sizeof(SDMA_PKT_FENCE ); }

 protected:
    // SDMA_PKT_FENCE struct contains all the packet's data
    SDMA_PKT_FENCE  packetData;
};

class SDMATrapPacket : public SDMAPacket 
{
 public:
    // Empty constructor, before using the packet call the init func
    explicit SDMATrapPacket(unsigned int eventID = 0);

    virtual ~SDMATrapPacket(void);

    // @returns Pointer to the packet
    virtual const void *GetPacket() const  { return &packetData; }
    // @brief Initialise the packet
    void InitPacket(unsigned int eventID);
    // @returns Packet size in bytes
    virtual unsigned int SizeInBytes() const { return sizeof(SDMA_PKT_TRAP); }

 protected:
    // SDMA_PKT_TRAP struct contains all the packet's data
    SDMA_PKT_TRAP  packetData;
};

class SDMATimePacket : public SDMAPacket 
{
 public:
    // Empty constructor, before using the packet call the init func
    SDMATimePacket(void*);

    virtual ~SDMATimePacket(void);

    // @returns Pointer to the packet
    virtual const void *GetPacket() const  { return &packetData; }
    // @brief Initialise the packet
    void InitPacket(void*);
    // @returns Packet size in bytes
    virtual unsigned int SizeInBytes() const { return sizeof(SDMA_PKT_TIMESTAMP); }

 protected:
    SDMA_PKT_TIMESTAMP  packetData;
};

class SDMANopPacket : public SDMAPacket 
{
 public:
    SDMANopPacket(unsigned int count = 1);
    virtual ~SDMANopPacket(void) {}

    // @returns Pointer to the packet
    virtual const void *GetPacket() const { return packetData; }
    // @returns Packet size in bytes
    virtual unsigned int SizeInBytes() const { return packetSize; }

 private:
    SDMA_PKT_NOP *packetData;
    unsigned int packetSize;
};


#endif  // __KFD_SDMA_PACKET__H__
