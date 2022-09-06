//
// Created by matvey on 30.08.22.
//

#include <limits>
#include <ctime>
#include <thread>
#include <utility>
#include <mutex>
#include <cfenv>
#include <iostream>
#include <list>
#include <deque>
#include <cstring>
#include "Search.h"
#include "Evaluation.h"
#include "Defs.h"
#include "../core/Fen.h"
#include "../core/Misc.h"

namespace chess::ai::details
{
	using namespace core::moves;

	static constexpr int CHECKMATE_THRESHOLD = 9500;
	static constexpr int CHECKMATE_SCORE = -10000;
	static constexpr int STALEMATE_SCORE = 0;

	namespace
	{
		int ApplyCheckmateCorrection(const int score, const int ply)
		{
			if (score > CHECKMATE_THRESHOLD)
			{
				return score - ply;
			}
			if (score < -CHECKMATE_THRESHOLD)
			{
				return score + ply;
			}
			return score;
		}
	}

	struct SharedData
	{
		explicit SharedData(const std::function<void(int, const Move*, int)>* hook)
				:m_Hook(hook)
		{
		}

		bool IsHighestCompletedDepth(const int depth)
		{
			std::scoped_lock lock(m_Mutex);
			m_MaxReachedDepth = std::max(m_MaxReachedDepth, depth);
			return m_MaxReachedDepth == depth;
		}

		void InvokeHook(const Move* pvBegin, const int pvCount)
		{
			if (!m_Hook)
			{
				return;
			}
			std::scoped_lock lock(m_Mutex);
			m_Hook->operator()(m_MaxReachedDepth, pvBegin, pvCount);
		}

		NODISCARD int GetHighestDepth() const
		{
			return m_MaxReachedDepth;
		}

	private:
		const std::function<void(int, const Move*, int)>* m_Hook;
		int m_MaxReachedDepth = 0;

		std::mutex m_Mutex;
	};

	struct Thread
	{
		explicit Thread(const core::Board& board, hash::TranspositionTable& transpositionTable,
				MoveSorter<MAX_PLY>& moveSorter, SharedData& sharedData)
				:Table{ transpositionTable }, Sorter{ moveSorter },
				 Board{ board.CloneWithoutHistory() }, m_SharedData{ sharedData }
		{
		}

		void Search(const bool isMain, const int depth, const bool verbose)
		{
			Reset();

			m_Ready = false;
			Depth = depth;

			static constexpr int SEARCH_MIN = -100'000, SEARCH_MAX = 100'000;

			int alpha = SEARCH_MIN;
			int beta = SEARCH_MAX;
			int score;

			static constexpr int INIT_ASP_WINDOW = 25;
			int window = INIT_ASP_WINDOW;

			if (depth >= 5)
			{
				beta = m_LastBestScore + window;
				alpha = m_LastBestScore - window;
			}

			// Aspiration window search
			while (!ShouldStop())
			{
				score = AlphaBeta<Node::PV>(depth, alpha, beta);
				if (score <= alpha)
				{
					beta = (alpha + beta) / 2;
					alpha = std::max(alpha - window, SEARCH_MIN);
				}
				else if (score >= beta)
				{
					beta = std::min(alpha + window, SEARCH_MAX);
				}
				else
				{
					break;
				}

				window += window / 3 + 5;
			}

			if (ShouldStop())
			{
				m_Ready = true;
				return;
			}

			m_LastBestScore = score;

			if (!m_SharedData.IsHighestCompletedDepth(depth))
			{
				m_Ready = true;
				return;
			}

			m_SharedData.InvokeHook(PV[0].begin(), PVLength[0]);

			if (verbose)
			{
				std::cout << "Info: depth " << depth << " score " << score << " pv ";

				for (int i = 0; i < PVLength[0]; i++)
				{
					std::cout << core::misc::MoveToString(PV[0][i]) << ' ';
				}
				std::cout << "nodes " << Stats.Nodes <<
						  " seldepth " << Stats.SelDepth <<
						  " tthits " << Stats.TTHits << (isMain ? " mainthread" : "") << '\n';
			}

			m_Ready = true;
		}

		template<Node NodeType>
		int AlphaBeta(int depth, int alpha, int beta)
		{
			if (ShouldStop())
			{
				return 0;
			}

			const bool startedInCheck = (bool)Board.checkers();
			if (startedInCheck)
			{
				depth++;
			}

			Stats.Nodes++;
			Stats.SelDepth = std::max(Stats.SelDepth, Ply);
			PVLength[Ply] = Ply;

			// Treat 2 repetitions as stalemate
			if (Board.halfMoves() >= 50 || Board.GetMaxRepetitions() >= 3)
			{
				return STALEMATE_SCORE;
			}

			if (Ply >= MAX_PLY)
			{
				return eval::EvaluateBoard(Board);
			}

			auto entryType = hash::EntryType::Alpha;
			std::optional<hash::TableEntry> ttEntry;

			const bool isRootNode = Ply == 0;

			if (!isRootNode)
			{
				ttEntry = Table.Probe(Board.hash());
				if (ttEntry.has_value() && !Board.IsLegal(ttEntry->BestMove))
				{
					ttEntry.reset();
				}
				if (ttEntry.has_value() && !ttEntry->FromQuiescence)
				{
					const auto hitType = ttEntry->Apply(depth, alpha, beta);
					if (hitType != hash::EntryType::None)
					{
						Stats.TTHits++;
						if (hitType == hash::EntryType::Exact)
						{
							return ApplyCheckmateCorrection(ttEntry->Value, Ply);
						}
					}
				}
			}

			if (NodeType == Node::PV && Ply > 2 && !ttEntry.has_value())
			{
				depth -= 2;
			}

			if (depth <= 0)
			{
				return Quiescence(depth, alpha, beta);
			}

			ScoredMove scoredMoves[MAX_MOVES];

			int count;
			{
				TypedMove typedMoves[MAX_MOVES];
				const auto end = GenerateMoves<core::moves::Legality::PseudoLegal>(Board, typedMoves);

				count = (int)(end - typedMoves);

				Sorter.Populate(typedMoves, end, scoredMoves, Ply,
						ttEntry.has_value() ? ttEntry->BestMove : Move::Empty());
			}

			bool doPvs = false;
			int bestScore = std::numeric_limits<int>::min();
			Move bestMove;

			int newDepth = depth;

			int legalMoves = 0;

			bool doCutMoves = !startedInCheck && NodeType != Node::PV && Ply > 2;
			const int cutCount = count * 2 / 3;

			// AlphaBeta move loop
			for (int moveIndex = 0; moveIndex < count; moveIndex++)
			{
				Sorter.SortTo(scoredMoves, count, moveIndex);
				const auto scoredMove = scoredMoves[moveIndex];
				const auto move = static_cast<Move>(scoredMove);

				if (doCutMoves
						&& moveIndex >= cutCount
						&& !scoredMove.IsCapture()
						&& scoredMove.movedPiece().type() != core::pieces::Type::Pawn)
				{
					break;
				}

				legalMoves++;
				Ply++;
				Board.MakeMove(move);

				const bool isInCheck = (bool)Board.checkers();
				int lmr = 0;

				if (!doPvs && legalMoves > 1 && depth >= 3 &&
						!startedInCheck && !isInCheck &&
						scoredMove.type() != core::moves::Type::Quiet &&
						scoredMove.movedPiece().type() != core::pieces::Type::Pawn &&
						!Sorter.IsKillerMove(scoredMove, Ply - 1))
				{
					lmr = 1 + (legalMoves > 6 ? Ply / 3 : 0);
					if constexpr (NodeType == Node::PV)
					{
						lmr = lmr * 2 / 3;
					}

					if (ttEntry.has_value() && ttEntry->BestMove.IsCapture())
					{
						lmr++;
					}
				}

				newDepth -= lmr;

				int score;

				if (doPvs)
				{
					score = -AlphaBeta<Node::PV>(newDepth - 1, -alpha - 1, -alpha);
					if (score > alpha && score < beta)
					{
						score = -AlphaBeta<Node::PV>(newDepth - 1, -beta, -alpha);
					}
				}
				else
				{
					score = -AlphaBeta<Node::NonPV>(newDepth - 1, -beta, -alpha);
					if (score > alpha && lmr > 0)
					{
						score = -AlphaBeta<Node::NonPV>(depth - 1, -beta, -alpha);
						newDepth = depth;
					}
				}

				Board.UndoMove();
				Ply--;

				if (score > bestScore)
				{
					bestScore = score;
					bestMove = move;
				}

				if (score >= beta)
				{
					Table.Insert(
							{
									.Hash = Board.hash(),
									.BestMove = bestMove,
									.Type = hash::EntryType::Beta,
									.Depth = newDepth,
									.Value = beta,
							});

					// Quiet promotions are not really quiet
					if (scoredMove.type() == core::moves::Type::Quiet)
					{
						Sorter.StoreKillerMove(scoredMove, Ply);
					}

					return beta;
				}

				if (score > alpha)
				{
					doCutMoves = false;

					doPvs = true;
					PV[Ply][Ply] = move;
					for (int i = Ply + 1; i < PVLength[Ply + 1]; i++)
					{
						PV[Ply][i] = PV[Ply + 1][i];
					}
					PVLength[Ply] = PVLength[Ply + 1];

					alpha = score;
					entryType = hash::EntryType::Exact;
				}
			}

			if (legalMoves == 0)
			{
				return startedInCheck ? CHECKMATE_SCORE + Ply : STALEMATE_SCORE;
			}

			Table.Insert(
					{
							.Hash = Board.hash(),
							.BestMove = bestMove,
							.Type = entryType,
							.Depth = newDepth,
							.Value = alpha,
							.FromQuiescence = false
					});

			return alpha;
		}

		int Quiescence(const int depth, int alpha, int beta)
		{
			if (ShouldStop())
			{
				return 0;
			}

			Stats.Nodes++;
			Stats.SelDepth = std::max(Stats.SelDepth, Ply);
			PVLength[Ply] = Ply;

			// Treat 2 repetitions as stalemate
			if (Board.halfMoves() >= 50 || Board.GetMaxRepetitions() >= 3)
			{
				return STALEMATE_SCORE;
			}

			const auto startAlpha = alpha;

			int standPat = std::numeric_limits<int>::min();
			const bool startedInCheck = (bool)Board.checkers();

			Move ttMove;

			if (!startedInCheck)
			{
				auto ttEntry = Table.Probe(Board.hash());
				if (ttEntry.has_value() && !Board.IsLegal(ttEntry->BestMove))
				{
					ttEntry.reset();
				}
				if (ttEntry.has_value())
				{
					ttMove = ttEntry->BestMove;
					const auto hitType = ttEntry->Apply(depth, alpha, beta);
					if (hitType != hash::EntryType::None)
					{
						Stats.TTHits++;
						if (hitType == hash::EntryType::Exact)
						{
							return ApplyCheckmateCorrection(ttEntry->Value, Ply);
						}
						if (alpha >= beta)
						{
							return alpha;
						}
					}
				}

				standPat = eval::EvaluateBoard(Board);
				alpha = std::max(alpha, standPat);
				if (alpha >= beta)
				{
					return standPat;
				}
			}

			if (Ply >= MAX_PLY)
			{
				return standPat;
			}

			ScoredMove scoredMoves[MAX_MOVES];

			int count;
			{
				TypedMove typedMoves[MAX_MOVES];
				TypedMove* end;
				if (!startedInCheck)
				{
					end = GenerateMoves<core::moves::Legality::PseudoLegal, true>(Board, typedMoves);
				}
				else
				{
					end = GenerateMoves<core::moves::Legality::PseudoLegal, false>(Board, typedMoves);
				}
				count = (int)(end - typedMoves);

				if (count == 0)
				{
					return startedInCheck ? CHECKMATE_SCORE + Ply : STALEMATE_SCORE;
				}

				Sorter.Populate(typedMoves, end, scoredMoves, Ply, ttMove);
			}

			Move bestMove;

			for (int moveIndex = 0; moveIndex < count; moveIndex++)
			{
				Sorter.SortTo(scoredMoves, count, moveIndex);
				const auto scoredMove = scoredMoves[moveIndex];
				const auto move = static_cast<Move>(scoredMove);

				std::vector<Move> newPv;

				Ply++;
				Board.MakeMove(move);

				const auto score = -Quiescence(depth - 1, -beta, -alpha);

				Board.UndoMove();
				Ply--;

				if (score >= beta)
				{
					break;
				}

				if (score > alpha)
				{
					bestMove = move;
					PV[Ply][Ply] = move;
					for (int i = Ply + 1; i < PVLength[Ply + 1]; i++)
					{
						PV[Ply][i] = PV[Ply + 1][i];
					}
					PVLength[Ply] = PVLength[Ply + 1];

					alpha = score;
				}
			}

			if (!startedInCheck)
			{
				auto entryType = hash::EntryType::Exact;
				if (alpha <= startAlpha)
				{
					entryType = hash::EntryType::Alpha;
				}
				else if (alpha >= beta)
				{
					entryType = hash::EntryType::Beta;
				}

				Table.Insert(
						{
								.Hash = Board.hash(),
								.BestMove = bestMove,
								.Type = entryType,
								.Depth = depth,
								.Value = alpha,
								.FromQuiescence = true
						});
			}

			return alpha;
		}

	public:
		hash::TranspositionTable& Table;
		MoveSorter<MAX_PLY>& Sorter;

		core::Board Board;

		std::array<std::array<Move, MAX_PLY + 1>, MAX_PLY + 1> PV;
		std::array<int, MAX_PLY + 1> PVLength;
		int Ply = 0;
		int Depth = 0;

		struct Stats
		{
			size_t Nodes = 0;
			size_t TTHits = 0;
			int SelDepth = 0;
		} Stats;

		void Stop()
		{
			m_StopFlag = true;
		}

		NODISCARD bool IsReady()
		{
			std::scoped_lock lock(m_Mutex);
			return m_Ready;
		}

	protected:
		SharedData& m_SharedData;
		std::mutex m_Mutex;

		void Reset()
		{
			std::scoped_lock lock(m_Mutex);

			PVLength.fill(0);
			Stats = {};
			m_StopFlag = false;
			m_StopCheckCounter = 0;
			Ply = 0;
		}

		virtual bool CheckStop()
		{
			return m_StopFlag;
		}

		bool ShouldStop()
		{
			if (++m_StopCheckCounter % CHECK_STOP_FLAG_EVERY == 0)
			{
				m_StopFlag = CheckStop();
			}

			return m_StopFlag;
		}

	private:
		static constexpr int CHECK_STOP_FLAG_EVERY = 2048;

		int m_StopCheckCounter = 0;
		bool m_StopFlag = false;

		bool m_FirstSearch = true;
		int m_LastBestScore = 0;
		bool m_Ready = true;
	};

	using Clock = std::chrono::steady_clock;

	class MainThread : Thread
	{
	public:
		MainThread(const core::Board& board, hash::TranspositionTable& table, details::MoveSorter<MAX_PLY>& sorter,
				SharedData& data, const std::atomic_bool& stopFlag)
				:Thread(board, table, sorter, data), m_StopFlag(stopFlag), m_MaxTime{ 0 }
		{
		}

		void InitSearch(const Clock::time_point startTime, const SearchParams& searchParams, const bool verbose)
		{
			m_StartTime = startTime;
			m_MaxTime = searchParams.MaxTime;

			int threadCount = std::max(0, searchParams.MaxWorkers - 1);
			threadCount = std::min(threadCount, (int)std::thread::hardware_concurrency());
			std::list<Thread> threads;

			for (int tid = 0; tid < threadCount; tid++)
			{
				threads.emplace_back(Board, Table, Sorter, m_SharedData);
			}

			const int maxDepth = searchParams.MaxDepth > 0 ? std::min(searchParams.MaxDepth, MAX_PLY + 1) : MAX_PLY + 1;

			const auto threadStarter = [](Thread* thread, const int depth, const bool verbose)
			{
				thread->Search(false, depth, verbose);
			};

			int counter = 0;
			int rootDepth = 1;
			while (rootDepth < maxDepth)
			{
				const bool shouldStop = ShouldStop();

				for (auto& thread : threads)
				{
					if (shouldStop)
					{
						thread.Stop();
					}
					else if (rootDepth < maxDepth && thread.IsReady())
					{
						std::thread(threadStarter, &thread, rootDepth, verbose).detach();
						if (counter++ % 2 == 0)
						{
							rootDepth++;
							counter = 0;
						}
					}
				}

				if (shouldStop)
				{
					break;
				}

				rootDepth += counter;

				if (rootDepth < maxDepth)
				{
					Search(true, rootDepth, verbose);
				}

				rootDepth = m_SharedData.GetHighestDepth() + 1;
			}

			// Wait for all threads to finish
			while (true)
			{
				const auto readyCount = std::count_if(threads.begin(), threads.end(),
						[](Thread& thread)
						{ return thread.IsReady(); });
				if (readyCount == threadCount)
				{
					break;
				}

				std::this_thread::yield();
			}
		}

	protected:
		bool CheckStop() override
		{
			if (m_StopFlag)
			{
				return true;
			}
			const auto passedTime = std::chrono::duration<double>(Clock::now() - m_StartTime);
			return passedTime.count() > m_MaxTime;
		}

	private:
		const std::atomic_bool& m_StopFlag;
		double m_MaxTime;
		Clock::time_point m_StartTime;
	};

	void Search::StartSearch(const core::Board& board, const SearchParams searchParams, const bool verbose,
			const SearchHook* depthSearchedHook)
	{
		// Preparations count towards search time
		const auto startTime = Clock::now();

		if (m_BookMoveSelectorPtr)
		{
			// Try select book move and instantly end search with selected move
			Move moves[MAX_MOVES];
			const auto end = GenerateMoves<core::moves::Legality::Legal>(board, moves);
			const auto bookMove = m_BookMoveSelectorPtr->TrySelect(board.hash(),
					moves, end, searchParams.BookTemperature);
			if (bookMove.IsValid())
			{
				if (verbose)
				{
					std::cout << "Info: book  score ???  move " << core::misc::MoveToString(bookMove)
							  << std::endl;
				}

				if (depthSearchedHook)
				{
					depthSearchedHook->operator()(0, &bookMove, 1);
				}
				return;
			}
		}

		hash::TranspositionTable transpositionTable;
		transpositionTable.Reset(searchParams.TableSize, searchParams.TableBucketSize);
		MoveSorter<MAX_PLY> moveSorter;

		m_StopFlag = false;

		SharedData sharedData(depthSearchedHook);
		MainThread mainThread(board, transpositionTable, moveSorter, sharedData, m_StopFlag);
		mainThread.InitSearch(startTime, searchParams, verbose);
	}
}