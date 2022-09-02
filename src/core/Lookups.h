//
// Created by matvey on 28.08.22.
//

#pragma once

#include "Common.h"

namespace chess::core::lookups
{
	template<bool MaskedOccupancy = false>
	NODISCARD Bitboard GetFileMoves(Square square, Bitboard occupancy);
	template<bool MaskedOccupancy = false>
	NODISCARD Bitboard GetRankMoves(Square square, Bitboard occupancy);
	template<bool MaskedOccupancy = false>
	NODISCARD Bitboard GetDiagonalMoves(Square square, Bitboard occupancy);
	template<bool MaskedOccupancy = false>
	NODISCARD Bitboard GetAntiDiagonalMoves(Square square, Bitboard occupancy);

	NODISCARD Bitboard GetPawnPushes(Square square, pieces::Color color);
	NODISCARD Bitboard GetPawnAttacks(Square square, pieces::Color color);
	NODISCARD Bitboard GetKnightMoves(Square square);
	NODISCARD Bitboard GetKingMoves(Square square);
	NODISCARD Bitboard GetBishopMoves(Square square, Bitboard boardOccupancy);
	NODISCARD Bitboard GetRookMoves(Square square, Bitboard boardOccupancy);
	NODISCARD Bitboard GetQueenMoves(Square square, Bitboard boardOccupancy);

	NODISCARD Bitboard GetAttackingKnights(Square square);
	NODISCARD Bitboard GetAttackingPawns(Square square, pieces::Color color);

	NODISCARD Bitboard GetRank(int rank);
	NODISCARD Bitboard GetRank(Square square);
	NODISCARD Bitboard GetFile(int file);
	NODISCARD Bitboard GetFile(Square square);
	NODISCARD Bitboard GetDiagonal(Square square);
	NODISCARD Bitboard GetAntiDiagonal(Square square);
	NODISCARD Bitboard GetInBetween(Square from, Square to);
}
