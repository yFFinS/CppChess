//
// Created by matvey on 31.08.22.
//

#pragma once

#include "../Common.h"

namespace chess::core::moves
{
	enum struct Type
	{
		Quiet = 0,
		DoublePawn = 1,
		LongCastle = 2,
		ShortCastle = 3,
		BishopPQ = 4,
		RookPQ = 5,
		KnightPQ = 6,
		QueenPQ = 7,
		Capture = 8,
		EnPassant = 9,
		BishopPC = 10,
		RookPC = 11,
		KnightPC = 12,
		QueenPC = 13,
	};

	struct Move
	{
	public:
		constexpr Move()
				:m_Value(-1)
		{
		}

		constexpr Move(const Square start, const Square end, const Type type)
				:m_Value(((int)type << 12 | (start.value() << 6) | end.value()))
		{

		}

		static constexpr Move Empty()
		{
			return {};
		}

		NODISCARD constexpr bool IsCapture() const
		{
			return type() == Type::EnPassant || type() == Type::Capture ||
					(Type::BishopPC <= type() && type() <= Type::QueenPC);
		}

		NODISCARD constexpr bool IsValid() const
		{
			return *this != Empty();
		}

		NODISCARD constexpr bool operator==(const Move& rhs) const
		{
			return m_Value == rhs.m_Value;
		}

		NODISCARD constexpr bool operator!=(const Move& rhs) const
		{
			return !(rhs == *this);
		}

		NODISCARD constexpr Square start() const
		{
			return Square((m_Value >> 6) & 0b111111);
		}

		NODISCARD constexpr Square end() const
		{
			return Square(m_Value & 0b111111);
		}

		NODISCARD constexpr Type type() const
		{
			return (Type)(m_Value >> 12);
		}

		NODISCARD constexpr uint16_t value() const
		{
			return m_Value;
		}

	private:
		uint16_t m_Value;
	};

	constexpr Square GetEnPassantCapturedPawnSquare(const Move move)
	{
		assert(move.IsValid());
		assert(move.type() == Type::EnPassant);
		const auto dy = move.end().rank() - move.start().rank();
		return move.end().OffsetBy(0, -dy);
	}

	constexpr Square GetEnPassantCapturedPawnSquare(const Square epSquare, const pieces::Color color)
	{
		assert(epSquare.IsValid());
		assert(pieces::IsValidColor(color));
		return Square(color == pieces::Color::Black ? epSquare.value() - 8 : epSquare.value() + 8);
	}

	struct TypedMove : Move
	{
	public:
		constexpr TypedMove() = default;

		constexpr TypedMove(const Move move, const pieces::Piece movedPiece, const pieces::Piece capturedPiece)
				:Move(move), m_MovedPiece(movedPiece), m_CapturedPiece(capturedPiece)
		{
		}

		NODISCARD constexpr pieces::Piece movedPiece() const
		{
			return m_MovedPiece;
		}

		NODISCARD constexpr pieces::Piece capturedPiece() const
		{
			return m_CapturedPiece;
		}

	private:
		pieces::Piece m_MovedPiece;
		pieces::Piece m_CapturedPiece;
	};
}
