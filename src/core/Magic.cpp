//
// Created by matvey on 03.09.22.
//

#include "Magic.h"

#include "Random.h"
#include "ScopedTimer.h"

namespace chess::core::lookups
{
	namespace
	{
		constexpr Bitboard GetRookMask(const Square square)
		{
			Bitboard result;
			const auto [file, rank] = square.xy();
			for (int r = rank + 1; r <= 6; r++)
			{
				result.SetAt(file + r * 8);
			}
			for (int r = rank - 1; r >= 1; r--)
			{
				result.SetAt(file + r * 8);
			}
			for (int f = file + 1; f <= 6; f++)
			{
				result.SetAt(rank * 8 + f);
			}
			for (int f = file - 1; f >= 1; f--)
			{
				result.SetAt(rank * 8 + f);
			}

			return result;
		}

		constexpr Bitboard GetBishopMask(const Square square)
		{
			Bitboard result;
			const auto [file, rank] = square.xy();
			for (int r = rank + 1, f = file + 1; r <= 6 && f <= 6; r++, f++)
			{
				result.SetAt(f + r * 8);
			}
			for (int r = rank + 1, f = file - 1; r <= 6 && f >= 1; r++, f--)
			{
				result.SetAt(f + r * 8);
			}
			for (int r = rank - 1, f = file + 1; r >= 1 && f <= 6; r--, f++)
			{
				result.SetAt(f + r * 8);
			}
			for (int r = rank - 1, f = file - 1; r >= 1 && f >= 1; r--, f--)
			{
				result.SetAt(f + r * 8);
			}

			return result;
		}

		constexpr Bitboard GetRookAttacks(const Square square, const Bitboard occupancy)
		{
			Bitboard result;
			const auto [file, rank] = square.xy();
			for (int r = rank + 1; r <= 7; r++)
			{
				result.SetAt(file + r * 8);
				if (occupancy.TestAt(file + r * 8))
				{
					break;
				}
			}
			for (int r = rank - 1; r >= 0; r--)
			{
				result.SetAt(file + r * 8);
				if (occupancy.TestAt(file + r * 8))
				{
					break;
				}
			}
			for (int f = file + 1; f <= 7; f++)
			{
				result.SetAt(f + rank * 8);
				if (occupancy.TestAt(f + rank * 8))
				{
					break;
				}
			}
			for (int f = file - 1; f >= 0; f--)
			{
				result.SetAt(f + rank * 8);
				if (occupancy.TestAt(f + rank * 8))
				{
					break;
				}
			}

			return result;
		}

		constexpr Bitboard GetBishopAttacks(const Square square, const Bitboard occupancy)
		{
			Bitboard result;
			const auto [file, rank] = square.xy();
			for (int r = rank + 1, f = file + 1; r <= 7 && f <= 7; r++, f++)
			{
				result.SetAt(f + r * 8);
				if (occupancy.TestAt(f + r * 8))
				{
					break;
				}
			}
			for (int r = rank + 1, f = file - 1; r <= 7 && f >= 0; r++, f--)
			{
				result.SetAt(f + r * 8);
				if (occupancy.TestAt(f + r * 8))
				{
					break;
				}
			}
			for (int r = rank - 1, f = file + 1; r >= 0 && f <= 7; r--, f++)
			{
				result.SetAt(f + r * 8);
				if (occupancy.TestAt(f + r * 8))
				{
					break;
				}
			}
			for (int r = rank - 1, f = file - 1; r >= 0 && f >= 0; r--, f--)
			{
				result.SetAt(f + r * 8);
				if (occupancy.TestAt(f + r * 8))
				{
					break;
				}
			}

			return result;
		}

		constexpr int GetIndex(const Bitboard blockers, const uint64_t magic, const int bits)
		{
			return (int)(((uint64_t)blockers * magic) >> (BOARD_SQUARES - bits));
		}

		constexpr Bitboard IndexToBitboard(const int index, const int maskBits, Bitboard mask)
		{
			Bitboard result;
			for (int i = 0; i < maskBits; i++)
			{
				const int bitIndex = mask.PopFirstSetBit();
				if (index & (1 << i))
				{
					result.SetAt(bitIndex);
				}
			}

			return result;
		}

		struct FancyMagic
		{
			Bitboard* Table;
			uint64_t Mask;
			int Bits;
			uint64_t Magic;
		};

		RandomGenerator s_Generator;

		uint64_t SparseRandom64()
		{
			return s_Generator.RandomUInt64() & s_Generator.RandomUInt64() & s_Generator.RandomUInt64();
		}

		template<pieces::Type Type>
		FancyMagic FindMagic(const Square square, const int bits, Bitboard* output)
		{
			static_assert(Type == pieces::Type::Bishop || Type == pieces::Type::Rook);

			const size_t size = 1ULL << bits;
			std::vector<Bitboard> blockers(size), attacks(size);
			const auto mask = Type == pieces::Type::Bishop ? GetBishopMask(square) : GetRookMask(square);
			const int count = mask.PopCount();

			for (int i = 0; i < (1 << count); i++)
			{
				blockers[i] = IndexToBitboard(i, count, mask);
				attacks[i] = Type == pieces::Type::Bishop ?
							 GetBishopAttacks(square, blockers[i]) : GetRookAttacks(square, blockers[i]);
			}

			for (int iter = 0; iter < 1'000'000; iter++)
			{
				auto magic = SparseRandom64();
				const auto mostSignificantRow = Bitboard(((uint64_t)mask * magic) & 0xFF00000000000000ULL);
				if (mostSignificantRow.PopCount() < 6)
				{
					continue;
				}

				for (size_t i = 0; i < size; i++)
				{
					output[i] = {};
				}
				bool ok = true;
				for (int i = 0; i < (1 << count); i++)
				{
					const int index = GetIndex(blockers[i], magic, bits);
					assert((size_t)index < size);
					if (!output[index])
					{
						output[index] = attacks[i];
					}
					else if (output[index] != attacks[i])
					{
						ok = false;
						break;
					}
				}

				if (ok)
				{
					return { .Table = output, .Mask = (uint64_t)mask, .Bits = bits, .Magic = magic };
				}
			}

			return {};
		}

		constexpr std::array<int, BOARD_SQUARES> ROOK_BITS = {
				12, 11, 11, 11, 11, 11, 11, 12,
				11, 10, 10, 10, 10, 10, 10, 11,
				11, 10, 10, 10, 10, 10, 10, 11,
				11, 10, 10, 10, 10, 10, 10, 11,
				11, 10, 10, 10, 10, 10, 10, 11,
				11, 10, 10, 10, 10, 10, 10, 11,
				11, 10, 10, 10, 10, 10, 10, 11,
				12, 11, 11, 11, 11, 11, 11, 12
		};

		constexpr std::array<int, BOARD_SQUARES> BISHOP_BITS = {
				6, 5, 5, 5, 5, 5, 5, 6,
				5, 5, 5, 5, 5, 5, 5, 5,
				5, 5, 7, 7, 7, 7, 5, 5,
				5, 5, 7, 9, 9, 7, 5, 5,
				5, 5, 7, 9, 9, 7, 5, 5,
				5, 5, 7, 7, 7, 7, 5, 5,
				5, 5, 5, 5, 5, 5, 5, 5,
				6, 5, 5, 5, 5, 5, 5, 6
		};

		template<size_t N>
		constexpr size_t ArraySumOfPow2(const std::array<int, N>& array)
		{
			size_t result = 0;
			for (size_t i = 0; i < N; i++)
			{
				result += 1 << array[i];
			}
			return result;
		}

		constexpr int ROOK_TABLE_SIZE = ArraySumOfPow2(ROOK_BITS);
		constexpr int BISHOP_TABLE_SIZE = ArraySumOfPow2(BISHOP_BITS);

		std::array<Bitboard, ROOK_TABLE_SIZE> ROOK_TABLE;
		std::array<Bitboard, BISHOP_TABLE_SIZE> BISHOP_TABLE;
		std::array<FancyMagic, BOARD_SQUARES> BISHOP_MAGIC, ROOK_MAGIC;

		bool FindSeed(const double maxTime, uint64_t& seed)
		{
			seed = RandomUInt64();
			s_Generator = RandomGenerator(seed);

			utils::ScopedTimer timer;

			size_t rookIndex = 0;
			for (int square = 0; square < BOARD_SQUARES; square++)
			{
				ROOK_MAGIC[square] = FindMagic<pieces::Type::Rook>(
						Square(square), ROOK_BITS[square], &ROOK_TABLE[rookIndex]);
				rookIndex += 1ULL << ROOK_BITS[square];
				if (timer.GetPassedTime() > maxTime || !ROOK_MAGIC[square].Bits)
				{
					return false;
				}
			}

			size_t bishopIndex = 0;
			for (int square = 0; square < BOARD_SQUARES; square++)
			{
				BISHOP_MAGIC[square] = FindMagic<pieces::Type::Bishop>(
						Square(square), BISHOP_BITS[square], &BISHOP_TABLE[bishopIndex]);
				bishopIndex += 1ULL << BISHOP_BITS[square];
				if (timer.GetPassedTime() > maxTime || !BISHOP_MAGIC[square].Bits)
				{
					return false;
				}
			}

			return true;
		}

		void InitMagic()
		{
			s_Generator = RandomGenerator(12338480473037317818ULL);

			utils::ScopedTimer timer;

			size_t rookIndex = 0;
			for (int square = 0; square < BOARD_SQUARES; square++)
			{
				ROOK_MAGIC[square] = FindMagic<pieces::Type::Rook>(
						Square(square), ROOK_BITS[square], &ROOK_TABLE[rookIndex]);
				rookIndex += 1ULL << ROOK_BITS[square];
				assert(ROOK_MAGIC[square].Table);
			}

			size_t bishopIndex = 0;
			for (int square = 0; square < BOARD_SQUARES; square++)
			{
				BISHOP_MAGIC[square] = FindMagic<pieces::Type::Bishop>(
						Square(square), BISHOP_BITS[square], &BISHOP_TABLE[bishopIndex]);
				bishopIndex += 1ULL << BISHOP_BITS[square];
				assert(BISHOP_MAGIC[square].Table);
			}
		}

		struct Initializer
		{
			Initializer()
			{
				PROFILE_INIT;

				InitMagic();

//				while (true)
//				{
//					// 12338480473037317818 - <0.15
//					uint64_t seed;
//					auto ok = FindSeed(0.12, seed);
//					if (ok)
//					{
//						std::cout << "Random seed: " << seed << '\n';
//						break;
//					}
//					std::cout << "Bad seed " << seed << '\n';
//				}
			}
		};

		Initializer s_Initializer;
	}

	template<pieces::Type Type>
	Bitboard GetSliderMoves(const Square square, Bitboard occupancy)
	{
		static_assert(Type == pieces::Type::Bishop || Type == pieces::Type::Rook || Type == pieces::Type::Queen);
		if constexpr (Type == pieces::Type::Queen)
		{
			return GetSliderMoves<pieces::Type::Bishop>(square, occupancy) |
					GetSliderMoves<pieces::Type::Rook>(square, occupancy);
		}

		const auto sq = square.value();
		const auto& fancyMagic = Type == pieces::Type::Bishop ? BISHOP_MAGIC[sq] : ROOK_MAGIC[sq];

		auto indexMagic = (uint64_t)occupancy;
		indexMagic &= fancyMagic.Mask;
		indexMagic *= fancyMagic.Magic;
		indexMagic >>= BOARD_SQUARES - fancyMagic.Bits;
		return fancyMagic.Table[indexMagic];
	}

	template Bitboard GetSliderMoves<pieces::Type::Bishop>(Square, Bitboard);
	template Bitboard GetSliderMoves<pieces::Type::Rook>(Square, Bitboard);
	template Bitboard GetSliderMoves<pieces::Type::Queen>(Square, Bitboard);
}

