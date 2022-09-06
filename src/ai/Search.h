//
// Created by matvey on 30.08.22.
//

#pragma once

#include <vector>
#include <atomic>

#include "../core/Board.h"
#include "hash/TranspositionTable.h"
#include "MoveSorter.h"
#include "Defs.h"
#include "../database/BookMoveSelector.h"
#include "Facade.h"
#include "../core/moves/Move.h"

namespace chess::ai::details
{
	static constexpr int MAX_PLY = 125;

	enum struct Node
	{
		PV, NonPV
	};

	using SearchHook = std::function<void(int, const core::moves::Move*, int)>;

	class Search
	{
	public:
		explicit Search(database::BookMoveSelector* bookMoveSelector = nullptr)
		{
			m_BookMoveSelectorPtr = bookMoveSelector;
		}

		void StartSearch(const core::Board& board, SearchParams searchParams, bool verbose = false,
				const SearchHook* depthSearchedHook = nullptr);

		void StopGrace()
		{
			m_StopFlag = true;
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
		database::BookMoveSelector* m_BookMoveSelectorPtr;
		std::atomic_bool m_StopFlag = false;
	};
}