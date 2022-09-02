//
// Created by matvey on 31.08.22.
//

#pragma once

#include <vector>
#include <optional>
#include <mutex>

#include "../../core/Common.h"
#include "../../core/moves/Move.h"

namespace chess::ai::hash
{
	enum struct EntryType
	{
		None, Exact, Alpha, Beta
	};

	struct TableEntry
	{
		uint64_t Hash{};
		core::moves::Move BestMove{};
		EntryType Type{};
		int Depth{};
		int Value{};

		constexpr EntryType Apply(const int depth, int& alpha, int& beta) const
		{
			if (Depth < depth)
			{
				return EntryType::None;
			}

			switch (Type)
			{
			case EntryType::Exact:
				return EntryType::Exact;
			case EntryType::Alpha:
				if (Value <= alpha)
				{
					alpha = Value;
					return EntryType::Alpha;
				}
				break;
			case EntryType::Beta:
				if (Value >= beta)
				{
					beta = Value;
					return EntryType::Beta;
				}
				break;
			case EntryType::None:
				assert(false);
			}

			return EntryType::None;
		}
	};

	class TranspositionTable
	{
	public:
		TranspositionTable(int maxSize, int bucketSize);

		void Insert(const TableEntry& entry);
		NODISCARD std::optional<TableEntry> Probe(uint64_t hash);

	private:
		NODISCARD std::vector<TableEntry>& GetBucket(uint64_t hash);
		NODISCARD const std::vector<TableEntry>& GetBucket(uint64_t hash) const;

		std::vector<std::vector<TableEntry>> m_Data;

		std::mutex m_Mutex;

		int m_MaxSize;
		int m_BucketSize;
	};
}
