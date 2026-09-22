#include "fen.hpp"

#include <cctype>
#include <optional>
#include <sstream>
#include <vector>

namespace chess {

namespace {

std::optional<Piece> piece_from_letter(char letter) {
	Color color = std::isupper(static_cast<unsigned char>(letter)) ? Color::White : Color::Black;
	switch (std::tolower(static_cast<unsigned char>(letter))) {
		case 'p':
			return Piece{ color, PieceType::Pawn };
		case 'n':
			return Piece{ color, PieceType::Knight };
		case 'b':
			return Piece{ color, PieceType::Bishop };
		case 'r':
			return Piece{ color, PieceType::Rook };
		case 'q':
			return Piece{ color, PieceType::Queen };
		case 'k':
			return Piece{ color, PieceType::King };
		default:
			return std::nullopt;
	}
}

char piece_letter(PieceType type, Color color) {
	const char *letters = color == Color::White ? "PNBRQK" : "pnbrqk";
	switch (type) {
		case PieceType::Pawn:
			return letters[0];
		case PieceType::Knight:
			return letters[1];
		case PieceType::Bishop:
			return letters[2];
		case PieceType::Rook:
			return letters[3];
		case PieceType::Queen:
			return letters[4];
		case PieceType::King:
			return letters[5];
	}
	return '?';
}

std::string square_name(const Square &square) {
	return std::string(1, char('a' + square.file)) + std::string(1, char('1' + square.rank));
}

bool parse_int(const std::string &text, int &out) {
	if (text.empty()) {
		return false;
	}
	for (char ch : text) {
		if (!std::isdigit(static_cast<unsigned char>(ch))) {
			return false;
		}
	}
	out = std::stoi(text);
	return true;
}

} // namespace

std::string GameState::to_fen() const {
	std::string placement;
	for (int rank = 7; rank >= 0; --rank) {
		int empty = 0;
		for (int file = 0; file < 8; ++file) {
			auto piece = board_.at(Square{ int8_t(file), int8_t(rank) });
			if (!piece) {
				empty++;
				continue;
			}
			if (empty > 0) {
				placement += std::to_string(empty);
				empty = 0;
			}
			placement += piece_letter(piece->type, piece->color);
		}
		if (empty > 0) {
			placement += std::to_string(empty);
		}
		if (rank > 0) {
			placement += '/';
		}
	}

	std::string fen = placement;
	fen += turn_ == Color::White ? " w " : " b ";

	std::string castling;
	if (rights_.white_king_side) {
		castling += 'K';
	}
	if (rights_.white_queen_side) {
		castling += 'Q';
	}
	if (rights_.black_king_side) {
		castling += 'k';
	}
	if (rights_.black_queen_side) {
		castling += 'q';
	}
	fen += castling.empty() ? "-" : castling;
	fen += ' ';
	fen += ep_ ? square_name(*ep_) : "-";
	fen += ' ';
	fen += std::to_string(halfmove_);
	fen += ' ';
	fen += std::to_string(fullmove_);
	return fen;
}

bool GameState::load_fen(const std::string &fen) {
	board_.clear();
	rights_ = CastlingRights{};
	ep_ = std::nullopt;
	halfmove_ = 0;
	fullmove_ = 1;
	turn_ = Color::White;
	history_.clear();

	std::istringstream stream(fen);
	std::vector<std::string> fields;
	std::string token;
	while (stream >> token) {
		fields.push_back(token);
	}
	if (fields.size() < 4) {
		return false;
	}

	int rank = 7;
	int file = 0;
	int white_kings = 0;
	int black_kings = 0;
	for (char ch : fields[0]) {
		if (ch == '/') {
			if (file != 8 || rank <= 0) {
				return false;
			}
			rank--;
			file = 0;
		} else if (ch >= '1' && ch <= '8') {
			file += ch - '0';
			if (file > 8) {
				return false;
			}
		} else {
			auto piece = piece_from_letter(ch);
			if (!piece || file > 7) {
				return false;
			}
			if (piece->type == PieceType::King) {
				if (piece->color == Color::White) {
					white_kings++;
				} else {
					black_kings++;
				}
			}
			board_.set_piece(Square{ int8_t(file), int8_t(rank) }, *piece);
			file++;
		}
	}
	if (rank != 0 || file != 8 || white_kings != 1 || black_kings != 1) {
		return false;
	}

	if (fields[1] == "w") {
		turn_ = Color::White;
	} else if (fields[1] == "b") {
		turn_ = Color::Black;
	} else {
		return false;
	}

	if (fields[2] != "-") {
		for (char ch : fields[2]) {
			switch (ch) {
				case 'K':
					rights_.white_king_side = true;
					break;
				case 'Q':
					rights_.white_queen_side = true;
					break;
				case 'k':
					rights_.black_king_side = true;
					break;
				case 'q':
					rights_.black_queen_side = true;
					break;
				default:
					return false;
			}
		}
	}

	if (fields[3] != "-") {
		if (fields[3].size() != 2 || fields[3][0] < 'a' || fields[3][0] > 'h' || fields[3][1] < '1' ||
				fields[3][1] > '8') {
			return false;
		}
		ep_ = Square{ int8_t(fields[3][0] - 'a'), int8_t(fields[3][1] - '1') };
	}

	if (fields.size() >= 5 && !parse_int(fields[4], halfmove_)) {
		return false;
	}
	if (fields.size() >= 6 && !parse_int(fields[5], fullmove_)) {
		return false;
	}
	return true;
}

} // namespace chess
