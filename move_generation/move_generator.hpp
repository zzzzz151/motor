#ifndef MOTOR_MOVE_GENERATOR_HPP
#define MOTOR_MOVE_GENERATOR_HPP

#include "move_list.hpp"
#include "../chess_board/board.hpp"

std::uint64_t get_checkmask(std::uint64_t checkers, Square square)
{
    if(checkers == 0) {
        return full_board;
    }

    Square square_of_checker = pop_lsb(checkers);

    if(checkers) {
        return 0ull;
    }

    return pinmask[square][square_of_checker];
}

template <Color our_color, bool captures_only, bool vertical_discovery, bool nonvertical_discovery>
void generate_pawn_moves(const board & chessboard, move_list & movelist, std::uint64_t pawn_bitboard, std::uint64_t checkmask,
                         std::uint64_t move_v, std::uint64_t move_a, std::uint64_t move_d, std::uint64_t enemy, std::uint64_t empty)
{
    constexpr Color their_color = (our_color == White) ? Black : White;
    constexpr std::uint64_t penultimate_rank = (our_color == White ? ranks[RANK_7] : ranks[RANK_2]);
    constexpr std::uint64_t not_penultimate_rank = (our_color == White) ? 0xFF00FFFFFFFFFFFFull : 0xFFFFFFFFFFFF00FFull;
    constexpr std::uint64_t enpassant_rank = (our_color == White ? ranks[RANK_3] : ranks[RANK_6]);
    constexpr Direction up   = (our_color == White ? NORTH   : SOUTH);
    constexpr Direction up_2 = (our_color == White ? NORTH_2 : SOUTH_2);
    constexpr Direction antidiagonal_capture   = (our_color == White ? NORTH_WEST : SOUTH_EAST);
    constexpr Direction diagonal_capture       = (our_color == White ? NORTH_EAST : SOUTH_WEST);
    constexpr Direction antidiagonal_enpassant = (our_color == Black ? NORTH_WEST : SOUTH_EAST);
    constexpr Direction diagonal_enpassant     = (our_color == Black ? NORTH_EAST : SOUTH_WEST);

    constexpr bool discovery = (vertical_discovery || nonvertical_discovery);

    const std::uint64_t pawns_penultimate     = pawn_bitboard & penultimate_rank;
    const std::uint64_t pawns_not_penultimate = pawn_bitboard & not_penultimate_rank;

    const std::uint64_t blocking_squares = empty & checkmask;
    const std::uint64_t threat_squares   = enemy & checkmask;

    // PAWN PUSH
    if constexpr (!captures_only) {
        std::uint64_t bitboard_push_1 = shift<up>(pawns_not_penultimate & move_v) & empty;
        std::uint64_t bitboard_push_2 = shift<up>(bitboard_push_1 & enpassant_rank) & blocking_squares;

        bitboard_push_1 &= checkmask;

        while (bitboard_push_1) {
            Square pawn_square = pop_lsb(bitboard_push_1);
            movelist.add(chess_move(pawn_square - up, pawn_square, MoveType::QUIET, nonvertical_discovery));
        }

        while (bitboard_push_2) {
            Square pawn_square = pop_lsb(bitboard_push_2);
            movelist.add(chess_move(pawn_square - up_2, pawn_square, MoveType::DOUBLE_PAWN_PUSH, nonvertical_discovery));
        }
    }

    // PAWNS PROMOTE
    if (pawns_penultimate)
    {
        std::uint64_t bitboard_promote_push = shift<up>(pawns_penultimate & move_v) & blocking_squares;
        std::uint64_t bitboard_promote_antidiagonal = shift<antidiagonal_capture>(pawns_penultimate & move_a) & threat_squares;
        std::uint64_t bitboard_promote_diagonal = shift<diagonal_capture>(pawns_penultimate & move_d) & threat_squares;

        while (bitboard_promote_push) {
            Square pawn_square = pop_lsb(bitboard_promote_push);
            movelist.add(chess_move(static_cast<Square>(pawn_square - up), pawn_square, QUEEN_PROMOTION, nonvertical_discovery));
            if constexpr (!captures_only) {
                movelist.add(chess_move(pawn_square - up, pawn_square, ROOK_PROMOTION, nonvertical_discovery));
                movelist.add(chess_move(pawn_square - up, pawn_square, BISHOP_PROMOTION, nonvertical_discovery));
                movelist.add(chess_move(pawn_square - up, pawn_square, KNIGHT_PROMOTION, nonvertical_discovery));
            }
        }

        while (bitboard_promote_antidiagonal) {
            Square pawn_square = pop_lsb(bitboard_promote_antidiagonal);
            movelist.add(chess_move(pawn_square - antidiagonal_capture, pawn_square, QUEEN_PROMOTION_CAPTURE, discovery));
            if constexpr (!captures_only) {
                movelist.add(chess_move(pawn_square - antidiagonal_capture, pawn_square, ROOK_PROMOTION_CAPTURE, discovery));
                movelist.add(chess_move(pawn_square - antidiagonal_capture, pawn_square, BISHOP_PROMOTION_CAPTURE, discovery));
                movelist.add(chess_move(pawn_square - antidiagonal_capture, pawn_square, KNIGHT_PROMOTION_CAPTURE, discovery));
            }
        }

        while (bitboard_promote_diagonal) {
            Square pawn_square = pop_lsb(bitboard_promote_diagonal);
            movelist.add(chess_move(pawn_square - diagonal_capture, pawn_square, QUEEN_PROMOTION_CAPTURE, discovery));
            if constexpr (!captures_only) {
                movelist.add(chess_move(pawn_square - diagonal_capture, pawn_square, ROOK_PROMOTION_CAPTURE, discovery));
                movelist.add(chess_move(pawn_square - diagonal_capture, pawn_square, BISHOP_PROMOTION_CAPTURE, discovery));
                movelist.add(chess_move(pawn_square - diagonal_capture, pawn_square, KNIGHT_PROMOTION_CAPTURE, discovery));
            }
        }
    }

    // PAWN CAPTURES
    // Pawn captures can give "all types" of discovery attacks (diagonal capture cannot give diagonal discovery but that pawn wouldn't even be in discovery square)
    std::uint64_t bitboard_left_capture = shift<antidiagonal_capture>(pawns_not_penultimate & move_a) & threat_squares;
    std::uint64_t bitboard_right_capture = shift<diagonal_capture>(pawns_not_penultimate & move_d) & threat_squares;

    while(bitboard_left_capture) {
        Square pawn_square = pop_lsb(bitboard_left_capture);
        movelist.add(chess_move(pawn_square - antidiagonal_capture, pawn_square, CAPTURE, discovery));
    }

    while (bitboard_right_capture) {
        Square pawn_square = pop_lsb(bitboard_right_capture);
        movelist.add(chess_move(pawn_square - diagonal_capture, pawn_square, CAPTURE, discovery));
    }

    // EN PASSANT
    // I set discovery automatically on discovery but it doesnt have to be true (checking would be too expensive)
    Square enpassant_square = chessboard.enpassant_square();
    if (enpassant_square != Square::Null_Square)
    {
        std::uint64_t enpassant_bitboard = (1ull << enpassant_square);
        std::uint64_t enpassant_antidiagonal = shift<antidiagonal_capture>(pawns_not_penultimate & move_a) & enpassant_bitboard;
        if(enpassant_antidiagonal && chessboard.check_legality_of_enpassant<their_color>(enpassant_square - antidiagonal_capture, enpassant_square - up)) {
            movelist.add(chess_move(enpassant_square - antidiagonal_capture, enpassant_square, EN_PASSANT, discovery));
        }
        std::uint64_t enpassant_diagonal = shift<diagonal_capture>(pawns_not_penultimate & move_d) & enpassant_bitboard;
        if(enpassant_diagonal && chessboard.check_legality_of_enpassant<their_color>(enpassant_square - diagonal_capture, enpassant_square - up)) {
            movelist.add(chess_move(enpassant_square - diagonal_capture, enpassant_square, EN_PASSANT, discovery));
        }
    }
}

template <bool captures_only, bool discovery>
constexpr void generate_knight_moves(const board & b, move_list & ml, std::uint64_t knight_bitboard, std::uint64_t target, std::uint64_t enemy, std::uint64_t empty, std::uint64_t occupancy)
{
    while(knight_bitboard) {
        Square square = pop_lsb(knight_bitboard);
        std::uint64_t attack_bitboard = KNIGHT_ATTACKS[square] & target;

        std::uint64_t captures = attack_bitboard & enemy;
        while(captures) {
            Square target_square = pop_lsb(captures);
            ml.add(chess_move(square, target_square, CAPTURE, discovery));
        }

        if constexpr (!captures_only) {
            std::uint64_t quiets = attack_bitboard & empty;
            while (quiets) {
                Square target_square = pop_lsb(quiets);
                ml.add(chess_move(square, target_square, QUIET, discovery));
            }
        }
    }
}

template <bool captures_only>
constexpr void generate_king_moves(const board & b, move_list & ml, Square king_square, std::uint64_t safe_squares, std::uint64_t empty, std::uint64_t enemy, std::uint64_t discovery_squares)
{
    std::uint64_t king_moves = KING_ATTACKS[king_square] & safe_squares;

    // king has to be on discovery square and has to move outside of discovery square
    bool king_can_discover = (1ull << king_square) & discovery_squares;

    std::uint64_t king_captures = king_moves & enemy;

    // to give discovery check king has to move outside of discovery ray
    std::uint64_t king_discovery_captures = king_captures & ~discovery_squares;

    while(king_discovery_captures) {
        Square target_square = pop_lsb(king_discovery_captures);
        ml.add(chess_move(king_square, target_square, CAPTURE, king_can_discover));
    }

    king_captures &= discovery_squares;
    while (king_captures) {
        Square target_square = pop_lsb(king_captures);
        ml.add(chess_move(king_square, target_square, CAPTURE));
    }

    if constexpr (!captures_only) {
        std::uint64_t king_quiets = king_moves & empty;
        std::uint64_t quiet_discovery = king_quiets & ~discovery_squares;

        while(quiet_discovery) {
            Square target_square = pop_lsb(quiet_discovery);
            ml.add(chess_move(king_square, target_square, QUIET, king_can_discover));
        }

        king_quiets &= discovery_squares;
        while (king_quiets) {
            Square target_square = pop_lsb(king_quiets);
            ml.add(chess_move(king_square, target_square, QUIET));
        }
    }
}

template <Color our_color>
constexpr void generate_castle_moves(move_list & ml, int castling_right, std::uint64_t safe_squares, std::uint64_t empty) {
    constexpr Square king_e_square = our_color == White ? E1 : E8;
    constexpr Square king_g_square = our_color == White ? G1 : G8;
    constexpr Square king_c_square = our_color == White ? C1 : C8;

    constexpr std::uint64_t kingside_castle_efg_mask  = our_color == White ? 0x70ull : 0x7000000000000000ull;
    constexpr std::uint64_t kingside_castle_fg_mask   = our_color == White ? 0x60ull : 0x6000000000000000ull;
    constexpr std::uint64_t queenside_castle_cde_mask = our_color == White ? 0x1cull : 0x1c00000000000000ull;
    constexpr std::uint64_t queenside_castle_bcd_mask = our_color == White ? 0xeull  : 0xe00000000000000ull;

    constexpr int kingside_castle_right  = our_color == White ? CASTLE_WHITE_KINGSIDE  : CASTLE_BLACK_KINGSIDE;
    constexpr int queenside_castle_right = our_color == White ? CASTLE_WHITE_QUEENSIDE : CASTLE_BLACK_QUEENSIDE;

    if(castling_right & kingside_castle_right
       && (safe_squares & kingside_castle_efg_mask) == kingside_castle_efg_mask
       && (empty & kingside_castle_fg_mask) == kingside_castle_fg_mask)
    {
        ml.add(chess_move(king_e_square, king_g_square, KING_CASTLE));
    }

    if(castling_right & queenside_castle_right
       && (safe_squares & queenside_castle_cde_mask) == queenside_castle_cde_mask
       && (empty & queenside_castle_bcd_mask) == queenside_castle_bcd_mask)
    {
        ml.add(chess_move(king_e_square, king_c_square, QUEEN_CASTLE));
    }
}

template <Ray ray, bool captures_only, bool discovery>
constexpr void generate_slider_moves(const board & b, move_list& ml, std::uint64_t piece_bitboard, std::uint64_t checkmask, std::uint64_t enemy, std::uint64_t empty, std::uint64_t occupancy)
{
    while (piece_bitboard) {
        Square square = pop_lsb(piece_bitboard);
        std::uint64_t attack_bitboard = attacks<ray>(square, occupancy) & checkmask;

        std::uint64_t captures = attack_bitboard & enemy;

        while (captures) {
            Square target_square = pop_lsb(captures);
            ml.add(chess_move(square, target_square, CAPTURE, discovery));
        }

        if constexpr (!captures_only) {
            std::uint64_t quiets = attack_bitboard & empty;
            while (quiets) {
                Square target_square = pop_lsb(quiets);
                ml.add(chess_move(square, target_square, QUIET, discovery));
            }
        }
    }
}


template <Color our_color, bool captures_only>
constexpr bool generate_all_moves(board & b, move_list & ml) {
    constexpr Color enemy_color = our_color == White ? Black : White;

    Square king_square = b.get_king_square();
    const std::uint64_t enemy_pieces = b.get_enemy_bitboard<enemy_color>();
    const std::uint64_t occupancy = b.get_occupancy();
    const std::uint64_t empty = ~occupancy;
    const std::uint64_t checkers = b.attackers<enemy_color>(king_square);

    const std::uint64_t safe_squares = b.get_safe_squares<enemy_color>(king_square);

    const std::uint64_t checkmask = get_checkmask(checkers, king_square);

    auto [discover_h, discover_v, discover_a, discover_d] = b.get_discovering_pieces<our_color, enemy_color>();

    uint64_t discover_hv = discover_h | discover_v;
    uint64_t discover_ad = discover_a | discover_d;
    uint64_t discovery_squares = discover_ad | discover_hv;

    generate_king_moves<captures_only>(b, ml, king_square, safe_squares, empty, enemy_pieces, discovery_squares);

    const auto [pin_h, pin_v, pin_a, pin_d] = b.get_pinners<our_color, enemy_color>(king_square);

    std::uint64_t pin_hv = pin_h | pin_v;
    std::uint64_t pin_ad = pin_a | pin_d;
    std::uint64_t non_pinned_pieces = ~(pin_ad | pin_hv);

    std::uint64_t rook_bitboard   = b.get_pieces(our_color, Rook);
    std::uint64_t bishop_bitboard = b.get_pieces(our_color, Bishop);
    std::uint64_t queen_bitboard  = b.get_pieces(our_color, Queen);
    std::uint64_t knight_bitboard = b.get_pieces(our_color, Knight);
    std::uint64_t pawn_bitboard   = b.get_pieces(our_color, Pawn);

    rook_bitboard   &= ~pin_ad; // rooks pinned diagonally cannot move
    bishop_bitboard &= ~pin_hv; // bishops pinned horizontally and vertically cannot move
    pawn_bitboard   &= ~pin_h;  // pawns pinned horizontally cannot move
    knight_bitboard &= non_pinned_pieces; // pinned knights cannot move

    std::uint64_t h_checkmask = checkmask & pin_h;
    std::uint64_t v_checkmask = checkmask & pin_v;
    std::uint64_t a_checkmask = checkmask & pin_a;
    std::uint64_t d_checkmask = checkmask & pin_d;

    // non-pinned rooks
    std::uint64_t nonpinned_rooks = rook_bitboard & ~pin_hv;
    generate_slider_moves<Ray::ROOK, captures_only, false>(b, ml, nonpinned_rooks & ~discover_ad, checkmask,enemy_pieces, empty, occupancy);
    generate_slider_moves<Ray::ROOK, captures_only, true >(b, ml, nonpinned_rooks &  discover_ad, checkmask,enemy_pieces, empty, occupancy);

    // horizontally/vertically pinned rooks
    std::uint64_t horizontally_pinned_rooks = rook_bitboard & pin_h;
    std::uint64_t vertically_pinned_rooks = rook_bitboard & pin_v;
    generate_slider_moves<Ray::HORIZONTAL, captures_only, false>(b, ml, horizontally_pinned_rooks &~discover_ad, h_checkmask,enemy_pieces, empty, occupancy);
    generate_slider_moves<Ray::VERTICAL, captures_only, false>(b, ml, vertically_pinned_rooks &~discover_ad, v_checkmask,enemy_pieces, empty, occupancy);

    generate_slider_moves<Ray::HORIZONTAL, captures_only, true>(b, ml, horizontally_pinned_rooks & discover_ad, h_checkmask,enemy_pieces, empty, occupancy);
    generate_slider_moves<Ray::VERTICAL, captures_only, true>(b, ml, vertically_pinned_rooks & discover_ad, v_checkmask,enemy_pieces, empty, occupancy);
    // non-pinned bishops
    std::uint64_t nonpinned_bishops = bishop_bitboard & ~pin_ad;
    generate_slider_moves<Ray::BISHOP, captures_only, false>(b, ml, nonpinned_bishops & ~discover_hv, checkmask,enemy_pieces, empty, occupancy);
    generate_slider_moves<Ray::BISHOP, captures_only, true>(b, ml, nonpinned_bishops &  discover_hv, checkmask,enemy_pieces, empty, occupancy);

    // antidiagonally/diagonally pinned bishops
    std::uint64_t antidiagonally_pinned_bishops = bishop_bitboard & pin_a;
    std::uint64_t diagonally_pinned_bishops = bishop_bitboard & pin_d;
    generate_slider_moves<Ray::ANTIDIAGONAL, captures_only, false>(b, ml, antidiagonally_pinned_bishops & ~discover_hv,a_checkmask, enemy_pieces, empty,occupancy);
    generate_slider_moves<Ray::DIAGONAL, captures_only, false>(b, ml, diagonally_pinned_bishops & ~discover_hv, d_checkmask,enemy_pieces, empty, occupancy);

    generate_slider_moves<Ray::ANTIDIAGONAL, captures_only, true>(b, ml, antidiagonally_pinned_bishops & discover_hv,a_checkmask, enemy_pieces, empty,occupancy);
    generate_slider_moves<Ray::DIAGONAL, captures_only, true>(b, ml, diagonally_pinned_bishops & discover_hv, d_checkmask,enemy_pieces, empty, occupancy);

    // non-pinned queens
    // queen cannot give discovery check
    generate_slider_moves<Ray::QUEEN,  captures_only, false>(b, ml, queen_bitboard & non_pinned_pieces,checkmask, enemy_pieces, empty, occupancy);
    generate_slider_moves<Ray::HORIZONTAL, captures_only, false>(b, ml, queen_bitboard & pin_h, h_checkmask,enemy_pieces, empty, occupancy);
    generate_slider_moves<Ray::VERTICAL, captures_only, false>(b, ml, queen_bitboard & pin_v, v_checkmask,enemy_pieces, empty, occupancy);
    generate_slider_moves<Ray::ANTIDIAGONAL, captures_only, false>(b, ml, queen_bitboard & pin_a,a_checkmask, enemy_pieces, empty,occupancy);
    generate_slider_moves<Ray::DIAGONAL, captures_only, false>(b, ml, queen_bitboard & pin_d, d_checkmask,enemy_pieces, empty, occupancy);

    generate_knight_moves<captures_only, false>(b, ml, knight_bitboard & ~discovery_squares, checkmask, enemy_pieces, empty, occupancy);
    generate_knight_moves<captures_only, true>(b, ml, knight_bitboard &  discovery_squares, checkmask, enemy_pieces, empty, occupancy);

    generate_pawn_moves<our_color, captures_only, false, false>(b, ml, pawn_bitboard & ~discovery_squares, checkmask, ~pin_ad, ~(pin_d | pin_v),~(pin_a | pin_v), enemy_pieces, empty);
    generate_pawn_moves<our_color, captures_only, true, false>(b, ml, pawn_bitboard & discover_v, checkmask, ~pin_ad, ~(pin_d | pin_v),~(pin_a | pin_v), enemy_pieces, empty);
    generate_pawn_moves<our_color, captures_only, false, true>(b, ml, pawn_bitboard & (discover_h | discover_ad), checkmask, ~pin_ad, ~(pin_d | pin_v),~(pin_a | pin_v), enemy_pieces, empty);

    if constexpr (!captures_only) {
        generate_castle_moves<our_color>(ml, b.get_castle_rights(), safe_squares, empty);
    }

    return checkmask == full_board;
}

#endif //MOTOR_MOVE_GENERATOR_HPP
