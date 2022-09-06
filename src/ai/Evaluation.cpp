//
// Created by matvey on 31.08.22.
//

#include "Evaluation.h"

#include "../core/Board.h"

namespace chess::ai::eval
{
	using namespace core;

	namespace
	{
		constexpr std::array<int, pieces::PIECES> PIECE_SCORES{ 100, 290, 310, 515, 900, 2000 };
		constexpr std::array<int, BOARD_SIZE> PAWN_PASSED_SCORES{ 0, 5, 10, 20, 40, 80, 160, 0 };
		constexpr std::array<int, pieces::PIECES> PIECE_PINNED_SCORES{ 10, 25, 25, 35, 100, 0 };

		constexpr int PAWN_ISOLATED_SCORE = -20;
		constexpr int ROOK_ON_OPEN_RANK_SCORE = 30;
		constexpr int ROOK_ON_SEMI_OPEN_RANK_SCORE = 13;
		constexpr int CHECK_SCORE = 10;
		constexpr int DOUBLE_CHECK_SCORE = 50;
		constexpr int BISHOP_PAIR_SCORE = 20;
		constexpr int BISHOP_PAIR_END_GAME_SCORE = 70;

		auto GetPieceScoreTweak(const Board& board, const Square square, const pieces::Piece piece,
				const auto pawnCount)
		{
			if (piece.type() == core::pieces::Type::Pawn)
			{
				int tweak = 0;

				const auto allyPawnsBB = board.GetPieces(piece.color(),
						core::pieces::Type::Pawn);
				const auto enemyPawnsBB = board.GetPieces(pieces::OppositeColor(piece.color()),
						core::pieces::Type::Pawn);

				const auto file = square.file();
				const auto pawnFile = lookups::GetFile(file);
				const auto leftFile = file > 0 ? lookups::GetFile(file - 1) : Bitboard();
				const auto rightFile = file < BOARD_SIZE - 1 ? lookups::GetFile(file + 1) : Bitboard();
				const bool isIsolated = !((leftFile | rightFile) & allyPawnsBB);

				if (isIsolated)
				{
					tweak += PAWN_ISOLATED_SCORE;
				}

				static constexpr auto filled = Bitboard(~0ULL);

				int passedTweak = 0;

				const auto adjacentFilesBB = pawnFile | leftFile | rightFile;
				const auto shift = (square.rank() + 1) * BOARD_SIZE;
				const auto passedCheckMask =
						piece.color() == core::pieces::Color::Black ?
						filled << shift : filled >> (BOARD_SQUARES - shift);

				if (!(passedCheckMask & adjacentFilesBB & enemyPawnsBB))
				{
					const int index = piece.color() == core::pieces::Color::Black ?
									  square.rank() : BOARD_SIZE - square.rank() - 1;
					passedTweak = PAWN_PASSED_SCORES[index];
				}

				tweak += board.IsEndGame() ? passedTweak * 2 : passedTweak;
				return tweak;
			}

			if (piece.type() == pieces::Type::Knight)
			{
				return (pawnCount - 10) * 6;
			}
			if (piece.type() == pieces::Type::Bishop)
			{
				return (10 - pawnCount) * 6;
			}
			if (piece.type() != pieces::Type::Rook)
			{
				return 0;
			}

			const auto rankBB = lookups::GetRank(square);
			const auto pawnsOnRankBB = board.GetPieces(pieces::Type::Pawn) & rankBB;
			if (!pawnsOnRankBB)
			{
				return ROOK_ON_OPEN_RANK_SCORE;
			}
			if (!(pawnsOnRankBB & board.GetPieces(piece.color())))
			{
				return ROOK_ON_SEMI_OPEN_RANK_SCORE;
			}

			return 0;
		}

		auto EvaluateAlivePieces(const Board& board, const Square* pieceSquares, const pieces::Piece* pieces,
				const auto count, const auto pawnCount)
		{
			int score = 0;
			for (int i = 0; i < count; i++)
			{
				const auto type = pieces[i].type();
				auto pieceScore = PIECE_SCORES[(int)type];
				pieceScore += GetPieceScoreTweak(board, pieceSquares[i], pieces[i], pawnCount);
				score += pieces[i].color() == core::pieces::Color::White ? pieceScore : -pieceScore;
			}

			return score;
		}

		auto EvaluatePieceCounts(const Board& board)
		{
			int score = 0;
			if (board.GetPieceCount(core::pieces::Color::White, core::pieces::Type::Bishop) >= 2)
			{
				score += board.IsEndGame() ? BISHOP_PAIR_END_GAME_SCORE : BISHOP_PAIR_SCORE;
			}
			if (board.GetPieceCount(core::pieces::Color::Black, core::pieces::Type::Bishop) >= 2)
			{
				score -= board.IsEndGame() ? BISHOP_PAIR_END_GAME_SCORE : BISHOP_PAIR_SCORE;
			}

			return score;
		}

		auto EvaluateCheckers(const Board& board)
		{
			const auto checkersCount = board.checkers().PopCount();
			return checkersCount == 0 ? 0 : checkersCount == 1 ? CHECK_SCORE : DOUBLE_CHECK_SCORE;
		}

		auto EvaluatePinnedPieces(const Board& board)
		{
			int score = 0;

			const auto whitePins = board.GetPins(core::pieces::Color::White).all()
					& board.GetPieces(core::pieces::Color::White);
			const auto blackPins = board.GetPins(core::pieces::Color::Black).all()
					& board.GetPieces(core::pieces::Color::Black);

			static constexpr int MAX_PINNED_PIECES = 8;

			Square whiteSquares[MAX_PINNED_PIECES], blackSquares[MAX_PINNED_PIECES];
			const auto whiteEnd = whitePins.BitScanForwardAll(whiteSquares);
			const auto blackEnd = blackPins.BitScanForwardAll(blackSquares);

			for (auto it = whiteSquares; it != whiteEnd; it++)
			{
				const auto piece = board.GetPiece(*it);
				assert(pieces::IsValidPiece(piece.type()));
				score += PIECE_PINNED_SCORES[(int)piece.type()];
			}

			for (auto it = blackSquares; it != blackEnd; it++)
			{
				const auto piece = board.GetPiece(*it);
				assert(pieces::IsValidPiece(piece.type()));
				score -= PIECE_PINNED_SCORES[(int)piece.type()];
			}

			return score;
		}

		auto EvaluatePieceSquares(const Board& board)
		{
			const auto& eval = board.eval();
			return eval.GetScore(core::pieces::Color::White, board.IsEndGame())
					- eval.GetScore(core::pieces::Color::Black, board.IsEndGame());
		}
	}

	int EvaluateBoard(const core::Board& board)
	{
		Square pieceSquares[32];
		pieces::Piece pieces[32];

		const auto end = board.occupancy().BitScanForwardAll(pieceSquares);
		const auto count = end - pieceSquares;
		for (int i = 0; i < count; i++)
		{
			pieces[i] = board.GetPiece(pieceSquares[i]);
		}

		const auto pawnCount = board.GetPieces(pieces::Type::Pawn).PopCount();

		int score = 0;

		score += EvaluateAlivePieces(board, pieceSquares, pieces, count, pawnCount);
		score += EvaluatePinnedPieces(board);
		score += EvaluatePieceSquares(board);
		score += EvaluatePieceCounts(board);

		if (board.colorToPlay() == core::pieces::Color::Black)
		{
			score *= -1;
		}

		score += EvaluateCheckers(board);
		return score;
	}
}