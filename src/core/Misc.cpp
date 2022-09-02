//
// Created by matvey on 29.08.22.
//

#include "Misc.h"
#include "Board.h"
#include "Fen.h"
#include "moves/MoveGeneration.h"

#include <iostream>

namespace chess::core::misc
{
	size_t Perft(Board& board, const int depth, std::vector<std::pair<moves::Move, size_t>>* divide)
	{
		moves::Move moves[moves::MAX_MOVES];
		const auto end = moves::GenerateLegalMoves(board, moves);
		const auto nodes = end - moves;

		if (depth == 1)
		{
			if (divide)
			{
				for (auto it = moves; it != end; it++)
				{
					divide->push_back(std::make_pair(*it, 1));
				}
			}
			return nodes;
		}

		size_t result = 0;
		for (auto it = moves; it != end; it++)
		{
			board.MakeMove(*it);
			const auto innerNodes = Perft(board, depth - 1, nullptr);
			if (divide)
			{
				divide->push_back(std::make_pair(*it, innerNodes));
			}
			result += innerNodes;
			board.UndoMove();
		}

		return result;
	}

	size_t Perft(const std::string_view fen, const int depth,
			std::vector<std::pair<moves::Move, size_t>>* divide)
	{
		assert(depth >= 0);

		if (depth == 0)
		{
			return 1;
		}

		Board board;
		if (!fen::SetFen(board, fen))
		{
			assert(false);
		}

		return Perft(board, depth, divide);
	}

	std::string GetSquareName(const Square square)
	{
		std::stringstream ss;
		ss << (char)('a' + square.file());
		ss << (BOARD_SIZE - square.rank());
		return ss.str();
	}

	Square GetSquareFromName(const std::string_view name)
	{
		if (name.size() != 2)
		{
			return Square::Invalid();
		}

		return Square(name[0] - 'a', name[1] - '1');
	}

	std::string MoveToString(const moves::Move& move)
	{
		auto str = GetSquareName(move.start()) + GetSquareName(move.end());
		switch (move.type())
		{
		case moves::Type::BishopPQ:
		case moves::Type::BishopPC:
			str += 'b';
			break;
		case moves::Type::KnightPQ:
		case moves::Type::KnightPC:
			str += 'n';
			break;
		case moves::Type::RookPQ:
		case moves::Type::RookPC:
			str += 'r';
			break;
		case moves::Type::QueenPQ:
		case moves::Type::QueenPC:
			str += 'q';
			break;
		default:
			break;
		}

		return str;
	}
}