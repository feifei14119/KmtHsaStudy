
#include <string.h>
#include <fstream>
#include <iostream>
#include <stdarg.h>
#include <time.h>

#include "ff_log.h"
#include "ff_file_opt.h"

namespace feifei 
{
#define		CHAR_BUFF_SIZE		(1024)

	static char log_char_buffer[CHAR_BUFF_SIZE];

	static bool en_log_time = true;
	static bool en_log_file = true;
	static bool en_log_short_file = true;

	/************************************************************************/
	/* 屏幕输出																*/
	/************************************************************************/
	void print_format_output(const char * format, ...)
	{
		memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
		va_list args;
		va_start(args, format);
		vsprintf(log_char_buffer, format, args);
		printf("%s", log_char_buffer);
		va_end(args);
		printf("\n");
	}
	void print_format_output(std::string msg, ...)
	{
		printf(msg.c_str());
		printf("\n");
	}
	void print_format_info(const char * format, ...)
	{
		printf("[INFO]");
		if (en_log_time == true)
		{
			time_t t = time(0);
			memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
			strftime(log_char_buffer, CHAR_BUFF_SIZE, "[%H:%M:%S]", localtime(&t));
			printf("%s", log_char_buffer);
		}
		memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
		va_list args;
		va_start(args, format);
		vsprintf(log_char_buffer, format, args);
		printf("%s", log_char_buffer);
		va_end(args);
		printf("\n");
	}
	void print_format_info(std::string msg, ...)
	{
		printf("[INFO]");
		if (en_log_time == true)
		{
			time_t t = time(0);
			memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
			strftime(log_char_buffer, CHAR_BUFF_SIZE, "[%H:%M:%S]", localtime(&t));
			printf("%s", log_char_buffer);
		}
		printf(msg.c_str());
		printf("\n");
	}
	void print_format_warn(const char *file, int line, const char * format, ...)
	{
		printf("[WARNING]");
		if (en_log_time == true)
		{
			time_t t = time(0);
			memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
			strftime(log_char_buffer, CHAR_BUFF_SIZE, "[%H:%M:%S]", localtime(&t));
			printf("%s", log_char_buffer);
		}
		memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
		va_list args;
		va_start(args, format);
		vsprintf(log_char_buffer, format, args);
		printf("%s", log_char_buffer);
		va_end(args);
		if (en_log_file)
		{
			char * p = (char * )file;
			if (en_log_short_file == true)
			{
				p = strrchr(p, '/');
				p++;
			}
			printf(" @%s:%d", p, line);
		}
		printf("\n");
	}
	void print_format_warn(const char *file, int line, std::string msg, ...)
	{
		printf("[WARNING]");
		if (en_log_time == true)
		{
			time_t t = time(0);
			memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
			strftime(log_char_buffer, CHAR_BUFF_SIZE, "[%H:%M:%S]", localtime(&t));
			printf("%s", log_char_buffer);
		}
		printf(msg.c_str());
		if (en_log_file)
		{
			char * p = (char *)file;
			if (en_log_short_file == true)
			{
				p = strrchr(p, '/');
				p++;
			}
			printf(" @%s:%d", p, line);
		}
		printf("\n");
	}
	E_ReturnState print_format_err(const char *file, int line, const char * format, ...)
	{
		printf("[ERROR]");
		if (en_log_time == true)
		{
			time_t t = time(0);
			memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
			strftime(log_char_buffer, CHAR_BUFF_SIZE, "[%H:%M:%S]", localtime(&t));
			printf("%s", log_char_buffer);
		}
		memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
		va_list args;
		va_start(args, format);
		vsprintf(log_char_buffer, format, args);
		printf("%s", log_char_buffer);
		va_end(args);
		if (en_log_file)
		{
			char * p = (char *)file;
			if (en_log_short_file == true)
			{
				p = strrchr(p, '/');
				p++;
			}
			printf(" @%s:%d", p, line);
		}
		printf("\n");
		return E_ReturnState::FAIL;
	}
	E_ReturnState print_format_err(const char *file, int line, std::string msg, ...)
	{
		printf("[ERROR]");
		if (en_log_time == true)
		{
			time_t t = time(0);
			memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
			strftime(log_char_buffer, CHAR_BUFF_SIZE, "[%H:%M:%S]", localtime(&t));
			printf("%s", log_char_buffer);
		}
		printf(msg.c_str());
		if (en_log_file)
		{
			char * p = (char *)file;
			if (en_log_short_file == true)
			{
				p = strrchr(p, '/');
				p++;
			}
			printf(" @%s:%d", p, line);
		}
		printf("\n");
		return E_ReturnState::FAIL;
	}
	void print_format_fatal(const char *file, int line, const char * format, ...)
	{
		printf("[FATAL]");
		if (en_log_time == true)
		{
			time_t t = time(0);
			memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
			strftime(log_char_buffer, CHAR_BUFF_SIZE, "[%H:%M:%S]", localtime(&t));
			printf("%s", log_char_buffer);
		}
		memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
		va_list args;
		va_start(args, format);
		vsprintf(log_char_buffer, format, args);
		printf("%s", log_char_buffer);
		va_end(args);
		if (en_log_file)
		{
			char * p = (char *)file;
			if (en_log_short_file == true)
			{
				p = strrchr(p, '/');
				p++;
			}
			printf(" @%s:%d", p, line);
		}
		printf("\n"); 
		exit(EXIT_FAILURE);
	}
	void print_format_fatal(const char *file, int line, std::string msg, ...)
	{
		printf("[FATAL]");
		if (en_log_time == true)
		{
			time_t t = time(0);
			memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
			strftime(log_char_buffer, CHAR_BUFF_SIZE, "[%H:%M:%S]", localtime(&t));
			printf("%s", log_char_buffer);
		}
		printf(msg.c_str());
		if (en_log_file)
		{
			char * p = (char *)file;
			if (en_log_short_file == true)
			{
				p = strrchr(p, '/');
				p++;
			}
			printf(" @%s:%d", p, line);
		}
		printf("\n");
		exit(EXIT_FAILURE);
	}
	
	/************************************************************************/
	/* 文件输出																*/
	/************************************************************************/
	LogFile::LogFile(std::string file_name)
	{
		log_char_buffer = (char *)malloc(CHAR_BUFF_SIZE);

		ensure_dir(".//log//");

		time_t t = time(0);
		memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
		strftime(log_char_buffer, CHAR_BUFF_SIZE, "_%F_%H-%M-%S.log", localtime(&t));
		file_name = ".//log//" + std::string(file_name) + log_char_buffer;

		log_file = new std::ofstream(file_name, std::ios::out | std::ios::trunc);

		if (!log_file->is_open())
		{
			WARN("can't init log file" + file_name);
			log_file = nullptr;
		}
	}
	LogFile::~LogFile()
	{
		free(log_char_buffer);
		if ((log_file != nullptr) && log_file->is_open())
		{
			log_file->close();
		}
	}
	
	void LogFile::Log(const char * format, ...)
	{
		int cn;

		if (log_file == nullptr)
		{
			WARN("can't open log file");
			return;
		}
		if (!log_file->is_open())
		{
			WARN("can't open log file");
			return;
		}
		
		time_t t = time(0);
		memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
		cn = strftime(log_char_buffer, CHAR_BUFF_SIZE, "[%F_%T]", localtime(&t));

		va_list args;
		va_start(args, format);
		cn += vsprintf(log_char_buffer+cn, format, args);
		va_end(args);
		cn += sprintf(log_char_buffer + cn, "\n");

		log_file->write(log_char_buffer, cn);
		log_file->flush();
	}
	void LogFile::Log(std::string msg, ...)
	{
		int cn;

		if (log_file == nullptr)
		{
			WARN("can't open log file");
			return;
		}
		if (!log_file->is_open())
		{
			WARN("can't open log file");
			return;
		}

		time_t t = time(0);
		memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
		cn = strftime(log_char_buffer, CHAR_BUFF_SIZE, "[%F_%T]", localtime(&t));
		
		log_file->write(msg.c_str(), cn);
		log_file->flush();
	}
}
