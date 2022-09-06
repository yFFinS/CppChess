//
// Created by matvey on 31.08.22.
//

#pragma once

#include "../core/moves/Move.h"
#include "../core/moves/MoveGeneration.h"

#include <limits>
#include <array>

namespace chess::ai::details
{
	static constexpr int MVV_LVA[core::pieces::PIECES - 1][core::pieces::PIECES] =
			{
					{ 15, 14, 13, 12, 11, 10 },
					{ 25, 24, 23, 22, 21, 20 },
					{ 35, 34, 33, 32, 31, 30 },
					{ 45, 44, 43, 42, 41, 40 },
					{ 55, 54, 53, 52, 51, 50 },
			};

	static constexpr int MVV_LVA_OFFSET = 2'000'000;
	static constexpr int KILLER_MOVE_OFFSET = 1'000'000;
	static constexpr int TT_MOVE_VALUE = MVV_LVA_OFFSET + 100;

	struct ScoredMove : core::moves::TypedMove
	{
		constexpr ScoredMove() = default;

		constexpr ScoredMove(const core::moves::TypedMove& move, const int score)
				:TypedMove(move), Score(score)
		{
		}

		int Score{};
	};

	template<int MaxPly, int MaxKillerMovePerPly = 2>
	class MoveSorter
	{
	public:
		void Populate(const core::moves::TypedMove* start,
				const core::moves::TypedMove* end, ScoredMove* output,
				const int ply, const core::moves::Move ttMove)
		{
			for (auto it = start; it != end; it++)
			{
				*output++ = ScoreMove(*it, ply, ttMove);
			}
		}

		void SortTo(ScoredMove* moves, const int count, const int index)
		{
			for (int i = index + 1; i < count; i++)
			{
				if (moves[i].Score > moves[index].Score)
				{
					std::swap(moves[i], moves[index]);
				}
			}
		}

		NODISCARD bool IsKillerMove(const core::moves::TypedMove& move, const int ply)
		{
			for (int i = 0; i < MaxKillerMovePerPly; i++)
			{
				if (m_KillerMoves[ply][i] == move)
				{
					return true;
				}
			}
			return false;
		}

		void StoreKillerMove(const ScoredMove& move, const int ply)
		{
			std::scoped_lock lock(m_Mutex);

			auto* destination = m_KillerMoves[ply];
			for (int i = 0; i < MaxKillerMovePerPly; i++)
			{
				if (!destination[i].IsValid())
				{
					destination[i] = move;
					return;
				}
			}

			ScoredMove* lowestScoreMove = std::min(destination, destination + MaxKillerMovePerPly,
					[](const auto& lhs, const auto& rhs)
					{
						return lhs->Score < rhs->Score;
					});

			if (lowestScoreMove->Score < move.Score)
			{
				*lowestScoreMove = move;
			}
		}
	private:
		NODISCARD ScoredMove ScoreMove(const core::moves::TypedMove& move,
				const int ply, const core::moves::Move ttMove)
		{
			int score = 0;
			if (ttMove == move)
			{
				score = TT_MOVE_VALUE;
			}
			else if (move.IsCapture())
			{
				const int attacking = (int)move.movedPiece().type();
				const int victim = (int)move.capturedPiece().type();
				score = MVV_LVA_OFFSET + MVV_LVA[victim][attacking];
			}
			else if (IsKillerMove(move, ply))
			{
				score = KILLER_MOVE_OFFSET;
			}

			score += (int)move.type();
			return { move, score }; // NOLINT(cppcoreguidelines-slicing)
		}

		std::mutex m_Mutex;
		ScoredMove m_KillerMoves[MaxPly][MaxKillerMovePerPly];
	};
}