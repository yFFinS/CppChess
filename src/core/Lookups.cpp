//
// Created by matvey on 28.08.22.
//

#include "Lookups.h"

#include <array>
#include <map>
#include <vector>
#include <cstdio>
#include <complex>

#include "Common.h"
#include "ScopedTimer.h"
#include "Random.h"

namespace chess::core
{
	namespace
	{
		std::array<std::array<Bitboard, BOARD_SQUARES>, 2> s_PawnAttacks;
		std::array<std::array<Bitboard, BOARD_SQUARES>, 2> s_PawnPushes;
		std::array<std::array<Bitboard, BOARD_SQUARES>, 2> s_PawnAttackedSquares;

		std::array<Bitboard, BOARD_SQUARES> s_KnightMoves;
		std::array<Bitboard, BOARD_SQUARES> s_KnightAttackedSquares;

		std::array<Bitboard, BOARD_SQUARES> s_KingMoves;
		std::array<Bitboard, BOARD_SQUARES> s_KingAttackedSquares;

		std::array<std::array<Bitboard, BOARD_SQUARES>, BOARD_SQUARES> s_InBetween;

		std::array<Bitboard, BOARD_SIZE> s_Files;
		std::array<Bitboard, BOARD_SIZE> s_Ranks;
		std::array<Bitboard, BOARD_SQUARES> s_Diagonals;
		std::array<Bitboard, BOARD_SQUARES> s_AntiDiagonals;

		using bb_map = std::map<Bitboard, Bitboard>;

		std::array<bb_map, BOARD_SQUARES> s_DiagonalMovesByOccupancy;
		std::array<bb_map, BOARD_SQUARES> s_AntiDiagonalMovesByOccupancy;
		std::array<bb_map, BOARD_SQUARES> s_FileMovesByOccupancy;
		std::array<bb_map, BOARD_SQUARES> s_RankMovesByOccupancy;

		void PreComputeSliderOccupancyMap(bb_map& destination, const Square square,
				const auto& directions)
		{
			const auto generateMovesWithOccupancy = [square, &directions](Bitboard occupancy)
			{
				std::vector<Square> moves;
				for (const auto& direction : directions)
				{
					for (int distance = 1; distance < 8; distance++)
					{
						const auto newSquare =
								square.OffsetBy(direction.first * distance, direction.second * distance);
						if (!newSquare.IsValid())
						{
							break;
						}

						moves.push_back(newSquare);
						if (occupancy.TestAt(newSquare))
						{
							break;
						}
					}
				}

				if (auto it = std::find(moves.begin(), moves.end(), square);
						it != moves.end())
				{
					moves.erase(it);
				}
				return moves;
			};

			auto allMoves = generateMovesWithOccupancy({});
			allMoves.push_back(square);

			const auto computeFromIndex =
					[generateMovesWithOccupancy, &allMoves, &destination]
							(const auto& self, size_t index, Bitboard occupancy)
					{
						while (true)
						{
							if (index == allMoves.size())
							{
								const auto moves = generateMovesWithOccupancy(occupancy);
								Bitboard bitboard;
								for (const auto square : moves)
								{
									bitboard.SetAt(square);
								}
								destination[occupancy] = bitboard;
								return;
							}

							self(self, index + 1, occupancy);
							occupancy.SetAt(allMoves[index++]);
						}
					};

			computeFromIndex(computeFromIndex, 0, {});
		}

		void PreComputeFileAndRankMovesByOccupancy()
		{
			for (int pos = 0; pos < 64; pos++)
			{
				static constexpr std::array directions = { std::make_pair(0, -1), std::make_pair(0, 1) };
				PreComputeSliderOccupancyMap(s_FileMovesByOccupancy[pos], Square(pos), directions);
			}

			for (int pos = 0; pos < 64; pos++)
			{
				static constexpr std::array directions = { std::make_pair(-1, 0), std::make_pair(1, 0) };
				PreComputeSliderOccupancyMap(s_RankMovesByOccupancy[pos], Square(pos), directions);
			}
		}

		void PreComputeDiagonalAndAntiDiagonalMovesByOccupancy()
		{
			for (int pos = 0; pos < 64; pos++)
			{
				static constexpr std::array directions = { std::make_pair(-1, -1), std::make_pair(1, 1) };
				PreComputeSliderOccupancyMap(s_DiagonalMovesByOccupancy[pos], Square(pos), directions);
			}

			for (int pos = 0; pos < 64; pos++)
			{
				static constexpr std::array directions = { std::make_pair(1, -1), std::make_pair(-1, 1) };
				PreComputeSliderOccupancyMap(s_AntiDiagonalMovesByOccupancy[pos], Square(pos), directions);
			}
		}

//		namespace magic
//		{
//			constexpr int MAGIC_ROOK_BITS = 12;
//			constexpr int MAGIC_BISHOP_BITS = 9;
//
//			std::array<std::array<Bitboard, 1 << MAGIC_ROOK_BITS>, BOARD_SQUARES> s_MagicRookTable;
//			std::array<std::array<Bitboard, 1 << MAGIC_BISHOP_BITS>, BOARD_SQUARES> s_MagicBishopTable;
//
//			struct PlainMagic
//			{
//				Bitboard Mask{};
//				uint64_t Magic{};
//			};
//
//			std::array<PlainMagic, BOARD_SQUARES> s_BishopMagic;
//			std::array<PlainMagic, BOARD_SQUARES> s_RookMagic;
//		}
//
//		void PreComputeRookMagic()
//		{
//			constexpr auto filled = Bitboard(-1);
//
//			std::array<bb_map, BOARD_SQUARES> perSquareMapping;
//
//			static constexpr std::array directions =
//					{ std::make_pair(-1, 0), std::make_pair(1, 0),
//					  std::make_pair(0, -1), std::make_pair(0, 1) };
//
//			for (int pos = 0; pos < BOARD_SQUARES; pos++)
//			{
//				PreComputeSliderOccupancyMap(perSquareMapping[pos], Square(pos), directions);
//			}
//
//			for (int pos = 0; pos < BOARD_SQUARES; pos++)
//			{
//				Square square(pos);
//				const auto mask = lookups::GetRank(square) | lookups::GetFile(square);
//
//				while (true)
//				{
//					bool ok = true;
//
//					auto& table = magic::s_MagicRookTable[pos];
//					table.fill(filled);
//					const auto magic = RandomUint64();
//
//					for (const auto& pair : perSquareMapping[pos])
//					{
//						auto occupancy = (uint64_t)pair.first;
//						const auto moves = pair.second;
//
//						occupancy *= magic;
//						const auto index = occupancy >> (64 - magic::MAGIC_ROOK_BITS);
//						if (table[index] != filled && table[index] != moves)
//						{
//							ok = false;
//							break;
//						}
//
//						table[index] = moves;
//					}
//
//					if (ok)
//					{
//						magic::s_RookMagic[pos] = magic::PlainMagic{ .Mask = mask, .Magic = magic };
//						break;
//					}
//				}
//			}
//		}
//
//		void PreComputeBishopMagic()
//		{
//			constexpr auto filled = Bitboard(-1);
//
//			std::array<bb_map, BOARD_SQUARES> perSquareMapping;
//
//			static constexpr std::array directions =
//					{ std::make_pair(-1, -1), std::make_pair(1, 1),
//					  std::make_pair(-1, 1), std::make_pair(1, -1) };
//
//			for (int pos = 0; pos < BOARD_SQUARES; pos++)
//			{
//				PreComputeSliderOccupancyMap(perSquareMapping[pos], Square(pos), directions);
//			}
//
//			for (int pos = 0; pos < BOARD_SQUARES; pos++)
//			{
//				const auto mask = s_Diagonals[pos] | s_AntiDiagonals[pos];
//
//				while (true)
//				{
//					bool ok = true;
//
//					auto& table = magic::s_MagicBishopTable[pos];
//					table.fill(filled);
//					const auto magic = RandomUint64();
//
//					for (const auto& pair : perSquareMapping[pos])
//					{
//						auto occupancy = (uint64_t)pair.first;
//						const auto moves = pair.second;
//
//						occupancy *= magic;
//						const auto index = occupancy >> (64 - magic::MAGIC_BISHOP_BITS);
//						if (table[index] != filled && table[index] != moves)
//						{
//							ok = false;
//							break;
//						}
//
//						table[index] = moves;
//					}
//
//					if (ok)
//					{
//						magic::s_BishopMagic[pos] = magic::PlainMagic{ .Mask = mask, .Magic = magic };
//						printf("%d ready\n", pos);
//						break;
//					}
//				}
//			}
//		}

		constexpr void PreComputeFilesAndRanks()
		{
			for (int file = 0; file < 8; file++)
			{
				s_Files[file] = Bitboard(0x0101010101010101UL) << file;
			}

			for (int rank = 0; rank < 8; rank++)
			{
				s_Ranks[rank] = Bitboard(0xFFUL) << (rank * 8);
			}
		}

		constexpr void PreComputeDiagonalsAndAntiDiagonals()
		{
			for (int pos = 0; pos < 64; pos++)
			{
				Square square(pos);

				Bitboard diagonal;

				for (int distance = -8; distance <= 8; distance++)
				{
					auto newSquare = square.OffsetBy(distance, distance);
					if (newSquare.IsValid())
					{
						diagonal.SetAt(newSquare);
					}
				}

				s_Diagonals[pos] = diagonal;
			}

			for (int pos = 0; pos < 64; pos++)
			{
				Square square(pos);
				Bitboard antiDiagonal;
				for (int distance = -8; distance <= 8; distance++)
				{
					auto newSquare = square.OffsetBy(distance, -distance);
					if (newSquare.IsValid())
					{
						antiDiagonal.SetAt(newSquare);
					}
				}

				s_AntiDiagonals[pos] = antiDiagonal;
			}
		}

		constexpr void PreComputeInBetween()
		{
			// https://www.chessprogramming.org/Square_Attacked_By

			const uint64_t m1 = ~0UL;
			const uint64_t a2a7 = 0x0001010101010100UL;
			const uint64_t b2g7 = 0x0040201008040200UL;
			const uint64_t h1b7 = 0x0002040810204080UL;

			for (int from = 0; from < 64; from++)
			{
				for (int to = 0; to < 64; to++)
				{
					auto between = (m1 << from) ^ (m1 << to);
					auto file = (ulong)((to & 7) - (from & 7));
					auto rank = (ulong)((to | 7) - from) >> 3;
					auto line = ((file & 7) - 1) & a2a7;
					line += 2 * (((rank & 7) - 1) >> 58);
					line += (((rank - file) & 15) - 1) & b2g7;
					line += (((rank + file) & 15) - 1) & h1b7;
					line *= (ulong)(between & -between);
					s_InBetween[from][to] = Bitboard(line & between);
				}
			}
		}

		constexpr void PreComputeKingMoves()
		{
			for (int pos = 0; pos < 64; pos++)
			{
				Square square(pos);
				for (int dx = -1; dx <= 1; dx++)
				{
					for (int dy = -1; dy <= 1; dy++)
					{
						const auto newSquare = square.OffsetBy(dx, dy);
						if ((dx == 0 && dy == 0) || !newSquare.IsValid())
						{
							continue;
						}

						s_KingMoves[pos].SetAt(newSquare);
						s_KingAttackedSquares[newSquare.value()].SetAt(pos);
					}
				}
			}
		}

		constexpr void PreComputeKnightMoves()
		{
			for (int pos = 0; pos < BOARD_SQUARES; pos++)
			{
				Square square(pos);
				for (const auto& direction :
						{ std::make_tuple(-2, -1), std::make_tuple(-2, 1), std::make_tuple(2, -1),
						  std::make_tuple(2, 1), std::make_tuple(1, 2), std::make_tuple(1, -2),
						  std::make_tuple(-1, 2), std::make_tuple(-1, -2) })
				{
					const auto newSquare = square.OffsetBy(get<0>(direction), get<1>(direction));
					if (!newSquare.IsValid())
					{
						continue;
					}

					s_KnightMoves[pos].SetAt(newSquare);
					s_KnightAttackedSquares[newSquare.value()].SetAt(pos);
				}
			}
		}

		void PreComputePawnMoves()
		{
			for (const auto color : { pieces::Color::Black, pieces::Color::White })
			{
				auto& destPushes = s_PawnPushes[(int)color];
				auto& destAttacks = s_PawnAttacks[(int)color];
				auto& destAttacked = s_PawnAttackedSquares[(int)color];

				auto dy = color == pieces::Color::Black ? 1 : -1;
				for (int pos = 0; pos < 64; pos++)
				{
					Square square(pos);
					std::vector blackOffsets{ std::make_pair(1, 9), std::make_pair(-1, 7), std::make_pair(0, 8) };
					std::vector whiteOffsets{ std::make_pair(-1, -9), std::make_pair(1, -7),
											  std::make_pair(0, -8) };
					auto offsets = color == pieces::Color::Black ? blackOffsets : whiteOffsets;
					for (const auto& offset : offsets)
					{
						int dx = offset.first;
						int off = offset.second;
						auto newSquare = square.OffsetBy(dx, dy);
						if (!newSquare.IsValid())
						{
							continue;
						}

						if (abs(off) != 8)
						{
							destAttacked[pos + off].SetAt(pos);
							destAttacks[pos].SetAt(pos + off);
						}
						else
						{
							destPushes[pos].SetAt(pos + off);
						}

						if ((square.rank() == 1 && color == pieces::Color::Black)
								|| (square.rank() == 6 && color == pieces::Color::White))
						{
							destPushes[pos].SetAt(pos + 16 * dy);
						}
					}
				}
			}
		}

		struct Initializer
		{
			Initializer()
			{
				PROFILE_INIT;

				PreComputeFilesAndRanks();
				PreComputeDiagonalsAndAntiDiagonals();
				PreComputeInBetween();

				PreComputePawnMoves();
				PreComputeKnightMoves();
				PreComputeKingMoves();

				PreComputeDiagonalAndAntiDiagonalMovesByOccupancy();
				PreComputeFileAndRankMovesByOccupancy();

//				PreComputeBishopMagic();
//				printf("Bishop ready\n");
//				PreComputeRookMagic();
//				printf("Rook ready\n");
			}
		};
	}

	namespace lookups
	{
		static Initializer s_Initializer{};

		template<bool MaskedOccupancy>
		Bitboard GetFileMoves(const Square square, Bitboard occupancy)
		{
			assert(square.IsValid());
			if constexpr (!MaskedOccupancy)
			{
				occupancy &= s_Files[square.file()];
			}
			return s_FileMovesByOccupancy[square.value()][occupancy];
		}

		template<bool MaskedOccupancy>
		Bitboard GetRankMoves(const Square square, Bitboard occupancy)
		{
			assert(square.IsValid());
			if constexpr (!MaskedOccupancy)
			{
				occupancy &= s_Ranks[square.rank()];
			}
			return s_RankMovesByOccupancy[square.value()][occupancy];
		}

		template<bool MaskedOccupancy>
		Bitboard GetDiagonalMoves(const Square square, Bitboard occupancy)
		{
			assert(square.IsValid());
			if constexpr (!MaskedOccupancy)
			{
				occupancy &= s_Diagonals[square.value()];
			}
			return s_DiagonalMovesByOccupancy[square.value()][occupancy];
		}

		template<bool MaskedOccupancy>
		Bitboard GetAntiDiagonalMoves(const Square square, Bitboard occupancy)
		{
			assert(square.IsValid());
			if constexpr (!MaskedOccupancy)
			{
				occupancy &= s_AntiDiagonals[square.value()];
			}
			return s_AntiDiagonalMovesByOccupancy[square.value()][occupancy];
		}

		Bitboard GetPawnPushes(const Square square, const pieces::Color color)
		{
			return s_PawnPushes[(int)color][square.value()];
		}

		Bitboard GetPawnAttacks(const Square square, const pieces::Color color)
		{
			return s_PawnAttacks[(int)color][square.value()];
		}

		Bitboard GetKnightMoves(const Square square)
		{
			return s_KnightMoves[square.value()];
		}

		Bitboard GetKingMoves(const Square square)
		{
			return s_KingMoves[square.value()];
		}

		Bitboard GetBishopMoves(const Square square, const Bitboard boardOccupancy)
		{
			const auto diagonalMoves = GetDiagonalMoves<false>(square, boardOccupancy);
			const auto antiDiagonalMoves = GetAntiDiagonalMoves<false>(square, boardOccupancy);
			return diagonalMoves | antiDiagonalMoves;
		}

		Bitboard GetRookMoves(const Square square, const Bitboard boardOccupancy)
		{
			const auto fileMoves = GetFileMoves<false>(square, boardOccupancy);
			const auto rankMoves = GetRankMoves<false>(square, boardOccupancy);
			return fileMoves | rankMoves;
		}

		Bitboard GetQueenMoves(const Square square, const Bitboard boardOccupancy)
		{
			return GetBishopMoves(square, boardOccupancy) | GetRookMoves(square, boardOccupancy);
		}

		Bitboard GetAttackingKnights(const Square square)
		{
			return s_KnightAttackedSquares[square.value()];
		}

		Bitboard GetAttackingPawns(const Square square, const pieces::Color color)
		{
			return s_PawnAttackedSquares[(int)color][square.value()];
		}

		Bitboard GetRank(const int rank)
		{
			assert(0 <= rank && rank < BOARD_SIZE);
			return s_Ranks[rank];
		}

		Bitboard GetRank(const Square square)
		{
			return GetRank(square.rank());
		}

		Bitboard GetFile(const int file)
		{
			assert(0 <= file && file < BOARD_SIZE);
			return s_Files[file];
		}

		Bitboard GetFile(const Square square)
		{
			return GetFile(square.file());
		}

		Bitboard GetInBetween(const Square from, const Square to)
		{
			assert(from.IsValid());
			assert(to.IsValid());
			return s_InBetween[from.value()][to.value()];
		}

		Bitboard GetDiagonal(const Square square)
		{
			return s_Diagonals[square.value()];
		}

		Bitboard GetAntiDiagonal(const Square square)
		{
			return s_AntiDiagonals[square.value()];
		}

		template Bitboard GetDiagonalMoves<false>(Square, Bitboard);
		template Bitboard GetDiagonalMoves<true>(Square, Bitboard);
		template Bitboard GetAntiDiagonalMoves<false>(Square, Bitboard);
		template Bitboard GetAntiDiagonalMoves<true>(Square, Bitboard);
		template Bitboard GetFileMoves<false>(Square, Bitboard);
		template Bitboard GetFileMoves<true>(Square, Bitboard);
		template Bitboard GetRankMoves<false>(Square, Bitboard);
		template Bitboard GetRankMoves<true>(Square, Bitboard);
	}
}
