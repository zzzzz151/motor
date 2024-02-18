#ifndef MOTOR_UCI_HPP
#define MOTOR_UCI_HPP

#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <fstream>

#include "../chess_board/board.hpp"
#include "../move_generation/move_list.hpp"
#include "../move_generation/move_generator.hpp"
#include "../search/time_keeper.hpp"
#include "../search/search.hpp"
#include "../search/bench.hpp"

bool parse_move(board & b, const std::string& move_string) {
    move_list ml;
    if (b.get_side() == White) {
        generate_all_moves<White, false>(b, ml);
    } else {
        generate_all_moves<Black, false>(b, ml);
    }

    for (const chess_move & m : ml) {
        if (m.to_string() == move_string) {
            if (b.get_side() == White) {
                make_move<White>(b, m);
            } else {
                make_move<Black>(b, m);
            }
            return true;
        }
    }
    return false;
}

void position_uci(board & b, const std::string & command) {
    // Initialize from FEN string
    int fen_pos = command.find("fen");
    int moves_pos = command.find("moves");

    if (command.find("startpos") != std::string::npos) {
        b.fen_to_board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    } else if (command.find("fen") != std::string::npos) {
        std::string fen = command.substr(fen_pos + 3, moves_pos);
        b.fen_to_board(fen);
    }

    if (moves_pos == std::string::npos) return;

    std::stringstream move_ss(command.substr(moves_pos + 5));
    std::vector<std::string> moves;
    std::string string_move;

    while (move_ss >> string_move) {
        moves.push_back(string_move);
    }

    for (const std::string & m : moves) {
        if (!parse_move(b, m)) return;
    }

    set_position(b);
}

void uci_go(board& b, const std::string& command) {
    std::stringstream ss(command);
    std::string token;
    std::vector<std::string> tokens;
    while (ss >> token) {
        tokens.push_back(token);
    }

    time_info info;

    for (unsigned int i = 0; i < tokens.size(); i += 2) {
        if (tokens[i] == "wtime") {
            info.wtime = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "btime") {
            info.btime = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "winc") {
            info.winc = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "binc") {
            info.binc = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "movestogo") {
            info.movestogo = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "depth") {
            info.max_depth = std::stoi(tokens[i + 1]); 
        } else if (tokens[i] == "movetime") {
            //info.movetime = std::stoi(tokens[i + 1]);  // NOT SUPPORTED RIGHT NOW
        } else if (tokens[i] == "infinite") {
            //info.infinite = true;                      // NOT SUPPORTED RIGHT NOW
        }
    }

    find_best_move(b, info);
}

void uci_process(board& b, const std::string& line) {
    std::stringstream ss(line);
    std::string command;

    ss >> command;

    if (command == "position") {
        position_uci(b, line.substr(9));
    }
    else if (command == "go") {
        uci_go(b, line.substr(3));
    }
    else if (command == "exit" || command == "quit" || command == "end") {
        exit(0);
    }
    else if (command == "isready") {
        std::cout << "readyok" << std::endl;
    }
    else if (command == "uci") {
        std::cout << "id name Motor"<< std::endl;
        std::cout << "id author Martin Novak" << std::endl;      
        std::cout << "option name Hash type spin default " << 8 << " min 1 max 1048576\n";
        std::cout << "uciok" << std::endl;
    }
    else if (command == "ucinewgame") {
        history_table.clear();
        tt.clear();
    }
    else if (command == "setoption") {
        std::string token;
        std::vector<std::string> tokens;
        while (ss >> token) {
            tokens.push_back(token);
        }

        if (tokens.size() >= 4) {
            if (tokens[1] == "hash") {
                tt.resize(std::stoi(tokens[3]) * 1024 * 1024);
            }
        } else {
            std::cout << "Command not found." << std::endl;
        }
    }
    else if (command == "bench") {
        bench(15);
    }
}

void uci_mainloop() {
    board chessboard;
    std::string line{};

    while (std::getline(std::cin, line)) {
        uci_process(chessboard, line);
    }
}

#endif //MOTOR_UCI_HPP