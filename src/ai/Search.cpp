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
#include "Search.h"
#include "Evaluation.h"
#include "Defs.h"
#include "../core/Fen.h"

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

	template<Node NodeType>
	int Search::AlphaBeta(int depth, int alpha, int beta, std::vector<Move>& pv, SearchRefs& refs)
	{
		if (ShouldStop())
		{
			return 0;
		}

		const bool startedInCheck = (bool)refs.Board.checkers();
		if (startedInCheck)
		{
			depth++;
		}

		if (depth <= 0)
		{
			return Quiescence(alpha, beta, pv, refs);
		}

		refs.Nodes++;
		refs.SelDepth = std::max(refs.SelDepth, refs.Ply);

		if (refs.Ply >= MAX_PLY)
		{
			return eval::EvaluateBoard(refs.Board);
		}

		auto entryType = hash::EntryType::Alpha;
		std::optional<hash::TableEntry> ttEntry;

		if (refs.Ply > 0)
		{
			ttEntry = m_TTable.Probe(refs.Board.hash());
			if (ttEntry.has_value())
			{
				const auto hitType = ttEntry->Apply(depth, alpha, beta);
				if (hitType != hash::EntryType::None)
				{
					refs.TTableHits++;
					if (hitType == hash::EntryType::Exact)
					{
						return ApplyCheckmateCorrection(ttEntry->Value, refs.Ply);
					}
				}
			}
		}

		bool doPvs = false;
		int bestScore = std::numeric_limits<int>::min();
		Move bestMove;

		int newDepth = depth;
		int count;

		ScoredMove scoredMoves[MAX_MOVES];

		{
			TypedMove typedMoves[MAX_MOVES];
			const auto end = GenerateMoves<core::moves::Legality::PseudoLegal>(refs.Board, typedMoves);

			count = (int)(end - typedMoves);

			m_MoveSorter.Populate(typedMoves, end, scoredMoves, refs.Ply,
					ttEntry.has_value() ? ttEntry->BestMove : Move::Empty());
		}

		int legalMoves = 0;
		for (int moveIndex = 0; moveIndex < count; moveIndex++)
		{
			m_MoveSorter.SortTo(scoredMoves, count, moveIndex);
			const auto move = scoredMoves[moveIndex];

			if (!refs.Board.IsLegal(move)) // NOLINT(cppcoreguidelines-slicing)
			{
				continue;
			}

			legalMoves++;
			refs.Ply++;
			refs.Board.MakeMove(move); // NOLINT(cppcoreguidelines-slicing)

			const bool isInCheck = (bool)refs.Board.checkers();
			int lmr = 0;

			if (!doPvs && legalMoves > 1 && depth >= 3 &&
					!startedInCheck && !isInCheck &&
					move.type() != core::moves::Type::Quiet &&
					move.movedPiece().type() != core::pieces::Type::Pawn &&
					!m_MoveSorter.IsKillerMove(move, refs.Ply - 1))
			{
				lmr = 1 + (legalMoves > 6 ? refs.Ply / 3 : 0);
				if constexpr (NodeType == Node::PV)
				{
					lmr = lmr * 2 / 3;
				}
			}

			newDepth -= lmr;

			int score;
			std::vector<Move> newPv;

			if (doPvs)
			{
				score = -AlphaBeta<Node::PV>(newDepth - 1, -alpha - 1, -alpha, newPv, refs);
				if (score > alpha && score < beta)
				{
					score = -AlphaBeta<Node::PV>(newDepth - 1, -beta, -alpha, newPv, refs);
				}
			}
			else
			{
				score = -AlphaBeta<Node::NonPV>(newDepth - 1, -beta, -alpha, newPv, refs);
				if (score > alpha && lmr > 0)
				{
					score = -AlphaBeta<Node::NonPV>(depth - 1, -beta, -alpha, newPv, refs);
				}
			}

			refs.Board.UndoMove();
			refs.Ply--;

			if (score >= bestScore)
			{
				bestScore = score;
				bestMove = move; // NOLINT(cppcoreguidelines-slicing)
			}

			if (score >= beta)
			{
				m_TTable.Insert(hash::TableEntry{
						.Hash = refs.Board.hash(),
						.BestMove = bestMove,
						.Type = hash::EntryType::Beta,
						.Depth = newDepth,
						.Value = beta,
				});

				// Quiet promotions are not really quiet
				if (move.type() == core::moves::Type::Quiet)
				{
					m_MoveSorter.StoreKillerMove(move, refs.Ply);
				}

				return beta;
			}

			if (score > alpha)
			{
				doPvs = true;
				pv.clear();
				pv.push_back(move);
				pv.reserve(newPv.size() + 1);
				pv.insert(pv.end(), newPv.begin(), newPv.end());

				alpha = score;
				entryType = hash::EntryType::Exact;
			}
		}

		if (legalMoves == 0)
		{
			return startedInCheck ? CHECKMATE_SCORE + refs.Ply : STALEMATE_SCORE;
		}

		m_TTable.Insert(hash::TableEntry{
				.Hash = refs.Board.hash(),
				.BestMove = bestMove,
				.Type = entryType,
				.Depth = newDepth,
				.Value = alpha
		});

		return alpha;
	}

	int Search::Quiescence(int alpha, int beta, std::vector<core::moves::Move>& pv, SearchRefs& refs)
	{
		if (ShouldStop())
		{
			return 0;
		}

		refs.Nodes++;
		refs.SelDepth = std::max(refs.SelDepth, refs.Ply);

		auto score = eval::EvaluateBoard(refs.Board);

		if (refs.Ply >= MAX_PLY)
		{
			return score;
		}

		if (score > beta)
		{
			return beta;
		}

		if (score > alpha)
		{
			alpha = score;
		}

		ScoredMove scoredMoves[MAX_MOVES];

		int count;
		{
			TypedMove typedMoves[MAX_MOVES];
			const auto end = GenerateMoves<core::moves::Legality::PseudoLegal, true>(refs.Board, typedMoves);
			count = (int)(end - typedMoves);

			m_MoveSorter.Populate(typedMoves, end, scoredMoves, refs.Ply, Move::Empty());
		}

		for (int i = 0; i < count; i++)
		{
			m_MoveSorter.SortTo(scoredMoves, count, i);
			const auto move = scoredMoves[i];

			if (!refs.Board.IsLegal(move))
			{
				continue;
			}

			std::vector<Move> newPv;

			refs.Ply++;
			refs.Board.MakeMove(move); // NOLINT(cppcoreguidelines-slicing)

			score = -Quiescence(-beta, -alpha, newPv, refs);

			refs.Board.UndoMove();
			refs.Ply--;

			if (score >= beta)
			{
				return beta;
			}

			if (score > alpha)
			{
				pv.clear();
				pv.push_back(move);
				pv.reserve(newPv.size() + 1);
				pv.insert(pv.end(), newPv.begin(), newPv.end());

				alpha = score;
			}
		}

		return alpha;
	}

	using SearchFunc = std::function<void(int, SearchRefs&)>;

	class SearchWorker
	{
	public:
		SearchWorker(const core::Board& board, SearchFunc searchFunc)
				:m_SearchFunc(std::move(searchFunc)), m_Refs{}, m_IsReady{ true }
		{
			m_Refs.Board = board.CloneWithoutHistory();
		}

		void Start(const int depth, const bool thisThread = false)
		{
			m_IsReady = false;
			const auto starter = [this](const int depth)
			{
				m_SearchFunc(depth, m_Refs);
				m_IsReady = true;
			};

			if (thisThread)
			{
				starter(depth);
				return;
			}

			std::thread thread(starter, depth);
			thread.detach();
		}

		NODISCARD bool IsReady() const
		{
			return m_IsReady;
		}

		NODISCARD const SearchRefs& refs() const
		{
			return m_Refs;
		}

	private:
		SearchFunc m_SearchFunc;
		SearchRefs m_Refs;
		std::vector<Move> m_PV;
		std::atomic<bool> m_IsReady;
	};

	void Search::StartSearch(const core::Board& board, database::BookMoveSelector* bookMoveSelector,
			const double maxTime, const int maxDepth, const int maxWorkers, const double bookTemperature,
			const std::function<void(SearchResult)>* depthSearchedHook)
	{
		m_StopRequested = false;
		m_StartTime = std::chrono::high_resolution_clock::now();
		// Preparations count towards search time

		m_MaxTime = maxTime;

		if (bookMoveSelector)
		{
			Move moves[MAX_MOVES];
			const auto end = GenerateMoves<core::moves::Legality::Legal>(board, moves);
			const auto bookMove = bookMoveSelector->TrySelect(board.hash(), moves, end, bookTemperature);
			if (bookMove.IsValid())
			{
				std::vector<Move> pv{ bookMove };
				SearchResult result{ .Depth = 0, .SelDepth = 0, .Score = 0, .Nodes = 0, .PV = pv };
				if (depthSearchedHook)
				{
					depthSearchedHook->operator()(result);
				}
				return;
			}
		}

		SearchResult result;
		std::mutex mutex;
		std::atomic<int> reachedDepth = 0;

		const auto searchFunc = [this, &reachedDepth, &result, &mutex, &depthSearchedHook]
				(const int depth, SearchRefs& refs)
		{
			std::vector<Move> pv;
			int score;
			while (true)
			{
				static constexpr int ASP_WINDOW = 25;

				pv.clear();
				score = AlphaBeta<Node::NonPV>(depth, refs.Alpha, refs.Beta, pv, refs);

				if (ShouldStop())
				{
					return;
				}

				if (score <= refs.Alpha)
				{
					refs.Alpha -= 2 * ASP_WINDOW;
				}
				else if (score >= refs.Beta)
				{
					refs.Beta += 2 * ASP_WINDOW;
				}
				else
				{
					refs.Alpha = score - ASP_WINDOW;
					refs.Beta = score + ASP_WINDOW;
					break;
				}
			}

			std::scoped_lock lock(mutex);

			result.Nodes += refs.Nodes;

			if (depth < result.Depth || (depth == result.Depth && refs.SelDepth <= result.SelDepth))
			{
				return;
			}

			reachedDepth = depth;
			result.SelDepth = refs.SelDepth;
			result.Depth = depth;
			result.PV = pv;
			result.Score = score;

			if (depthSearchedHook)
			{
				depthSearchedHook->operator()(result);
			}
		};

		if (maxWorkers <= 1)
		{
			SearchWorker worker{ board, searchFunc };
			int depth = 1;
			while (!ShouldStop() && reachedDepth < maxDepth)
			{
				worker.Start(depth++, true);
			}
			return;
		}

		std::vector<std::unique_ptr<SearchWorker>> workers;
		for (int i = 0; i < maxWorkers; i++)
		{
			workers.emplace_back(std::make_unique<SearchWorker>(board, searchFunc));
		}

		size_t idx = 0;
		int depth = 1;
		int counter = 0;

		while (!ShouldStop() && reachedDepth < maxDepth)
		{
			if (workers[idx]->IsReady())
			{
				if (counter == 4 || counter == 5 || (counter >= 6 && counter % 2 == 0))
				{
					depth++;
				}
				counter++;

				if (depth <= maxDepth)
				{
					workers[idx]->Start(depth);
				}
			}

			idx = (idx + 1) % workers.size();
			std::this_thread::yield();
		}

		for (auto& awaitingWorker : workers)
		{
			while (!awaitingWorker->IsReady())
			{
				std::this_thread::yield();
			}
		}
	}

	bool Search::ShouldStop() const
	{
		if (m_StopRequested)
		{
			return true;
		}
		if (m_MaxTime < 0)
		{
			return false;
		}
		const auto now = std::chrono::high_resolution_clock::now();
		const auto passedTime = std::chrono::duration<double>(now - m_StartTime);
		return passedTime.count() > m_MaxTime;
	}

	Search::Search(const int tableSize, const int tableBucketSize)
			:m_TTable(tableSize, tableBucketSize), m_MoveSorter()
	{
	}

	template int Search::AlphaBeta<Node::NonPV>(int, int, int, std::vector<Move>&, SearchRefs&);
	template int Search::AlphaBeta<Node::PV>(int, int, int, std::vector<Move>&, SearchRefs&);
}