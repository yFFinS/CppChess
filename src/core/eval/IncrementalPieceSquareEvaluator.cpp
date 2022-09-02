//
// Created by matvey on 29.08.22.
//
#include "IncrementalPieceSquareEvaluator.h"

namespace chess::core::eval
{
	namespace
	{
		std::array<int, BOARD_SQUARES> s_PawnValues
				{
						0, 0, 0, 0, 0, 0, 0, 0,
						50, 50, 50, 50, 50, 50, 50, 50,
						10, 10, 20, 30, 30, 20, 10, 10,
						5, 5, 10, 27, 27, 10, 5, 5,
						0, 0, 0, 25, 25, 0, 0, 0,
						5, -5, -10, 0, 0, -10, -5, 5,
						5, 10, 10, -25, -25, 10, 10, 5,
						0, 0, 0, 0, 0, 0, 0, 0
				};

		std::array<int, BOARD_SQUARES> s_KnightValues
				{
						-50, -40, -30, -30, -30, -30, -40, -50,
						-40, -20, 0, 0, 0, 0, -20, -40,
						-30, 0, 10, 15, 15, 10, 0, -30,
						-30, 5, 15, 20, 20, 15, 5, -30,
						-30, 0, 15, 20, 20, 15, 0, -30,
						-30, 5, 10, 15, 15, 10, 5, -30,
						-40, -20, 0, 5, 5, 0, -20, -40,
						-50, -40, -20, -30, -30, -20, -40, -50,
				};

		std::array<int, BOARD_SQUARES> s_BishopValues
				{
						-20, -10, -10, -10, -10, -10, -10, -20,
						-10, 0, 0, 0, 0, 0, 0, -10,
						-10, 0, 5, 10, 10, 5, 0, -10,
						-10, 5, 5, 10, 10, 5, 5, -10,
						-10, 0, 10, 10, 10, 10, 0, -10,
						-10, 10, 10, 10, 10, 10, 10, -10,
						-10, 5, 0, 0, 0, 0, 5, -10,
						-20, -10, -40, -10, -10, -40, -10, -20,
				};

		std::array<int, BOARD_SQUARES> s_RookValues
				{
						0, 0, 0, 0, 0, 0, 0, 0,
						5, 10, 10, 10, 10, 10, 10, 5,
						-5, 0, 0, 0, 0, 0, 0, -5,
						-5, 0, 0, 0, 0, 0, 0, -5,
						-5, 0, 0, 0, 0, 0, 0, -5,
						-5, 0, 0, 0, 0, 0, 0, -5,
						-5, 0, 0, 0, 0, 0, 0, -5,
						0, 0, 0, 5, 5, 0, 0, 0
				};

		std::array<int, BOARD_SQUARES> s_QueenValues
				{
						-20, -10, -10, -5, -5, -10, -10, -20,
						-10, 0, 5, 0, 0, 0, 0, -10,
						-10, 0, 0, 0, 0, 0, 0, -10,
						-10, 0, 5, 5, 5, 5, 0, -10,
						-10, 5, 5, 5, 5, 5, 0, -10,
						0, 0, 5, 5, 5, 5, 0, -5,
						-5, 0, 5, 5, 5, 5, 0, -5,
						-20, -10, -10, -5, -5, -10, -10, -20
				};

		std::array<int, BOARD_SQUARES> s_KingValuesEarlyGame
				{
						-30, -40, -40, -50, -50, -40, -40, -30,
						-30, -40, -40, -50, -50, -40, -40, -30,
						-30, -40, -40, -50, -50, -40, -40, -30,
						-30, -40, -40, -50, -50, -40, -40, -30,
						-20, -30, -30, -40, -40, -30, -30, -20,
						-10, -20, -20, -20, -20, -20, -20, -10,
						20, 20, 0, 0, 0, 0, 20, 20,
						20, 30, 10, 0, 0, 10, 30, 20
				};

		std::array<int, BOARD_SQUARES> s_KingValuesEndGame
				{
						-50, -40, -30, -20, -20, -30, -40, -50,
						-30, -20, -10, 0, 0, -10, -20, -30,
						-30, -10, 20, 30, 30, 20, -10, -30,
						-30, -10, 30, 40, 40, 30, -10, -30,
						-30, -10, 30, 40, 40, 30, -10, -30,
						-30, -10, 20, 30, 30, 20, -10, -30,
						-30, -30, 0, 0, 0, 0, -30, -30,
						-50, -30, -30, -30, -30, -30, -30, -50
				};

		std::array<std::array<int, BOARD_SQUARES>, 5> s_Values
				{
						s_PawnValues,
						s_KnightValues,
						s_BishopValues,
						s_RookValues,
						s_QueenValues
				};
	}

	void IncrementalPieceSquareEvaluator::FeedRemoveAt(const Square square, const pieces::Piece piece)
	{
		Feed(square, piece, -1);
	}

	void IncrementalPieceSquareEvaluator::FeedSetAt(const chess::core::Square square, const pieces::Piece piece)
	{
		Feed(square, piece, 1);
	}

	void IncrementalPieceSquareEvaluator::Feed(const chess::core::Square square,
			const pieces::Piece piece, const int sign)
	{
		const auto index = piece.color() == pieces::Color::White ? square.value() : BOARD_SQUARES - 1 - square.value();
		int earlyGameChange, endGameChange;

		if (piece.type() == pieces::Type::King)
		{
			earlyGameChange = s_KingValuesEarlyGame[index];
			endGameChange = s_KingValuesEndGame[index];
		}
		else
		{
			earlyGameChange = s_Values[(int)piece.type()][index];
			endGameChange = earlyGameChange;
		}

		m_EarlyGameScores[(int)piece.color()] += earlyGameChange * sign;
		m_EndGameScores[(int)piece.color()] += endGameChange * sign;
	}
}

