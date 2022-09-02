//
// Created by matvey on 28.08.22.
//

#include <iostream>
#include <utility>
#include <vector>
#include <ctime>
#include <array>
#include <iomanip>
#include <thread>
#include "core/Misc.h"

#include "core/Fen.h"
#include "core/Board.h"
#include "ai/Facade.h"

#include "database/BookMoveSelector.h"

class ChessState
{
public:
	struct SearchResult
	{
		uint16_t Move;
		int Score;
		int Depth;
		int SelDepth;
		uint64_t Nodes;
	};

	void LoadBook(const std::string_view path)
	{
		std::scoped_lock lock(m_AccessMutex, m_SearchMutex);
		m_BookMoveSelectorPtr = std::make_unique<chess::database::BookMoveSelector>(path);
	}

	void SetFen(const std::string_view fen)
	{
		std::scoped_lock lock(m_AccessMutex, m_SearchMutex);
		chess::core::fen::SetFen(m_Board, fen);
	}

	void WaitForUnlock()
	{
		std::scoped_lock lock(m_AccessMutex, m_SearchMutex);
	}

	NODISCARD const chess::core::Board& board() const
	{
		return m_Board;
	}

	void StopSearch()
	{
		std::scoped_lock lock(m_AccessMutex);
		if (m_SearchPtr)
		{
			m_SearchPtr->StopGrace();
		}
	}

	SearchResult Search(const chess::ai::SearchParams params)
	{
		chess::ai::SearchResult bestResult{};

		const std::function<void(chess::ai::SearchResult)> hook = [&bestResult](chess::ai::SearchResult result)
		{
			bestResult = std::move(result);
		};

		{
			std::scoped_lock lock(m_AccessMutex);
			m_SearchPtr = std::make_unique<chess::ai::details::Search>(params.TableSize, params.TableBucketSize);
		}

		std::scoped_lock lock(m_SearchMutex);
		m_SearchPtr->StartSearch(m_Board.CloneWithoutHistory(), m_BookMoveSelectorPtr.get(),
				params.MaxTime, params.MaxDepth, params.MaxWorkers, params.BookTemperature, &hook);

		const auto result = bestResult;
		const auto move = !result.PV.empty() ? result.PV[0] : chess::core::moves::Move::Empty();
		return { .Move = move.value(), .Score = result.Score, .Depth = result.Depth, .SelDepth = result
				.SelDepth, .Nodes = result.Nodes };
	}
private:
	std::mutex m_SearchMutex, m_AccessMutex;
	chess::core::Board m_Board;
	std::unique_ptr<chess::database::BookMoveSelector> m_BookMoveSelectorPtr;
	std::unique_ptr<chess::ai::details::Search> m_SearchPtr = nullptr;
};

extern "C"
{
ChessState* CreateState()
{
	return new ChessState;
}

void LoadOpeningBook(ChessState* state, const char* const path)
{
	assert(state);
	state->LoadBook(path);
}

void FreeState(ChessState* state)
{
	assert(state);
	delete state;
}

void WaitForSearchEnd(ChessState* state)
{
	assert(state);
	state->WaitForUnlock();
}

void SetFen(ChessState* state, const char* const fen)
{
	state->SetFen(fen);

	std::cout << "FEN=" << fen << '\n';
	std::ios::fmtflags f(std::cout.flags());
	std::cout << "Key=" << std::hex << state->board().hash() << '\n';
	std::cout.flags(f);
}

void StopSearch(ChessState* state)
{
	assert(state);
	state->StopSearch();
}
}

void CheckPerft(std::string_view fen, int depth, size_t expected)
{
	try
	{
		auto actual = chess::core::misc::Perft(fen, depth);
		if (actual == expected)
		{
			std::cout << fen << " " << "Depth: " << depth << "  - OK!\n";
		}
		else
		{
			std::cout << fen << " " << "Depth: " << depth << "  - ERROR! Expected: "
					  << expected << " Got: " << actual << '\n';
		}
	}
	catch (const std::exception& e)
	{
		std::cout << fen << " " << "Depth: " << depth << "  - ERROR!" << e.what() << '\n';
	}
}

void TimePerft(std::string_view fen, int depth)
{
	auto start_t = std::clock();
	auto nodes = chess::core::misc::Perft(fen, depth);
	auto passed_t = (double)(std::clock() - start_t) / CLOCKS_PER_SEC;
	std::cout << fen << "  - Nodes: " << nodes << ". MNodes p/s: " <<
			  std::fixed << std::setprecision(3) << (double)nodes / passed_t / 1000'000.0 << '\n';
}

void DividePerft(std::string_view fen, int depth)
{
	std::vector<std::pair<chess::core::moves::Move, size_t>> divide;
	const auto nodes = chess::core::misc::Perft(fen, depth, &divide);
	for (const auto& pair : divide)
	{
		std::cout << chess::core::misc::MoveToString(pair.first) << ": " << pair.second << '\n';
	}

	std::cout << '\n' << "Nodes: " << nodes << '\n';
}

extern "C"
{

ChessState::SearchResult Search(ChessState* state, const chess::ai::SearchParams params)
{
	std::cout << "Starting search:\n"
	          << "State address=" << &state << '\n'
			  << "Max time=" << params.MaxTime << '\n'
			  << "Max workers=" << params.MaxWorkers << '\n'
			  << "Max depth=" << params.MaxDepth << '\n'
			  << "Table size=" << params.TableSize << '\n'
			  << "Table bucket size=" << params.TableBucketSize << '\n'
			  << "Book temperature=" << params.BookTemperature << '\n';
	return state->Search(params);
}

int HealthCheck()
{
	const std::string startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
#ifdef NDEBUG
	TimePerft(startFen, 6);
#endif
	CheckPerft(startFen, 1, 20);
	CheckPerft(startFen, 2, 400);
	CheckPerft(startFen, 3, 8'902);
	CheckPerft(startFen, 4, 197'281);
#ifndef NDEBUG
	CheckPerft(startFen, 5, 4'865'609);
	TimePerft(startFen, 5);
#endif

	const std::string fen2 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
	CheckPerft(fen2, 1, 48);
	CheckPerft(fen2, 2, 2039);
	CheckPerft(fen2, 3, 97862);
	CheckPerft(fen2, 4, 4085603);

	const std::string fen3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
	CheckPerft(fen3, 1, 14);
	CheckPerft(fen3, 2, 191);
	CheckPerft(fen3, 3, 2812);
	CheckPerft(fen3, 4, 43238);

#ifdef NDEBUG
	CheckPerft(fen3, 5, 674624);
#endif

	const std::string fen4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
	CheckPerft(fen4, 1, 6);
	CheckPerft(fen4, 2, 264);
	CheckPerft(fen4, 3, 9467);
	CheckPerft(fen4, 4, 422333);

#ifdef NDEBUG
	CheckPerft(fen4, 5, 15833292);
#endif

	return 0;
}
}

void Thread(ChessState* state)
{
	chess::ai::SearchParams params{ .MaxTime= 3, .MaxWorkers = 6, .TableSize = 1000000, .TableBucketSize = 4 };
	const auto res = Search(state, params);
	std::cout << res.Nodes << ' ' << res.SelDepth << '\n';
}

int main()
{
	auto* state = CreateState();
	state->LoadBook("../database/book.bin");
	using namespace std::chrono_literals;
	SetFen(state, chess::core::fen::START_FEN);

	for (int i = 0; i < 10; i++)
	{
		Thread(state);
		chess::core::moves::Move moves[256];
		const auto end = chess::core::moves::GenerateLegalMoves(state->board(), moves);
		auto& board = const_cast<chess::core::Board&>(state->board());
		board.MakeMove(*(end - 1));
//		std::thread thread(lambda);
//		thread.detach();
//		std::this_thread::sleep_for(2s);
//		StopSearch(state);
//		WaitForSearchEnd(state);
	}

	FreeState(state);
	return 0;
}
