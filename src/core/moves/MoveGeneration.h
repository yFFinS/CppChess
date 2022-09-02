//
// Created by matvey on 28.08.22.
//

#pragma once

#include "Move.h"

namespace chess::core
{
	struct Board;
}

namespace chess::core::moves
{
	static constexpr int MAX_MOVES = 256;

	template<bool CapturesOnly = false>
	TypedMove* GenerateLegalMoves(const Board& board, TypedMove* output);

	template<bool CapturesOnly = false>
	Move* GenerateLegalMoves(const Board& board, Move* output);

	NODISCARD TypedMove GetTypedMove(const Board& board, Move move);
}