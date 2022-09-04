//
// Created by matvey on 30.08.22.
//

#pragma once

#include <random>

namespace chess::core
{
	class RandomGenerator
	{
	public:
		explicit RandomGenerator(const uint64_t seed = std::mt19937_64::default_seed) : m_Generator(seed)
		{
		}

		uint64_t RandomUInt64()
		{
			return m_Distribution(m_Generator);
		}

	private:
		std::mt19937_64 m_Generator;
		std::uniform_int_distribution<uint64_t> m_Distribution;
	};

	uint64_t RandomUInt64();
}