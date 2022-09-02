//
// Created by matvey on 29.08.22.
//

#include <sstream>
#include <cstring>
#include "Fen.h"

#include "Board.h"
#include "Misc.h"

namespace chess::core
{
	namespace
	{
		std::array<char, pieces::PIECES> s_PieceCharMap{ 'p', 'n', 'b', 'r', 'q', 'k' };
	}

	struct FenSetter
	{
		static bool SetFen(Board& board, const std::vector<std::string>& split)
		{
			board.Clear();
			if (split[1] == "w")
			{
				board.ChangeSidesInternal();
			}
			else if (split[1] != "b")
			{
				return false;
			}

			pieces::CastlingRights cr;
			cr.SetCastleAllowed(pieces::Color::Black, pieces::Castle::Short, split[2].contains('k'));
			cr.SetCastleAllowed(pieces::Color::Black, pieces::Castle::Long, split[2].contains('q'));
			cr.SetCastleAllowed(pieces::Color::White, pieces::Castle::Short, split[2].contains('K'));
			cr.SetCastleAllowed(pieces::Color::White, pieces::Castle::Long, split[2].contains('Q'));

			board.SetCastlingRightsInternal(cr);

			if (split[3] != "-")
			{
				const auto epSquare = misc::GetSquareFromName(split[3]);
				if (!epSquare.IsValid())
				{
					return false;
				}
				board.SetEpFileInternal(epSquare.file());
			}

			try
			{
				board.m_HalfMoves = split.size() > 4 ? std::stoi(split[4]) : 0;
				board.m_FullMoves = split.size() > 5 ? std::stoi(split[5]) : 1;
			}
			catch (const std::exception&)
			{
				return false;
			}

			int index = 0;
			for (const auto ch : split[0])
			{
				if (std::isdigit(ch))
				{
					index += ch - '0';
				}
				else if (ch != '/')
				{
					const auto typeId = std::find(s_PieceCharMap.begin(), s_PieceCharMap.end(), tolower(ch));
					if (typeId == s_PieceCharMap.end())
					{
						return false;
					}

					const auto type = (pieces::Type)(typeId - s_PieceCharMap.begin());
					const auto color = isupper(ch) ? pieces::Color::White : pieces::Color::Black;
					board.SetPiece(Square(index++), pieces::Piece(color, type));
				}
			}

			board.UpdateBitboards();
			board.RecalculateEndGameWeight();
			return true;
		}
	};
}

namespace chess::core::fen
{
	std::string ExtractFen(const Board& board)
	{
		std::stringstream fen;

		int skipLength = 0;
		for (int pos = 0; pos < BOARD_SQUARES; pos++)
		{
			const auto piece = board.GetPiece(Square(pos));
			if (piece.IsValid())
			{
				auto ch = s_PieceCharMap[(int)piece.type()];
				if (piece.color() == pieces::Color::White)
				{
					ch = (char)toupper(ch);
				}

				if (skipLength > 0)
				{
					fen << skipLength;
					skipLength = 0;
				}

				fen << ch;
			}
			else
			{
				skipLength++;
			}

			if (pos % 8 == 0 && pos != 0)
			{
				if (skipLength > 0)
				{
					fen << skipLength;
					skipLength = 0;
				}

				fen << '/';
			}
		}

		fen << ' ' << (board.colorToPlay() == pieces::Color::Black ? 'b' : 'w') << ' ';
		const auto cr = board.castlingRights();
		if (cr.CanCastle(pieces::Color::White, pieces::Castle::Short))
		{
			fen << 'K';
		}
		if (cr.CanCastle(pieces::Color::White, pieces::Castle::Long))
		{
			fen << 'Q';
		}
		if (cr.CanCastle(pieces::Color::Black, pieces::Castle::Short))
		{
			fen << 'k';
		}
		if (cr.CanCastle(pieces::Color::Black, pieces::Castle::Long))
		{
			fen << 'q';
		}
		if (cr.value() == 0)
		{
			fen << '-';
		}

		fen << ' ';

		const auto epSquare = board.GetEpSquare();
		fen << (epSquare.IsValid() ? misc::GetSquareName(epSquare) : "-");
		fen << ' ' << board.halfMoves() << ' ' << board.fullMoves();

		return fen.str();
	}

	bool SetFen(Board& board, const std::string_view fen)
	{
		std::string string(fen);
		const char* token = strtok(string.data(), " ");
		std::vector<std::string> split;

		while (token)
		{
			split.emplace_back(token);
			token = strtok(nullptr, " ");
		}

		if (split.size() < 4 || split.size() > 6)
		{
			return false;
		}

		return FenSetter::SetFen(board, split);
	}
}