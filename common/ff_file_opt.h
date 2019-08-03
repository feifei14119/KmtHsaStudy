#pragma once
#include <vector>

namespace feifei
{
	extern std::string get_curr_path();
	extern void ensure_dir(const char * dir);
	extern void exec_cmd(std::string cmd);
	extern E_ReturnState dump2_bin_file(std::string file_name, std::vector<char> *binary);
	extern E_ReturnState dump2_txt_file(std::string file_name, std::string str);
	extern uint read_bin_file(std::string file_name, char * binary);
	extern std::string get_file_path(std::string fileName);
	extern std::string get_file_name(std::string fileName);
}
