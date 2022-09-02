//
// Created by matvey on 28.08.22.
//

#pragma once

#include <sstream>
#include <vector>
#include "Common.h"
#include "moves/Move.h"

namespace chess::core::misc
{
	NODISCARD std::string GetSquareName(Square square);

	NODISCARD Square GetSquareFromName(std::string_view name);

	NODISCARD std::string MoveToString(const moves::Move& move);

	size_t Perft(std::string_view fen, int depth,
			std::vector<std::pair<moves::Move, size_t>>* divide = nullptr);
}