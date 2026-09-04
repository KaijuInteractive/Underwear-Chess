#include "raylib.h"
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <limits>

// ============================================================
// UNDIES CHESS v1.0
// Kaiju Interactive
//
// PAWN   = Briefs
// ROOK   = Long Sock
// KNIGHT = Short Sock
// BISHOP = Jock
// QUEEN  = Boxers
// KING   = Boxer Briefs
//
// FULL RULES:
// - Legal movement
// - Check
// - Checkmate
// - Stalemate
// - Self-check prevention
// - Castling
// - En passant
// - Promotion choice
// ============================================================

const int BOARD_SIZE = 8;
const int TILE_SIZE = 90;

const int BOARD_X = 40;
const int BOARD_Y = 40;

const int SCREEN_WIDTH = 1120;
const int SCREEN_HEIGHT = 800;

// ============================================================
// ENUMS
// ============================================================

enum PieceType
{
    EMPTY,
    PAWN,
    ROOK,
    KNIGHT,
    BISHOP,
    QUEEN,
    KING
};

enum PieceColor
{
    NONE,
    WHITE_SIDE,
    BLACK_SIDE
};

// ============================================================
// STRUCTS
// ============================================================

struct Piece
{
    PieceType type = EMPTY;
    PieceColor color = NONE;
    bool hasMoved = false;
};

struct BoardPosition
{
    int row;
    int col;
};

struct PieceTextures
{
    Texture2D whitePawn;
    Texture2D whiteRook;
    Texture2D whiteKnight;
    Texture2D whiteBishop;
    Texture2D whiteQueen;
    Texture2D whiteKing;

    Texture2D blackPawn;
    Texture2D blackRook;
    Texture2D blackKnight;
    Texture2D blackBishop;
    Texture2D blackQueen;
    Texture2D blackKing;
};

// ============================================================
// GLOBAL GAME STATE
// ============================================================

Piece board[BOARD_SIZE][BOARD_SIZE];

PieceColor currentTurn = WHITE_SIDE;

int selectedRow = -1;
int selectedCol = -1;

bool pieceSelected = false;

std::vector<BoardPosition> legalMoves;

int moveNumber = 1;

std::string lastMove = "Game started";

bool gameOver = false;
bool stalemate = false;

PieceColor winner = NONE;

// ============================================================
// RELEASE / MENU / AI STATE
// ============================================================

enum ScreenState { SPLASH_SCREEN, TITLE_SCREEN, MODE_SCREEN, DIFFICULTY_SCREEN, OPTIONS_SCREEN, GAME_SCREEN };
enum GameMode { TWO_PLAYER, VS_COMPUTER };
enum AIDifficulty { LAUNDRY_DAY, CASUAL_FRIDAY, TIGHT_FIT, COMMANDO, JOCKED };

ScreenState screenState = SPLASH_SCREEN;
GameMode gameMode = VS_COMPUTER;
AIDifficulty aiDifficulty = LAUNDRY_DAY;
PieceColor humanColor = WHITE_SIDE;
PieceColor aiColor = BLACK_SIDE;
bool aiThinking = false;
double aiMoveAt = 0.0;
double splashStartTime = 0.0;
bool quitRequested = false;

// ============================================================
// SOUND EFFECTS
// Generated in code, so no extra .wav files are required.
// ============================================================

Sound moveSound = {};
Sound captureSound = {};
Sound menuMusic = {};
bool audioReady = false;
bool suppressMoveSounds = false;
bool sfxEnabled = true;
bool musicEnabled = true;
float masterVolume = 0.70f;


// ============================================================
// MOVE HISTORY / REVIEW MODE
// ============================================================

struct MoveHistoryEntry
{
    Piece b[BOARD_SIZE][BOARD_SIZE];
    PieceColor turn = WHITE_SIDE;
    int moveNo = 1;
    int epRow = -1;
    int epCol = -1;
    bool over = false;
    bool stale = false;
    PieceColor win = NONE;
    std::string text = "Game started";
    int sr = -1, sc = -1, er = -1, ec = -1;
    PieceColor mover = NONE;
};

std::vector<MoveHistoryEntry> moveHistory;
bool reviewMode = false;
int reviewIndex = 0;
bool suppressMoveHistory = false;

// Promotion is one move split across MovePiece() and PromotePawn(), so
// remember its origin until the player/AI chooses the promoted piece.
int pendingHistorySr = -1;
int pendingHistorySc = -1;
int pendingHistoryEr = -1;
int pendingHistoryEc = -1;
PieceColor pendingHistoryMover = NONE;

void ResetMoveHistory();
void RecordMoveHistory(int sr, int sc, int er, int ec, PieceColor mover);

Sound CreateToneSound(float frequency, float duration, float volume)
{
    const unsigned int sampleRate = 44100;
    const unsigned int frameCount = (unsigned int)(sampleRate * duration);

    short* samples = (short*)MemAlloc(frameCount * sizeof(short));

    for (unsigned int i = 0; i < frameCount; i++)
    {
        float t = (float)i / (float)sampleRate;
        float progress = (float)i / (float)frameCount;
        float envelope = 1.0f - progress;

        // A tiny second harmonic gives the click a little more character.
        float sample =
            std::sin(2.0f * PI * frequency * t) * 0.82f +
            std::sin(2.0f * PI * frequency * 2.0f * t) * 0.18f;

        sample *= envelope * volume;
        samples[i] = (short)(sample * 32767.0f);
    }

    Wave wave = {};
    wave.frameCount = frameCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = samples;

    Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave);

    return sound;
}

Sound CreateMenuMusic()
{
    const unsigned int sampleRate = 44100;
    const float duration = 8.0f;
    const unsigned int frameCount = (unsigned int)(sampleRate * duration);
    short* samples = (short*)MemAlloc(frameCount * sizeof(short));

    const float notes[] = { 261.63f, 329.63f, 392.00f, 329.63f, 293.66f, 349.23f, 440.00f, 349.23f };
    const int noteCount = 8;

    for (unsigned int i = 0; i < frameCount; i++)
    {
        float t = (float)i / (float)sampleRate;
        int noteIndex = ((int)(t / 1.0f)) % noteCount;
        float local = std::fmod(t, 1.0f);
        float freq = notes[noteIndex];
        float envelope = 0.55f * (1.0f - local * 0.45f);

        float pad =
            std::sin(2.0f * PI * freq * t) * 0.55f +
            std::sin(2.0f * PI * (freq * 0.5f) * t) * 0.25f +
            std::sin(2.0f * PI * (freq * 1.5f) * t) * 0.10f;

        samples[i] = (short)(pad * envelope * 0.10f * 32767.0f);
    }

    Wave wave = {};
    wave.frameCount = frameCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = samples;

    Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return sound;
}

void ApplyAudioSettings()
{
    if (!audioReady) return;
    SetSoundVolume(moveSound, masterVolume * 0.55f);
    SetSoundVolume(captureSound, masterVolume * 0.72f);
    SetSoundVolume(menuMusic, masterVolume * 0.42f);
}

void InitGameAudio()
{
    if (!IsAudioDeviceReady())
        return;

    // Soft wooden-ish click for an ordinary move.
    moveSound = CreateToneSound(520.0f, 0.055f, 0.34f);

    // Lower, heavier click for captures.
    captureSound = CreateToneSound(190.0f, 0.090f, 0.46f);

    // Tiny procedural menu loop -- no external audio asset needed.
    menuMusic = CreateMenuMusic();

    audioReady = true;
    ApplyAudioSettings();
}

void PlayPieceMoveSound(bool capture)
{
    if (!audioReady || suppressMoveSounds || !sfxEnabled)
        return;

    if (capture)
        PlaySound(captureSound);
    else
        PlaySound(moveSound);
}

void UnloadGameAudio()
{
    if (!audioReady)
        return;

    UnloadSound(moveSound);
    UnloadSound(captureSound);
    UnloadSound(menuMusic);
    audioReady = false;
}


// ------------------------------------------------------------
// EN PASSANT
//
// This stores the square BEHIND a pawn that just moved two
// squares. An enemy pawn may capture into this square on the
// immediately following move.
// ------------------------------------------------------------

int enPassantRow = -1;
int enPassantCol = -1;

// ------------------------------------------------------------
// PROMOTION
// ------------------------------------------------------------

bool promotionPending = false;

int promotionRow = -1;
int promotionCol = -1;

// ============================================================
// BASIC HELPERS
// ============================================================

bool IsInsideBoard(int row, int col)
{
    return
        row >= 0 &&
        row < BOARD_SIZE &&
        col >= 0 &&
        col < BOARD_SIZE;
}

// ------------------------------------------------------------

PieceColor OppositeColor(PieceColor color)
{
    if (color == WHITE_SIDE)
        return BLACK_SIDE;

    if (color == BLACK_SIDE)
        return WHITE_SIDE;

    return NONE;
}

// ------------------------------------------------------------

const char* GetColorName(PieceColor color)
{
    if (color == WHITE_SIDE)
        return "WHITE";

    if (color == BLACK_SIDE)
        return "BLACK";

    return "";
}

// ------------------------------------------------------------

std::string GetPieceName(PieceType type)
{
    switch (type)
    {
    case PAWN:
        return "Briefs";

    case ROOK:
        return "Long Sock";

    case KNIGHT:
        return "Short Sock";

    case BISHOP:
        return "Jock";

    case QUEEN:
        return "Boxers";

    case KING:
        return "Boxer Briefs";

    default:
        return "Nothing";
    }
}

// ------------------------------------------------------------

std::string GetChessPieceName(PieceType type)
{
    switch (type)
    {
    case PAWN:
        return "Pawn";

    case ROOK:
        return "Rook";

    case KNIGHT:
        return "Knight";

    case BISHOP:
        return "Bishop";

    case QUEEN:
        return "Queen";

    case KING:
        return "King";

    default:
        return "Empty";
    }
}

// ------------------------------------------------------------

char GetPieceLetter(PieceType type)
{
    switch (type)
    {
    case PAWN:
        return 'P';

    case ROOK:
        return 'R';

    case KNIGHT:
        return 'N';

    case BISHOP:
        return 'B';

    case QUEEN:
        return 'Q';

    case KING:
        return 'K';

    default:
        return ' ';
    }
}

// ------------------------------------------------------------

std::string GetSquareName(int row, int col)
{
    char file = 'a' + col;
    char rank = '8' - row;

    std::string result;

    result += file;
    result += rank;

    return result;
}


void CopyBoardToHistoryEntry(MoveHistoryEntry& e)
{
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
            e.b[r][c] = board[r][c];

    e.turn = currentTurn;
    e.moveNo = moveNumber;
    e.epRow = enPassantRow;
    e.epCol = enPassantCol;
    e.over = gameOver;
    e.stale = stalemate;
    e.win = winner;
    e.text = lastMove;
}

void ResetMoveHistory()
{
    moveHistory.clear();
    reviewMode = false;
    reviewIndex = 0;

    MoveHistoryEntry start;
    CopyBoardToHistoryEntry(start);
    start.text = "Game started";
    moveHistory.push_back(start);
}

void RecordMoveHistory(int sr, int sc, int er, int ec, PieceColor mover)
{
    if (suppressMoveHistory)
        return;

    MoveHistoryEntry e;
    CopyBoardToHistoryEntry(e);
    e.sr = sr;
    e.sc = sc;
    e.er = er;
    e.ec = ec;
    e.mover = mover;
    moveHistory.push_back(e);
    reviewIndex = (int)moveHistory.size() - 1;
}

void LoadHistoryEntryForDisplay(const MoveHistoryEntry& e)
{
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
            board[r][c] = e.b[r][c];

    currentTurn = e.turn;
    moveNumber = e.moveNo;
    enPassantRow = e.epRow;
    enPassantCol = e.epCol;
    gameOver = e.over;
    stalemate = e.stale;
    winner = e.win;
    lastMove = e.text;

    pieceSelected = false;
    selectedRow = selectedCol = -1;
    legalMoves.clear();
}

// ============================================================
// BOARD SETUP
// ============================================================

void ClearBoard()
{
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            board[row][col] = {};
        }
    }
}

// ------------------------------------------------------------

void SetupBackRank(int row, PieceColor color)
{
    board[row][0] = { ROOK, color, false };
    board[row][1] = { KNIGHT, color, false };
    board[row][2] = { BISHOP, color, false };
    board[row][3] = { QUEEN, color, false };
    board[row][4] = { KING, color, false };
    board[row][5] = { BISHOP, color, false };
    board[row][6] = { KNIGHT, color, false };
    board[row][7] = { ROOK, color, false };
}

// ------------------------------------------------------------

void ResetGame()
{
    ClearBoard();

    SetupBackRank(0, BLACK_SIDE);

    for (int col = 0; col < BOARD_SIZE; col++)
    {
        board[1][col] =
        {
            PAWN,
            BLACK_SIDE,
            false
        };
    }

    SetupBackRank(7, WHITE_SIDE);

    for (int col = 0; col < BOARD_SIZE; col++)
    {
        board[6][col] =
        {
            PAWN,
            WHITE_SIDE,
            false
        };
    }

    currentTurn = WHITE_SIDE;

    selectedRow = -1;
    selectedCol = -1;

    pieceSelected = false;

    legalMoves.clear();

    moveNumber = 1;

    lastMove = "Game started";

    gameOver = false;
    stalemate = false;

    winner = NONE;

    enPassantRow = -1;
    enPassantCol = -1;

    promotionPending = false;
    promotionRow = -1;
    promotionCol = -1;

    pendingHistorySr = pendingHistorySc = -1;
    pendingHistoryEr = pendingHistoryEc = -1;
    pendingHistoryMover = NONE;

    ResetMoveHistory();
}

// ============================================================
// PATH CHECKING
// ============================================================

bool IsPathClear(
    int startRow,
    int startCol,
    int endRow,
    int endCol)
{
    int rowStep = 0;
    int colStep = 0;

    if (endRow > startRow)
        rowStep = 1;
    else if (endRow < startRow)
        rowStep = -1;

    if (endCol > startCol)
        colStep = 1;
    else if (endCol < startCol)
        colStep = -1;

    int row = startRow + rowStep;
    int col = startCol + colStep;

    while (row != endRow ||
        col != endCol)
    {
        if (board[row][col].type != EMPTY)
        {
            return false;
        }

        row += rowStep;
        col += colStep;
    }

    return true;
}

// ============================================================
// ATTACK DETECTION
// ============================================================

bool PieceAttacksSquare(
    int startRow,
    int startCol,
    int targetRow,
    int targetCol)
{
    Piece piece =
        board[startRow][startCol];

    if (piece.type == EMPTY)
    {
        return false;
    }

    int rowDifference =
        targetRow - startRow;

    int colDifference =
        targetCol - startCol;

    int absRow =
        std::abs(rowDifference);

    int absCol =
        std::abs(colDifference);

    switch (piece.type)
    {
    case PAWN:
    {
        int direction =
            piece.color == WHITE_SIDE
            ? -1
            : 1;

        return
            rowDifference == direction &&
            absCol == 1;
    }

    case KNIGHT:

        return
            (absRow == 2 && absCol == 1)
            ||
            (absRow == 1 && absCol == 2);

    case BISHOP:

        if (absRow != absCol)
            return false;

        return IsPathClear(
            startRow,
            startCol,
            targetRow,
            targetCol
        );

    case ROOK:

        if (startRow != targetRow &&
            startCol != targetCol)
        {
            return false;
        }

        return IsPathClear(
            startRow,
            startCol,
            targetRow,
            targetCol
        );

    case QUEEN:
    {
        bool straight =
            startRow == targetRow ||
            startCol == targetCol;

        bool diagonal =
            absRow == absCol;

        if (!straight && !diagonal)
        {
            return false;
        }

        return IsPathClear(
            startRow,
            startCol,
            targetRow,
            targetCol
        );
    }

    case KING:

        return
            absRow <= 1 &&
            absCol <= 1 &&
            !(absRow == 0 &&
                absCol == 0);

    default:

        return false;
    }
}

// ------------------------------------------------------------

bool IsSquareAttacked(
    int row,
    int col,
    PieceColor attackingColor)
{
    for (int testRow = 0;
        testRow < BOARD_SIZE;
        testRow++)
    {
        for (int testCol = 0;
            testCol < BOARD_SIZE;
            testCol++)
        {
            Piece piece =
                board[testRow][testCol];

            if (piece.type == EMPTY)
                continue;

            if (piece.color != attackingColor)
                continue;

            if (PieceAttacksSquare(
                testRow,
                testCol,
                row,
                col))
            {
                return true;
            }
        }
    }

    return false;
}

// ============================================================
// KING DETECTION
// ============================================================

bool FindKing(
    PieceColor color,
    int& kingRow,
    int& kingCol)
{
    for (int row = 0;
        row < BOARD_SIZE;
        row++)
    {
        for (int col = 0;
            col < BOARD_SIZE;
            col++)
        {
            if (board[row][col].type == KING &&
                board[row][col].color == color)
            {
                kingRow = row;
                kingCol = col;

                return true;
            }
        }
    }

    return false;
}

// ------------------------------------------------------------

bool IsKingInCheck(PieceColor color)
{
    int kingRow = -1;
    int kingCol = -1;

    if (!FindKing(
        color,
        kingRow,
        kingCol))
    {
        return false;
    }

    return IsSquareAttacked(
        kingRow,
        kingCol,
        OppositeColor(color)
    );
}

// ============================================================
// CASTLING
// ============================================================

bool CanCastle(
    PieceColor color,
    bool kingSide)
{
    int row =
        color == WHITE_SIDE
        ? 7
        : 0;

    Piece king =
        board[row][4];

    if (king.type != KING ||
        king.color != color ||
        king.hasMoved)
    {
        return false;
    }

    // Can't castle while already in check
    if (IsKingInCheck(color))
    {
        return false;
    }

    PieceColor enemy =
        OppositeColor(color);

    if (kingSide)
    {
        Piece rook =
            board[row][7];

        if (rook.type != ROOK ||
            rook.color != color ||
            rook.hasMoved)
        {
            return false;
        }

        // Squares between King and rook
        if (board[row][5].type != EMPTY ||
            board[row][6].type != EMPTY)
        {
            return false;
        }

        // King cannot cross or land on attacked squares
        if (IsSquareAttacked(
            row,
            5,
            enemy) ||
            IsSquareAttacked(
                row,
                6,
                enemy))
        {
            return false;
        }

        return true;
    }
    else
    {
        Piece rook =
            board[row][0];

        if (rook.type != ROOK ||
            rook.color != color ||
            rook.hasMoved)
        {
            return false;
        }

        if (board[row][1].type != EMPTY ||
            board[row][2].type != EMPTY ||
            board[row][3].type != EMPTY)
        {
            return false;
        }

        if (IsSquareAttacked(
            row,
            3,
            enemy) ||
            IsSquareAttacked(
                row,
                2,
                enemy))
        {
            return false;
        }

        return true;
    }
}

// ============================================================
// PSEUDO-LEGAL PIECE MOVEMENT
// ============================================================

bool IsPawnPseudoLegal(
    int startRow,
    int startCol,
    int endRow,
    int endCol)
{
    Piece piece =
        board[startRow][startCol];

    int direction =
        piece.color == WHITE_SIDE
        ? -1
        : 1;

    int startingRow =
        piece.color == WHITE_SIDE
        ? 6
        : 1;

    int rowDifference =
        endRow - startRow;

    int colDifference =
        endCol - startCol;

    // --------------------------------------------------------
    // ONE SQUARE FORWARD
    // --------------------------------------------------------

    if (colDifference == 0 &&
        rowDifference == direction &&
        board[endRow][endCol].type == EMPTY)
    {
        return true;
    }

    // --------------------------------------------------------
    // TWO SQUARES FROM STARTING RANK
    // --------------------------------------------------------

    if (colDifference == 0 &&
        startRow == startingRow &&
        !piece.hasMoved &&
        rowDifference == direction * 2 &&
        board[endRow][endCol].type == EMPTY)
    {
        int middleRow =
            startRow + direction;

        if (board[middleRow][startCol].type ==
            EMPTY)
        {
            return true;
        }
    }

    // --------------------------------------------------------
    // NORMAL CAPTURE
    // --------------------------------------------------------

    if (std::abs(colDifference) == 1 &&
        rowDifference == direction)
    {
        Piece destination =
            board[endRow][endCol];

        if (destination.type != EMPTY &&
            destination.color != piece.color &&
            destination.type != KING)
        {
            return true;
        }

        // ----------------------------------------------------
        // EN PASSANT
        // ----------------------------------------------------

        if (destination.type == EMPTY &&
            endRow == enPassantRow &&
            endCol == enPassantCol)
        {
            int capturedPawnRow =
                endRow - direction;

            Piece capturedPawn =
                board[capturedPawnRow][endCol];

            if (capturedPawn.type == PAWN &&
                capturedPawn.color != piece.color)
            {
                return true;
            }
        }
    }

    return false;
}

// ------------------------------------------------------------

bool IsRookPseudoLegal(
    int startRow,
    int startCol,
    int endRow,
    int endCol)
{
    if (startRow != endRow &&
        startCol != endCol)
    {
        return false;
    }

    return IsPathClear(
        startRow,
        startCol,
        endRow,
        endCol
    );
}

// ------------------------------------------------------------

bool IsKnightPseudoLegal(
    int startRow,
    int startCol,
    int endRow,
    int endCol)
{
    int rowDifference =
        std::abs(endRow - startRow);

    int colDifference =
        std::abs(endCol - startCol);

    return
        (rowDifference == 2 &&
            colDifference == 1)
        ||
        (rowDifference == 1 &&
            colDifference == 2);
}

// ------------------------------------------------------------

bool IsBishopPseudoLegal(
    int startRow,
    int startCol,
    int endRow,
    int endCol)
{
    int rowDifference =
        std::abs(endRow - startRow);

    int colDifference =
        std::abs(endCol - startCol);

    if (rowDifference != colDifference)
    {
        return false;
    }

    return IsPathClear(
        startRow,
        startCol,
        endRow,
        endCol
    );
}

// ------------------------------------------------------------

bool IsQueenPseudoLegal(
    int startRow,
    int startCol,
    int endRow,
    int endCol)
{
    bool straight =
        startRow == endRow ||
        startCol == endCol;

    bool diagonal =
        std::abs(endRow - startRow) ==
        std::abs(endCol - startCol);

    if (!straight &&
        !diagonal)
    {
        return false;
    }

    return IsPathClear(
        startRow,
        startCol,
        endRow,
        endCol
    );
}

// ------------------------------------------------------------

bool IsKingPseudoLegal(
    int startRow,
    int startCol,
    int endRow,
    int endCol)
{
    Piece king =
        board[startRow][startCol];

    int rowDifference =
        std::abs(endRow - startRow);

    int colDifference =
        std::abs(endCol - startCol);

    // Normal king movement
    if (rowDifference <= 1 &&
        colDifference <= 1 &&
        !(rowDifference == 0 &&
            colDifference == 0))
    {
        return true;
    }

    // --------------------------------------------------------
    // CASTLING
    // --------------------------------------------------------

    int homeRow =
        king.color == WHITE_SIDE
        ? 7
        : 0;

    if (startRow == homeRow &&
        startCol == 4 &&
        endRow == homeRow)
    {
        // King-side castle
        if (endCol == 6)
        {
            return CanCastle(
                king.color,
                true
            );
        }

        // Queen-side castle
        if (endCol == 2)
        {
            return CanCastle(
                king.color,
                false
            );
        }
    }

    return false;
}

// ============================================================
// MASTER PSEUDO-LEGAL CHECK
// ============================================================

bool IsPseudoLegalMove(
    int startRow,
    int startCol,
    int endRow,
    int endCol)
{
    if (!IsInsideBoard(
        startRow,
        startCol))
    {
        return false;
    }

    if (!IsInsideBoard(
        endRow,
        endCol))
    {
        return false;
    }

    if (startRow == endRow &&
        startCol == endCol)
    {
        return false;
    }

    Piece movingPiece =
        board[startRow][startCol];

    Piece destination =
        board[endRow][endCol];

    if (movingPiece.type == EMPTY)
    {
        return false;
    }

    if (destination.type != EMPTY &&
        destination.color == movingPiece.color)
    {
        return false;
    }

    // Kings cannot be captured
    if (destination.type == KING)
    {
        return false;
    }

    switch (movingPiece.type)
    {
    case PAWN:

        return IsPawnPseudoLegal(
            startRow,
            startCol,
            endRow,
            endCol
        );

    case ROOK:

        return IsRookPseudoLegal(
            startRow,
            startCol,
            endRow,
            endCol
        );

    case KNIGHT:

        return IsKnightPseudoLegal(
            startRow,
            startCol,
            endRow,
            endCol
        );

    case BISHOP:

        return IsBishopPseudoLegal(
            startRow,
            startCol,
            endRow,
            endCol
        );

    case QUEEN:

        return IsQueenPseudoLegal(
            startRow,
            startCol,
            endRow,
            endCol
        );

    case KING:

        return IsKingPseudoLegal(
            startRow,
            startCol,
            endRow,
            endCol
        );

    default:

        return false;
    }
}

// ============================================================
// REAL LEGAL MOVE CHECK
//
// Temporarily performs the move, including en passant and
// castling, then checks whether our own King would be attacked.
// ============================================================

bool IsMoveLegal(
    int startRow,
    int startCol,
    int endRow,
    int endCol)
{
    if (!IsPseudoLegalMove(
        startRow,
        startCol,
        endRow,
        endCol))
    {
        return false;
    }

    Piece movingPiece =
        board[startRow][startCol];

    Piece destinationPiece =
        board[endRow][endCol];

    bool enPassantMove = false;
    Piece enPassantCapturedPiece = {};
    int enPassantCapturedRow = -1;

    bool castlingMove = false;
    Piece rookPiece = {};
    Piece rookDestinationPiece = {};
    int rookStartCol = -1;
    int rookEndCol = -1;

    // --------------------------------------------------------
    // DETECT EN PASSANT
    // --------------------------------------------------------

    if (movingPiece.type == PAWN &&
        startCol != endCol &&
        destinationPiece.type == EMPTY &&
        endRow == enPassantRow &&
        endCol == enPassantCol)
    {
        int direction =
            movingPiece.color == WHITE_SIDE
            ? -1
            : 1;

        enPassantCapturedRow =
            endRow - direction;

        enPassantCapturedPiece =
            board[enPassantCapturedRow][endCol];

        enPassantMove = true;
    }

    // --------------------------------------------------------
    // DETECT CASTLING
    // --------------------------------------------------------

    if (movingPiece.type == KING &&
        std::abs(endCol - startCol) == 2)
    {
        castlingMove = true;

        if (endCol == 6)
        {
            rookStartCol = 7;
            rookEndCol = 5;
        }
        else
        {
            rookStartCol = 0;
            rookEndCol = 3;
        }

        rookPiece =
            board[startRow][rookStartCol];

        rookDestinationPiece =
            board[startRow][rookEndCol];
    }

    // --------------------------------------------------------
    // TEMPORARILY MAKE MOVE
    // --------------------------------------------------------

    board[endRow][endCol] =
        movingPiece;

    board[startRow][startCol] =
    {};

    if (enPassantMove)
    {
        board[enPassantCapturedRow][endCol] =
        {};
    }

    if (castlingMove)
    {
        board[startRow][rookEndCol] =
            rookPiece;

        board[startRow][rookStartCol] =
        {};
    }

    bool kingInCheck =
        IsKingInCheck(
            movingPiece.color
        );

    // --------------------------------------------------------
    // RESTORE EVERYTHING
    // --------------------------------------------------------

    board[startRow][startCol] =
        movingPiece;

    board[endRow][endCol] =
        destinationPiece;

    if (enPassantMove)
    {
        board[enPassantCapturedRow][endCol] =
            enPassantCapturedPiece;
    }

    if (castlingMove)
    {
        board[startRow][rookStartCol] =
            rookPiece;

        board[startRow][rookEndCol] =
            rookDestinationPiece;
    }

    return !kingInCheck;
}

// ============================================================
// MOVE GENERATION
// ============================================================

void GenerateLegalMoves(
    int row,
    int col)
{
    legalMoves.clear();

    for (int targetRow = 0;
        targetRow < BOARD_SIZE;
        targetRow++)
    {
        for (int targetCol = 0;
            targetCol < BOARD_SIZE;
            targetCol++)
        {
            if (IsMoveLegal(
                row,
                col,
                targetRow,
                targetCol))
            {
                legalMoves.push_back(
                    {
                        targetRow,
                        targetCol
                    }
                );
            }
        }
    }
}

// ------------------------------------------------------------

bool IsHighlightedMove(
    int row,
    int col)
{
    for (const BoardPosition& move :
        legalMoves)
    {
        if (move.row == row &&
            move.col == col)
        {
            return true;
        }
    }

    return false;
}

// ============================================================
// UX: CAN THIS PIECE SAVE THE KING?
//
// When the current player is in check, this lets us highlight
// every friendly piece that has at least one legal response.
// ============================================================

bool PieceHasLegalMove(
    int row,
    int col)
{
    Piece piece =
        board[row][col];

    if (piece.type == EMPTY)
    {
        return false;
    }

    for (int targetRow = 0;
        targetRow < BOARD_SIZE;
        targetRow++)
    {
        for (int targetCol = 0;
            targetCol < BOARD_SIZE;
            targetCol++)
        {
            if (IsMoveLegal(
                row,
                col,
                targetRow,
                targetCol))
            {
                return true;
            }
        }
    }

    return false;
}

// ------------------------------------------------------------

bool HasAnyLegalMove(PieceColor color)
{
    for (int row = 0;
        row < BOARD_SIZE;
        row++)
    {
        for (int col = 0;
            col < BOARD_SIZE;
            col++)
        {
            Piece piece =
                board[row][col];

            if (piece.type == EMPTY ||
                piece.color != color)
            {
                continue;
            }

            if (PieceHasLegalMove(
                row,
                col))
            {
                return true;
            }
        }
    }

    return false;
}

// ============================================================
// GAME STATE
// ============================================================

void EvaluateGameState()
{
    bool inCheck =
        IsKingInCheck(currentTurn);

    bool hasMove =
        HasAnyLegalMove(currentTurn);

    if (inCheck &&
        !hasMove)
    {
        gameOver = true;
        stalemate = false;

        winner =
            OppositeColor(currentTurn);

        lastMove += " #";

        return;
    }

    if (!inCheck &&
        !hasMove)
    {
        gameOver = true;
        stalemate = true;

        winner = NONE;

        return;
    }

    if (inCheck)
    {
        lastMove += " +";
    }
}

// ============================================================
// FINISH TURN
// ============================================================

void FinishTurn()
{
    if (currentTurn == WHITE_SIDE)
    {
        currentTurn =
            BLACK_SIDE;
    }
    else
    {
        currentTurn =
            WHITE_SIDE;

        moveNumber++;
    }

    pieceSelected = false;

    selectedRow = -1;
    selectedCol = -1;

    legalMoves.clear();

    EvaluateGameState();
}

// ============================================================
// MOVE PIECE
// ============================================================

void MovePiece(
    int startRow,
    int startCol,
    int endRow,
    int endCol)
{
    Piece movingPiece =
        board[startRow][startCol];

    Piece capturedPiece =
        board[endRow][endCol];

    bool isEnPassant = false;
    bool isCastling = false;

    std::string from =
        GetSquareName(
            startRow,
            startCol
        );

    std::string to =
        GetSquareName(
            endRow,
            endCol
        );

    lastMove =
        GetChessPieceName(
            movingPiece.type
        )
        + " "
        + from
        + " -> "
        + to;

    // ========================================================
    // EN PASSANT CAPTURE
    // ========================================================

    if (movingPiece.type == PAWN &&
        startCol != endCol &&
        capturedPiece.type == EMPTY &&
        endRow == enPassantRow &&
        endCol == enPassantCol)
    {
        int direction =
            movingPiece.color == WHITE_SIDE
            ? -1
            : 1;

        int capturedPawnRow =
            endRow - direction;

        Piece capturedPawn =
            board[capturedPawnRow][endCol];

        if (capturedPawn.type == PAWN &&
            capturedPawn.color !=
            movingPiece.color)
        {
            board[capturedPawnRow][endCol] =
            {};

            isEnPassant = true;

            lastMove +=
                " en passant";
        }
    }

    // ========================================================
    // NORMAL CAPTURE TEXT
    // ========================================================

    if (capturedPiece.type != EMPTY)
    {
        lastMove +=
            " captured "
            + GetChessPieceName(
                capturedPiece.type
            );
    }

    // ========================================================
    // DETECT CASTLING
    // ========================================================

    if (movingPiece.type == KING &&
        std::abs(endCol - startCol) == 2)
    {
        isCastling = true;
    }

    // ========================================================
    // MOVE PIECE
    // ========================================================

    board[endRow][endCol] =
        movingPiece;

    board[endRow][endCol].hasMoved =
        true;

    board[startRow][startCol] =
    {};

    // ========================================================
    // MOVE ROOK DURING CASTLING
    // ========================================================

    if (isCastling)
    {
        if (endCol == 6)
        {
            board[endRow][5] =
                board[endRow][7];

            board[endRow][5].hasMoved =
                true;

            board[endRow][7] =
            {};

            lastMove +=
                " O-O";
        }
        else
        {
            board[endRow][3] =
                board[endRow][0];

            board[endRow][3].hasMoved =
                true;

            board[endRow][0] =
            {};

            lastMove +=
                " O-O-O";
        }
    }

    // ========================================================
    // UPDATE EN PASSANT TARGET
    //
    // It is only valid for the opponent's NEXT move.
    // ========================================================

    enPassantRow = -1;
    enPassantCol = -1;

    if (movingPiece.type == PAWN &&
        std::abs(endRow - startRow) == 2)
    {
        enPassantRow =
            (startRow + endRow) / 2;

        enPassantCol =
            startCol;
    }

    // Normal moves get a soft click; captures get a heavier one.
    PlayPieceMoveSound(
        capturedPiece.type != EMPTY || isEnPassant
    );

    // ========================================================
    // PROMOTION
    // ========================================================

    if (board[endRow][endCol].type == PAWN &&
        (endRow == 0 ||
            endRow == 7))
    {
        promotionPending = true;

        promotionRow = endRow;
        promotionCol = endCol;

        pieceSelected = false;
        legalMoves.clear();

        lastMove +=
            " promotes...";

        pendingHistorySr = startRow;
        pendingHistorySc = startCol;
        pendingHistoryEr = endRow;
        pendingHistoryEc = endCol;
        pendingHistoryMover = movingPiece.color;

        return;
    }

    PieceColor moverColor = movingPiece.color;
    FinishTurn();
    RecordMoveHistory(startRow, startCol, endRow, endCol, moverColor);
}

// ============================================================
// PROMOTION
// ============================================================

void PromotePawn(PieceType type)
{
    if (!promotionPending)
    {
        return;
    }

    if (type != QUEEN &&
        type != ROOK &&
        type != BISHOP &&
        type != KNIGHT)
    {
        return;
    }

    board[promotionRow][promotionCol].type =
        type;

    board[promotionRow][promotionCol].hasMoved =
        true;

    lastMove =
        "Pawn promoted to "
        + GetChessPieceName(type);

    promotionPending = false;

    promotionRow = -1;
    promotionCol = -1;

    int historySr = pendingHistorySr;
    int historySc = pendingHistorySc;
    int historyEr = pendingHistoryEr;
    int historyEc = pendingHistoryEc;
    PieceColor historyMover = pendingHistoryMover;

    pendingHistorySr = pendingHistorySc = -1;
    pendingHistoryEr = pendingHistoryEc = -1;
    pendingHistoryMover = NONE;

    FinishTurn();
    RecordMoveHistory(historySr, historySc, historyEr, historyEc, historyMover);
}

// ============================================================
// TEXTURES
// ============================================================

Texture2D GetPieceTexture(
    const Piece& piece,
    const PieceTextures& textures)
{
    if (piece.color == WHITE_SIDE)
    {
        switch (piece.type)
        {
        case PAWN:
            return textures.whitePawn;

        case ROOK:
            return textures.whiteRook;

        case KNIGHT:
            return textures.whiteKnight;

        case BISHOP:
            return textures.whiteBishop;

        case QUEEN:
            return textures.whiteQueen;

        case KING:
            return textures.whiteKing;

        default:
            break;
        }
    }

    if (piece.color == BLACK_SIDE)
    {
        switch (piece.type)
        {
        case PAWN:
            return textures.blackPawn;

        case ROOK:
            return textures.blackRook;

        case KNIGHT:
            return textures.blackKnight;

        case BISHOP:
            return textures.blackBishop;

        case QUEEN:
            return textures.blackQueen;

        case KING:
            return textures.blackKing;

        default:
            break;
        }
    }

    return {};
}

// ------------------------------------------------------------

void SetTexturePixelMode(Texture2D texture)
{
    SetTextureFilter(
        texture,
        TEXTURE_FILTER_POINT
    );
}

// ============================================================
// DRAW PIECE
// ============================================================

void DrawPieceTexture(
    const Piece& piece,
    int row,
    int col,
    const PieceTextures& textures)
{
    if (piece.type == EMPTY)
    {
        return;
    }

    Texture2D texture =
        GetPieceTexture(
            piece,
            textures
        );

    if (texture.id == 0)
    {
        return;
    }

    float padding = 12.0f;

    Rectangle source =
    {
        0.0f,
        0.0f,
        (float)texture.width,
        (float)texture.height
    };

    Rectangle destination =
    {
        BOARD_X +
            col * TILE_SIZE +
            padding,

        BOARD_Y +
            row * TILE_SIZE +
            padding,

        TILE_SIZE -
            padding * 2,

        TILE_SIZE -
            padding * 2
    };

    DrawTexturePro(
        texture,
        source,
        destination,
        { 0.0f, 0.0f },
        0.0f,
        WHITE
    );
}

// ============================================================
// PIECE LABEL
// ============================================================

void DrawPieceLabel(
    const Piece& piece,
    int x,
    int y)
{
    if (piece.type == EMPTY)
    {
        return;
    }

    Color badgeColor;

    if (piece.color == WHITE_SIDE)
    {
        badgeColor =
        {
            45,
            45,
            45,
            225
        };
    }
    else
    {
        badgeColor =
        {
            190,
            35,
            35,
            235
        };
    }

    DrawCircle(
        x + 13,
        y + 13,
        11,
        badgeColor
    );

    DrawText(
        TextFormat(
            "%c",
            GetPieceLetter(piece.type)
        ),
        x + 8,
        y + 5,
        16,
        RAYWHITE
    );
}

// ============================================================
// DRAW BOARD
// ============================================================

void DrawBoard(
    const PieceTextures& textures)
{
    Color lightSquare =
    {
        232,
        213,
        180,
        255
    };

    Color darkSquare =
    {
        145,
        108,
        80,
        255
    };

    Color selectedColor =
    {
        255,
        214,
        64,
        180
    };

    Color moveHighlight =
    {
        65,
        190,
        120,
        160
    };

    Color captureHighlight =
    {
        220,
        70,
        70,
        255
    };

    Color saveKingHighlight =
    {
        40,
        220,
        220,
        255
    };

    bool currentPlayerInCheck =
        !gameOver &&
        IsKingInCheck(currentTurn);

    for (int row = 0;
        row < BOARD_SIZE;
        row++)
    {
        for (int col = 0;
            col < BOARD_SIZE;
            col++)
        {
            int x =
                BOARD_X +
                col * TILE_SIZE;

            int y =
                BOARD_Y +
                row * TILE_SIZE;

            Rectangle square =
            {
                (float)x,
                (float)y,
                (float)TILE_SIZE,
                (float)TILE_SIZE
            };

            bool light =
                (row + col) % 2 == 0;

            DrawRectangleRec(
                square,
                light
                ? lightSquare
                : darkSquare
            );

            // ------------------------------------------------
            // SELECTED
            // ------------------------------------------------

            if (pieceSelected &&
                row == selectedRow &&
                col == selectedCol)
            {
                DrawRectangleRec(
                    square,
                    selectedColor
                );
            }

            // ------------------------------------------------
            // LEGAL MOVE DESTINATIONS
            // ------------------------------------------------

            if (IsHighlightedMove(
                row,
                col))
            {
                if (board[row][col].type ==
                    EMPTY)
                {
                    DrawCircle(
                        x + TILE_SIZE / 2,
                        y + TILE_SIZE / 2,
                        12,
                        moveHighlight
                    );
                }
                else
                {
                    DrawRectangleLinesEx(
                        square,
                        6,
                        captureHighlight
                    );
                }
            }

            DrawPieceTexture(
                board[row][col],
                row,
                col,
                textures
            );

            DrawPieceLabel(
                board[row][col],
                x,
                y
            );

            // ------------------------------------------------
            // KING IN CHECK
            // ------------------------------------------------

            if (board[row][col].type == KING &&
                IsKingInCheck(
                    board[row][col].color))
            {
                DrawRectangleLinesEx(
                    square,
                    7,
                    RED
                );
            }

            // ------------------------------------------------
            // UX:
            // Highlight friendly pieces capable of responding
            // to check.
            // ------------------------------------------------

            if (currentPlayerInCheck &&
                board[row][col].type != EMPTY &&
                board[row][col].color ==
                currentTurn &&
                PieceHasLegalMove(
                    row,
                    col))
            {
                DrawRectangleLinesEx(
                    {
                        (float)x + 5,
                        (float)y + 5,
                        TILE_SIZE - 10.0f,
                        TILE_SIZE - 10.0f
                    },
                    4,
                    saveKingHighlight
                );
            }
        }
    }

    // ========================================================
    // FILE LETTERS
    // ========================================================

    for (int col = 0;
        col < BOARD_SIZE;
        col++)
    {
        char letter =
            'a' + col;

        DrawText(
            TextFormat(
                "%c",
                letter
            ),
            BOARD_X +
            col * TILE_SIZE +
            TILE_SIZE / 2 -
            5,
            BOARD_Y +
            BOARD_SIZE * TILE_SIZE +
            7,
            18,
            LIGHTGRAY
        );
    }

    // ========================================================
    // RANK NUMBERS
    // ========================================================

    for (int row = 0;
        row < BOARD_SIZE;
        row++)
    {
        int number =
            8 - row;

        DrawText(
            TextFormat(
                "%i",
                number
            ),
            BOARD_X - 25,
            BOARD_Y +
            row * TILE_SIZE +
            TILE_SIZE / 2 -
            10,
            18,
            LIGHTGRAY
        );
    }
}

// ============================================================
// LEGEND
// ============================================================

void DrawLegendLine(
    int x,
    int y,
    const char* letter,
    const char* underwear,
    const char* chessPiece)
{
    DrawCircle(
        x + 10,
        y + 10,
        10,
        DARKGRAY
    );

    DrawText(
        letter,
        x + 5,
        y + 2,
        16,
        RAYWHITE
    );

    DrawText(
        underwear,
        x + 30,
        y,
        17,
        GOLD
    );

    DrawText(
        chessPiece,
        x + 30,
        y + 20,
        14,
        GRAY
    );
}

// ============================================================
// SIDE PANEL
// ============================================================

void DrawSidePanel()
{
    int panelX =
        BOARD_X +
        BOARD_SIZE * TILE_SIZE +
        35;

    DrawText(
        "UNDIES",
        panelX,
        35,
        28,
        RAYWHITE
    );

    DrawText(
        "CHESS",
        panelX,
        68,
        40,
        GOLD
    );

    DrawLine(
        panelX,
        120,
        SCREEN_WIDTH - 25,
        120,
        DARKGRAY
    );

    // ========================================================
    // GAME STATUS
    // ========================================================

    if (!gameOver)
    {
        DrawText(
            "TURN",
            panelX,
            140,
            18,
            GRAY
        );

        Color turnColor =
            currentTurn == WHITE_SIDE
            ? RAYWHITE
            : RED;

        DrawText(
            GetColorName(
                currentTurn
            ),
            panelX,
            165,
            26,
            turnColor
        );

        DrawText(
            TextFormat(
                "Move %i",
                moveNumber
            ),
            panelX,
            200,
            18,
            LIGHTGRAY
        );

        if (IsKingInCheck(
            currentTurn))
        {
            DrawText(
                "CHECK!",
                panelX + 100,
                165,
                26,
                RED
            );

            DrawText(
                "Cyan = can save King",
                panelX,
                215,
                14,
                SKYBLUE
            );
        }
    }
    else
    {
        if (stalemate)
        {
            DrawText(
                "STALEMATE",
                panelX,
                150,
                27,
                GOLD
            );

            DrawText(
                "DRAW",
                panelX,
                185,
                22,
                LIGHTGRAY
            );
        }
        else
        {
            DrawText(
                "CHECKMATE",
                panelX,
                145,
                27,
                GOLD
            );

            Color winnerColor =
                winner == WHITE_SIDE
                ? RAYWHITE
                : RED;

            DrawText(
                TextFormat(
                    "%s WINS",
                    GetColorName(winner)
                ),
                panelX,
                180,
                24,
                winnerColor
            );
        }
    }

    // ========================================================
    // SELECTED
    // ========================================================

    DrawLine(
        panelX,
        245,
        SCREEN_WIDTH - 25,
        245,
        DARKGRAY
    );

    DrawText(
        "SELECTED",
        panelX,
        260,
        17,
        GRAY
    );

    if (pieceSelected)
    {
        Piece piece =
            board[selectedRow][selectedCol];

        DrawText(
            GetChessPieceName(
                piece.type
            ).c_str(),
            panelX,
            285,
            22,
            RAYWHITE
        );

        DrawText(
            GetPieceName(
                piece.type
            ).c_str(),
            panelX,
            312,
            17,
            GOLD
        );

        DrawText(
            GetSquareName(
                selectedRow,
                selectedCol
            ).c_str(),
            panelX,
            337,
            17,
            LIGHTGRAY
        );

        // ----------------------------------------------------
        // NEW UX:
        // Explicitly tell player when this piece can't move.
        // ----------------------------------------------------

        if (legalMoves.empty())
        {
            DrawText(
                "NO LEGAL MOVES",
                panelX,
                362,
                16,
                RED
            );
        }
        else
        {
            DrawText(
                TextFormat(
                    "%i legal moves",
                    (int)legalMoves.size()
                ),
                panelX,
                362,
                15,
                LIGHTGRAY
            );
        }
    }
    else
    {
        DrawText(
            "None",
            panelX,
            290,
            20,
            DARKGRAY
        );
    }

    // ========================================================
    // LAST MOVE
    // ========================================================

    DrawLine(
        panelX,
        395,
        SCREEN_WIDTH - 25,
        395,
        DARKGRAY
    );

    DrawText(
        "LAST MOVE",
        panelX,
        410,
        17,
        GRAY
    );

    DrawText(
        lastMove.c_str(),
        panelX,
        435,
        15,
        LIGHTGRAY
    );

    // ========================================================
    // LEGEND
    // ========================================================

    DrawLine(
        panelX,
        470,
        SCREEN_WIDTH - 25,
        470,
        DARKGRAY
    );

    DrawText(
        "THE UNDERWEAR ARMY",
        panelX,
        485,
        18,
        RAYWHITE
    );

    DrawLegendLine(
        panelX,
        515,
        "P",
        "Briefs",
        "Pawn"
    );

    DrawLegendLine(
        panelX,
        552,
        "R",
        "Long Sock",
        "Rook"
    );

    DrawLegendLine(
        panelX,
        589,
        "N",
        "Short Sock",
        "Knight"
    );

    DrawLegendLine(
        panelX,
        626,
        "B",
        "Jock",
        "Bishop"
    );

    DrawLegendLine(
        panelX,
        663,
        "Q",
        "Boxers",
        "Queen"
    );

    DrawLegendLine(
        panelX,
        700,
        "K",
        "Boxer Briefs",
        "King"
    );

}

// ============================================================
// PROMOTION SCREEN
// ============================================================

void DrawPromotionScreen()
{
    if (!promotionPending)
    {
        return;
    }

    DrawRectangle(
        0,
        0,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        Color{
            0,
            0,
            0,
            190
        }
    );

    const char* title =
        "PROMOTION!";

    int titleSize = 52;

    int titleWidth =
        MeasureText(
            title,
            titleSize
        );

    DrawText(
        title,
        SCREEN_WIDTH / 2 -
        titleWidth / 2,
        245,
        titleSize,
        GOLD
    );

    const char* subtitle =
        "THE BRIEFS HAVE ASCENDED";

    int subtitleSize = 22;

    int subtitleWidth =
        MeasureText(
            subtitle,
            subtitleSize
        );

    DrawText(
        subtitle,
        SCREEN_WIDTH / 2 -
        subtitleWidth / 2,
        315,
        subtitleSize,
        RAYWHITE
    );

    const char* choices =
        "Q = Boxers     R = Long Sock";

    int choicesWidth =
        MeasureText(
            choices,
            21
        );

    DrawText(
        choices,
        SCREEN_WIDTH / 2 -
        choicesWidth / 2,
        375,
        21,
        LIGHTGRAY
    );

    const char* choices2 =
        "B = Jock       N = Short Sock";

    int choicesWidth2 =
        MeasureText(
            choices2,
            21
        );

    DrawText(
        choices2,
        SCREEN_WIDTH / 2 -
        choicesWidth2 / 2,
        415,
        21,
        LIGHTGRAY
    );
}

// ============================================================
// END GAME SCREEN
// ============================================================

void DrawEndGameScreen()
{
    if (!gameOver)
    {
        return;
    }

    DrawRectangle(
        0,
        0,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        Color{
            0,
            0,
            0,
            175
        }
    );

    if (stalemate)
    {
        const char* title =
            "STALEMATE";

        int titleSize = 58;

        int titleWidth =
            MeasureText(
                title,
                titleSize
            );

        DrawText(
            title,
            SCREEN_WIDTH / 2 -
            titleWidth / 2,
            280,
            titleSize,
            GOLD
        );

        const char* subtitle =
            "THE UNDERWEAR WAR ENDS IN A DRAW";

        int subtitleSize = 22;

        int subtitleWidth =
            MeasureText(
                subtitle,
                subtitleSize
            );

        DrawText(
            subtitle,
            SCREEN_WIDTH / 2 -
            subtitleWidth / 2,
            355,
            subtitleSize,
            RAYWHITE
        );
    }
    else
    {
        bool gotJocked =
            gameMode == VS_COMPUTER &&
            aiDifficulty == JOCKED &&
            winner == aiColor;

        std::string title;

        if (gotJocked)
            title = "YOU GOT JOCKED!";
        else
            title =
            std::string(
                GetColorName(winner)
            )
            + " WINS!";

        int titleSize = gotJocked ? 54 : 58;

        int titleWidth =
            MeasureText(
                title.c_str(),
                titleSize
            );

        DrawText(
            title.c_str(),
            SCREEN_WIDTH / 2 -
            titleWidth / 2,
            270,
            titleSize,
            GOLD
        );

        const char* checkmateText =
            "CHECKMATE";

        int checkmateSize = 32;

        int checkmateWidth =
            MeasureText(
                checkmateText,
                checkmateSize
            );

        DrawText(
            checkmateText,
            SCREEN_WIDTH / 2 -
            checkmateWidth / 2,
            345,
            checkmateSize,
            RAYWHITE
        );

        const char* underwearText =
            gotJocked
            ? "NO AMOUNT OF SUPPORT COULD SAVE YOU"
            : "THE BOXER BRIEFS HAVE BEEN DEFEATED";

        int underwearSize = 20;

        int underwearWidth =
            MeasureText(
                underwearText,
                underwearSize
            );

        DrawText(
            underwearText,
            SCREEN_WIDTH / 2 -
            underwearWidth / 2,
            395,
            underwearSize,
            LIGHTGRAY
        );
    }

    const char* rematch =
        "Choose your fate";

    int rematchSize = 22;

    int rematchWidth =
        MeasureText(
            rematch,
            rematchSize
        );

    DrawText(
        rematch,
        SCREEN_WIDTH / 2 -
        rematchWidth / 2,
        455,
        rematchSize,
        GRAY
    );
}

// ============================================================
// COMPUTER PLAYER
// ============================================================

struct AIMove { int sr, sc, er, ec; };

struct GameSnapshot
{
    Piece b[BOARD_SIZE][BOARD_SIZE];
    PieceColor turn;
    int epRow, epCol, moveNo;
    bool over, stale, promo;
    PieceColor win;
    int promoRow, promoCol;
    std::string last;
    int selRow, selCol;
    bool selected;
    std::vector<BoardPosition> moves;
};

GameSnapshot SaveSnapshot()
{
    GameSnapshot s{};
    for (int r = 0;r < BOARD_SIZE;r++) for (int c = 0;c < BOARD_SIZE;c++) s.b[r][c] = board[r][c];
    s.turn = currentTurn; s.epRow = enPassantRow; s.epCol = enPassantCol; s.moveNo = moveNumber;
    s.over = gameOver; s.stale = stalemate; s.promo = promotionPending; s.win = winner;
    s.promoRow = promotionRow; s.promoCol = promotionCol; s.last = lastMove;
    s.selRow = selectedRow; s.selCol = selectedCol; s.selected = pieceSelected; s.moves = legalMoves;
    return s;
}

void LoadSnapshot(const GameSnapshot& s)
{
    for (int r = 0;r < BOARD_SIZE;r++) for (int c = 0;c < BOARD_SIZE;c++) board[r][c] = s.b[r][c];
    currentTurn = s.turn; enPassantRow = s.epRow; enPassantCol = s.epCol; moveNumber = s.moveNo;
    gameOver = s.over; stalemate = s.stale; promotionPending = s.promo; winner = s.win;
    promotionRow = s.promoRow; promotionCol = s.promoCol; lastMove = s.last;
    pieceSelected = s.selected; selectedRow = s.selRow; selectedCol = s.selCol; legalMoves = s.moves;
}

std::vector<AIMove> GetAllMoves(PieceColor color)
{
    std::vector<AIMove> moves;
    for (int r = 0;r < 8;r++) for (int c = 0;c < 8;c++)
    {
        if (board[r][c].type == EMPTY || board[r][c].color != color) continue;
        for (int er = 0;er < 8;er++) for (int ec = 0;ec < 8;ec++)
            if (IsMoveLegal(r, c, er, ec)) moves.push_back({ r,c,er,ec });
    }
    return moves;
}

void MakeAIMoveForSearch(const AIMove& m)
{
    bool oldSuppress = suppressMoveSounds;
    bool oldHistorySuppress = suppressMoveHistory;
    suppressMoveSounds = true;
    suppressMoveHistory = true;

    MovePiece(m.sr, m.sc, m.er, m.ec);
    if (promotionPending)
        PromotePawn(QUEEN);

    suppressMoveSounds = oldSuppress;
    suppressMoveHistory = oldHistorySuppress;
}

int PieceValue(PieceType t)
{
    switch (t) {
    case PAWN:return 100; case KNIGHT:return 320; case BISHOP:return 330;
    case ROOK:return 500; case QUEEN:return 900; case KING:return 20000; default:return 0;
    }
}

int EvaluateBoard()
{
    if (gameOver)
    {
        if (stalemate) return 0;
        return winner == aiColor ? 1000000 : -1000000;
    }
    int score = 0;
    for (int r = 0;r < 8;r++) for (int c = 0;c < 8;c++)
    {
        Piece p = board[r][c]; if (p.type == EMPTY) continue;
        int v = PieceValue(p.type);
        // Small center bonus makes the computer feel less random.
        if (r >= 2 && r <= 5 && c >= 2 && c <= 5) v += 6;
        score += (p.color == aiColor ? v : -v);
    }
    if (IsKingInCheck(OppositeColor(aiColor))) score += 25;
    if (IsKingInCheck(aiColor)) score -= 25;
    return score;
}

int Minimax(int depth, int alpha, int beta)
{
    if (depth <= 0 || gameOver) return EvaluateBoard();
    PieceColor side = currentTurn;
    std::vector<AIMove> moves = GetAllMoves(side);
    if (moves.empty()) { EvaluateGameState(); return EvaluateBoard(); }

    bool maximizing = (side == aiColor);
    int best = maximizing ? -2000000000 : 2000000000;
    for (const AIMove& m : moves)
    {
        GameSnapshot snap = SaveSnapshot();
        MakeAIMoveForSearch(m);
        int score = Minimax(depth - 1, alpha, beta);
        LoadSnapshot(snap);
        if (maximizing) { best = std::max(best, score); alpha = std::max(alpha, best); }
        else { best = std::min(best, score); beta = std::min(beta, best); }
        if (beta <= alpha) break;
    }
    return best;
}

int AIDepth()
{
    switch (aiDifficulty) {
    case LAUNDRY_DAY:return 1; case CASUAL_FRIDAY:return 1;
    case TIGHT_FIT:return 2; case COMMANDO:return 3; case JOCKED:return 4;
    }
    return 2;
}

const char* DifficultyName()
{
    switch (aiDifficulty) {
    case LAUNDRY_DAY:return "LAUNDRY DAY"; case CASUAL_FRIDAY:return "CASUAL FRIDAY";
    case TIGHT_FIT:return "TIGHT FIT"; case COMMANDO:return "COMMANDO"; case JOCKED:return "JOCKED";
    }
    return "";
}

void DoComputerMove()
{
    // Keep one authoritative copy of the LIVE position. Every search branch
    // is forced back to this exact state before the real move is made.
    // This prevents a hypothetical minimax branch from leaking onto the board.
    GameSnapshot liveRoot = SaveSnapshot();

    std::vector<AIMove> moves = GetAllMoves(aiColor);
    if (moves.empty())
    {
        EvaluateGameState();
        return;
    }

    int randomChance = aiDifficulty == LAUNDRY_DAY ? 70
        : (aiDifficulty == CASUAL_FRIDAY ? 30 : 0);

    AIMove chosen = moves[GetRandomValue(0, (int)moves.size() - 1)];

    if (GetRandomValue(1, 100) > randomChance)
    {
        int best = -2000000000;
        std::vector<AIMove> bestMoves;
        int depth = AIDepth();

        for (const AIMove& m : moves)
        {
            // Always begin each candidate from the exact live position.
            LoadSnapshot(liveRoot);

            MakeAIMoveForSearch(m);
            int score = Minimax(depth - 1, -2000000000, 2000000000);

            // Hard restore after EVERY branch, even if internals changed state.
            LoadSnapshot(liveRoot);

            if (score > best)
            {
                best = score;
                bestMoves.clear();
                bestMoves.push_back(m);
            }
            else if (score == best)
            {
                bestMoves.push_back(m);
            }
        }

        if (!bestMoves.empty())
            chosen = bestMoves[GetRandomValue(0, (int)bestMoves.size() - 1)];
    }

    // One final restore immediately before the ONLY real computer move.
    LoadSnapshot(liveRoot);

    Piece expectedPiece = board[chosen.sr][chosen.sc];

    // INTERNAL AFFAIRS: log every REAL AI move to the Visual Studio Output window.
    // Search moves never reach this line, so this is an authoritative audit trail.
    TraceLog(LOG_INFO, "AI REAL MOVE %i: %s %s -> %s",
        moveNumber,
        GetChessPieceName(expectedPiece.type).c_str(),
        GetSquareName(chosen.sr, chosen.sc).c_str(),
        GetSquareName(chosen.er, chosen.ec).c_str());

    MovePiece(chosen.sr, chosen.sc, chosen.er, chosen.ec);
    if (promotionPending)
        PromotePawn(QUEEN);

    // Debug guard: if something impossible happened, print it to VS Output.
    if (board[chosen.er][chosen.ec].type == EMPTY ||
        board[chosen.er][chosen.ec].color != expectedPiece.color)
    {
        TraceLog(LOG_WARNING,
            "AI MOVE INTEGRITY WARNING: expected move %s -> %s did not survive",
            GetSquareName(chosen.sr, chosen.sc).c_str(),
            GetSquareName(chosen.er, chosen.ec).c_str());
    }
}

const Color MENU_TEAL = Color{ 20, 153, 148, 255 };
const Color MENU_TEAL_DARK = Color{ 13, 92, 91, 255 };
const Color MENU_TEAL_LIGHT = Color{ 204, 241, 238, 255 };
const Color MENU_WHITE = Color{ 247, 252, 251, 255 };
const Color MENU_INK = Color{ 22, 67, 68, 255 };

void DrawMenuBackdrop()
{
    ClearBackground(MENU_WHITE);

    // Clean teal framing with a few playful geometric accents.
    DrawRectangle(0, 0, SCREEN_WIDTH, 18, MENU_TEAL);
    DrawRectangle(0, SCREEN_HEIGHT - 18, SCREEN_WIDTH, 18, MENU_TEAL);

    DrawCircle(-30, 150, 120, Fade(MENU_TEAL_LIGHT, 0.65f));
    DrawCircle(SCREEN_WIDTH + 20, 650, 150, Fade(MENU_TEAL_LIGHT, 0.55f));

    DrawRectangleRounded({ 72, 95, 150, 14 }, 0.5f, 6, Fade(MENU_TEAL, 0.20f));
    DrawRectangleRounded({ 885, 115, 160, 14 }, 0.5f, 6, Fade(MENU_TEAL, 0.20f));
    DrawRectangleRounded({ 55, 690, 210, 10 }, 0.5f, 6, Fade(MENU_TEAL, 0.15f));
    DrawRectangleRounded({ 875, 705, 180, 10 }, 0.5f, 6, Fade(MENU_TEAL, 0.15f));
}

bool MenuButton(Rectangle r, const char* text)
{
    Vector2 m = GetMousePosition();
    bool hover = CheckCollisionPointRec(m, r);

    if (hover)
        DrawRectangleRec({ r.x + 5, r.y + 5, r.width, r.height }, Fade(MENU_TEAL_DARK, 0.16f));

    DrawRectangleRec(r, hover ? MENU_TEAL : RAYWHITE);
    DrawRectangleLinesEx(r, 2, MENU_TEAL);

    // Compact in-game controls need a smaller label than the big menu buttons.
    int fs = (r.height <= 40.0f) ? 16 : 24;
    int tw = MeasureText(text, fs);
    DrawText(text,
        (int)(r.x + r.width / 2 - tw / 2),
        (int)(r.y + r.height / 2 - fs / 2),
        fs,
        hover ? RAYWHITE : MENU_TEAL_DARK);

    bool clicked = hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (clicked && audioReady && sfxEnabled)
        PlaySound(moveSound);
    return clicked;
}

void DrawCenteredText(const char* text, int y, int size, Color color)
{
    DrawText(text, SCREEN_WIDTH / 2 - MeasureText(text, size) / 2, y, size, color);
}

void DrawSplashScreen()
{
    DrawMenuBackdrop();

    double elapsed = GetTime() - splashStartTime;
    float alpha = 1.0f;
    if (elapsed < 0.6) alpha = (float)(elapsed / 0.6);
    else if (elapsed > 2.2) alpha = (float)((2.8 - elapsed) / 0.6);
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    Color tealFade = Fade(MENU_TEAL, alpha);
    Color darkFade = Fade(MENU_TEAL_DARK, alpha);
    Color grayFade = Fade(Color{ 91, 120, 120, 255 }, alpha);

    DrawCenteredText("KAIJU", 280, 76, tealFade);
    DrawCenteredText("INTERACTIVE", 365, 34, darkFade);
    DrawCenteredText("presents", 420, 18, grayFade);

    if (elapsed >= 2.8 || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        screenState = TITLE_SCREEN;
}

void DrawDecorativeSprite(Texture2D texture, float x, float y, float size, float rotation)
{
    if (texture.id == 0) return;

    // Smaller, cleaner laundry badge. The slightly darker center and
    // teal ring give pale/white sprites enough contrast to read clearly.
    float badgeRadius = size * 0.53f;

    DrawCircle(
        (int)x + 3,
        (int)y + 5,
        badgeRadius + 2.0f,
        Fade(MENU_TEAL_DARK, 0.10f)
    );

    DrawCircle(
        (int)x,
        (int)y,
        badgeRadius,
        Color{ 205, 239, 237, 245 }
    );

    // Two thin rings look cleaner than one heavy outline.
    DrawCircleLines(
        (int)x,
        (int)y,
        badgeRadius,
        Fade(MENU_TEAL, 0.70f)
    );

    DrawCircleLines(
        (int)x,
        (int)y,
        badgeRadius - 2.0f,
        Fade(MENU_TEAL, 0.22f)
    );

    Rectangle source =
    {
        0.0f,
        0.0f,
        (float)texture.width,
        (float)texture.height
    };

    Vector2 origin =
    {
        size / 2.0f,
        size / 2.0f
    };

    // A tiny teal silhouette behind the art makes white garments read
    // against the pale menu without altering the original sprite.
    Rectangle shadowDest =
    {
        x + 3.0f,
        y + 4.0f,
        size,
        size
    };

    DrawTexturePro(
        texture,
        source,
        shadowDest,
        origin,
        rotation,
        Fade(MENU_TEAL_DARK, 0.22f)
    );

    Rectangle dest =
    {
        x,
        y,
        size,
        size
    };

    DrawTexturePro(
        texture,
        source,
        dest,
        origin,
        rotation,
        WHITE
    );
}

void DrawTitleScreen(const PieceTextures& textures)
{
    DrawMenuBackdrop();

    // A few members of the underwear army tossed around the title like
    // somebody emptied a laundry basket onto the menu.
    DrawDecorativeSprite(textures.whitePawn, 155, 175, 105, -13.0f);
    DrawDecorativeSprite(textures.blackKnight, 955, 190, 100, 12.0f);
    DrawDecorativeSprite(textures.whiteBishop, 170, 565, 115, 10.0f);
    DrawDecorativeSprite(textures.blackQueen, 945, 565, 104, -10.0f);
    DrawDecorativeSprite(textures.whiteRook, 275, 685, 86, -8.0f);
    DrawDecorativeSprite(textures.blackKing, 825, 665, 86, 8.0f);

    DrawCenteredText("UNDIES", 95, 64, MENU_TEAL_DARK);
    DrawCenteredText("CHESS", 155, 82, MENU_TEAL);
    DrawCenteredText("Kaiju Interactive", 255, 20, Color{ 87, 117, 117, 255 });

    if (MenuButton({ 360,330,400,68 }, "PLAY")) screenState = MODE_SCREEN;
    if (MenuButton({ 360,415,400,62 }, "OPTIONS")) screenState = OPTIONS_SCREEN;
    if (MenuButton({ 360,495,400,58 }, "QUIT")) quitRequested = true;

    DrawCenteredText("The ancient game of strategy. In your underwear.", 620, 18, MENU_INK);
    DrawCenteredText("v1.0 release candidate", 668, 14, Color{ 112, 145, 145, 255 });
}

void DrawModeScreen()
{
    DrawMenuBackdrop();
    DrawText("CHOOSE YOUR BATTLE", SCREEN_WIDTH / 2 - MeasureText("CHOOSE YOUR BATTLE", 42) / 2, 150, 42, MENU_TEAL);
    if (MenuButton({ 340,285,440,70 }, "VS COMPUTER")) { gameMode = VS_COMPUTER; screenState = DIFFICULTY_SCREEN; }
    if (MenuButton({ 340,380,440,70 }, "2 PLAYERS")) { gameMode = TWO_PLAYER; ResetGame(); screenState = GAME_SCREEN; }
    if (MenuButton({ 340,500,440,58 }, "BACK")) screenState = TITLE_SCREEN;
}

void DrawOptionsScreen()
{
    DrawMenuBackdrop();
    DrawCenteredText("OPTIONS", 95, 46, MENU_TEAL);
    DrawCenteredText("A little support goes a long way.", 155, 18, MENU_INK);

    const char* sfxLabel = sfxEnabled ? "SOUND EFFECTS: ON" : "SOUND EFFECTS: OFF";
    const char* musicLabel = musicEnabled ? "MENU MUSIC: ON" : "MENU MUSIC: OFF";

    if (MenuButton({ 330,245,460,62 }, sfxLabel)) sfxEnabled = !sfxEnabled;
    if (MenuButton({ 330,325,460,62 }, musicLabel)) musicEnabled = !musicEnabled;

    DrawCenteredText(TextFormat("VOLUME: %i%%", (int)(masterVolume * 100.0f + 0.5f)), 430, 24, MENU_TEAL_DARK);

    if (MenuButton({ 365,480,170,58 }, "VOLUME -"))
    {
        masterVolume -= 0.10f;
        if (masterVolume < 0.0f) masterVolume = 0.0f;
        ApplyAudioSettings();
    }

    if (MenuButton({ 585,480,170,58 }, "VOLUME +"))
    {
        masterVolume += 0.10f;
        if (masterVolume > 1.0f) masterVolume = 1.0f;
        ApplyAudioSettings();
    }

    if (MenuButton({ 410,590,300,58 }, "BACK")) screenState = TITLE_SCREEN;
}

void StartAIGame(AIDifficulty d)
{
    aiDifficulty = d; gameMode = VS_COMPUTER; ResetGame(); aiThinking = false; screenState = GAME_SCREEN;
}

void DrawDifficultyScreen()
{
    DrawMenuBackdrop();
    DrawText("CHOOSE YOUR OPPONENT", SCREEN_WIDTH / 2 - MeasureText("CHOOSE YOUR OPPONENT", 38) / 2, 80, 38, MENU_TEAL);
    DrawText("You are White", SCREEN_WIDTH / 2 - MeasureText("You are White", 18) / 2, 130, 18, MENU_INK);
    const char* names[5] = { "LAUNDRY DAY","CASUAL FRIDAY","TIGHT FIT","COMMANDO","JOCKED" };
    for (int i = 0;i < 5;i++) if (MenuButton({ 310.0f,190.0f + i * 82.0f,500.0f,60.0f }, names[i])) StartAIGame((AIDifficulty)i);
    if (MenuButton({ 410,625,300,52 }, "BACK")) screenState = MODE_SCREEN;
}


void DrawReviewMoveHighlight()
{
    if (!reviewMode || moveHistory.empty())
        return;

    const MoveHistoryEntry& e = moveHistory[reviewIndex];
    if (e.sr < 0 || e.er < 0)
        return;

    Rectangle fromRect = {
        (float)(BOARD_X + e.sc * TILE_SIZE),
        (float)(BOARD_Y + e.sr * TILE_SIZE),
        (float)TILE_SIZE,
        (float)TILE_SIZE
    };

    Rectangle toRect = {
        (float)(BOARD_X + e.ec * TILE_SIZE),
        (float)(BOARD_Y + e.er * TILE_SIZE),
        (float)TILE_SIZE,
        (float)TILE_SIZE
    };

    DrawRectangleLinesEx(fromRect, 5, SKYBLUE);
    DrawRectangleLinesEx(toRect, 5, GOLD);
}

void DrawReviewBanner()
{
    if (!reviewMode || moveHistory.empty())
        return;

    const MoveHistoryEntry& e = moveHistory[reviewIndex];

    DrawRectangle(BOARD_X, 4, BOARD_SIZE * TILE_SIZE, 31, Color{ 0, 0, 0, 205 });
    DrawText(
        TextFormat("REVIEW  %i / %i", reviewIndex, (int)moveHistory.size() - 1),
        BOARD_X + 10, 10, 17, GOLD);

    std::string description = e.text;
    if (e.sr >= 0)
        description += "   [" + GetSquareName(e.sr, e.sc) + " -> " + GetSquareName(e.er, e.ec) + "]";

    int tw = MeasureText(description.c_str(), 15);
    DrawText(description.c_str(), BOARD_X + BOARD_SIZE * TILE_SIZE - tw - 10, 12, 15, RAYWHITE);
}

void ExitReviewMode()
{
    reviewMode = false;
    reviewIndex = moveHistory.empty() ? 0 : (int)moveHistory.size() - 1;
}

void DrawGameControls()
{
    if (promotionPending)
        return;

    if (reviewMode)
    {
        bool canPrev = reviewIndex > 0;
        bool canNext = reviewIndex < (int)moveHistory.size() - 1;

        if (canPrev && MenuButton({ 790,748,82,38 }, "< PREV")) reviewIndex--;
        else if (!canPrev) { DrawRectangle(790, 748, 82, 38, Color{ 40,40,44,255 }); DrawText("< PREV", 799, 759, 15, DARKGRAY); }

        if (canNext && MenuButton({ 878,748,82,38 }, "NEXT >")) reviewIndex++;
        else if (!canNext) { DrawRectangle(882, 748, 82, 38, Color{ 40,40,44,255 }); DrawText("NEXT >", 890, 759, 15, DARKGRAY); }

        const char* returnLabel = gameOver ? "RESULT" : "LIVE";
        if (MenuButton({ 974,748,101,38 }, returnLabel)) ExitReviewMode();
        return;
    }

    if (!gameOver)
    {
        if (MenuButton({ 790,748,84,38 }, "REVIEW"))
        {
            reviewMode = true;
            reviewIndex = (int)moveHistory.size() - 1;
            aiThinking = false;
        }

        if (MenuButton({ 884,748,91,38 }, "REMATCH"))
        {
            ResetGame();
            aiThinking = false;
        }

        if (MenuButton({ 985,748,90,38 }, "MENU"))
        {
            aiThinking = false;
            screenState = MODE_SCREEN;
        }
    }
}

void DrawEndGameButtons()
{
    if (!gameOver) return;

    // Post-game hub: let the player inspect the finished game
    // before deciding whether to rematch or return to the menu.
    if (MenuButton({ 245,500,190,58 }, "REVIEW GAME"))
    {
        if (!moveHistory.empty())
        {
            reviewMode = true;
            reviewIndex = (int)moveHistory.size() - 1;
            aiThinking = false;
        }
    }

    if (MenuButton({ 465,500,190,58 }, "REMATCH"))
    {
        ResetGame();
        aiThinking = false;
    }

    if (MenuButton({ 685,500,190,58 }, "MENU"))
    {
        aiThinking = false;
        screenState = MODE_SCREEN;
    }
}

// ============================================================
// INPUT
// ============================================================

void HandleInput()
{
    if (reviewMode)
    {
        if (IsKeyPressed(KEY_LEFT) && reviewIndex > 0) reviewIndex--;
        if (IsKeyPressed(KEY_RIGHT) && reviewIndex < (int)moveHistory.size() - 1) reviewIndex++;
        if (IsKeyPressed(KEY_HOME)) reviewIndex = 0;
        if (IsKeyPressed(KEY_END)) reviewIndex = (int)moveHistory.size() - 1;
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_V)) ExitReviewMode();
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) { screenState = MODE_SCREEN; return; }

    // Promotion must get first crack at R, because R is also reset.
    if (promotionPending)
    {
        if (gameMode == VS_COMPUTER && currentTurn == aiColor) { PromotePawn(QUEEN); return; }
        if (IsKeyPressed(KEY_Q)) { PromotePawn(QUEEN); return; }
        if (IsKeyPressed(KEY_R)) { PromotePawn(ROOK); return; }
        if (IsKeyPressed(KEY_B)) { PromotePawn(BISHOP); return; }
        if (IsKeyPressed(KEY_N)) { PromotePawn(KNIGHT); return; }
        return;
    }

    if (IsKeyPressed(KEY_R)) { ResetGame(); aiThinking = false; return; }
    if (gameOver) return;

    if (gameMode == VS_COMPUTER && currentTurn == aiColor)
    {
        if (!aiThinking) { aiThinking = true; aiMoveAt = GetTime() + 0.35; }
        if (GetTime() >= aiMoveAt) { DoComputerMove(); aiThinking = false; }
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    {
        pieceSelected = false; selectedRow = selectedCol = -1; legalMoves.clear(); return;
    }
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;

    Vector2 mouse = GetMousePosition();
    if (mouse.x < BOARD_X || mouse.x >= BOARD_X + BOARD_SIZE * TILE_SIZE ||
        mouse.y < BOARD_Y || mouse.y >= BOARD_Y + BOARD_SIZE * TILE_SIZE) return;
    int col = (int)((mouse.x - BOARD_X) / TILE_SIZE);
    int row = (int)((mouse.y - BOARD_Y) / TILE_SIZE);
    Piece clickedPiece = board[row][col];

    if (!pieceSelected)
    {
        if (clickedPiece.type != EMPTY && clickedPiece.color == currentTurn)
        {
            selectedRow = row; selectedCol = col; pieceSelected = true; GenerateLegalMoves(row, col);
        }
        return;
    }
    if (clickedPiece.type != EMPTY && clickedPiece.color == currentTurn)
    {
        selectedRow = row; selectedCol = col; GenerateLegalMoves(row, col); return;
    }
    if (IsMoveLegal(selectedRow, selectedCol, row, col)) MovePiece(selectedRow, selectedCol, row, col);
}

// ============================================================
// MAIN
// ============================================================

int main()
{
    InitWindow(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        "Undies Chess"
    );

    InitAudioDevice();
    InitGameAudio();
    splashStartTime = GetTime();

    SetTargetFPS(60);

    PieceTextures textures;

    // ========================================================
    // LOAD WHITE
    // ========================================================

    textures.whitePawn =
        LoadTexture(
            "Assets/white_pawn.png"
        );

    textures.whiteRook =
        LoadTexture(
            "Assets/white_rook.png"
        );

    textures.whiteKnight =
        LoadTexture(
            "Assets/white_knight.png"
        );

    textures.whiteBishop =
        LoadTexture(
            "Assets/white_bishop.png"
        );

    textures.whiteQueen =
        LoadTexture(
            "Assets/white_queen.png"
        );

    textures.whiteKing =
        LoadTexture(
            "Assets/white_king.png"
        );

    // ========================================================
    // LOAD BLACK
    // ========================================================

    textures.blackPawn =
        LoadTexture(
            "Assets/black_pawn.png"
        );

    textures.blackRook =
        LoadTexture(
            "Assets/black_rook.png"
        );

    textures.blackKnight =
        LoadTexture(
            "Assets/black_knight.png"
        );

    textures.blackBishop =
        LoadTexture(
            "Assets/black_bishop.png"
        );

    textures.blackQueen =
        LoadTexture(
            "Assets/black_queen.png"
        );

    textures.blackKing =
        LoadTexture(
            "Assets/black_king.png"
        );

    // ========================================================
    // CRISP PIXEL ART
    // ========================================================

    SetTexturePixelMode(
        textures.whitePawn
    );

    SetTexturePixelMode(
        textures.whiteRook
    );

    SetTexturePixelMode(
        textures.whiteKnight
    );

    SetTexturePixelMode(
        textures.whiteBishop
    );

    SetTexturePixelMode(
        textures.whiteQueen
    );

    SetTexturePixelMode(
        textures.whiteKing
    );

    SetTexturePixelMode(
        textures.blackPawn
    );

    SetTexturePixelMode(
        textures.blackRook
    );

    SetTexturePixelMode(
        textures.blackKnight
    );

    SetTexturePixelMode(
        textures.blackBishop
    );

    SetTexturePixelMode(
        textures.blackQueen
    );

    SetTexturePixelMode(
        textures.blackKing
    );

    ResetGame();

    // ========================================================
    // GAME LOOP
    // ========================================================

    while (!WindowShouldClose() && !quitRequested)
    {
        // Procedural music only plays in front-end menus, never over a chess game.
        bool inMenu = screenState != GAME_SCREEN && screenState != SPLASH_SCREEN;
        if (audioReady)
        {
            if (musicEnabled && inMenu)
            {
                if (!IsSoundPlaying(menuMusic)) PlaySound(menuMusic);
            }
            else if (IsSoundPlaying(menuMusic))
            {
                StopSound(menuMusic);
            }
        }

        if (screenState == GAME_SCREEN) HandleInput();

        BeginDrawing();

        ClearBackground(
            Color{
                22,
                24,
                28,
                255
            }
        );

        if (screenState == SPLASH_SCREEN)
        {
            DrawSplashScreen();
        }
        else if (screenState == TITLE_SCREEN)
        {
            DrawTitleScreen(textures);
        }
        else if (screenState == MODE_SCREEN)
        {
            DrawModeScreen();
        }
        else if (screenState == DIFFICULTY_SCREEN)
        {
            DrawDifficultyScreen();
        }
        else if (screenState == OPTIONS_SCREEN)
        {
            DrawOptionsScreen();
        }
        else
        {
            if (reviewMode && !moveHistory.empty())
            {
                GameSnapshot liveState = SaveSnapshot();
                LoadHistoryEntryForDisplay(moveHistory[reviewIndex]);

                DrawBoard(textures);
                DrawSidePanel();
                DrawReviewMoveHighlight();
                DrawReviewBanner();

                LoadSnapshot(liveState);
                DrawGameControls();
            }
            else
            {
                DrawBoard(textures);
                DrawSidePanel();
                if (gameMode == VS_COMPUTER)
                {
                    DrawText(TextFormat("VS AI: %s", DifficultyName()), 800, 10, 14, GRAY);
                    if (aiThinking) DrawText("Computer is thinking...", 800, 225, 16, GOLD);
                }
                DrawGameControls();
                DrawPromotionScreen();
                DrawEndGameScreen();
                DrawEndGameButtons();
            }
        }

        EndDrawing();
    }

    // ========================================================
    // CLEANUP
    // ========================================================

    UnloadTexture(
        textures.whitePawn
    );

    UnloadTexture(
        textures.whiteRook
    );

    UnloadTexture(
        textures.whiteKnight
    );

    UnloadTexture(
        textures.whiteBishop
    );

    UnloadTexture(
        textures.whiteQueen
    );

    UnloadTexture(
        textures.whiteKing
    );

    UnloadTexture(
        textures.blackPawn
    );

    UnloadTexture(
        textures.blackRook
    );

    UnloadTexture(
        textures.blackKnight
    );

    UnloadTexture(
        textures.blackBishop
    );

    UnloadTexture(
        textures.blackQueen
    );

    UnloadTexture(
        textures.blackKing
    );

    UnloadGameAudio();
    CloseAudioDevice();

    CloseWindow();

    return 0;
}