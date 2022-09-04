//
// Created by matvey on 30.08.22.
//

#include <random>
#include "Random.h"

namespace chess::core
{
	uint64_t RandomUInt64()
	{
		static RandomGenerator s_Gen;
		return s_Gen.RandomUInt64();
	}
}