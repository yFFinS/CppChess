//
// Created by matvey on 29.08.22.
//

#pragma once

#include <string>
#include "Common.h"

namespace chess::core
{
	struct Board;
}

namespace chess::core::fen
{
	static constexpr const char* START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

	NODISCARD std::string ExtractFen(const Board& board);
	bool SetFen(Board& board, std::string_view fen);
}