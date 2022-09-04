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

namespace chess::ai::details
{
	static constexpr int MAX_PLY = 125;

	enum struct Node
	{
		PV, NonPV
	};

	struct SearchRefs
	{
		core::Board Board;
		int Ply{};
		int SelDepth{};
		size_t Nodes{};
		size_t TTableHits{};
		int Alpha = std::numeric_limits<int>::min() + 1;
		int Beta = std::numeric_limits<int>::max() - 1;
	};

	class Search
	{
	public:
		Search(int tableSize, int tableBucketSize);

		void StartSearch(const core::Board& board, database::BookMoveSelector* bookMoveSelector,
				double maxTime, int maxDepth = MAX_PLY, int maxWorkers = 2, double bookTemperature = 1,
				const std::function<void(SearchResult)>* depthSearchedHook = nullptr);

		void StopGrace()
		{
			m_StopRequested = true;
		}

	private:
		template<Node NodeType>
		int AlphaBeta(int depth, int alpha, int beta, std::vector<core::moves::Move>& pv, SearchRefs& refs);
		int Quiescence(int depth, int alpha, int beta, std::vector<core::moves::Move>& pv, SearchRefs& refs);

		NODISCARD bool ShouldStop() const;

		hash::TranspositionTable m_TTable;
		MoveSorter<MAX_PLY> m_MoveSorter;
		double m_MaxTime = 0;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;

		std::atomic<bool> m_StopRequested = false;
	};
}