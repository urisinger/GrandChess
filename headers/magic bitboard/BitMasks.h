#pragma once
#include "misc.h"

int countBits(uint64_t);
int getLSB(uint64_t value);
void print_BitBoard(uint64_t BitBoards);

class Masks {
public:
    static uint64_t bishopMasks[64];
    static uint64_t rookMasks[64];
    static uint64_t queenAttack[64];
    static uint64_t knightAttack[64];
    static uint64_t kingAttack[64];

    static uint64_t bishopMagic[64];
    static uint64_t rookMagic[64];

    static uint64_t bishopAttacks[64][512];
    static uint64_t rookAttacks[64][4096];

    static uint64_t pieceKeys[12][64];
    static uint64_t enPeasentKeys[64];
    static uint64_t CastleKeys[16];

    static uint64_t SideKey;



    static void initBitmasks() {
        generateBishopBitmasks();
        generateRookBitmasks();
        generateKnightBitmasks();
        generateKingBitmasks();

        generateBishopAttacks();
        generateRookAttacks();

        generateHashKeys(); 
    }

    static void generateHashKeys();

    static void generateBishopBitmasks();

    static void generateKingBitmasks();

    static void generateRookBitmasks();

    static void generateBishopAttacks();

    static void generateRookAttacks();


    static void generateKnightBitmasks();

    static void generateBishopMagicNumbers();

    static void generateRookMagicNumbers();

private:
    static uint64_t getBishopAttacks(int square, uint64_t occupancy);
    static uint64_t getRookAttacks(int square, uint64_t occupancy);
    static uint64_t generateMagicNumber(int square, bool is_bishop);
};