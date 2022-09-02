//
// Created by matvey on 01.09.22.
//

#pragma once
#include <string_view>
#include <unordered_map>
#include <random>
#include "../core/moves/Move.h"

namespace chess::database
{
	struct Entry
	{
		core::Square Start;
		core::Square End;
		chess::core::pieces::Type Promotion{};
		uint16_t Weight{};
	};

	class BookMoveSelector
	{
	public:
		explicit BookMoveSelector(std::string_view binPath);

		core::moves::Move TrySelect(uint64_t key,
				const core::moves::Move* begin, const core::moves::Move* end,
				double temperature = 1.0);
	private:
		std::unordered_map<uint64_t, std::vector<Entry>> m_Entries;
		std::mt19937_64 m_Gen;
		std::uniform_real_distribution<double> m_Distribution;
	};
}