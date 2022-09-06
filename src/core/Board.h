//
// Created by matvey on 28.08.22.
//

#pragma once

#include <array>
#include <vector>
#include <map>
#include "hash/Zobrist.h"
#include "eval/IncrementalPieceSquareEvaluator.h"
#include "Lookups.h"
#include "moves/Move.h"

namespace chess::core
{
	struct PinsInfo
	{
		Bitboard DiagonalPins;
		Bitboard OrthogonalPins;
		Bitboard BishopMoves;
		Bitboard RookMoves;

		NODISCARD Bitboard GetBishopMask(const Square square) const
		{
			return BishopMoves & (lookups::GetDiagonal(square) | lookups::GetAntiDiagonal(square));
		}

		NODISCARD Bitboard GetRookMask(const Square square) const
		{
			const auto mask = (lookups::GetFile(square) | lookups::GetRank(square));
			return RookMoves & mask;
		}

		NODISCARD constexpr Bitboard all() const
		{
			return DiagonalPins | OrthogonalPins;
		}
	};

	struct MoveUndoInfo
	{
		moves::Move Move;
		pieces::Piece CapturedPiece;
		int EpFile{};
		int HalfMoves{};

		pieces::CastlingRights CastlingRights;

		Bitboard CheckersBB;

		std::array<PinsInfo, pieces::COLORS> Pins;
		std::array<Bitboard, pieces::COLORS> AttackedBBs;
		int MaxRepetitions;
		std::map<uint64_t, int> Repetitions;
		uint64_t ValidHash;
	};

	class Board
	{
		friend struct FenSetter;
	public:
		Board() = default;

		NODISCARD constexpr pieces::Color colorToPlay() const
		{
			return m_ColorToPlay;
		}

		NODISCARD constexpr Bitboard checkers() const
		{
			return m_CheckersBB;
		}

		NODISCARD constexpr Bitboard occupancy() const
		{
			return m_OccupancyBB;
		}

		NODISCARD constexpr PinsInfo GetPins(const pieces::Color color) const
		{
			assert(pieces::IsValidColor(color));
			return m_PinsInfos[(int)color];
		}

		NODISCARD constexpr Bitboard GetAttacked(const pieces::Color color) const
		{
			assert(pieces::IsValidColor(color));
			return m_AttackedBBs[(int)color];
		}

		NODISCARD constexpr Bitboard GetPieces(const pieces::Color color) const
		{
			assert(pieces::IsValidColor(color));
			return m_ColorBBs[(int)color];
		}

		NODISCARD constexpr Bitboard GetPieces(const pieces::Type type) const
		{
			assert(pieces::IsValidPiece(type));
			return m_PieceBBs[(int)type];
		}

		NODISCARD constexpr Bitboard GetPieces(const pieces::Color color, const pieces::Type type) const
		{
			return GetPieces(color) & GetPieces(type);
		}

		NODISCARD constexpr Square GetEpSquare() const
		{
			if (m_EpFile == INVALID_FILE)
			{
				return Square::Invalid();
			}

			const auto square = colorToPlay() == pieces::Color::White ? 16 + m_EpFile : 40 + m_EpFile;
			return Square(square);
		}

		NODISCARD Bitboard GetAttackedBy(Square square) const;
		NODISCARD bool IsAttacked(Square square) const;

		pieces::Piece RemovePiece(Square square);

		template<bool TryRemove = true>
		void SetPiece(Square square, pieces::Piece piece);

		void Clear();

		void MakeMove(moves::Move move);
		void UndoMove();

		NODISCARD constexpr pieces::Piece GetPiece(const Square square) const
		{
			assert(square.IsValid());
			return m_Pieces[square.value()];
		}

		NODISCARD constexpr int halfMoves() const
		{
			return m_HalfMoves;
		}

		NODISCARD constexpr int fullMoves() const
		{
			return m_FullMoves;
		}

		NODISCARD constexpr pieces::CastlingRights castlingRights() const
		{
			return m_CastlingRights;
		}

		NODISCARD constexpr uint64_t hash() const
		{
			return m_Zobrist.value();
		}

		NODISCARD constexpr const eval::IncrementalPieceSquareEvaluator& eval() const
		{
			return m_Evaluator;
		}

		NODISCARD constexpr Square GetKingSquare(const pieces::Color color) const
		{
			const auto index = (GetPieces(pieces::Type::King) & GetPieces(color)).BitScanForward();
			return Square(index);
		}

		NODISCARD Board CloneWithoutHistory() const;

		NODISCARD int GetPieceCount(const pieces::Color color, const pieces::Type type) const
		{
			assert(pieces::IsValidColor(color));
			assert(pieces::IsValidPiece(type));
			return m_PieceCounts[(int)color][(int)type];
		}

		NODISCARD constexpr int endGameWeights() const
		{
			return m_EndGameWeight;
		}

		NODISCARD constexpr bool IsEndGame() const
		{
			return endGameWeights() >= 0;
		}

		NODISCARD bool IsLegal(moves::Move move) const;
		NODISCARD int GetMaxRepetitions() const;
	private:
		void ChangeSidesInternal();
		void SetCastlingRightsInternal(pieces::CastlingRights cr);
		void SetEpFileInternal(int file);
		void UpdateCastlingRights(moves::Move, pieces::Piece movingPiece, pieces::Piece capturedPiece);
		void UpdateBitboards();
		void RecalculateEndGameWeight();

		pieces::Color m_ColorToPlay{};

		std::array<Bitboard, pieces::COLORS> m_ColorBBs{};
		std::array<Bitboard, pieces::PIECES> m_PieceBBs{};
		std::array<Bitboard, pieces::COLORS> m_AttackedBBs{};
		std::array<PinsInfo, pieces::COLORS> m_PinsInfos{};

		Bitboard m_CheckersBB{};
		Bitboard m_OccupancyBB{};

		std::array<pieces::Piece, BOARD_SQUARES> m_Pieces{};
		std::array<std::array<int, pieces::PIECES>, pieces::COLORS> m_PieceCounts{};

		pieces::CastlingRights m_CastlingRights{};

		int m_EpFile = INVALID_FILE;
		int m_HalfMoves = 0, m_FullMoves = 0;
		int m_EndGameWeight = 0;

		hash::ZobristHash m_Zobrist{};
		eval::IncrementalPieceSquareEvaluator m_Evaluator{};

		std::vector<MoveUndoInfo> m_MoveHistory{};

		std::map<uint64_t, int> m_Repetitions{};
		int m_MaxRepetitions = 0;
	};
}
