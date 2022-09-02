//
// Created by matvey on 28.08.22.
//

#pragma once

#include <ctime>
#include <iostream>
#include <utility>

namespace chess::utils
{
	class ScopedInitProfiler
	{
	public:
		explicit ScopedInitProfiler(std::string file)
				:m_Start(std::clock()), m_File(std::move(file))
		{
		}

		~ScopedInitProfiler()
		{
			const auto passedTime = (double)(std::clock() - m_Start) / CLOCKS_PER_SEC;
			std::cout << m_File << " Initialized. Passed time: " << passedTime << "s\n";
		}
	private:
		std::clock_t m_Start;
		std::string m_File;
	};
}

#define PROFILE_INIT chess::utils::ScopedInitProfiler _prof(__FILE__)