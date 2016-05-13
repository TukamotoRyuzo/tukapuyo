#include"log.h"

std::ofstream& operator<<(std::ofstream& ofs, History& h)
{
	while(!h.history_.empty())
	{
		ofs << h.pop() << " ";	
	}
	ofs << std::endl;
	return ofs;
}

std::ifstream& operator>>(std::ifstream& ifs, History& h)
{
	std::string str;
	std::getline(ifs, str);
	std::stringstream ss(str);
	int i;
	while(!ss.eof())
	{
		ss >> i;
		h.push(i);
	}
	for (int x = 0; x < 100; x++)
	{
		h.push(0);
	}
	return ifs;
}
