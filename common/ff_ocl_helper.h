#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <CL/cl.h>

#include "ff_cmd_args.h"

namespace feifei
{
#define		clCheckFunc(val)			cl_checkFuncRet((val), #val, __FILE__, __LINE__)
#define		clCheckErr(val)				cl_checkErrNum((val), __FILE__, __LINE__)
#define		clErrInfo(val)				cl_printErrInfo((val), __FILE__, __LINE__)

	typedef struct PlatformInfoType
	{
		std::string version;
		std::string name;
		std::string vendor;
	}T_PlatformInfo;

	typedef struct DeviceInfoType
	{
		cl_device_type type;
		std::string name;			// ISA
		cl_uint vendorId;
		std::string vendor;
		std::string drvVersion;
		std::string clVersion;
		cl_bool errCorrectSupport;
		cl_bool endianLittle;
		cl_bool devAvailable;
		cl_bool complierAvailable;
		std::string profiler;
		std::string extensions;
		size_t profTimerResolution;
		cl_device_exec_capabilities execCapabilities;
		cl_command_queue_properties queueProper;
		cl_uint freqMHz;
		cl_uint cuNum;
		cl_uint maxWorkItemDim;
		std::vector<size_t> maxWorkItemSize;
		size_t maxWorkGroupSize;

		cl_uint addrBit;
		cl_ulong maxMemAllocSize;
		cl_uint memBaseAddrAlign;
		cl_uint minDataAlignSize;
		cl_ulong glbMemSize;
		cl_ulong glbCacheSize;
		cl_uint glbCacheLineSize;
		cl_device_mem_cache_type glbMemCacheType;
		cl_ulong constBuffSize;
		cl_uint maxConstArgs;
		cl_ulong ldsMemSize;
		cl_device_local_mem_type ldsMemType;
		cl_bool hostUnifiedMem;

		cl_uint prefVctWidthChar;
		cl_uint prefVctWidthShort;
		cl_uint prefVctWidthInt;
		cl_uint prefVctWidthLong;
		cl_uint prefVctWidthFloat;
		cl_uint prefVctWidthDouble;
		cl_uint prefVctWidthHalf;
		cl_uint natvVctWidthChar;
		cl_uint natvVctWidthShort;
		cl_uint natvVctWidthInt;
		cl_uint natvVctWidthLong;
		cl_uint natvVctWidthFloat;
		cl_uint natvVctWidthDouble;
		cl_uint natvVctWidthHalf;
		cl_device_fp_config singleFpCfg;

		cl_bool supportImage;
		cl_uint maxReadImageArgs;
		cl_uint maxWriteImageArgs;
		size_t maxImage2DWidth;
		size_t maxImage2DHeight;
		size_t maxImage3DWidth;
		size_t maxImage3DHeight;
		size_t maxImage3DDepth;
		cl_uint maxSamples;
		size_t maxParamSize;
	}T_DeviceInfo;

	typedef enum ProgramTypeEnum
	{
		PRO_OCL_FILE,
		PRO_OCL_STRING,
		PRO_GAS_FILE,
		PRO_GAS_STRING,
		PRO_BIN_FILE,
		PRO_BIN_STRING
	}E_ProgramType;

	typedef struct dim3
	{
		size_t x, y, z;
		dim3(size_t vx = 1, size_t vy = 1, size_t vz = 1) : x(vx), y(vy), z(vz) {}
		size_t * arr() { return new size_t[3]{ x,y,z }; }

		dim3 operator/(dim3 b)
		{
			int x2 = this->x / b.x;
			int y2 = this->y / b.y;
			int z2 = this->z / b.z;
			dim3 c(x2, y2, z2);
			return c;
		}
	}dim3;

	typedef struct KernelWriteHelpInfoType
	{
		dim3 local_size;
		dim3 group_size;
		dim3 global_size;
		std::string kernel_name;
	}T_KernelWriteHelpInfo;

	static const char *clGetErrorInfo(cl_int error)
	{
		switch (error)
		{
		case CL_SUCCESS:
			return "CL_SUCCESS";
		case CL_DEVICE_NOT_FOUND:
			return "CL_DEVICE_NOT_FOUND";
		case CL_DEVICE_NOT_AVAILABLE:
			return "CL_DEVICE_NOT_AVAILABLE";
		case CL_COMPILER_NOT_AVAILABLE:
			return "CL_COMPILER_NOT_AVAILABLE";
		case CL_MEM_OBJECT_ALLOCATION_FAILURE:
			return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
		case CL_OUT_OF_RESOURCES:
			return "CL_OUT_OF_RESOURCES";
		case CL_OUT_OF_HOST_MEMORY:
			return "CL_OUT_OF_HOST_MEMORY";
		case CL_PROFILING_INFO_NOT_AVAILABLE:
			return "CL_PROFILING_INFO_NOT_AVAILABLE";
		case CL_MEM_COPY_OVERLAP:
			return "CL_MEM_COPY_OVERLAP";
		case CL_IMAGE_FORMAT_MISMATCH:
			return "CL_IMAGE_FORMAT_MISMATCH";
		case CL_IMAGE_FORMAT_NOT_SUPPORTED:
			return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
		case CL_BUILD_PROGRAM_FAILURE:
			return "CL_BUILD_PROGRAM_FAILURE";
		case CL_MAP_FAILURE:
			return "CL_MAP_FAILURE";
		case CL_MISALIGNED_SUB_BUFFER_OFFSET:
			return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
		case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
			return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
		case CL_COMPILE_PROGRAM_FAILURE:
			return "CL_COMPILE_PROGRAM_FAILURE";
		case CL_LINKER_NOT_AVAILABLE:
			return "CL_LINKER_NOT_AVAILABLE";
		case CL_LINK_PROGRAM_FAILURE:
			return "CL_LINK_PROGRAM_FAILURE";
		case CL_DEVICE_PARTITION_FAILED:
			return "CL_DEVICE_PARTITION_FAILED";
		case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:
			return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

		case CL_INVALID_VALUE:
			return "CL_INVALID_VALUE";
		case CL_INVALID_DEVICE_TYPE:
			return "CL_INVALID_DEVICE_TYPE";
		case CL_INVALID_PLATFORM:
			return "CL_INVALID_PLATFORM";
		case CL_INVALID_DEVICE:
			return "CL_INVALID_DEVICE";
		case CL_INVALID_CONTEXT:
			return "CL_INVALID_CONTEXT";
		case CL_INVALID_QUEUE_PROPERTIES:
			return "CL_INVALID_QUEUE_PROPERTIES";
		case CL_INVALID_COMMAND_QUEUE:
			return "CL_INVALID_COMMAND_QUEUE";
		case CL_INVALID_HOST_PTR:
			return "CL_INVALID_HOST_PTR";
		case CL_INVALID_MEM_OBJECT:
			return "CL_INVALID_MEM_OBJECT";
		case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
			return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
		case CL_INVALID_IMAGE_SIZE:
			return "CL_INVALID_IMAGE_SIZE";
		case CL_INVALID_SAMPLER:
			return "CL_INVALID_SAMPLER";
		case CL_INVALID_BINARY:
			return "CL_INVALID_BINARY";
		case CL_INVALID_BUILD_OPTIONS:
			return "CL_INVALID_BUILD_OPTIONS";
		case CL_INVALID_PROGRAM:
			return "CL_INVALID_PROGRAM";
		case CL_INVALID_PROGRAM_EXECUTABLE:
			return "CL_INVALID_PROGRAM_EXECUTABLE";
		case CL_INVALID_KERNEL_NAME:
			return "CL_INVALID_KERNEL_NAME";
		case CL_INVALID_KERNEL_DEFINITION:
			return "CL_INVALID_KERNEL_DEFINITION";
		case CL_INVALID_KERNEL:
			return "CL_INVALID_KERNEL";
		case CL_INVALID_ARG_INDEX:
			return "CL_INVALID_ARG_INDEX";
		case CL_INVALID_ARG_VALUE:
			return "CL_INVALID_ARG_VALUE";
		case CL_INVALID_ARG_SIZE:
			return "CL_INVALID_ARG_SIZE";
		case CL_INVALID_KERNEL_ARGS:
			return "CL_INVALID_KERNEL_ARGS";
		case CL_INVALID_WORK_DIMENSION:
			return "CL_INVALID_WORK_DIMENSION";
		case CL_INVALID_WORK_GROUP_SIZE:
			return "CL_INVALID_WORK_GROUP_SIZE";
		case CL_INVALID_WORK_ITEM_SIZE:
			return "CL_INVALID_WORK_ITEM_SIZE";
		case CL_INVALID_GLOBAL_OFFSET:
			return "CL_INVALID_GLOBAL_OFFSET";
		case CL_INVALID_EVENT_WAIT_LIST:
			return "CL_INVALID_EVENT_WAIT_LIST";
		case CL_INVALID_EVENT:
			return "CL_INVALID_EVENT";
		case CL_INVALID_OPERATION:
			return "CL_INVALID_OPERATION";
		case CL_INVALID_GL_OBJECT:
			return "CL_INVALID_GL_OBJECT";
		case CL_INVALID_BUFFER_SIZE:
			return "CL_INVALID_BUFFER_SIZE";
		case CL_INVALID_MIP_LEVEL:
			return "CL_INVALID_MIP_LEVEL";
		case CL_INVALID_GLOBAL_WORK_SIZE:
			return "CL_INVALID_GLOBAL_WORK_SIZE";
		case CL_INVALID_PROPERTY:
			return "CL_INVALID_PROPERTY";
		case CL_INVALID_IMAGE_DESCRIPTOR:
			return "CL_INVALID_IMAGE_DESCRIPTOR";
		case CL_INVALID_COMPILER_OPTIONS:
			return "CL_INVALID_COMPILER_OPTIONS";
		case CL_INVALID_LINKER_OPTIONS:
			return "CL_INVALID_LINKER_OPTIONS";
		case CL_INVALID_DEVICE_PARTITION_COUNT:
			return "CL_INVALID_DEVICE_PARTITION_COUNT";
		case CL_INVALID_PIPE_SIZE:
			return "CL_INVALID_PIPE_SIZE";
		case CL_INVALID_DEVICE_QUEUE:
			return "CL_INVALID_DEVICE_QUEUE";
		}

		return "<unknown>";
	}

	static void cl_checkFuncRet(cl_int errNum, char const *const func, const char *const file, int const line)
	{
		if (errNum)
		{
			fprintf(stderr, "OpenCL error at %s:%d code=%d(%s) \"%s\" \n", file, line, static_cast<unsigned int>(errNum), clGetErrorInfo(errNum), func);

			exit(EXIT_FAILURE);
		}
	}

	static void cl_checkErrNum(cl_int errNum, const char *const file, int const line)
	{
		if (errNum)
		{
			fprintf(stderr, "OpenCL error at %s:%d code=%d(%s) \n", file, line, static_cast<unsigned int>(errNum), clGetErrorInfo(errNum));

			exit(EXIT_FAILURE);
		}
	}

	static void cl_printErrInfo(cl_int errNum, const char *const file, int const line)
	{
		if (errNum)
		{
			fprintf(stderr, "OpenCL error at %s:%d code=%d(%s) \n", file, line, static_cast<unsigned int>(errNum), clGetErrorInfo(errNum));
		}
	}

	static std::string GetKernelTempPath()
	{
		CmdArgs * cmd = CmdArgs::GetCmdArgs();
		return cmd->ExecutePath() + "/kernel";
	}
}
