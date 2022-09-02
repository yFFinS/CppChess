//
// Created by matvey on 29.08.22.
//

#pragma once

#include <array>
#include "../Common.h"

namespace chess::core::eval
{
	class IncrementalPieceSquareEvaluator
	{
	public:
		NODISCARD constexpr int GetScore(const pieces::Color color, const bool isEndGame) const
		{
			return isEndGame ? m_EndGameScores[(int)color] : m_EarlyGameScores[(int)color];
		}

		void FeedRemoveAt(Square square, pieces::Piece piece);
		void FeedSetAt(Square square, pieces::Piece piece);

		void Reset()
		{
			m_EarlyGameScores.fill(0);
			m_EndGameScores.fill(0);
		}

	private:
		void Feed(Square square, pieces::Piece piece, int sign);

		std::array<int, pieces::COLORS> m_EarlyGameScores{};
		std::array<int, pieces::COLORS> m_EndGameScores{};
	};
}