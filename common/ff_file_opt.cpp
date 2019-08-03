
#include <unistd.h>
#include <sys/stat.h>
#include <string>
#include <fstream>
#include <vector>

#include "ff_basic.h"
#include "ff_log.h"
#include "ff_file_opt.h"

namespace feifei
{
	void ensure_dir(const char * dir)
	{
		if (access(dir, F_OK) == -1)
		{
			::mkdir(dir, 0777);
		}
	}

	void exec_cmd(std::string cmd)
	{
		FILE * pfp = popen(cmd.c_str(), "r");
		auto status = pclose(pfp);
		WEXITSTATUS(status);
	}

	E_ReturnState dump2_bin_file(std::string file_name, std::vector<char> *binary)
	{
		std::ofstream fout(file_name.c_str(), std::ios::out | std::ios::binary);
		if (!fout.is_open())
		{
			ERR("can't open save file: " + file_name);
		}
		fout.write(binary->data(), binary->size());
		fout.close();
	}

	E_ReturnState dump2_txt_file(std::string file_name, std::string str)
	{
		std::ofstream fout(file_name.c_str(), std::ios::out);
		if (!fout.is_open())
		{
			ERR("can't open save file: " + file_name);
		}
		fout.write(str.c_str(), str.length());
		fout.close();
	}

	uint read_bin_file(std::string file_name, char * binary)
	{
		FILE * fp = fopen(file_name.c_str(), "rb");
		if (fp == NULL)
		{
			ERR("can't open bin file: " + file_name);
			return 0;
		}

		size_t binSize;
		fseek(fp, 0, SEEK_END);
		binSize = ftell(fp);
		rewind(fp);

		unsigned char * binProgram = new unsigned char[binSize];
		fread(binProgram, 1, binSize, fp);
		fclose(fp);

		return binSize;
	}

	std::string get_curr_path()
	{
		char * buf = getcwd(nullptr, 0);
		std::string path(buf);
		free(buf);
		return path;
	}

	std::string get_file_path(std::string fileName)
	{
		size_t p, p1, p2;
		p1 = fileName.rfind('\\');
		p2 = fileName.rfind('/');
		if (p1 == std::string::npos)
		{
			p = p2;
		}
		else
		{
			p = (p1 > p2) ? p1 : p2;
		}
		return fileName.substr(0, p);
	}

	std::string get_file_name(std::string fileName)
	{
		size_t p1, p2;
		p1 = fileName.rfind('\\');
		p2 = fileName.rfind('/');
		if (p1 == std::string::npos)
		{
			p1 = p2;
		}
		else if (p2 > p1)
		{
			p1 = p2;
		}
		p2 = fileName.rfind(".");
		return fileName.substr(p1 + 1, p2 - p1 - 1);
	}
}
