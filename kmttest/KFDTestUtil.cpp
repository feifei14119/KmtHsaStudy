#include "KFDTestUtil.hpp"

#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <string.h>
//#include "BaseQueue.hpp"
//#include "Dispatch.hpp"
//#include "SDMAPacket.hpp"

uint64_t RoundToPowerOf2(uint64_t val) 
{
  int bytes = sizeof(uint64_t);

  val--;

  for (int i = 0; i < bytes; i++) 
  {
    val |= val >> (1 << i);
  }

  val++;

  return val;
}

bool WaitOnValue(const volatile unsigned int *buf, unsigned int value, unsigned int timeOut) 
{
    while (timeOut > 0 && *buf != value) 
	{
        usleep(1000);

        if (timeOut != HSA_EVENTTIMEOUT_INFINITE)
            timeOut--;
    }

    return *buf == value;
}

void SplitU64(const HSAuint64 value, unsigned int& rLoPart, unsigned int& rHiPart) 
{
    rLoPart = static_cast<unsigned int>(value);
    rHiPart = static_cast<unsigned int>(value >> 32);
}


HSAKMT_STATUS CreateQueueTypeEvent(
    bool                ManualReset,            // IN
    bool                IsSignaled,             // IN
    unsigned int        NodeId,                 // IN
    HsaEvent**          Event                   // OUT
    ) 
{
    HsaEventDescriptor Descriptor;

// TODO: Create per-OS header with this sort of definitions
#ifdef _WIN32
    Descriptor.EventType = HSA_EVENTTYPE_QUEUE_EVENT;
#else
    Descriptor.EventType = HSA_EVENTTYPE_SIGNAL;
#endif
    Descriptor.SyncVar.SyncVar.UserData = (void*)0xABCDABCD;
    Descriptor.NodeId = NodeId;

    return hsaKmtCreateEvent(&Descriptor, ManualReset, IsSignaled, Event);
}




const HsaMemoryBuffer HsaMemoryBuffer::Null;

HsaMemoryBuffer::HsaMemoryBuffer(HSAuint64 size, unsigned int node, 
	bool zero, 
	bool isLocal, 
	bool isExec,
	bool isScratch, 
	bool isReadOnly)
	
    :m_Size(size),
    m_pUser(NULL),
    m_pBuf(NULL),
    m_Local(isLocal),
    m_Node(node)
{
    m_Flags.Value = 0;

    HsaMemMapFlags mapFlags = {0};
    bool map_specific_gpu = (node && !isScratch);

    if (isScratch)
	{
        m_Flags.ui32.Scratch = 1;
        m_Flags.ui32.HostAccess = 1;
    } 
	else 
	{
        m_Flags.ui32.PageSize = HSA_PAGE_SIZE_4KB;

        if (isLocal) 
		{
            m_Flags.ui32.HostAccess = 0;
            m_Flags.ui32.NonPaged = 1;
        } 
		else 
		{
            m_Flags.ui32.HostAccess = 1;
            m_Flags.ui32.NonPaged = 0;
        }

        if (isExec)
            m_Flags.ui32.ExecuteAccess = 1;
    }
	
    if (isReadOnly)
        m_Flags.ui32.ReadOnly = 1;

    hsaKmtAllocMemory(m_Node, m_Size, m_Flags, &m_pBuf);
    hsaKmtMapMemoryToGPUNodes(m_pBuf, m_Size, NULL, mapFlags, 1, &m_Node);
    m_MappedNodes = 1 << m_Node;

    if (zero && !isLocal)
        Fill(0);
}

HsaMemoryBuffer::HsaMemoryBuffer(void *addr, HSAuint64 size):
    m_Size(size),
    m_pUser(addr),
    m_pBuf(NULL),
    m_Local(false),
    m_Node(0) 
{
    HSAuint64 gpuva = 0;
    hsaKmtRegisterMemory(m_pUser, m_Size);
    hsaKmtMapMemoryToGPU(m_pUser, m_Size, &gpuva);
    m_pBuf = gpuva ? (void *)gpuva : m_pUser;
}

HsaMemoryBuffer::HsaMemoryBuffer()
    :m_Size(0),
    m_pBuf(NULL) {
}

void HsaMemoryBuffer::Fill(unsigned char value, HSAuint64 offset, HSAuint64 size)
{
    HSAuint32 uiValue;

    //EXPECT_EQ(m_Local, 0) << "Local Memory. Call Fill(HSAuint32 value, BaseQueue& baseQueue)";

    size = size ? size : m_Size;
    //ASSERT_TRUE(size + offset <= m_Size) << "Buffer Overflow" << std::endl;

    if (m_pUser != NULL)
        memset(reinterpret_cast<char *>(m_pUser) + offset, value, size);
    else if (m_pBuf != NULL)
        memset(reinterpret_cast<char *>(m_pBuf) + offset, value, size);
    else
        std::cout << "Invalid HsaMemoryBuffer";
}

/* Fill CPU accessible buffer with the value. */
void HsaMemoryBuffer::Fill(HSAuint32 value, HSAuint64 offset, HSAuint64 size) 
{
    HSAuint64 i;
    HSAuint32 *ptr = NULL;

   // EXPECT_EQ(m_Local, 0) << "Local Memory. Call Fill(HSAuint32 value, BaseQueue& baseQueue)";
    size = size ? size : m_Size;
    //EXPECT_EQ((size & (sizeof(HSAuint32) - 1)), 0) << "Not word aligned. Call Fill(unsigned char)";
    //ASSERT_TRUE(size + offset <= m_Size) << "Buffer Overflow" << std::endl;

    if (m_pUser != NULL)
        ptr = reinterpret_cast<HSAuint32 *>(reinterpret_cast<char *>(m_pUser) + offset);
    else if (m_pBuf != NULL)
        ptr = reinterpret_cast<HSAuint32 *>(reinterpret_cast<char *>(m_pBuf) + offset);

    //ASSERT_NOTNULL(ptr);

    for (i = 0; i < size / sizeof(HSAuint32); i++)
        ptr[i] = value;
}

/* Fill GPU only accessible Local memory with @value using SDMA Constant Fill Command */
void HsaMemoryBuffer::Fill(HSAuint32 value, BaseQueue& baseQueue, HSAuint64 offset, HSAuint64 size) 
{
    /*HsaEvent* event = NULL;

    EXPECT_NE(m_Local, 0) << "Not Local Memory. Call Fill(HSAuint32 value)";

    ASSERT_SUCCESS(CreateQueueTypeEvent(false, false, m_Node, &event));
    ASSERT_EQ(baseQueue.GetQueueType(), HSA_QUEUE_SDMA) << "Only SDMA queues supported";

    size = size ? size : m_Size;
    ASSERT_TRUE(size + offset <= m_Size) << "Buffer Overflow" << std::endl;

    baseQueue.PlacePacket(SDMAFillDataPacket((reinterpret_cast<void *>(this->As<char*>() + offset)), value, size));
    baseQueue.PlacePacket(SDMAFencePacket(reinterpret_cast<void*>(event->EventData.HWData2), event->EventId));
    baseQueue.PlaceAndSubmitPacket(SDMATrapPacket(event->EventId));
    EXPECT_SUCCESS(hsaKmtWaitOnEvent(event, g_TestTimeOut));

    hsaKmtDestroyEvent(event);*/
}

/* Check if HsaMemoryBuffer[location] has the pattern specified.
 * Return TRUE if correct pattern else return FALSE
 * HsaMemoryBuffer has to be CPU accessible
 */
bool HsaMemoryBuffer::IsPattern(HSAuint64 location, HSAuint32 pattern) 
{
#if 0
    HSAuint32 *ptr = NULL;

    EXPECT_EQ(m_Local, 0) << "Local Memory. Call IsPattern(..baseQueue& baseQueue)";

    if (location >= m_Size) /* Out of bounds */
        return false;

    if (m_pUser != NULL)
        ptr = reinterpret_cast<HSAuint32 *>(m_pUser);
    else if (m_pBuf != NULL)
        ptr = reinterpret_cast<HSAuint32 *>(m_pBuf);
    else
        return false;

    if (ptr)
        return (ptr[location/sizeof(HSAuint32)] == pattern);

    return false;
#endif
	return false;
}

/* Check if HsaMemoryBuffer[location] has the pattern specified.
 * Return TRUE if correct pattern else return FALSE
 * HsaMemoryBuffer is supposed to be only GPU accessible
 * Use @baseQueue to copy the HsaMemoryBuffer[location] to stack and check the value
 */
bool HsaMemoryBuffer::IsPattern(HSAuint64 location, HSAuint32 pattern, BaseQueue& baseQueue, volatile HSAuint32 *tmp) 
{
#if 0
    HsaEvent* event = NULL;
    int ret;

    EXPECT_NE(m_Local, 0) << "Not Local Memory. Call IsPattern(HSAuint64 location, HSAuint32 pattern)";
    EXPECT_EQ(baseQueue.GetQueueType(), HSA_QUEUE_SDMA) << "Only SDMA queues supported";

    if (location >= m_Size) /* Out of bounds */
        return false;

    ret = CreateQueueTypeEvent(false, false, m_Node, &event);
    if (ret)
        return false;

    *tmp = ~pattern;
    baseQueue.PlacePacket(SDMACopyDataPacket((void *)tmp,
            reinterpret_cast<void *>(this->As<HSAuint64>() + location),
            sizeof(HSAuint32)));
    baseQueue.PlacePacket(SDMAFencePacket(reinterpret_cast<void*>(event->EventData.HWData2),
            event->EventId));
    baseQueue.PlaceAndSubmitPacket(SDMATrapPacket(event->EventId));

    ret = hsaKmtWaitOnEvent(event, g_TestTimeOut);
    hsaKmtDestroyEvent(event);
    if (ret)
        return false;

    return WaitOnValue(tmp, pattern);
#endif
	return false;
}

int HsaMemoryBuffer::MapMemToNodes(unsigned int *nodes, unsigned int nodes_num)
{
    int ret, bit;

    ret = hsaKmtRegisterMemoryToNodes(m_pBuf, m_Size, nodes_num, nodes);
    if (ret != 0)
        return ret;
    ret = hsaKmtMapMemoryToGPU(m_pBuf, m_Size, NULL);
    if (ret != 0) 
	{
        hsaKmtDeregisterMemory(m_pBuf);
        return ret;
    }

    for (unsigned int i = 0; i < nodes_num; i++) 
	{
        bit = 1 << nodes[i];
        m_MappedNodes |= bit;
    }

    return 0;
}

int HsaMemoryBuffer::UnmapMemToNodes(unsigned int *nodes, unsigned int nodes_num)
{
    int ret, bit;

    ret = hsaKmtUnmapMemoryToGPU(m_pBuf);
    if (ret)
        return ret;

    hsaKmtDeregisterMemory(m_pBuf);
    for (unsigned int i = 0; i < nodes_num; i++)
	{
        bit = 1 << nodes[i];
        m_MappedNodes &= ~bit;
    }

    return 0;
}

void HsaMemoryBuffer::UnmapAllNodes()
{
    unsigned int *Arr, size, i, j;
    int bit;

    size = 0;
    for (i = 0; i < 8; i++) 
	{
        bit = 1 << i;
        if (m_MappedNodes & bit)
            size++;
    }

    Arr = (unsigned int *)malloc(sizeof(unsigned int) * size);
    if (!Arr)
        return;

    for (i = 0, j =0; i < 8; i++)
	{
        bit = 1 << i;
        if (m_MappedNodes & bit)
            Arr[j++] = i;
    }

    /*
     * TODO: When thunk is updated, use hsaKmtRegisterToNodes. Then nodes will be used
     */
    hsaKmtUnmapMemoryToGPU(m_pBuf);
    hsaKmtDeregisterMemory(m_pBuf);

    m_MappedNodes = 0;

    free(Arr);
}

HsaMemoryBuffer::~HsaMemoryBuffer() 
{
    if (m_pUser != NULL) 
	{
        hsaKmtUnmapMemoryToGPU(m_pUser);
        hsaKmtDeregisterMemory(m_pUser);
    } 
	else if (m_pBuf != NULL) 
	{
            if (m_MappedNodes)
			{
                hsaKmtUnmapMemoryToGPU(m_pBuf);
                hsaKmtDeregisterMemory(m_pBuf);
            }
        hsaKmtFreeMemory(m_pBuf, m_Size);
    }
    m_pBuf = NULL;
}

HsaInteropMemoryBuffer::HsaInteropMemoryBuffer(HSAuint64 device_handle, HSAuint64 buffer_handle, HSAuint64 size, unsigned int node)
    :m_Size(0),
     m_pBuf(NULL),
     m_graphic_handle(0),
     m_Node(node) 
{
    HSAuint64 flat_address;
    hsaKmtMapGraphicHandle(m_Node, device_handle, buffer_handle, 0, size, &flat_address);
    m_pBuf = reinterpret_cast<void*>(flat_address);
}

HsaInteropMemoryBuffer::~HsaInteropMemoryBuffer() 
{
    hsaKmtUnmapGraphicHandle(m_Node, (HSAuint64)m_pBuf, m_Size);
}


HsaNodeInfo::HsaNodeInfo() {}

/* Init - Get and store information about all the HSA nodes from the Thunk Library.
 * @NumOfNodes - Number to system nodes returned by hsaKmtAcquireSystemProperties
 * @Return - false: if no node information is available
 */
bool HsaNodeInfo::Init(int NumOfNodes) 
{
    HsaNodeProperties *nodeProperties;
    _HSAKMT_STATUS status;
    bool ret = false;

    for (int i = 0; i < NumOfNodes; i++)
	{
        nodeProperties = new HsaNodeProperties();

        status = hsaKmtGetNodeProperties(i, nodeProperties);
        /* This is not a fatal test (not using assert), since even when it fails for one node
         * we want to get information regarding others.
         */
        //EXPECT_SUCCESS(status) << "Node index: " << i << "hsaKmtGetNodeProperties returned status " << status;

        if (status == HSAKMT_STATUS_SUCCESS) 
		{
            m_HsaNodeProps.push_back(nodeProperties);
            ret = true;  // Return true if atleast one information is available

            if (nodeProperties->NumFComputeCores)
                m_NodesWithGPU.push_back(i);
            else
                m_NodesWithoutGPU.push_back(i);
        } 
		else 
		{
            delete nodeProperties;
        }
    }

    return ret;
}

HsaNodeInfo::~HsaNodeInfo() {
    const HsaNodeProperties *nodeProperties;

    for (unsigned int i = 0; i < m_HsaNodeProps.size(); i++)
        delete m_HsaNodeProps.at(i);

    m_HsaNodeProps.clear();
}

const std::vector<int>& HsaNodeInfo::GetNodesWithGPU() const {
    return m_NodesWithGPU;
}

const HsaNodeProperties* HsaNodeInfo::GetNodeProperties(int NodeNum) const {
    return m_HsaNodeProps.at(NodeNum);
}

const HsaNodeProperties* HsaNodeInfo::HsaDefaultGPUNodeProperties() const {
    int NodeNum = HsaDefaultGPUNode();
    if (NodeNum < 0)
        return NULL;
    return GetNodeProperties(NodeNum);
}

const int HsaNodeInfo::HsaDefaultGPUNode() const 
{
    if (m_NodesWithGPU.size() == 0)
        return -1;

	uint32_t g_TestNodeId = 1;
    if (g_TestNodeId >= 0) 
	{
        // Check if this is a valid Id, if so use this else use first available
        for (unsigned int i = 0; i < m_NodesWithGPU.size(); i++) {
            if (g_TestNodeId == m_NodesWithGPU.at(i))
                return g_TestNodeId;
        }
    }

    return m_NodesWithGPU.at(0);
}

void HsaNodeInfo::PrintNodeInfo() const 
{
    const HsaNodeProperties *nodeProperties;

    for (unsigned int i = 0; i < m_HsaNodeProps.size(); i++) 
	{
        nodeProperties = m_HsaNodeProps.at(i);

        std::cout << "***********************************" << std::endl;
        std::cout << "Node " << i << std::endl;
        std::cout << "NumCPUCores=\t" << nodeProperties->NumCPUCores << std::endl;
        std::cout << "NumFComputeCores=\t" << nodeProperties->NumFComputeCores << std::endl;
        std::cout << "NumMemoryBanks=\t" << nodeProperties->NumMemoryBanks << std::endl;
        std::cout << "VendorId=\t" << nodeProperties->VendorId << std::endl;
        std::cout << "DeviceId=\t" << nodeProperties->DeviceId << std::endl;
        std::cout << "***********************************" << std::endl;
    }

    std::cout << "Default GPU NODE " << HsaDefaultGPUNode() << std::endl;
}

const bool HsaNodeInfo::IsGPUNodeLargeBar(int node) const 
{
    const HsaNodeProperties *pNodeProperties;

    pNodeProperties = GetNodeProperties(node);
    if (pNodeProperties) 
	{
        HsaMemoryProperties *memoryProperties = new HsaMemoryProperties[pNodeProperties->NumMemoryBanks];
        hsaKmtGetNodeMemoryProperties(node, pNodeProperties->NumMemoryBanks, memoryProperties);
		for (unsigned bank = 0; bank < pNodeProperties->NumMemoryBanks; bank++)
		{
			if (memoryProperties[bank].HeapType == HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC)
			{
				delete[] memoryProperties;
				return true;
			}
		}
        delete [] memoryProperties;
    }

    return false;
}

const int HsaNodeInfo::FindLargeBarGPUNode() const {
    const std::vector<int> gpuNodes = GetNodesWithGPU();

    for (unsigned i = 0; i < gpuNodes.size(); i++)
        if (IsGPUNodeLargeBar(gpuNodes.at(i)))
            return gpuNodes.at(i);

    return -1;
}
