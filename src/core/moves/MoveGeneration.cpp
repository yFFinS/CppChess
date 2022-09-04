//
// Created by matvey on 28.08.22.
//

#include "MoveGeneration.h"
#include "../Board.h"
#include "Move.h"
#include "../Magic.h"

namespace chess::core::moves
{
	namespace
	{
		std::array<Bitboard, 2> s_ShortCastleOccupation
				{
						Bitboard().WithSet(5).WithSet(6),
						Bitboard().WithSet(61).WithSet(62)
				};

		std::array<Bitboard, 2> s_LongCastleOccupation
				{
						Bitboard().WithSet(1).WithSet(2).WithSet(3),
						Bitboard().WithSet(57).WithSet(58).WithSet(59)
				};

		std::array<Bitboard, 2> s_ShortCastleSafe
				{
						Bitboard().WithSet(4).WithSet(5).WithSet(6),
						Bitboard().WithSet(60).WithSet(61).WithSet(62)
				};

		std::array<Bitboard, 2> s_LongCastleSafe
				{
						Bitboard().WithSet(2).WithSet(3).WithSet(4),
						Bitboard().WithSet(58).WithSet(59).WithSet(60)
				};

		std::array<Type, 4> s_CapturePromotions{ Type::QueenPC, Type::KnightPC, Type::RookPC, Type::BishopPC };
		std::array<Type, 4> s_QuietPromotions{ Type::QueenPQ, Type::KnightPQ, Type::RookPQ, Type::BishopPQ };

		Move* WriteMoves(const Square moveStart, const Type moveType,
				Square const* const movesBegin, Square const* const movesEnd, Move* output)
		{
			for (auto it = movesBegin; it != movesEnd; it++)
			{
				*output++ = Move(moveStart, *it, moveType);
			}

			return output;
		}

		template<bool CapturesOnly>
		Move* GenerateKingMoves(const Board& board, const Square kingSquare, Move* output)
		{
			const auto us = board.colorToPlay();
			const auto them = pieces::OppositeColor(us);
			const auto attackedBB = board.GetAttacked(them);
			const auto movesBB = lookups::GetKingMoves(kingSquare) & ~attackedBB;

			Square moves[8];

			{
				const auto capturesBB = movesBB & board.GetPieces(them);
				output = WriteMoves(kingSquare, Type::Capture, moves,
						capturesBB.BitScanForwardAll(moves), output);
			}

			if constexpr (CapturesOnly)
			{
				return output;
			}

			const auto occupancyBB = board.occupancy();

			{
				const auto quietBB = movesBB & ~occupancyBB;
				output = WriteMoves(kingSquare, Type::Quiet, moves,
						quietBB.BitScanForwardAll(moves), output);
			}

			if (board.checkers())
			{
				return output;
			}

			const auto cr = board.castlingRights();

			if (cr.CanCastle(us, pieces::Castle::Short) &&
					!(s_ShortCastleOccupation[(int)us] & occupancyBB) && !(s_ShortCastleSafe[(int)us] & attackedBB))
			{
				*output++ = Move(kingSquare,
						pieces::GetCastleRookStart(us, pieces::Castle::Short), Type::ShortCastle);
			}

			if (cr.CanCastle(us, pieces::Castle::Long) &&
					!(s_LongCastleOccupation[(int)us] & occupancyBB) && !(s_LongCastleSafe[(int)us] & attackedBB))
			{
				*output++ = Move(kingSquare,
						pieces::GetCastleRookStart(us, pieces::Castle::Long), Type::LongCastle);
			}

			return output;
		}

		template<bool CapturesOnly>
		Move* GenerateKnightMoves(const Board& board, const Square knightSquare, Move* output,
				const Bitboard pushMask, const Bitboard captureMask)
		{
			const auto us = board.colorToPlay();
			const auto pins = board.GetPins(us);

			if ((pins.OrthogonalPins | pins.DiagonalPins).TestAt(knightSquare))
			{
				return output;
			}

			const auto movesBB = lookups::GetKnightMoves(knightSquare);
			const auto enemiesBB = board.GetPieces(pieces::OppositeColor(us));

			Square moves[8];

			{
				const auto capturesBB = movesBB & enemiesBB & captureMask;
				output = WriteMoves(knightSquare, Type::Capture, moves,
						capturesBB.BitScanForwardAll(moves), output);
			}

			if constexpr (CapturesOnly)
			{
				return output;
			}

			{
				const auto quietsBB = movesBB & ~board.occupancy() & pushMask;
				output = WriteMoves(knightSquare, Type::Quiet, moves,
						quietsBB.BitScanForwardAll(moves), output);
			}

			return output;
		}

		template<bool CapturesOnly>
		Move* GenerateBishopMoves(const Board& board, const Square bishopSquare, Move* output,
				Bitboard pushMask, Bitboard captureMask)
		{
			const auto us = board.colorToPlay();
			const auto pins = board.GetPins(us);

			if (pins.OrthogonalPins.TestAt(bishopSquare))
			{
				return output;
			}

			if (pins.DiagonalPins.TestAt(bishopSquare))
			{
				pushMask &= pins.BishopMoves;
				captureMask &= pins.BishopMoves;
			}

			const auto occupancyBB = board.occupancy();
			const auto movesBB = lookups::GetSliderMoves<pieces::Type::Bishop>(bishopSquare, occupancyBB);

			Square moves[14];

			{
				const auto capturesBB = movesBB &
						board.GetPieces(pieces::OppositeColor(us)) & captureMask;
				output = WriteMoves(bishopSquare, Type::Capture,
						moves, capturesBB.BitScanForwardAll(moves), output);
			}

			if constexpr (CapturesOnly)
			{
				return output;
			}

			{
				const auto quietsBB = movesBB & ~occupancyBB & pushMask;
				output = WriteMoves(bishopSquare, Type::Quiet,
						moves, quietsBB.BitScanForwardAll(moves), output);
			}

			return output;
		}

		template<bool CapturesOnly>
		Move* GenerateRookMoves(const Board& board, const Square rookSquare, Move* output,
				Bitboard pushMask, Bitboard captureMask)
		{
			const auto us = board.colorToPlay();
			const auto pins = board.GetPins(us);

			if (pins.DiagonalPins.TestAt(rookSquare))
			{
				return output;
			}
			else if (pins.OrthogonalPins.TestAt(rookSquare))
			{
				pushMask &= pins.RookMoves;
				captureMask &= pins.RookMoves;
			}

			const auto occupancyBB = board.occupancy();
			const auto movesBB = lookups::GetSliderMoves<pieces::Type::Rook>(rookSquare, occupancyBB);

			Square moves[14];

			{
				const auto capturesBB = movesBB &
						board.GetPieces(pieces::OppositeColor(us)) & captureMask;
				output = WriteMoves(rookSquare, Type::Capture,
						moves, capturesBB.BitScanForwardAll(moves), output);
			}

			if constexpr (CapturesOnly)
			{
				return output;
			}

			{
				const auto quietsBB = movesBB & ~occupancyBB & pushMask;
				output = WriteMoves(rookSquare, Type::Quiet,
						moves, quietsBB.BitScanForwardAll(moves), output);
			}

			return output;
		}

		template<bool CapturesOnly>
		Move* GenerateQueenMoves(const Board& board, const Square queenSquare, Move* output,
				Bitboard pushMask, Bitboard captureMask)
		{
			const auto us = board.colorToPlay();
			const auto pins = board.GetPins(us);

			if (pins.DiagonalPins.TestAt(queenSquare))
			{
				const auto mask = pins.GetBishopMask(queenSquare);
				pushMask &= mask;
				captureMask &= mask;
			}
			else if (pins.OrthogonalPins.TestAt(queenSquare))
			{
				const auto mask = pins.GetRookMask(queenSquare);
				pushMask &= mask;
				captureMask &= mask;
			}

			const auto occupancyBB = board.occupancy();
			const auto movesBB = lookups::GetSliderMoves<pieces::Type::Queen>(queenSquare, occupancyBB);

			Square moves[28];

			{
				const auto capturesBB = movesBB &
						board.GetPieces(pieces::OppositeColor(us)) & captureMask;
				output = WriteMoves(queenSquare, Type::Capture,
						moves, capturesBB.BitScanForwardAll(moves), output);
			}

			if constexpr (CapturesOnly)
			{
				return output;
			}

			{
				const auto quietsBB = movesBB & ~occupancyBB & pushMask;
				output = WriteMoves(queenSquare, Type::Quiet,
						moves, quietsBB.BitScanForwardAll(moves), output);
			}

			return output;
		}

		bool IsLegalEnPassant(const Board& board, const Square pawnSquare, const Square epSquare)
		{
			const auto us = board.colorToPlay();
			const auto pawnsRankBB = lookups::GetRank(pawnSquare);
			const auto kingSquare = board.GetKingSquare(us);

			if (!pawnsRankBB.TestAt(kingSquare))
			{
				return true;
			}

			const auto rooksBB = board.GetPieces(pieces::Type::Rook) | board.GetPieces(pieces::Type::Queen);
			const auto enemyRooksOnPawnsRankBB = rooksBB & board.GetPieces(pieces::OppositeColor(us)) & pawnsRankBB;

			if (!enemyRooksOnPawnsRankBB)
			{
				return true;
			}

			const auto captureSquare = GetEnPassantCapturedPawnSquare(epSquare, us);
			const auto rankOccupancyAfterEpBB = (board.occupancy() & pawnsRankBB)
					.WithReset(pawnSquare).WithReset(captureSquare);

			Bitboard sliderAttacks;
			Square sliders[5];

			const auto end = enemyRooksOnPawnsRankBB.BitScanForwardAll(sliders);
			for (auto it = sliders; it != end; it++)
			{
				sliderAttacks |= lookups::GetRankMoves<true>(*it, rankOccupancyAfterEpBB);
			}

			return !sliderAttacks.TestAt(kingSquare);
		}

		template<bool CapturesOnly>
		Move* GeneratePawnMoves(const Board& board, const Square pawnSquare, Move* output,
				Bitboard pushMask, Bitboard captureMask)
		{
			const auto us = board.colorToPlay();
			const auto pins = board.GetPins(us);

			if (pins.DiagonalPins.TestAt(pawnSquare))
			{
				pushMask = {};
				captureMask &= pins.BishopMoves;
			}
			else if (pins.OrthogonalPins.TestAt(pawnSquare))
			{
				pushMask &= pins.RookMoves;
				captureMask = {};
			}

			const auto dy = us == pieces::Color::Black ? 1 : -1;
			const auto attacksBB = lookups::GetPawnAttacks(pawnSquare, us);
			const auto enemiesBB = board.GetPieces(pieces::OppositeColor(us));

			Square moves[4];

			{
				const auto capturesBB = attacksBB & enemiesBB & captureMask;
				const auto end = capturesBB.BitScanForwardAll(moves);
				const int rank = pawnSquare.OffsetBy(0, dy).rank();
				if (rank == 0 || rank == 7)
				{
					for (auto it = moves; it != end; it++)
					{
						for (const auto promotion : s_CapturePromotions)
						{
							*output++ = Move(pawnSquare, *it, promotion);
						}
					}
				}
				else
				{
					output = WriteMoves(pawnSquare, Type::Capture, moves, end, output);
				}
			}

			{
				const auto epSquare = board.GetEpSquare();
				if (epSquare.IsValid())
				{
					const auto epAttacksBB = (us == pieces::Color::White ? attacksBB << BOARD_SIZE : attacksBB
							>> BOARD_SIZE);
					const auto capturedSquare = GetEnPassantCapturedPawnSquare(epSquare, us);
					const auto canCapture = (epAttacksBB & captureMask).TestAt(capturedSquare);
					const auto isLegal = IsLegalEnPassant(board, pawnSquare, epSquare);
					if (canCapture && isLegal)
					{
						*output++ = Move(pawnSquare, epSquare, Type::EnPassant);
					}
				}
			}

			if constexpr (CapturesOnly)
			{
				return output;
			}

			if (board.GetPieces(pieces::Type::Pawn).TestAt(pawnSquare.OffsetBy(0, dy)))
			{
				return output;
			}

			{
				const auto pushBB = pushMask & lookups::GetPawnPushes(pawnSquare, us);
				const auto end = pushBB.BitScanForwardAll(moves);
				for (auto it = moves; it != end; it++)
				{
					if (abs(it->rank() - pawnSquare.rank()) == 1)
					{
						const auto rank = it->rank();
						if (rank == 0 || rank == 7)
						{
							for (const auto promotion : s_QuietPromotions)
							{
								*output++ = Move(pawnSquare, *it, promotion);
							}
						}
						else
						{
							*output++ = Move(pawnSquare, *it, Type::Quiet);
						}
					}
					else
					{
						*output++ = Move(pawnSquare, *it, Type::DoublePawn);
					}
				}
			}

			return output;
		}
	}

	Bitboard GeneratePushMaskFromChecker(const Board& board, const Square checkerSquare, const Square kingSquare)
	{
		const auto type = board.GetPiece(checkerSquare).type();
		return type == pieces::Type::Rook || type == pieces::Type::Bishop || type == pieces::Type::Queen ?
			   lookups::GetInBetween(checkerSquare, kingSquare) : Bitboard();
	}

	template<Legality Legality, bool CapturesOnly>
	TypedMove* GenerateMoves(const Board& board, TypedMove* output)
	{
		Move moves[MAX_MOVES];
		const auto end = GenerateMoves<Legality, CapturesOnly>(board, moves);
		for (auto it = moves; it != end; it++)
		{
			const auto move = GetTypedMove(board, *it);
			assert(move.movedPiece().IsValid());
			*output++ = move;
		}

		return output;
	}

	template<Legality Legality, bool CapturesOnly>
	Move* GenerateMoves(const Board& board, Move* output)
	{
		static_assert(Legality == Legality::PseudoLegal || Legality == Legality::Legal);
		assert(output);

		auto outputEndCopy = output;

		const auto checkersBB = board.checkers();
		const auto checkersCount = checkersBB.PopCount();

		const auto us = board.colorToPlay();

		if (checkersCount > 1)
		{
			const auto kingSquare = board.GetKingSquare(us);
			return GenerateKingMoves<false>(board, kingSquare, output);
		}

		Bitboard pushMask{ ~0ULL };
		Bitboard captureMask{ ~0ULL };

		if (checkersCount == 1)
		{
			const auto kingSquare = board.GetKingSquare(us);
			captureMask = checkersBB;
			const auto checker = Square(checkersBB.BitScanForward());
			pushMask = GeneratePushMaskFromChecker(board, checker, kingSquare);
		}

		const auto freeBB = ~board.occupancy();
		const auto ourPawnsBB = board.GetPieces(us, pieces::Type::Pawn);
		const auto canDoublePushRank = lookups::GetRank(us == pieces::Color::Black ? 1 : 6);

		Bitboard pawnPushMask;

		static constexpr auto shiftLeft = [](const Bitboard value, const int shift)
		{
			return shift >= 0 ? value << shift : value >> -shift;
		};

		const int shift = us == pieces::Color::Black ? 8 : -8;

		const auto pawnsSinglePushMask = shiftLeft(ourPawnsBB, shift) & freeBB;
		auto pawnsDoublePushMask = shiftLeft(ourPawnsBB & canDoublePushRank, shift) & freeBB;
		pawnsDoublePushMask = shiftLeft(pawnsDoublePushMask, shift) & freeBB;
		pawnPushMask = pushMask & (pawnsSinglePushMask | pawnsDoublePushMask);

		Square pieces[16];
		const auto piecesBB = board.GetPieces(us);
		const auto end = piecesBB.BitScanForwardAll(pieces);

		for (auto it = pieces; it != end; it++)
		{
			const auto piece = board.GetPiece(*it);
			switch (piece.type())
			{
			case pieces::Type::Pawn:
				output = GeneratePawnMoves<CapturesOnly>(board, *it, output, pawnPushMask, captureMask);
				break;
			case pieces::Type::Knight:
				output = GenerateKnightMoves<CapturesOnly>(board, *it, output, pushMask, captureMask);
				break;
			case pieces::Type::Bishop:
				output = GenerateBishopMoves<CapturesOnly>(board, *it, output, pushMask, captureMask);
				break;
			case pieces::Type::Rook:
				output = GenerateRookMoves<CapturesOnly>(board, *it, output, pushMask, captureMask);
				break;
			case pieces::Type::Queen:
				output = GenerateQueenMoves<CapturesOnly>(board, *it, output, pushMask, captureMask);
				break;
			case pieces::Type::King:
				output = GenerateKingMoves<CapturesOnly>(board, *it, output);
				break;
			}
		}

//		if constexpr (Legality == Legality::Legal)
//		{
//			Move legalMoves[MAX_MOVES];
//			auto legalMovesEnd = legalMoves;
//			for (auto it = outputEndCopy; it != output; it++)
//			{
//				if (board.IsLegal(*it))
//				{
//					*legalMovesEnd++ = *it;
//				}
//			}
//
//			for (auto it = legalMoves; it != legalMovesEnd; it++)
//			{
//				*outputEndCopy++ = *it;
//			}
//
//			return outputEndCopy;
//		}

		return output;
	}

	TypedMove GetTypedMove(const Board& board, const Move move)
	{
		const auto capturedSquare = move.type() == Type::EnPassant ? GetEnPassantCapturedPawnSquare(move) : move.end();
		return { move, board.GetPiece(move.start()), board.GetPiece(capturedSquare) };
	}

	template Move* GenerateMoves<Legality::PseudoLegal, false>(const Board&, Move*);
	template Move* GenerateMoves<Legality::Legal, false>(const Board&, Move*);
	template Move* GenerateMoves<Legality::PseudoLegal, true>(const Board&, Move*);
	template Move* GenerateMoves<Legality::Legal, true>(const Board&, Move*);
	template TypedMove* GenerateMoves<Legality::PseudoLegal, false>(const Board& board, TypedMove* output);
	template TypedMove* GenerateMoves<Legality::PseudoLegal, true>(const Board& board, TypedMove* output);
	template TypedMove* GenerateMoves<Legality::Legal, false>(const Board& board, TypedMove* output);
	template TypedMove* GenerateMoves<Legality::Legal, true>(const Board& board, TypedMove* output);
}

