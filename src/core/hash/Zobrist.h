//
// Created by matvey on 28.08.22.
//

#pragma once

#include "../Common.h"

namespace chess::core::hash
{
	struct ZobristHash
	{
	public:
		void ToggleColorToPlay();
		void TogglePiece(Square square, pieces::Piece piece);
		void ToggleEpFile(int file);
		void ToggleCastlingRights(pieces::CastlingRights castlingRights);

		constexpr void Reset()
		{
			m_Value = 0;
		}

		NODISCARD constexpr uint64_t value() const
		{
			return m_Value;
		}

	private:
		uint64_t m_Value;
	};
}
