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
#include <memory>
#include "core/Misc.h"

#include "core/Fen.h"
#include "core/Board.h"
#include "ai/Facade.h"

#include "database/BookMoveSelector.h"
#include "ai/Search.h"

class ChessState
{
public:
	void LoadBook(const std::string_view path)
	{
		std::scoped_lock lock(m_Mutex);
		m_BookMoveSelectorPtr = std::make_unique<chess::database::BookMoveSelector>(path);
	}

	chess::core::pieces::Color SetFen(const std::string_view fen)
	{
		std::scoped_lock lock(m_Mutex);
		chess::core::fen::SetFen(m_Board, fen);
		return m_Board.colorToPlay();
	}

	void WaitForUnlock()
	{
		std::scoped_lock lock(m_Mutex);
	}

	NODISCARD chess::core::Board& board()
	{
		return m_Board;
	}

	void StopSearch()
	{
		if (m_SearchPtr)
		{
			m_SearchPtr->StopGrace();
		}
	}

	chess::core::moves::Move Search(const chess::ai::SearchParams params, const bool verbose)
	{
		chess::core::moves::Move bestMove;

		const chess::ai::details::SearchHook hook = [&bestMove](int, const chess::core::moves::Move* pv, int)
		{
			bestMove = *pv;
		};

		std::scoped_lock lock(m_Mutex);
		m_SearchPtr = std::make_unique<chess::ai::details::Search>(m_BookMoveSelectorPtr.get());
		m_SearchPtr->StartSearch(m_Board.CloneWithoutHistory(), params,
				verbose, &hook);
		return bestMove;
	}

	void MakeMove(const chess::core::moves::Move move)
	{
		std::scoped_lock lock(m_Mutex);
		m_Board.MakeMove(move);
	}

	void UndoMove()
	{
		std::scoped_lock lock(m_Mutex);
		m_Board.UndoMove();
	}

private:
	std::mutex m_Mutex;
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

int SetFen(ChessState* state, const char* const fen)
{
	const auto color = state->SetFen(fen);

	std::cout << "FEN=" << fen << '\n';
	std::ios::fmtflags f(std::cout.flags());
	std::cout << "Key=" << std::hex << state->board().hash() << '\n';
	std::cout.flags(f);

	return (int)color;
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
	auto start_t = std::chrono::high_resolution_clock::now();
	auto nodes = chess::core::misc::Perft(fen, depth);
	auto passed_t = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start_t);
	std::cout << fen << "  - Nodes: " << nodes << ". MNodes p/s: " <<
			  std::fixed << std::setprecision(3) << (double)nodes / passed_t.count() / 1000'000.0 << '\n';
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
void MakeMove(ChessState* state, int rawMove)
{
	assert(state);

	chess::core::moves::Move move(rawMove);
	state->MakeMove(move);
}

void UndoMove(ChessState* state)
{
	assert(state);
	state->UndoMove();
}

int GetBoardState(ChessState* state)
{
	static constexpr int PLAYING = 1;
	static constexpr int CHECKMATE = 2;
	static constexpr int NO_MOVES_STALEMATE = 3;
	static constexpr int HALF_MOVES_STALEMATE = 4;
	static constexpr int REPETITION_STALEMATE = 5;

	assert(state);
	if (state->board().halfMoves() >= 50)
	{
		return HALF_MOVES_STALEMATE;
	}
	if (state->board().GetMaxRepetitions() == 3)
	{
		return REPETITION_STALEMATE;
	}

	using namespace chess::core::moves;

	Move moves[MAX_MOVES];
	const auto end = GenerateMoves<Legality::Legal>(state->board(), moves);
	const bool hasMoves = end != moves;
	if (!hasMoves)
	{
		return state->board().checkers() ? CHECKMATE : NO_MOVES_STALEMATE;
	}

	return PLAYING;
}

int Search(ChessState* state, const chess::ai::SearchParams params, const int verbose)
{
	std::cout << "Starting search:\n"
			  << "State address=" << &state << '\n'
			  << "Max time=" << params.MaxTime << '\n'
			  << "Max workers=" << params.MaxWorkers << '\n'
			  << "Max depth=" << params.MaxDepth << '\n'
			  << "Table size=" << params.TableSize << '\n'
			  << "Table bucket size=" << params.TableBucketSize << '\n'
			  << "Book temperature=" << params.BookTemperature << '\n'
			  << "Verbose=" << verbose << std::endl;
	const auto bestMove = state->Search(params, verbose);
	return bestMove.value();
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
	chess::ai::SearchParams params{ .MaxTime = 3, .MaxWorkers = 4, .TableSize = 2'000'000, .TableBucketSize = 4 };
	Search(state, params, true);
}

int main()
{
	auto* state = CreateState();
	//state->LoadBook("../database/book.bin");
	using namespace std::chrono_literals;
	SetFen(state, chess::core::fen::START_FEN);
//	MakeMove(state,
//			chess::core::moves::Move(chess::core::Square(52), chess::core::Square(36), chess::core::moves::Type::DoublePawn)
//					.value());

	chess::ai::SearchParams params{ .MaxTime = 5, .MaxWorkers = 6, .TableSize = 256000,
			.TableBucketSize = 4, .BookTemperature = 0.2 };
	Search(state, params, 1);

	FreeState(state);
	return 0;
}
