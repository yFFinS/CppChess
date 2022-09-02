//
// Created by matvey on 31.08.22.
//

#pragma once

#include <vector>
#include "../core/Common.h"

namespace chess::ai
{
	struct SearchResult
	{
		int Depth{};
		int SelDepth{};
		int Score{};
		size_t Nodes{};
		std::vector<core::moves::Move> PV{};
	};
}
