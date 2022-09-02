//
// Created by matvey on 31.08.22.
//

#include "TranspositionTable.h"

namespace chess::ai::hash
{
	TranspositionTable::TranspositionTable(int maxSize, int bucketSize)
			:m_MaxSize(maxSize), m_BucketSize(bucketSize)
	{
		for (auto i = 0; i < maxSize; i++)
		{
			m_Data.emplace_back();
		}
	}

	void TranspositionTable::Insert(const TableEntry& entry)
	{
		std::scoped_lock lock(m_Mutex);

		auto& bucket = GetBucket(entry.Hash);

		if ((int)bucket.size() < m_BucketSize)
		{
			bucket.push_back(entry);
			return;
		}

		for (auto& bucketEntry : bucket)
		{
			if (bucketEntry.Depth < entry.Depth)
			{
				bucketEntry = entry;
				return;
			}
		}
	}

	std::optional<TableEntry> TranspositionTable::Probe(const uint64_t hash)
	{
		std::scoped_lock lock(m_Mutex);

		const auto& bucket = GetBucket(hash);
		auto it = std::find_if(bucket.begin(), bucket.end(),
				[hash](const TableEntry& entry)
				{
					return entry.Hash == hash;
				});

		return it == bucket.end() ? std::optional<TableEntry>() : *it;
	}

	std::vector<TableEntry>& TranspositionTable::GetBucket(const uint64_t hash)
	{
		const auto& bucket = const_cast<const TranspositionTable*>(this)->GetBucket(hash);
		return const_cast<std::vector<TableEntry>&>(bucket);
	}

	const std::vector<TableEntry>& TranspositionTable::GetBucket(const uint64_t hash) const
	{
		const int index = (int)(hash % m_MaxSize);
		return m_Data[index];
	}

}

