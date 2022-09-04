//
// Created by matvey on 28.08.22.
//

#pragma once

#include <chrono>
#include <iostream>
#include <utility>

namespace chess::utils
{
	class ScopedTimer
	{
	public:
		explicit ScopedTimer(std::string file = "", bool printInitTime = false)
				:m_Start(std::chrono::high_resolution_clock::now()),
				 m_File(std::move(file)),
				 m_PrintInitTime(printInitTime)
		{
		}

		NODISCARD double GetPassedTime() const
		{
			return std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - m_Start).count();
		}

		~ScopedTimer()
		{
			if (m_PrintInitTime)
			{
				std::cout << m_File << " Initialized. Passed time: " << GetPassedTime() << "s\n";
			}
		}
	private:
		std::chrono::high_resolution_clock::time_point m_Start;
		std::string m_File;
		bool m_PrintInitTime;
	};
}

#define PROFILE_INIT chess::utils::ScopedTimer _prof(__FILE__, true)