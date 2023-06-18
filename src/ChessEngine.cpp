#include "ChessEngine.h"
#include <algorithm>
#include <cstring>

Move ChessEngine::killer_moves[2][max_ply];

int ChessEngine::history_moves[12][64] = { 0 };

int ChessEngine::pv_length[max_ply] = { 0 };
Move ChessEngine::pv_table[max_ply][max_ply];

int ChessEngine::count;


int ChessEngine::ply;


HashTable ChessEngine::Trasposition(HASH_SIZE);



    

//this is needed becuase the value of the piece enum is used for rendering too
constexpr static int translator[6] =
{
    5,
    6,
    4,
    2,
    3,
    1
};


int score_move(const Move& move) {
    if (ChessEngine::pv_table[0][ChessEngine::ply] == move) {
        return 100000;
    }

    if (move.getCapturedPiece() != EMPTY) {
        // Compare capture moves
        //translator is just a shortcut i use cuz  i use the vaalue of the piece enum for rendering too, just treat it as the pieces ordered by value
        return (6 - translator[move.getPiece() - 1]) + (translator[move.getCapturedPiece() - 1]) * 100 + 10000;
    }
    else {



        if (ChessEngine::killer_moves[0][ChessEngine::ply] == move) {
            return 9000;
        }
        if (ChessEngine::killer_moves[1][ChessEngine::ply] == move) {
            return 8000;
        }            
        
        return ChessEngine::history_moves[(move.getPiece() - 1) * (move.getColor() + 1)][move.getTo()];

    }


}

int compareMoves(const void* a, const void* b) {
    const Move* moveA = static_cast<const Move*>(a);
    const Move* moveB = static_cast<const Move*>(b);

    const int scoreA = score_move(*moveA);
    const int scoreB = score_move(*moveB);



    // Compare based on move flags
    if (scoreA > scoreB) {
        return -1;
    }
    else if (scoreB > scoreA) {
        return 1;
    }
    return 0;
}


int ChessEngine::quiescence(const Board& board, int alpha, int beta) {
    count++;

    int standPat = board.currentPlayer == WHITE ? board.eval() : -board.eval(); // Evaluate the current position without considering captures or promotions


    if (ply >= max_ply) {
        return standPat;
    }

    if (standPat >= beta) {
        return beta; // Return beta if the standPat score is already greater than or equal to beta for the maximizing player
    }

    alpha = std::max(alpha, standPat); // Update alpha with the maximum value between alpha and standPat for the maximizing player

    auto captures = board.GenerateCaptureMoves(board.currentPlayer); // Generate all capture moves

    qsort(captures.moves, captures.count, sizeof(Move), compareMoves);

    for (int i = 0; i < captures.count; i++) {
        Board newboard = board;
        newboard.movePiece(captures.moves[i]);

        if ((newboard.isKingAttacked(board.currentPlayer))) {
            // Move doesn't result in own king being in check
            continue;

        }

        if (captures.moves[i].getFlags() == QUEEN_CASTLE || captures.moves[i].getFlags() == KING_CASTLE) {
            if (board.isKingAttacked(board.currentPlayer)) {
                continue;                }

        }

        ply++;
        int score = -quiescence(newboard, -beta, -alpha); // Recursively evaluate the capture move with the opposite maximizingPlayer flag
        ply--;

        alpha = std::max(alpha, score); // Update alpha with the maximum value between alpha and score

        if (score >= beta) {
            return beta; // Perform a cutoff if alpha is greater than or equal to beta
        }
    }

    return alpha;
}


const int ChessEngine::reductionLimits = 3;
const int ChessEngine::fullDepthMoves = 4;

#define R 2

int ChessEngine::Minimax(int depth, const Board& board, int alpha, int beta) {
    pv_length[ply] = ply;
    count++;

    int score;

    HashFlags hashFlag = HASH_ALPHA;
    int movesSearched = 0;

    if ((score = Trasposition.ReadHash(board.hashKey, alpha, beta, depth)) != NO_HASH_ENTRY)
    {
        return score;
    }

    bool in_check = board.isKingAttacked(board.currentPlayer);

    depth += in_check;
    if (depth == 0) {
        // Evaluate the current board position and return the evaluation score
        score = quiescence(board, alpha, beta);
        return score;
    }

    if (depth >= R + 1 && !in_check && ply) {
        Board nullBoard = board;

        nullBoard.currentPlayer = nullBoard.currentPlayer == WHITE ? BLACK : WHITE;

        if (nullBoard.enPassantSquare != -1) {
            nullBoard.hashKey ^= Bitboard::enPeasentKeys[nullBoard.enPassantSquare];
        }
        nullBoard.enPassantSquare = -1;

        nullBoard.hashKey ^= Bitboard::SideKey;

        ply += R + 1;
        int score = -Minimax(depth - 1 - R, nullBoard, -beta, -beta + 1);
        ply -= R + 1;

        if (score >= beta)
            return beta;
    }

    auto moves = board.GenerateLegalMoves(board.currentPlayer);

    qsort(moves.moves, moves.count, sizeof(Move), compareMoves);

    if (moves.count == 0) {
        // Handle the case where no legal moves are available
        return board.isKingAttacked(board.currentPlayer) ? MIN_SCORE + ply : 0;
    }


    for (int i = 0; i < moves.count; i++) {
        Board newboard = board;
        newboard.movePiece(moves.moves[i]);

        if ((newboard.isKingAttacked(board.currentPlayer))) {
            // Move doesn't result in own king being in check
            continue;

        }

        if (moves.moves[i].getFlags() == QUEEN_CASTLE || moves.moves[i].getFlags() == KING_CASTLE) {
            if (board.isKingAttacked(board.currentPlayer)) {
                continue;
            }

        }

        if (movesSearched == 0) {
            ply++;
            score = -Minimax(depth - 1, newboard, -beta, -alpha);
            ply--;
        }
        else {
            if (movesSearched >= fullDepthMoves && depth >= reductionLimits && !in_check && moves.moves[i].getFlags() < 4) {
                ply += 2;
                score = -Minimax(depth - 2, newboard, -alpha - 1, -alpha);
                ply -= 2;
            }
            else
                score = alpha + 1;

            if (score > alpha) {
                ply++;
                score = -Minimax(depth - 1, newboard, -alpha - 1, -alpha);
                ply--;

                if (score > alpha && score < beta) {
                    ply++;
                    score = -Minimax(depth - 1, newboard, -beta, -alpha);
                    ply--;
                }
            }
        }

        movesSearched++;

        if (score > alpha) {
            hashFlag = HASH_EXSACT;

            if (moves.moves[i].getCapturedPiece() == EMPTY) {
                history_moves[(moves.moves[i].getPiece() - 1) * (moves.moves[i].getColor() + 1)][moves.moves[i].getTo()] += depth;
            }
            pv_table[ply][ply] = moves.moves[i];

            for (int next_ply = ply + 1; next_ply < pv_length[ply + 1]; next_ply++) {
                pv_table[ply][next_ply] = pv_table[ply + 1][next_ply];
            }

            pv_length[ply] = pv_length[ply + 1];

            alpha = score;
        }

        if (score >= beta) {
            Trasposition.WriteHash(board.hashKey, score, depth, HASH_BETA);
            killer_moves[1][ply] = killer_moves[0][ply];
            killer_moves[0][ply] = moves.moves[i];
            return beta;
        }

    }
    Trasposition.WriteHash(board.hashKey, score, depth, hashFlag);

    return alpha;
}


#define valWINDOW 50

Move ChessEngine::BestMove(double maxTime) {
    auto moves = curBoard->GenerateLegalMoves(curBoard->currentPlayer);
    if (moves.count == 0) {
        // Handle the case where no legal moves are available
        Move empty;
        return empty; // Return an empty move
    }
    clearTables();


    int currentScore = 0;
    int alpha = MIN_SCORE;
    int beta = MAX_SCORE;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 1;currentScore < 9000 && currentScore > -9000 && (std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)).count() < maxTime/4; i++) {

        currentScore = Minimax(i, *curBoard, alpha, beta);

        std::cout << "depth: " << i << ", ";    

        std::cout << "nodes searched: " << count << " ";
        for (int i = 0; i < pv_length[0]; i++) {
            std::cout << pv_table[0][i].to_str() << " ";
        }
        std::cout << " score: " << currentScore << " time: "<< (std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)).count();
        std::cout << std::endl;

        if (currentScore <= alpha || currentScore >= beta) {
            alpha = MIN_SCORE;
            beta = MAX_SCORE;
            continue;
        }

        alpha = currentScore - valWINDOW;
        beta = currentScore + valWINDOW;
    }

    std::cout << std::endl;
    return pv_table[0][0];
}


void ChessEngine::Perft(int depth, const Board& board) {
    count++;
    if (depth == 0) {
        int test = board.eval();
        ++test;
        return;
    }

    auto moves = board.GenerateLegalMoves(board.currentPlayer);

    for (int i = 0; i < moves.count; i++) {
        Board newboard = board;
        newboard.movePiece(moves.moves[i]);
        if ((newboard.isKingAttacked(board.currentPlayer))) {
            // Move doesn't result in own king being in check
            continue;

        }

        if (moves.moves[i].getFlags() == QUEEN_CASTLE || moves.moves[i].getFlags() == KING_CASTLE) {
            if (board.isKingAttacked(board.currentPlayer)) {
                continue;
            }
        }
        Perft(depth-1,newboard);
        // Recursively count the number of nodes at the next depth
    }
}

void ChessEngine::RunPerftTest(int depth) {
    count = 0;
    auto start = std::chrono::high_resolution_clock::now();
    Perft(depth,*curBoard);
    double totaltime = (std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)).count();
    std::cout << "Perft Test Results:\n";
    std::cout << "time: " << totaltime <<std::endl;
    std::cout << "Nodes: " << count << "\n";
    std::cout << "NPS: " << count/totaltime << "\n";
}



void ChessEngine::clearTables() {
    memset(killer_moves, 0, sizeof(killer_moves));
    memset(history_moves, 0, sizeof(history_moves));
    memset(Trasposition.hashTable, 0, sizeof(THash)*Trasposition.size);
    count = 0;
    ply = 0;
}