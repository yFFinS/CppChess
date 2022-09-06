//
// Created by matvey on 31.08.22.
//

#include "TranspositionTable.h"

namespace chess::ai::hash
{
	void TranspositionTable::Insert(const TableEntry& entry)
	{
		std::scoped_lock lock(m_Mutex);

		auto* bucket = GetBucket(entry.Hash);

		if ((int)bucket->size() < m_BucketSize)
		{
			bucket->push_back(entry);
			return;
		}

		for (auto& bucketEntry : *bucket)
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
		const auto* bucket = GetBucket(hash);
		if (!bucket)
		{
			return {};
		}
		for (const auto entry : *bucket)
		{
			if (entry.Hash == hash)
			{
				return entry;
			}
		}

		return {};
	}

	std::vector<TableEntry>* TranspositionTable::GetBucket(const uint64_t hash)
	{
		const auto& bucket = const_cast<const TranspositionTable*>(this)->GetBucket(hash);
		return const_cast<std::vector<TableEntry>*>(bucket);
	}

	const std::vector<TableEntry>* TranspositionTable::GetBucket(const uint64_t hash) const
	{
		const int index = (int)(hash % m_MaxSize);
		return &m_Data[index];
	}

	void TranspositionTable::Reset(const int maxSize, const int bucketSize)
	{
		std::scoped_lock lock(m_Mutex);

		m_MaxSize = maxSize;
		m_BucketSize = bucketSize;

		m_Data.clear();
		for (auto i = 0; i < m_MaxSize; i++)
		{
			m_Data.emplace_back();
		}
	}
}

