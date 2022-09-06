//
// Created by matvey on 31.08.22.
//

#pragma once

#include <string>

namespace chess::ai
{
	struct SearchParams
	{
		double MaxTime{};
		int MaxWorkers{};
		int TableSize{};
		int TableBucketSize{};
		int MaxDepth{};
		double BookTemperature = 1.0;
	};

}