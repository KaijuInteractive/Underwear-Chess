#include "raylib.h"
#include <string>
#include <vector>
#include <cmath>

// ============================================================
// UNDERWEAR CHESS v1.0
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

        return;
    }

    FinishTurn();
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

    FinishTurn();
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
        "UNDERWEAR",
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

    DrawText(
        "R = Reset",
        panelX,
        755,
        15,
        GRAY
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
        std::string title =
            std::string(
                GetColorName(winner)
            )
            + " WINS!";

        int titleSize = 58;

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
            "THE BOXER BRIEFS HAVE BEEN DEFEATED";

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
        "Press R for a rematch";

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
// INPUT
// ============================================================

void HandleInput()
{
    // ========================================================
    // RESET
    // ========================================================

    if (IsKeyPressed(KEY_R))
    {
        ResetGame();
        return;
    }

    // ========================================================
    // GAME OVER
    // ========================================================

    if (gameOver)
    {
        return;
    }

    // ========================================================
    // PROMOTION CHOICE
    // ========================================================

    if (promotionPending)
    {
        if (IsKeyPressed(KEY_Q))
        {
            PromotePawn(QUEEN);
            return;
        }

        if (IsKeyPressed(KEY_R))
        {
            PromotePawn(ROOK);
            return;
        }

        if (IsKeyPressed(KEY_B))
        {
            PromotePawn(BISHOP);
            return;
        }

        if (IsKeyPressed(KEY_N))
        {
            PromotePawn(KNIGHT);
            return;
        }

        return;
    }

    // ========================================================
    // RIGHT CLICK = CANCEL
    // ========================================================

    if (IsMouseButtonPressed(
        MOUSE_BUTTON_RIGHT))
    {
        pieceSelected = false;

        selectedRow = -1;
        selectedCol = -1;

        legalMoves.clear();

        return;
    }

    if (!IsMouseButtonPressed(
        MOUSE_BUTTON_LEFT))
    {
        return;
    }

    Vector2 mouse =
        GetMousePosition();

    // Important: reject clicks outside the actual board before
    // converting them to row/column coordinates.
    if (mouse.x < BOARD_X ||
        mouse.x >= BOARD_X +
        BOARD_SIZE * TILE_SIZE ||
        mouse.y < BOARD_Y ||
        mouse.y >= BOARD_Y +
        BOARD_SIZE * TILE_SIZE)
    {
        return;
    }

    int col =
        (int)(
            (mouse.x - BOARD_X)
            / TILE_SIZE
            );

    int row =
        (int)(
            (mouse.y - BOARD_Y)
            / TILE_SIZE
            );

    Piece clickedPiece =
        board[row][col];

    // ========================================================
    // SELECT
    // ========================================================

    if (!pieceSelected)
    {
        if (clickedPiece.type != EMPTY &&
            clickedPiece.color ==
            currentTurn)
        {
            selectedRow = row;
            selectedCol = col;

            pieceSelected = true;

            GenerateLegalMoves(
                row,
                col
            );
        }

        return;
    }

    // ========================================================
    // SELECT DIFFERENT FRIENDLY PIECE
    // ========================================================

    if (clickedPiece.type != EMPTY &&
        clickedPiece.color ==
        currentTurn)
    {
        selectedRow = row;
        selectedCol = col;

        GenerateLegalMoves(
            row,
            col
        );

        return;
    }

    // ========================================================
    // MOVE
    // ========================================================

    if (IsMoveLegal(
        selectedRow,
        selectedCol,
        row,
        col))
    {
        MovePiece(
            selectedRow,
            selectedCol,
            row,
            col
        );
    }
}

// ============================================================
// MAIN
// ============================================================

int main()
{
    InitWindow(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        "Underwear Chess"
    );

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

    while (!WindowShouldClose())
    {
        HandleInput();

        BeginDrawing();

        ClearBackground(
            Color{
                22,
                24,
                28,
                255
            }
        );

        DrawBoard(
            textures
        );

        DrawSidePanel();

        DrawPromotionScreen();

        DrawEndGameScreen();

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

    CloseWindow();

    return 0;
}