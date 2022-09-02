//
// Created by matvey on 31.08.22.
//

#pragma once

namespace chess::core
{
	struct Board;
}

namespace chess::ai::eval
{
	int EvaluateBoard(const core::Board& board);
}
