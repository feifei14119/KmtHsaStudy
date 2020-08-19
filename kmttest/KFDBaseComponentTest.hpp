
#ifndef __KFD_BASE_COMPONENT_TEST__H__
#define __KFD_BASE_COMPONENT_TEST__H__

#include <gtest/gtest.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <xf86drm.h>
#include <amdgpu.h>
#include <amdgpu_drm.h>
#include <sys/param.h>
#include "hsakmt.h"
#include "OSWrapper.hpp"
#include "KFDTestUtil.hpp"

//  @class KFDBaseComponentTest
class KFDBaseComponentTest : public testing::Test
{
 public:
    KFDBaseComponentTest(void) { m_MemoryFlags.Value = 0; }
    ~KFDBaseComponentTest(void) {}

    HSAuint64 GetSysMemSize();
    HSAuint64 GetVramSize(int defaultGPUNode);
#define MAX_RENDER_NODES 64
    struct 
	{
        int fd;
        uint32_t major_version;
        uint32_t minor_version;
        amdgpu_device_handle device_handle;
        uint32_t bdf;
    } m_RenderNodes[MAX_RENDER_NODES];

// @brief Finds DRM Render node corresponding to gpuNode
// @return DRM Render Node if successful or -1 on failure
    int FindDRMRenderNode(int gpuNode);
    unsigned int GetFamilyIdFromNodeId(unsigned int nodeId);
    unsigned int GetFamilyIdFromDefaultNode(){ return m_FamilyId; }

    // @brief Executed before the first test that uses KFDBaseComponentTest.
    static  void SetUpTestCase();
    // @brief Executed after the last test from KFDBaseComponentTest.
    static  void TearDownTestCase();

 protected:
    HsaVersionInfo  m_VersionInfo;
    HsaSystemProperties m_SystemProperties;
    unsigned int m_FamilyId;
    unsigned int m_numCpQueues;
    unsigned int m_numSdmaEngines;
    unsigned int m_numSdmaXgmiEngines;
    unsigned int m_numSdmaQueuesPerEngine;
    HsaMemFlags m_MemoryFlags;
    HsaNodeInfo m_NodeInfo;

    // @brief Executed before every test that uses KFDBaseComponentTest class and sets all common settings for the tests.
    virtual void SetUp();
    // @brief Executed after every test that uses KFDBaseComponentTest class.
    virtual void TearDown();
};

extern KFDBaseComponentTest* g_baseTest;
#endif  //  __KFD_BASE_COMPONENT_TEST__H__
