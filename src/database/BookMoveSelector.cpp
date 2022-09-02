//
// Created by matvey on 01.09.22.
//

#include "BookMoveSelector.h"

#include <fstream>
#include <ranges>
#include <cmath>
#include <iostream>
#include <bitset>
#include <cstring>

namespace chess::database
{
	using namespace core::moves;
	using namespace core::pieces;

	namespace
	{
		NODISCARD bool IsSameMove(const Move move, const Entry& entry)
		{
			if (entry.Start != move.start() || entry.End == move.end())
			{
				return false;
			}

			auto movePromotion = core::pieces::Type::Pawn;
			const auto type = move.type();
			using core::moves::Type;

			if (type == Type::BishopPQ || type == Type::BishopPC)
			{
				movePromotion = core::pieces::Type::Bishop;
			}
			else if (type == Type::KnightPQ || type == Type::KnightPC)
			{
				movePromotion = core::pieces::Type::Knight;
			}
			else if (type == Type::RookPQ || type == Type::RookPC)
			{
				movePromotion = core::pieces::Type::Rook;
			}
			else if (type == Type::QueenPQ || type == Type::QueenPC)
			{
				movePromotion = core::pieces::Type::Queen;
			}

			return movePromotion == entry.Promotion;
		}
	}

	BookMoveSelector::BookMoveSelector(const std::string_view binPath)
			:m_Gen(std::random_device()())
	{
		std::ifstream fin(binPath.data(), std::ios::binary);

		struct BinEntry
		{
			uint64_t Key;
			uint16_t Move;
			uint16_t Weight;
			uint32_t Dummy;
		};
		BinEntry binEntry{};

		size_t count = 0;

		while (!fin.eof())
		{
			char buffer[sizeof(BinEntry)];
			fin.read(buffer, sizeof(BinEntry));
			std::reverse(buffer, buffer + 8);
			std::reverse(buffer + 8, buffer + 10);
			std::reverse(buffer + 10, buffer + 12);
			memcpy(&binEntry, buffer, sizeof(BinEntry));

			if (binEntry.Move == 0)
			{
				// Invalid book move
				continue;
			}

			auto promotion = (core::pieces::Type)((binEntry.Move >> 12) & 0b111);
			assert(IsValidPiece(promotion));

			int startRank = (binEntry.Move >> 9) & 0b111;
			int startFile = (binEntry.Move >> 6) & 0b111;
			int endRank = (binEntry.Move >> 3) & 0b111;
			int endFile = binEntry.Move & 0b111;

			const auto startSquare = core::Square(startFile, core::BOARD_SIZE - 1 - startRank);
			const auto endSquare = core::Square(endFile, core::BOARD_SIZE - 1 - endRank);

			assert(startSquare.IsValid() && endSquare.IsValid());

			Entry entry{ .Start = startSquare, .End = endSquare, .Promotion = promotion, .Weight = binEntry.Weight };
			if (m_Entries.find(binEntry.Key) == m_Entries.end())
			{
				m_Entries[binEntry.Key] = std::vector<Entry>();
			}
			m_Entries[binEntry.Key].push_back(entry);
			count++;
		}

		for (auto& pair : m_Entries)
		{
			std::sort(pair.second.begin(), pair.second.end(), [](const Entry& lhs, const Entry& rhs)
			{
				return lhs.Weight > rhs.Weight;
			});
		}

		std::cout << "Loaded " << count << " moves.\n";
	}

	Move BookMoveSelector::TrySelect(const uint64_t key, const Move* const begin, const Move* const end,
			const double temperature)
	{
		const auto bookPair = m_Entries.find(key);
		if (bookPair == m_Entries.end())
		{
			std::cout << "No key\n";
			return Move::Empty();
		}

		if (fabs(temperature) < 1e-8)
		{
			const auto entry = bookPair->second[0];
			for (auto it = begin; it != end; it++)
			{
				if (IsSameMove(*it, entry))
				{
					return *it;
				}
			}
			std::cout << "No book moves\n";
			return Move::Empty();
		}

		std::vector<std::pair<Move, double>> candidates;
		double sumWeight = 0;
		for (auto it = begin; it != end; it++)
		{
			for (const auto& entry : bookPair->second)
			{
				if (IsSameMove(*it, entry))
				{
					const auto adjustedWeight = pow((double)entry.Weight, temperature);
					sumWeight += adjustedWeight;
					candidates.emplace_back(*it, adjustedWeight);
				}
			}
		}

		if (candidates.empty())
		{
			std::cout << "No book moves\n";
			return Move::Empty();
		}

		for (auto& pair : candidates)
		{
			pair.second /= sumWeight;
		}

		double random = m_Distribution(m_Gen);
		for (const auto& pair : candidates)
		{
			if (random < pair.second)
			{
				std::cout << "Found move with weight " << pair.second << '\n';
				return pair.first;
			}

			random -= pair.second;
		}

		return candidates.back().first;
	}

}
