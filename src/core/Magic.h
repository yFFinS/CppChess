//
// Created by matvey on 03.09.22.
//

#include "Common.h"

namespace chess::core::lookups
{
	template<pieces::Type Type>
	NODISCARD Bitboard GetSliderMoves(Square square, Bitboard occupancy);
}
