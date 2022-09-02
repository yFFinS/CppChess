//
// Created by matvey on 31.08.22.
//

#pragma once

#include <string>

#include "Search.h"

namespace chess::ai
{
	struct SearchParams
	{
		double MaxTime{};
		int MaxWorkers{};
		int TableSize{};
		int TableBucketSize{};
		int MaxDepth = details::MAX_PLY;
		double BookTemperature = 1.0;
	};

}