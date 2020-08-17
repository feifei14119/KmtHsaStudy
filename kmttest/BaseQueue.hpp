#ifndef __KFD_BASE_QUEUE__H__
#define __KFD_BASE_QUEUE__H__

#include <vector>
#include "../libhsakmt/hsakmt.h"
#include "KFDTestUtil.hpp"
#include "BasePacket.hpp"

// @class BasePacket
class BaseQueue 
{
 public:
    static const unsigned int DEFAULT_QUEUE_SIZE = 4096;
    static const HSA_QUEUE_PRIORITY DEFAULT_PRIORITY = HSA_QUEUE_PRIORITY_NORMAL;
    static const unsigned int DEFAULT_QUEUE_PERCENTAGE  = 100;
    static const unsigned int ZERO_QUEUE_PERCENTAGE     = 0;
    static const unsigned int     FLUSH_GPU_CACHES_TO   = 1000;

    BaseQueue(void);
    virtual ~BaseQueue(void);

    /** Create the queue.
     *  @see hsaKmtCreateQueue
     *  @param pointers is used only for creating AQL queues. Otherwise it is omitted.
     */
    virtual HSAKMT_STATUS Create(unsigned int NodeId, unsigned int size = DEFAULT_QUEUE_SIZE, uint64_t *pointers = nullptr);
    /** Update the queue.
     *  @see hsaKmtUpdateQueue
     *  @param percent New queue percentage
     *  @param priority New queue priority
     *  @param nullifyBuffer
     *      If 'true', set the new buffer address to NULL and the size to 0. Otherwise
     *      don't change the queue buffer address/size.
     */
    virtual HSAKMT_STATUS Update(unsigned int percent, HSA_QUEUE_PRIORITY priority, bool nullifyBuffer);
    virtual HSAKMT_STATUS SetCUMask(unsigned int *mask, unsigned int mask_count);
    /** Destroy the queue.
     *  @see hsaKmtDestroyQueue
     */
    virtual HSAKMT_STATUS Destroy();
    /** Wait for all the packets submitted to the queue to be consumed. (i.e. wait until RPTR=WPTR).
     *  Note that all packets being consumed is not the same as all packets being processed.
     */
    virtual void Wait4PacketConsumption(HsaEvent *event = nullptr, unsigned int timeOut = 2000);
    /** @brief Place packet and submit it in one function
     */
    virtual void PlaceAndSubmitPacket(const BasePacket &packet);
    /** @brief Copy packet to queue and update write pointer
     */
    virtual void PlacePacket(const BasePacket &packet);
    /** @brief Update queue write pointer and set the queue doorbell to the queue write pointer
     */
    virtual void SubmitPacket() = 0;
    /** @brief Check if all packets in queue are already processed
     *  Compare queue read and write pointers
     */
    bool AllPacketsSubmitted();

    void SetSkipWaitConsump(int val) { m_SkipWaitConsumption = val; }
    int GetSkipWaitConsump() { return m_SkipWaitConsumption; }
    int Size() { return m_QueueBuf->Size(); }

    HsaQueueResource *GetResource() { return &m_Resources; }
    unsigned int GetPendingWptr() { return m_pendingWptr; }
    HSAuint64 GetPendingWptr64() { return m_pendingWptr64; }
    virtual _HSA_QUEUE_TYPE GetQueueType() = 0;
    unsigned int GetNodeId() { return m_Node; }

 protected:
    static const unsigned int CMD_NOP_TYPE_2        = 0x80000000;
    static const unsigned int CMD_NOP_TYPE_3        = 0xFFFF1002;

    unsigned int CMD_NOP;
    unsigned int m_pendingWptr;
    HSAuint64 m_pendingWptr64;
    HsaQueueResource m_Resources;
    HsaMemoryBuffer *m_QueueBuf;
    unsigned int m_Node;

    // @return Write pointer modulo queue size in dwords
    virtual unsigned int Wptr() = 0;
    // @return Read pointer modulo queue size in dwords
    virtual unsigned int Rptr() = 0;
    // @return Expected m_Resources.Queue_read_ptr when all packets consumed
    virtual unsigned int RptrWhenConsumed() = 0;
    virtual PACKETTYPE PacketTypeSupported() = 0;

 private:
    // Some tests(such as exception) may not need wait pm4 packet consumption on CZ.
    int m_SkipWaitConsumption;
};


#endif  // __KFD_BASE_QUEUE__H__
