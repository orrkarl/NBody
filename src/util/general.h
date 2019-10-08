#pragma once

#include <algorithm>
#include <string>
#include <vector>

std::vector<char> readFile(const std::string &path);

template <typename T>
T clamp(const T &min, const T &value, const T &max)
{
	return std::max(min, std::min(value, max));
}
