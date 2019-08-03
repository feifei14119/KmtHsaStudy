#pragma once

#include   <map> 
#include "ff_basic.h"

namespace feifei
{
	/*
	* NEED TO DO!
	*/
	typedef enum ArgIdEnum
	{
		CMD_ARG_HELP,
		CMD_ARG_DEVICE,
		CMD_ARG_EVINFO,
		CMD_ARG_WH,
		CMD_ARG_C,
		CMD_ARG_K,
		CMD_ARG_N,
		CMD_ARG_UV,
		CMD_ARG_BIAS,
		CMD_ARG_RELU
	} E_ArgId;

	typedef struct CmdArgType
	{
		E_ArgId id;
		std::string longName;
		char shortName;
		std::string value;
		int iValue;
		double fValue;
		std::string sValue;
		E_DataType type;
		std::string helpText;
	} T_CmdArg;

	class CmdArgs
	{
	public:
		CmdArgs(int argc, char *argv[]);
		void * GetOneArg(E_ArgId id);
		static CmdArgs * GetCmdArgs();
		std::string ExecutePath() { return executePath; }
		
	private:
		static CmdArgs * pCmdArgs;
		int argsNum = 0;
		std::map<E_ArgId, T_CmdArg*> * argsMap;
		std::string executePath;
		
		virtual void initCmdArgs();
		void helpText();
		void addOneArg(E_ArgId id, E_DataType dType, std::string defaultVal, char sName = '\0', std::string lName = "", std::string tHelp = "");
		void paserCmdArgs(int argc, char *argv[]);
		E_ReturnState setOneArg(char sName, std::string value);
		E_ReturnState setOneArg(std::string lName, std::string value);
		E_ReturnState setOneArgValue(T_CmdArg * arg, std::string value);
		void * getOneArgValue(T_CmdArg * arg);
	};
}
