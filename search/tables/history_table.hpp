#ifndef MOTOR_BUTTERFLY_TABLE_HPP
#define MOTOR_BUTTERFLY_TABLE_HPP

#include <array>
#include "../../chess_board/board.hpp"
#include "../../chess_board/chess_move.hpp"
#include "../../move_generation/move_list.hpp"
#include "../search_data.hpp"

std::array<std::array<std::array<int, 64>, 64>, 2> history_table = {};
std::array<std::array<std::array<std::array<int, 64>, 6>, 64>, 6> continuation_table = {};

int history_bonus(int depth) {
    return 160 * depth;
}

void update_history(int& value, int bonus) {
    value += bonus - (value * std::abs(bonus) / 16384);
}

template <Color color, bool is_root>
void update_quiet_history(search_data & data, board & chessboard, const chess_move & best_move, move_list & quiets, int depth) {
    int bonus = history_bonus(depth);

    auto [piece, from, to] = data.prev_moves[data.get_ply()];
    history_move prev = {};

    update_history(history_table[color][from][to], bonus);

    if constexpr (!is_root) {
        prev = data.prev_moves[data.get_ply() - 1];
        update_history(continuation_table[prev.piece_type][prev.to][piece][to], bonus);
    }

    int index = 0;
    for (const auto& quiet : quiets) {
        index++;
        int malus = -bonus;
        update_history(history_table[color][quiet.get_from()][quiet.get_to()], malus);

        if constexpr (!is_root) {
            update_history(continuation_table[prev.piece_type][prev.to][chessboard.get_piece(quiet.get_from())][quiet.get_to()], malus);
        }
    }
}

template <Color color>
int get_history(search_data & data, Square from, Square to, Piece piece) {
    int move_score = history_table[color][from][to];
    if (data.get_ply()) {
        auto prev = data.prev_moves[data.get_ply() - 1];
        move_score += continuation_table[prev.piece_type][prev.to][piece][to];
    }
    return move_score;
}

#endif //MOTOR_BUTTERFLY_TABLE_HPP
