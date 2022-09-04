//
// Created by matvey on 28.08.22.
//

#include <cstring>
#include "Lookups.h"
#include "Board.h"
#include "Magic.h"

namespace chess::core
{
	namespace
	{
		constexpr std::pair<Bitboard, Bitboard> CalculateAttackedBitboards(const Board& board)
		{
			Bitboard whiteAttackedBB, blackAttackedBB;

			const auto blackOccupancyBB = board.occupancy()
					.WithReset(board.GetKingSquare(pieces::Color::White));
			const auto whiteOccupancyBB = board.occupancy()
					.WithReset(board.GetKingSquare(pieces::Color::Black));

			Square pieceSquares[32];
			const auto end = board.occupancy().BitScanForwardAll(pieceSquares);
			for (auto it = pieceSquares; it != end; it++)
			{
				const auto piece = board.GetPiece(*it);
				const auto occupancyBB = piece.color() == pieces::Color::Black
										 ? blackOccupancyBB : whiteOccupancyBB;
				Bitboard attackedBB;
				switch (piece.type())
				{
				case pieces::Type::Pawn:
					attackedBB = lookups::GetPawnAttacks(*it, piece.color());
					break;
				case pieces::Type::Knight:
					attackedBB = lookups::GetKnightMoves(*it);
					break;
				case pieces::Type::Bishop:
					attackedBB = lookups::GetSliderMoves<pieces::Type::Bishop>(*it, occupancyBB);
					break;
				case pieces::Type::Rook:
					attackedBB = lookups::GetSliderMoves<pieces::Type::Rook>(*it, occupancyBB);
					break;
				case pieces::Type::Queen:
					attackedBB = lookups::GetSliderMoves<pieces::Type::Queen>(*it, occupancyBB);
					break;
				case pieces::Type::King:
					attackedBB = lookups::GetKingMoves(*it);
					break;
				}

				if (piece.color() == pieces::Color::Black)
				{
					blackAttackedBB |= attackedBB;
				}
				else
				{
					whiteAttackedBB |= attackedBB;
				}
			}

			return std::make_pair(blackAttackedBB, whiteAttackedBB);
		}

		PinsInfo CalculatePinsInfo(const Board& board, const pieces::Color us)
		{
			Bitboard diagonalPinnedMoves, orthogonalPinnedMoves;
			Bitboard diagonalPins, orthogonalPins;

			const auto kingSquare = board.GetKingSquare(us);
			const auto them = pieces::OppositeColor(us);

			const auto enemiesBB = board.GetPieces(them);

			auto diagonalKingMoves =
					lookups::GetDiagonalMoves<false>(kingSquare, enemiesBB);
			auto antiDiagonalKingMoves =
					lookups::GetAntiDiagonalMoves<false>(kingSquare, enemiesBB);
			auto fileKingMoves =
					lookups::GetFileMoves<false>(kingSquare, enemiesBB);
			auto rankKingMoves =
					lookups::GetRankMoves<false>(kingSquare, enemiesBB);

			Square diagonalSliders[2], antiDiagonalSliders[2], fileSliders[2], rankSliders[2];

			const auto actualDiagonalSliders =
					board.GetPieces(pieces::Type::Bishop) | board.GetPieces(pieces::Type::Queen);
			const auto actualOrthogonalSliders =
					board.GetPieces(pieces::Type::Rook) | board.GetPieces(pieces::Type::Queen);

			const auto diagonalSliderBB = enemiesBB & diagonalKingMoves & actualDiagonalSliders;
			const auto antiDiagonalSliderBB = enemiesBB & antiDiagonalKingMoves & actualDiagonalSliders;
			const auto fileSliderBB = enemiesBB & fileKingMoves & actualOrthogonalSliders;
			const auto rankSliderBB = enemiesBB & rankKingMoves & actualOrthogonalSliders;

			const auto occupancyBB = board.occupancy();
			diagonalKingMoves = lookups::GetDiagonalMoves<false>(kingSquare, occupancyBB);
			antiDiagonalKingMoves = lookups::GetAntiDiagonalMoves<false>(kingSquare, occupancyBB);
			fileKingMoves = lookups::GetFileMoves<false>(kingSquare, occupancyBB);
			rankKingMoves = lookups::GetRankMoves<false>(kingSquare, occupancyBB);

			const auto diagonalEnd = diagonalSliderBB.BitScanForwardAll(diagonalSliders);
			for (auto it = diagonalSliders; it != diagonalEnd; it++)
			{
				const auto attacks = lookups::GetDiagonalMoves(*it, occupancyBB);
				const auto pin = attacks & diagonalKingMoves;
				if (pin)
				{
					diagonalPins |= pin;
					diagonalPinnedMoves |= attacks | diagonalKingMoves;
					diagonalPinnedMoves.SetAt(*it);
				}
			}

			const auto antiDiagonalEnd = antiDiagonalSliderBB.BitScanForwardAll(antiDiagonalSliders);
			for (auto it = antiDiagonalSliders; it != antiDiagonalEnd; it++)
			{
				const auto attacks = lookups::GetAntiDiagonalMoves(*it, occupancyBB);
				const auto pin = attacks & antiDiagonalKingMoves;
				if (pin)
				{
					diagonalPins |= pin;
					diagonalPinnedMoves |= attacks | antiDiagonalKingMoves;
					diagonalPinnedMoves.SetAt(*it);
				}
			}

			const auto fileEnd = fileSliderBB.BitScanForwardAll(fileSliders);
			for (auto it = fileSliders; it != fileEnd; it++)
			{
				const auto attacks = lookups::GetFileMoves(*it, occupancyBB);
				const auto pin = attacks & fileKingMoves;
				if (pin)
				{
					orthogonalPins |= pin;
					orthogonalPinnedMoves |= attacks | fileKingMoves;
					orthogonalPinnedMoves.SetAt(*it);
				}
			}

			const auto rankEnd = rankSliderBB.BitScanForwardAll(rankSliders);
			for (auto it = rankSliders; it != rankEnd; it++)
			{
				const auto attacks = lookups::GetRankMoves(*it, occupancyBB);
				const auto pin = attacks & rankKingMoves;
				if (pin)
				{
					orthogonalPins |= pin;
					orthogonalPinnedMoves |= attacks | rankKingMoves;
					orthogonalPinnedMoves.SetAt(*it);
				}
			}

			return {
					.DiagonalPins = diagonalPins,
					.OrthogonalPins = orthogonalPins,
					.BishopMoves = diagonalPinnedMoves,
					.RookMoves = orthogonalPinnedMoves
			};
		}
	}

	pieces::Piece Board::RemovePiece(const Square square)
	{
		assert(square.IsValid());

		const auto removedPiece = GetPiece(square);
		if (!removedPiece.IsValid())
		{
			return removedPiece;
		}

		m_Evaluator.FeedRemoveAt(square, removedPiece);
		m_Zobrist.TogglePiece(square, removedPiece);

		m_Pieces[square.value()] = pieces::Piece::Invalid();
		m_PieceCounts[(int)removedPiece.color()][(int)removedPiece.type()]--;

		const auto invBB = ~Bitboard().WithSet(square);

		for (auto& colorBB : m_ColorBBs)
		{
			colorBB &= invBB;
		}

		for (auto& pieceBB : m_PieceBBs)
		{
			pieceBB &= invBB;
		}

		m_OccupancyBB &= invBB;

		return removedPiece;
	}

	template<bool TryRemove>
	void Board::SetPiece(const Square square, const pieces::Piece piece)
	{
		assert(square.IsValid());

		if constexpr (TryRemove)
		{
			const auto removedPiece = RemovePiece(square);
			assert(!removedPiece.IsValid() || removedPiece.color() != piece.color());
		}

		assert(!GetPiece(square).IsValid());

		if (!piece.IsValid())
		{
			return;
		}

		const auto squareBB = Bitboard().WithSet(square);

		m_ColorBBs[(int)piece.color()] |= squareBB;
		m_PieceBBs[(int)piece.type()] |= squareBB;
		m_OccupancyBB |= squareBB;

		m_Pieces[square.value()] = piece;
		m_PieceCounts[(int)piece.color()][(int)piece.type()]++;

		m_Evaluator.FeedSetAt(square, piece);
		m_Zobrist.TogglePiece(square, piece);
	}

	void Board::Clear()
	{
		m_ColorToPlay = {};

		m_ColorBBs.fill({});
		m_PieceBBs.fill({});
		m_Pieces.fill(pieces::Piece::Invalid());
		m_PieceCounts[0].fill(0);
		m_PieceCounts[1].fill(0);

		m_AttackedBBs = {};
		m_OccupancyBB = {};
		m_PinsInfos.fill({});

		m_EpFile = INVALID_FILE;
		m_CastlingRights = {};

		m_HalfMoves = 0;
		m_FullMoves = 0;
		m_EndGameWeight = 0;

		m_Evaluator = {};
		m_Zobrist = {};
	}

	void Board::MakeMove(const moves::Move move)
	{
		auto movingPiece = RemovePiece(move.start());
		assert(movingPiece.IsValid());

		ChangeSidesInternal();

		const auto undoCastlingRights = m_CastlingRights;
		const auto undoHalfMoves = m_HalfMoves++;
		const auto undoEpFile = m_EpFile;

		pieces::Piece capturedPiece;

		if (movingPiece.type() == pieces::Type::Pawn)
		{
			m_HalfMoves = 0;
		}

		int newEpFile = INVALID_FILE;

		switch (move.type())
		{
		case moves::Type::Capture:
		case moves::Type::Quiet:
			break;
		case moves::Type::DoublePawn:
		{
			const auto file = move.end().file();
			const auto rank = move.end().rank();
			const auto them = pieces::OppositeColor(movingPiece.color());
			const auto enemyPawn = pieces::Piece(them, pieces::Type::Pawn);
			const bool hasAdjacentEnemyPawn =
					(file > 0 && GetPiece(Square(file - 1, rank)) == enemyPawn) ||
							(file < BOARD_SIZE - 1 && GetPiece(Square(file + 1, rank)) == enemyPawn);

			newEpFile = hasAdjacentEnemyPawn ? file : INVALID_FILE;
			break;
		}
		case moves::Type::LongCastle:
		{
			const auto rook = RemovePiece(pieces::GetCastleRookStart(movingPiece.color(), pieces::Castle::Long));
			SetPiece<false>(pieces::GetCastleRookEnd(movingPiece.color(), pieces::Castle::Long), rook);
			SetPiece<false>(pieces::GetCastleKingEnd(movingPiece.color(), pieces::Castle::Long), movingPiece);
			break;
		}
		case moves::Type::ShortCastle:
		{
			const auto rook =
					RemovePiece(pieces::GetCastleRookStart(movingPiece.color(), pieces::Castle::Short));
			SetPiece<false>(pieces::GetCastleRookEnd(movingPiece.color(), pieces::Castle::Short), rook);
			SetPiece<false>(pieces::GetCastleKingEnd(movingPiece.color(), pieces::Castle::Short), movingPiece);
			break;
		}
		case moves::Type::BishopPC:
		case moves::Type::BishopPQ:
			movingPiece = movingPiece.Promote(pieces::Type::Bishop);
			break;
		case moves::Type::RookPC:
		case moves::Type::RookPQ:
			movingPiece = movingPiece.Promote(pieces::Type::Rook);
			break;
		case moves::Type::KnightPC:
		case moves::Type::KnightPQ:
			movingPiece = movingPiece.Promote(pieces::Type::Knight);
			break;
		case moves::Type::QueenPC:
		case moves::Type::QueenPQ:
			movingPiece = movingPiece.Promote(pieces::Type::Queen);
			break;
		case moves::Type::EnPassant:
			capturedPiece = RemovePiece(moves::GetEnPassantCapturedPawnSquare(move));
			break;
		}

		if (move.type() == moves::Type::Capture ||
				(moves::Type::BishopPC <= move.type() && move.type() <= moves::Type::QueenPC))
		{
			capturedPiece = RemovePiece(move.end());
			m_HalfMoves = 0;
			RecalculateEndGameWeight();
		}

		if (move.type() != moves::Type::ShortCastle && move.type() != moves::Type::LongCastle)
		{
			SetPiece<false>(move.end(), movingPiece);
		}
		UpdateCastlingRights(move, movingPiece, capturedPiece);

		if (movingPiece.color() == pieces::Color::Black)
		{
			m_FullMoves++;
		}

		assert(capturedPiece.type() != pieces::Type::King);

		std::array<Bitboard, pieces::COLORS> undoAttackedBBs{ m_AttackedBBs };
		std::array<PinsInfo, pieces::COLORS> undoPinsInfos{ m_PinsInfos };

		SetEpFileInternal(newEpFile);

		m_MoveHistory.push_back(
				MoveUndoInfo{
						.Move = move,
						.CapturedPiece = capturedPiece,
						.EpFile = undoEpFile,
						.HalfMoves = undoHalfMoves,
						.CastlingRights = undoCastlingRights,
						.CheckersBB = m_CheckersBB,
						.Pins = undoPinsInfos,
						.AttackedBBs = undoAttackedBBs,
				});

		UpdateBitboards();
	}

	void Board::UndoMove()
	{
		assert(!m_MoveHistory.empty());

		const auto undoInfo = m_MoveHistory.back();
		m_MoveHistory.pop_back();

		ChangeSidesInternal();
		SetEpFileInternal(undoInfo.EpFile);
		SetCastlingRightsInternal(undoInfo.CastlingRights);

		m_CheckersBB = undoInfo.CheckersBB;
		std::copy(undoInfo.AttackedBBs.begin(), undoInfo.AttackedBBs.end(), m_AttackedBBs.begin());
		std::copy(undoInfo.Pins.begin(), undoInfo.Pins.end(), m_PinsInfos.begin());

		m_HalfMoves = undoInfo.HalfMoves;

		const auto move = undoInfo.Move;
		const auto capturedPiece = undoInfo.CapturedPiece;

		pieces::Piece movedPiece;
		if (move.type() == moves::Type::LongCastle)
		{
			movedPiece = RemovePiece(pieces::GetCastleKingEnd(colorToPlay(), pieces::Castle::Long));
		}
		else if (move.type() == moves::Type::ShortCastle)
		{
			movedPiece = RemovePiece(pieces::GetCastleKingEnd(colorToPlay(), pieces::Castle::Short));
		}
		else
		{
			movedPiece = RemovePiece(move.end());
		}
		assert(movedPiece.IsValid());

		if (movedPiece.color() == pieces::Color::Black)
		{
			m_FullMoves--;
		}

		switch (move.type())
		{
		case moves::Type::Quiet:
		case moves::Type::DoublePawn:
		case moves::Type::Capture:
			break;
		case moves::Type::LongCastle:
		{
			const auto rook = RemovePiece(
					pieces::GetCastleRookEnd(movedPiece.color(), pieces::Castle::Long));
			SetPiece<false>(pieces::GetCastleRookStart(movedPiece.color(), pieces::Castle::Long), rook);
			break;
		}
		case moves::Type::ShortCastle:
		{
			const auto rook = RemovePiece(
					pieces::GetCastleRookEnd(movedPiece.color(), pieces::Castle::Short));
			SetPiece<false>(pieces::GetCastleRookStart(movedPiece.color(), pieces::Castle::Short), rook);
			break;
		}
		case moves::Type::BishopPC:
		case moves::Type::RookPC:
		case moves::Type::KnightPC:
		case moves::Type::QueenPC:
		case moves::Type::BishopPQ:
		case moves::Type::RookPQ:
		case moves::Type::KnightPQ:
		case moves::Type::QueenPQ:
			movedPiece = movedPiece.Demote();
			break;
		case moves::Type::EnPassant:
			SetPiece<false>(moves::GetEnPassantCapturedPawnSquare(move), capturedPiece);
			break;
		}

		if (move.type() == moves::Type::Capture ||
				(moves::Type::BishopPC <= move.type() && move.type() <= moves::Type::QueenPC))
		{
			SetPiece<false>(move.end(), capturedPiece);
			RecalculateEndGameWeight();
		}

		SetPiece<false>(move.start(), movedPiece);
	}

	void Board::ChangeSidesInternal()
	{
		m_Zobrist.ToggleColorToPlay();
		m_ColorToPlay = (pieces::Color)(1 - (int)m_ColorToPlay);
	}

	void Board::SetCastlingRightsInternal(const pieces::CastlingRights cr)
	{
		m_Zobrist.ToggleCastlingRights(m_CastlingRights);
		m_CastlingRights = cr;
		m_Zobrist.ToggleCastlingRights(m_CastlingRights);
	}

	void Board::SetEpFileInternal(const int file)
	{
		assert(file == INVALID_FILE || (0 <= file && file < BOARD_SIZE));

		m_Zobrist.ToggleEpFile(m_EpFile);
		m_EpFile = file;
		m_Zobrist.ToggleEpFile(m_EpFile);
	}

	void Board::UpdateCastlingRights(const moves::Move move,
			const pieces::Piece movingPiece, const pieces::Piece capturedPiece)
	{
		auto cr = m_CastlingRights;

		if (capturedPiece.IsValid())
		{
			if (pieces::GetCastleRookStart(capturedPiece.color(), pieces::Castle::Short) == move.end())
			{
				cr.DisallowCastle(capturedPiece.color(), pieces::Castle::Short);
			}
			else if (pieces::GetCastleRookStart(capturedPiece.color(), pieces::Castle::Long) == move.end())
			{
				cr.DisallowCastle(capturedPiece.color(), pieces::Castle::Long);
			}
		}

		if (movingPiece.type() == pieces::Type::King)
		{
			cr.DisallowCastle(movingPiece.color(), pieces::Castle::Short);
			cr.DisallowCastle(movingPiece.color(), pieces::Castle::Long);
		}
		else if (movingPiece.type() == pieces::Type::Rook)
		{
			if (pieces::GetCastleRookStart(movingPiece.color(), pieces::Castle::Short) == move.start())
			{
				cr.DisallowCastle(movingPiece.color(), pieces::Castle::Short);
			}
			else if (pieces::GetCastleRookStart(movingPiece.color(), pieces::Castle::Long) == move.start())
			{
				cr.DisallowCastle(movingPiece.color(), pieces::Castle::Long);
			}
		}

		SetCastlingRightsInternal(cr);
	}

	void Board::UpdateBitboards()
	{
//		const auto attackedBBs = CalculateAttackedBitboards(*this);
//		m_AttackedBBs[(int)pieces::Color::Black] = attackedBBs.first;
//		m_AttackedBBs[(int)pieces::Color::White] = attackedBBs.second;
//
//		m_PinsInfos[(int)pieces::Color::Black] = CalculatePinsInfo(*this, pieces::Color::Black);
//		m_PinsInfos[(int)pieces::Color::White] = CalculatePinsInfo(*this, pieces::Color::White);

		m_CheckersBB = GetAttackedBy(GetKingSquare(colorToPlay()));
	}

	Board Board::CloneWithoutHistory() const
	{
		Board board{ *this };
		board.m_MoveHistory = std::vector<MoveUndoInfo>();
		assert(&m_MoveHistory != &board.m_MoveHistory);
		return board;
	}

	void Board::RecalculateEndGameWeight()
	{
		const auto queensCount =
				GetPieceCount(pieces::Color::White, pieces::Type::Queen) +
						GetPieceCount(pieces::Color::Black, pieces::Type::Queen);
		const auto rooksCount =
				GetPieceCount(pieces::Color::White, pieces::Type::Rook) +
						GetPieceCount(pieces::Color::Black, pieces::Type::Rook);
		const auto minorPiecesCount =
				GetPieceCount(pieces::Color::White, pieces::Type::Bishop) +
						GetPieceCount(pieces::Color::White, pieces::Type::Knight) +
						GetPieceCount(pieces::Color::Black, pieces::Type::Bishop) +
						GetPieceCount(pieces::Color::Black, pieces::Type::Knight);

		m_EndGameWeight = -70 * queensCount + (2 - rooksCount) * 30 + (4 - minorPiecesCount) * 20;
	}

	Bitboard Board::GetAttackedBy(const Square square) const
	{
		Bitboard attackedByBB;

		const auto occupancyBB = occupancy();
		const auto them = pieces::OppositeColor(colorToPlay());

		attackedByBB |= lookups::GetAttackingKnights(square) & GetPieces(pieces::Type::Knight);
		attackedByBB |= lookups::GetAttackingPawns(square, them) & GetPieces(pieces::Type::Pawn);

		const auto bishopMoves = lookups::GetSliderMoves<pieces::Type::Bishop>(square, occupancyBB);
		const auto rookMoves = lookups::GetSliderMoves<pieces::Type::Rook>(square, occupancyBB);

		attackedByBB |= GetPieces(pieces::Type::Bishop) & bishopMoves;
		attackedByBB |= GetPieces(pieces::Type::Rook) & rookMoves;
		attackedByBB |= GetPieces(pieces::Type::Queen) & (rookMoves | bishopMoves);

		return attackedByBB & GetPieces(them);
	}

	bool Board::IsAttacked(const Square square) const
	{
		const auto occupancyBB = occupancy();
		const pieces::Color them = pieces::OppositeColor(colorToPlay());
		const auto themBB = GetPieces(them);

		if (lookups::GetAttackingKnights(square) & GetPieces(pieces::Type::Knight) & themBB)
		{
			return true;
		}
		if (lookups::GetAttackingPawns(square, them) & GetPieces(pieces::Type::Pawn))
		{
			return true;
		}

		const auto queensBB = GetPieces(pieces::Type::Queen);
		if (lookups::GetSliderMoves<pieces::Type::Bishop>(square, occupancyBB) &
				(GetPieces(pieces::Type::Bishop) | queensBB) & themBB)
		{
			return true;
		}

		if (lookups::GetSliderMoves<pieces::Type::Rook>(square, occupancyBB) &
				(GetPieces(pieces::Type::Rook) | queensBB) & themBB)
		{
			return true;
		}

		return false;
	}

	bool Board::IsLegal(const moves::Move move) const
	{
		auto* nonConstThis = const_cast<Board*>(this);

		assert(move.IsValid());
		assert(move.type() != moves::Type::LongCastle && move.type() != moves::Type::ShortCastle);

		const auto undoMovedPiece = nonConstThis->RemovePiece(move.start());
		const auto undoCaptureSquare =
				move.type() == moves::Type::EnPassant ? moves::GetEnPassantCapturedPawnSquare(move) : move.end();
		const auto undoCapturedPiece = nonConstThis->RemovePiece(undoCaptureSquare);

		nonConstThis->SetPiece<false>(move.end(), undoMovedPiece);

		const bool isLegal = !IsAttacked(GetKingSquare(colorToPlay()));

		nonConstThis->SetPiece<false>(move.start(), undoMovedPiece);
		nonConstThis->RemovePiece(move.end());
		nonConstThis->SetPiece<false>(undoCaptureSquare, undoCapturedPiece);

		return isLegal;
	}

	template void Board::SetPiece<false>(chess::core::Square, pieces::Piece);
	template void Board::SetPiece<true>(chess::core::Square, pieces::Piece);
}