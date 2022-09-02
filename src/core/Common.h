//
// Created by matvey on 28.08.22.
//

#pragma once

#include <cstdint>
#include <cassert>
#include <bit>
#include <x86gprintrin.h>
#include <functional>

#define NODISCARD [[nodiscard]]

namespace chess::core
{
	static constexpr int BOARD_SQUARES = 64;
	static constexpr int BOARD_SIZE = 8;

	static constexpr int INVALID_FILE = -1;

	constexpr bool IsInBounds(const int x, const int y)
	{
		return 0 <= x && x < BOARD_SIZE && 0 <= y && y < BOARD_SIZE;
	}

	constexpr bool IsInBounds(const int value)
	{
		return 0 <= value && value < BOARD_SQUARES;
	}

	static constexpr int INVALID_SQUARE = -1;

	struct Square
	{
	public:
		constexpr explicit Square()
				:m_Value(INVALID_SQUARE)
		{
		}

		constexpr explicit Square(const int file, const int rank)
				:m_Value(IsInBounds(file, rank) ? (file + rank * BOARD_SIZE) : INVALID_SQUARE)
		{
		}

		constexpr explicit Square(const int value)
				:m_Value(IsInBounds(value) ? value : INVALID_SQUARE)
		{
		}

		NODISCARD constexpr bool operator==(const Square square) const
		{
			return square == m_Value;
		}

		NODISCARD constexpr bool operator!=(const Square square) const
		{
			return square != m_Value;
		}

		NODISCARD constexpr bool operator==(const int square) const
		{
			return m_Value == square;
		}

		NODISCARD constexpr bool operator!=(const int square) const
		{
			return m_Value != square;
		}

		NODISCARD constexpr bool operator>(const Square square) const
		{
			return m_Value > square.m_Value;
		}

		NODISCARD constexpr bool operator<(const Square square) const
		{
			return m_Value < square.m_Value;
		}

		NODISCARD constexpr bool operator>=(const Square square) const
		{
			return m_Value >= square.m_Value;
		}

		NODISCARD constexpr bool operator<=(const Square square) const
		{
			return m_Value <= square.m_Value;
		}

		NODISCARD constexpr bool IsValid() const
		{
			return m_Value != INVALID_SQUARE;
		}

		NODISCARD constexpr int file() const
		{
			return m_Value % BOARD_SIZE;
		}

		NODISCARD constexpr int rank() const
		{
			return m_Value / BOARD_SIZE;
		}

		NODISCARD constexpr int value() const
		{
			return m_Value;
		}

		NODISCARD constexpr Square OffsetBy(const int dx, const int dy) const
		{
			return Square(file() + dx, rank() + dy);
		}

		static constexpr Square Invalid()
		{
			return Square(INVALID_SQUARE);
		}

	private:
		int m_Value;
	};

	struct Bitboard
	{
	public:
		constexpr explicit operator uint64_t() const
		{
			return m_Value;
		}

		constexpr Bitboard()
				:Bitboard(0)
		{
		}

		constexpr Bitboard(const Bitboard& bitboard) = default;
		constexpr Bitboard(Bitboard&& bitboard) noexcept = default;

		constexpr Bitboard& operator=(const Bitboard&) = default;
		constexpr Bitboard& operator=(Bitboard&&) = default;

		constexpr explicit Bitboard(uint64_t value)
				:m_Value(value)
		{
		}

		NODISCARD constexpr explicit operator bool() const
		{
			return m_Value;
		}

		NODISCARD constexpr Bitboard operator>>(const int shift) const
		{
			assert(shift >= 0);
			auto copy = *this;
			return copy >>= shift;
		}

		NODISCARD constexpr Bitboard operator<<(const int shift) const
		{
			assert(shift >= 0);
			auto copy = *this;
			return copy <<= shift;
		}

		NODISCARD constexpr Bitboard operator|(const Bitboard mask) const
		{
			auto copy = *this;
			return copy |= mask;
		}

		NODISCARD constexpr Bitboard operator&(const Bitboard mask) const
		{
			auto copy = *this;
			return copy &= mask;
		}

		NODISCARD constexpr Bitboard operator^(const Bitboard mask) const
		{
			auto copy = *this;
			return copy ^= mask;
		}

		constexpr Bitboard& operator<<=(const int shift)
		{
			assert(shift >= 0);
			m_Value <<= shift;
			return *this;
		}

		constexpr Bitboard& operator>>=(const int shift)
		{
			assert(shift >= 0);
			m_Value >>= shift;
			return *this;
		}

		constexpr Bitboard& operator|=(const Bitboard mask)
		{
			m_Value |= mask.m_Value;
			return *this;
		}

		constexpr Bitboard& operator&=(const Bitboard mask)
		{
			m_Value &= mask.m_Value;
			return *this;
		}

		NODISCARD constexpr Bitboard& operator^=(const Bitboard mask)
		{
			m_Value ^= mask.m_Value;
			return *this;
		}

		NODISCARD constexpr Bitboard operator~() const
		{
			return Bitboard(~m_Value);
		}

		NODISCARD constexpr bool operator==(const Bitboard bitboard) const
		{
			return m_Value == bitboard.m_Value;
		}

		NODISCARD constexpr bool operator!=(const Bitboard bitboard) const
		{
			return m_Value != bitboard.m_Value;
		}

		NODISCARD constexpr bool operator<(const Bitboard bitboard) const
		{
			return m_Value < bitboard.m_Value;
		}

		NODISCARD constexpr bool operator>(const Bitboard bitboard) const
		{
			return m_Value > bitboard.m_Value;
		}

		NODISCARD constexpr bool operator<=(const Bitboard bitboard) const
		{
			return m_Value <= bitboard.m_Value;
		}

		NODISCARD constexpr bool operator>=(const Bitboard bitboard) const
		{
			return m_Value >= bitboard.m_Value;
		}

		NODISCARD constexpr bool TestAt(const int index) const
		{
			return TestAt(Square(index));
		}

		constexpr void Inverse()
		{
			m_Value = ~m_Value;
		}

		NODISCARD constexpr bool TestAt(const Square square) const
		{
			assert(square.IsValid());
			return (1ULL << square.value()) & m_Value;
		}

		constexpr void SetAt(const int index)
		{
			SetAt(Square(index));
		}

		constexpr void SetAt(const Square square)
		{
			assert(square.IsValid());
			m_Value |= 1ULL << square.value();
		}

		constexpr void ResetAt(const int index)
		{
			ResetAt(Square(index));
		}

		constexpr void ResetAt(const Square square)
		{
			assert(square.IsValid());
			m_Value &= ~(1ULL << square.value());
		}

		NODISCARD constexpr Bitboard WithSet(const int index) const
		{
			return WithSet(Square(index));
		}

		NODISCARD constexpr Bitboard WithSet(const Square square) const
		{
			auto copy = *this;
			copy.SetAt(square);
			return copy;
		}

		NODISCARD constexpr Bitboard WithReset(const int index) const
		{
			return WithReset(Square(index));
		}

		NODISCARD constexpr Bitboard WithReset(const Square square) const
		{
			auto copy = *this;
			copy.ResetAt(square);
			return copy;
		}

		NODISCARD constexpr int PopCount() const
		{
			return std::popcount(m_Value);
		}

		NODISCARD constexpr int BitScanForward() const
		{
			assert(m_Value);

			if (std::is_constant_evaluated())
			{
				return std::popcount((m_Value & -m_Value) - 1);
			}

			return __builtin_ffsll((long long)m_Value) - 1;
		}

		NODISCARD constexpr int* BitScanForwardAll(int* output) const
		{
			auto value = m_Value;

			while (value)
			{
				const auto index = Bitboard(value).BitScanForward();
				value &= ~(1ULL << index);
				*output++ = index;
			}

			return output;
		}

		NODISCARD constexpr Square* BitScanForwardAll(Square* output) const
		{
			auto value = m_Value;

			while (value)
			{
				const auto index = Bitboard(value).BitScanForward();
				value &= ~(1ULL << index);
				*output++ = Square(index);
			}

			return output;
		}

		struct Hash
		{
			auto operator()(const Bitboard bitboard) const
			{
				return std::hash<uint64_t>()(bitboard.m_Value);
			}
		};

	private:
		uint64_t m_Value;
	};

	namespace pieces
	{
		static constexpr int COLORS = 2;

		enum struct Color
		{
			Black = 0,
			White = 1
		};

		constexpr Color OppositeColor(const Color color)
		{
			return (Color)(1 - (int)color);
		}

		constexpr bool IsValidColor(const Color color)
		{
			return Color::Black <= color && color <= Color::White;
		}

		static constexpr int PIECES = 6;

		enum struct Type
		{
			Pawn = 0,
			Knight = 1,
			Bishop = 2,
			Rook = 3,
			Queen = 4,
			King = 5
		};

		constexpr bool IsValidPiece(const Type type)
		{
			return Type::Pawn <= type && type <= Type::King;
		}

		struct Piece
		{
		public:
			constexpr Piece()
					:m_Color((Color)-1), m_Type((Type)-1)
			{
			}

			constexpr Piece(const Color color, const Type type)
					:m_Color(color), m_Type(type)
			{
				assert(IsValidColor(color));
				assert(IsValidPiece(type));
			}

			NODISCARD constexpr Color color() const
			{
				return m_Color;
			}

			NODISCARD constexpr Type type() const
			{
				return m_Type;
			}

			NODISCARD constexpr bool IsValid() const
			{
				return IsValidColor(color());
			}

			NODISCARD constexpr bool operator==(const Piece rhs) const
			{
				return m_Color == rhs.m_Color &&
						m_Type == rhs.m_Type;
			}

			NODISCARD constexpr bool operator!=(const Piece rhs) const
			{
				return !(rhs == *this);
			}

			NODISCARD constexpr Piece Promote(const Type type) const
			{
				assert(IsValidPiece(type));
				assert(type != Type::Pawn && type != Type::King);
				return { color(), type };
			}

			NODISCARD constexpr Piece Demote() const
			{
				assert(type() != Type::Pawn && type() != Type::King);
				return { color(), Type::Pawn };
			}

			NODISCARD static constexpr Piece Invalid()
			{
				return {};
			}

		private:
			Color m_Color;
			Type m_Type;
		};

		enum struct Castle
		{
			Short = 1,
			Long = 2
		};

		constexpr Square GetCastleKingStart(const Color color)
		{
			return Square(color == Color::Black ? 4 : 60);
		}

		constexpr Square GetCastleKingEnd(const Color color, const Castle castle)
		{
			if (castle == Castle::Short)
			{
				return Square(color == Color::Black ? 6 : 62);
			}
			else
			{
				return Square(color == Color::Black ? 2 : 58);
			}
		}

		constexpr Square GetCastleRookStart(const Color color, const Castle castle)
		{
			if (castle == Castle::Short)
			{
				return Square(color == Color::Black ? 7 : 63);
			}
			else
			{
				return Square(color == Color::Black ? 0 : 56);
			}
		}

		constexpr Square GetCastleRookEnd(const Color color, const Castle castle)
		{
			if (castle == Castle::Short)
			{
				return Square(color == Color::Black ? 5 : 61);
			}
			else
			{
				return Square(color == Color::Black ? 3 : 59);
			}
		}

		struct CastlingRights
		{
			constexpr CastlingRights()
					:m_Value(0)
			{
			}

			constexpr CastlingRights(bool whiteShort, bool whiteLong, bool blackShort, bool blackLong)
					:CastlingRights()
			{
				SetCastleAllowed(Color::White, Castle::Short, whiteShort);
				SetCastleAllowed(Color::White, Castle::Long, whiteLong);
				SetCastleAllowed(Color::Black, Castle::Short, blackShort);
				SetCastleAllowed(Color::Black, Castle::Long, blackLong);
			}

			constexpr void
			SetCastleAllowed(const Color color, const Castle castle, const bool allowed)
			{
				if (allowed)
				{
					AllowCastle(color, castle);
				}
				else
				{
					DisallowCastle(color, castle);
				}
			}

			constexpr void AllowCastle(const Color color, const Castle castle)
			{
				m_Value |= GetMask(color, castle);
			}

			constexpr void DisallowCastle(const Color color, const Castle castle)
			{
				m_Value &= ~GetMask(color, castle);
			}

			NODISCARD constexpr bool CanCastle(const Color color, const Castle castle) const
			{
				return m_Value & GetMask(color, castle);
			}

			NODISCARD constexpr int value() const
			{
				return m_Value;
			}
		private:
			static constexpr int GetMask(const Color color, const Castle castle)
			{
				return color == Color::Black ? (int)castle : (int)castle << 2;
			}

			int m_Value;
		};
	}

}
