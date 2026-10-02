#include <itu_engine.hpp>

const char* itu_lib_fileutils_get_file_name(const char* path)
{
	const char* ret = path;

	while(*path)
	{
		if(is_path_separator(*path))
			ret = path + 1;
		++path;
	}

	return ret;
}