#pragma once

#include <string>

namespace feifei
{
	/************************************************************************/
	/* 返回类型定义															*/
	/************************************************************************/
	typedef enum ReturnStateEnum
	{
		SUCCESS = 0,
		FAIL = 1
	} E_ReturnState;

	static void checkFuncRet(E_ReturnState retVel, char const *const func, const char *const file, int const line)
	{
		if (retVel != E_ReturnState::SUCCESS)
		{
			fprintf(stderr, "[!ERROR!] at %s:%d \"%s\" \n", file, line, func);
			
			exit(EXIT_FAILURE);
		}
	}

	static void checkErrNum(E_ReturnState errNum, const char *const file, int const line)
	{
		if (errNum != E_ReturnState::SUCCESS)
		{
			fprintf(stderr, "[!ERROR!] at %s:%d\n", file, line);

			exit(EXIT_FAILURE);
		}
	}

#define CheckFunc(val)		do{if(val != E_ReturnState::SUCCESS) return E_ReturnState::FAIL;}while(0)
//#define CheckFunc(val)					checkFuncRet((val), #val, __FILE__, __LINE__)
//#define CheckErr(val)					checkErrNum((val), __FILE__, __LINE__)
	typedef void(*PVoidFunc)();
	typedef E_ReturnState(*PRetFunc)();
	typedef E_ReturnState(*PRetFunc2)(void* param);

	/************************************************************************/
	/* 数据类型定义															*/
	/************************************************************************/
	typedef enum DataTypeEnum
	{
		Float,
		Int,
		Fp32,
		Fp64,
		String,
		UInt8,
		Int8,
		Uint16,
		Int16,
		Int32
	} E_DataType;
}
