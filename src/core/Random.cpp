//
// Created by matvey on 30.08.22.
//

#include <random>
#include "Random.h"

namespace chess::core
{
	uint64_t RandomUint64()
	{
		static std::random_device s_Device;
		static std::mt19937_64 s_Generator(s_Device());
		static std::uniform_int_distribution<uint64_t> s_Distribution;
		return s_Distribution(s_Generator);
	}
}