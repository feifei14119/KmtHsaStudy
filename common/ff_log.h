#pragma once

#include "ff_basic.h"

#define COMMENT_LEN			(73)
#define OUTPUT(fmt,...)		feifei::print_format_output(fmt,##__VA_ARGS__)
#define	INFO(fmt,...)		feifei::print_format_info(fmt,##__VA_ARGS__)
#define WARN(fmt,...)		feifei::print_format_warn(__FILE__,__LINE__,fmt,##__VA_ARGS__)
#define ERR(fmt,...)		do{feifei::print_format_err(__FILE__,__LINE__,fmt,##__VA_ARGS__);return E_ReturnState::FAIL;}while(0)
#define FATAL(fmt,...)		feifei::print_format_fatal(__FILE__,__LINE__,fmt,##__VA_ARGS__)
#define	PRINT_SEPARATOR1()	do{for(int i=0;i<COMMENT_LEN;i++) printf("*"); printf("\n");}while(0)
#define	PRINT_SEPARATOR2()	do{for(int i=0;i<COMMENT_LEN;i++) printf("="); printf("\n");}while(0)
#define	PRINT_SEPARATOR3()	do{for(int i=0;i<COMMENT_LEN;i++) printf("-"); printf("\n");}while(0)
#define	PRINT_SEPARATOR(c)	do{for(int i=0;i<COMMENT_LEN;i++) printf("%c",c); printf("\n");}while(0)

namespace feifei
{
	/************************************************************************/
	/* 屏幕输出																*/
	/************************************************************************/
	extern void print_format_output(const char * format, ...);
	extern void print_format_output(std::string msg, ...);
	extern void print_format_info(const char * format, ...);
	extern void print_format_info(std::string msg,...);
	extern void print_format_warn(const char *file, int line, const char * format, ...);
	extern void print_format_warn(const char *file, int line, std::string msg, ...);
	extern E_ReturnState print_format_err(const char *file, int line, const char * format, ...);
	extern E_ReturnState print_format_err(const char *file, int line, std::string msg, ...);
	extern void print_format_fatal(const char *file, int line, const char * format, ...);
	extern void print_format_fatal(const char *file, int line, std::string msg, ...);

	/************************************************************************/
	/* 文件输出																*/
	/************************************************************************/
	class LogFile
	{
	public:
		LogFile(std::string file_name);
		~LogFile();
		void Log(const char * format, ...);
		void Log(std::string msg, ...);

	protected:
		std::string file_name;
		std::ofstream * log_file; 
		char * log_char_buffer;
	};
}
