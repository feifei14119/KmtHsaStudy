#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <hsa.h>

#include "ff_cmd_args.h"

namespace feifei
{
#define		hsaCheckFunc(val)			hsa_checkFuncRet((val), #val, __FILE__, __LINE__)
#define		hsaCheckErr(val)			hsa_checkErrNum((val), __FILE__, __LINE__)
#define		hsaErrInfo(val)				hsa_printErrInfo((val), __FILE__, __LINE__)

	typedef struct HsaSystemInfoType
	{
		uint16_t major, minor;
	}T_HsaSystemInfo;

	typedef struct HsaAgentInfoType 
	{
		std::string name;
		std::string vendor_name;
		hsa_agent_feature_t agent_feature;
		hsa_profile_t agent_profile;
		hsa_default_float_rounding_mode_t float_rounding_mode;
		uint32_t max_queue;
		uint32_t queue_min_size;
		uint32_t queue_max_size;
		hsa_queue_type_t queue_type;
		uint32_t node;
		hsa_device_type_t device_type;
		uint32_t cache_size[4];
		uint32_t chip_id;
		uint32_t cacheline_size;
		uint32_t max_clock_freq;
		uint32_t compute_unit;
		uint32_t wavefront_size;
		uint32_t workgroup_max_size;
		uint32_t grid_max_size;
		uint32_t fbarrier_max_size;
		uint32_t waves_per_cu;
		hsa_isa_t agent_isa;
		hsa_dim3_t grid_max_dim;
		uint16_t workgroup_max_dim[3];
		uint16_t bdf_id;
		bool fast_f16;
	} T_HsaAgentInfo;

	static const char *hsaGetErrorInfo(hsa_status_t status)
	{
		switch (status)
		{
		case HSA_STATUS_SUCCESS: return "HSA_STATUS_SUCCESS";
		case HSA_STATUS_INFO_BREAK: return "HSA_STATUS_INFO_BREAK";
		case HSA_STATUS_ERROR: return "HSA_STATUS_ERROR";
		case HSA_STATUS_ERROR_INVALID_ARGUMENT: return "HSA_STATUS_ERROR_INVALID_ARGUMENT";
		case HSA_STATUS_ERROR_INVALID_QUEUE_CREATION: return "HSA_STATUS_ERROR_INVALID_QUEUE_CREATION";
		case HSA_STATUS_ERROR_INVALID_ALLOCATION: return "HSA_STATUS_ERROR_INVALID_ALLOCATION";
		case HSA_STATUS_ERROR_INVALID_AGENT: return "HSA_STATUS_ERROR_INVALID_AGENT";
		case HSA_STATUS_ERROR_INVALID_REGION: return "HSA_STATUS_ERROR_INVALID_REGION";
		case HSA_STATUS_ERROR_INVALID_SIGNAL: return "HSA_STATUS_ERROR_INVALID_SIGNAL";
		case HSA_STATUS_ERROR_INVALID_QUEUE: return "HSA_STATUS_ERROR_INVALID_QUEUE";
		case HSA_STATUS_ERROR_OUT_OF_RESOURCES: return "HSA_STATUS_ERROR_OUT_OF_RESOURCES";
		case HSA_STATUS_ERROR_INVALID_PACKET_FORMAT: return "HSA_STATUS_ERROR_INVALID_PACKET_FORMAT";
		case HSA_STATUS_ERROR_RESOURCE_FREE: return "HSA_STATUS_ERROR_RESOURCE_FREE";
		case HSA_STATUS_ERROR_NOT_INITIALIZED: return "HSA_STATUS_ERROR_NOT_INITIALIZED";
		case HSA_STATUS_ERROR_REFCOUNT_OVERFLOW: return "HSA_STATUS_ERROR_REFCOUNT_OVERFLOW";
		case HSA_STATUS_ERROR_INCOMPATIBLE_ARGUMENTS: return "HSA_STATUS_ERROR_INCOMPATIBLE_ARGUMENTS";
		case HSA_STATUS_ERROR_INVALID_INDEX: return "HSA_STATUS_ERROR_INVALID_INDEX";
		case HSA_STATUS_ERROR_INVALID_ISA: return "HSA_STATUS_ERROR_INVALID_ISA";
		case HSA_STATUS_ERROR_INVALID_ISA_NAME: return "HSA_STATUS_ERROR_INVALID_ISA_NAME";
		case HSA_STATUS_ERROR_INVALID_CODE_OBJECT: return "HSA_STATUS_ERROR_INVALID_CODE_OBJECT";
		case HSA_STATUS_ERROR_INVALID_EXECUTABLE: return "HSA_STATUS_ERROR_INVALID_EXECUTABLE";
		case HSA_STATUS_ERROR_FROZEN_EXECUTABLE: return "HSA_STATUS_ERROR_FROZEN_EXECUTABLE";
		case HSA_STATUS_ERROR_INVALID_SYMBOL_NAME: return "HSA_STATUS_ERROR_INVALID_SYMBOL_NAME";
		case HSA_STATUS_ERROR_VARIABLE_ALREADY_DEFINED: return "HSA_STATUS_ERROR_VARIABLE_ALREADY_DEFINED";
		case HSA_STATUS_ERROR_VARIABLE_UNDEFINED: return "HSA_STATUS_ERROR_VARIABLE_UNDEFINED";
		case HSA_STATUS_ERROR_EXCEPTION: return "HSA_STATUS_ERROR_EXCEPTION";
		case HSA_STATUS_ERROR_INVALID_CODE_SYMBOL: return "HSA_STATUS_ERROR_INVALID_CODE_SYMBOL";
		case HSA_STATUS_ERROR_INVALID_EXECUTABLE_SYMBOL: return "HSA_STATUS_ERROR_INVALID_EXECUTABLE_SYMBOL";
		case HSA_STATUS_ERROR_INVALID_FILE: return "HSA_STATUS_ERROR_INVALID_FILE";
		case HSA_STATUS_ERROR_INVALID_CODE_OBJECT_READER: return "HSA_STATUS_ERROR_INVALID_CODE_OBJECT_READER";
		case HSA_STATUS_ERROR_INVALID_CACHE: return "HSA_STATUS_ERROR_INVALID_CACHE";
		case HSA_STATUS_ERROR_INVALID_WAVEFRONT: return "HSA_STATUS_ERROR_INVALID_WAVEFRONT";
		case HSA_STATUS_ERROR_INVALID_SIGNAL_GROUP: return "HSA_STATUS_ERROR_INVALID_SIGNAL_GROUP";
		case HSA_STATUS_ERROR_INVALID_RUNTIME_STATE: return "HSA_STATUS_ERROR_INVALID_RUNTIME_STATE";
		case HSA_STATUS_ERROR_FATAL: return "HSA_STATUS_ERROR_FATAL";
		}
		return "<unknown>";
	}

	static void hsa_checkFuncRet(hsa_status_t status, char const *const func, const char *const file, int const line)
	{
		if (status)
		{
			fprintf(stderr, "HSA error at %s:%d code=%d(%s) \"%s\" \n", file, line, static_cast<unsigned int>(status), hsaGetErrorInfo(status), func);

			exit(EXIT_FAILURE);
		}
	}

	static void hsa_checkErrNum(hsa_status_t status, const char *const file, int const line)
	{
		if (status)
		{
			fprintf(stderr, "HSA error at %s:%d code=%d(%s) \n", file, line, static_cast<unsigned int>(status), hsaGetErrorInfo(status));

			exit(EXIT_FAILURE);
		}
	}

	static void hsa_printErrInfo(hsa_status_t status, const char *const file, int const line)
	{
		if (status)
		{
			fprintf(stderr, "HSA error at %s:%d code=%d(%s) \n", file, line, static_cast<unsigned int>(status), hsaGetErrorInfo(status));
		}
	}
}
