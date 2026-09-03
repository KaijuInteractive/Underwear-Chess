#include "raylib.h"
#include <string>
#include <vector>
#include <cmath>

// ============================================================
// UNDERWEAR CHESS v0.3
// Kaiju Interactive
//
// PAWN   = Briefs
// ROOK   = Long Sock
// KNIGHT = Short Sock
// BISHOP = Jock
// QUEEN  = Boxers
// KING   = Boxer Briefs
//
// Current rules:
// - Normal piece movement
// - Captures
// - Alternating turns
// - Pawn double move
// - Automatic queen promotion
// - Capture the King = Victory
//
// Coming later:
// - Check
// - Checkmate
// - Stalemate
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
PieceColor winner = NONE;

// ============================================================
// HELPERS
// ============================================================

bool IsInsideBoard(int row, int col)
{
    return row >= 0 &&
        row < BOARD_SIZE &&
        col >= 0 &&
        col < BOARD_SIZE;
}

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

std::string GetSquareName(int row, int col)
{
    char file = 'a' + col;
    char rank = '8' - row;

    std::string result;

    result += file;
    result += rank;

    return result;
}

const char* GetColorName(PieceColor color)
{
    if (color == WHITE_SIDE)
        return "WHITE";

    if (color == BLACK_SIDE)
        return "BLACK";

    return "";
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

void ResetGame()
{
    ClearBoard();

    // Black
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

    // White
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
    winner = NONE;
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
// PAWN
// ============================================================

bool IsPawnMoveLegal(
    int startRow,
    int startCol,
    int endRow,
    int endCol)
{
    Piece piece =
        board[startRow][startCol];

    int direction;

    if (piece.color == WHITE_SIDE)
        direction = -1;
    else
        direction = 1;

    int rowDifference =
        endRow - startRow;

    int colDifference =
        endCol - startCol;

    // One square forward
    if (colDifference == 0)
    {
        if (rowDifference == direction &&
            board[endRow][endCol].type == EMPTY)
        {
            return true;
        }

        // Two squares on first move
        if (!piece.hasMoved &&
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
    }

    // Capture
    if (std::abs(colDifference) == 1 &&
        rowDifference == direction)
    {
        if (board[endRow][endCol].type != EMPTY &&
            board[endRow][endCol].color !=
            piece.color)
        {
            return true;
        }
    }

    return false;
}

// ============================================================
// ROOK
// ============================================================

bool IsRookMoveLegal(
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

// ============================================================
// KNIGHT
// ============================================================

bool IsKnightMoveLegal(
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

// ============================================================
// BISHOP
// ============================================================

bool IsBishopMoveLegal(
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

// ============================================================
// QUEEN
// ============================================================

bool IsQueenMoveLegal(
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

    if (!straight && !diagonal)
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

// ============================================================
// KING
// ============================================================

bool IsKingMoveLegal(
    int startRow,
    int startCol,
    int endRow,
    int endCol)
{
    int rowDifference =
        std::abs(endRow - startRow);

    int colDifference =
        std::abs(endCol - startCol);

    return rowDifference <= 1 &&
        colDifference <= 1;
}

// ============================================================
// MASTER MOVE CHECK
// ============================================================

bool IsMoveLegal(
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

    // Cannot capture own pieces
    if (destination.type != EMPTY &&
        destination.color ==
        movingPiece.color)
    {
        return false;
    }

    switch (movingPiece.type)
    {
    case PAWN:

        return IsPawnMoveLegal(
            startRow,
            startCol,
            endRow,
            endCol
        );

    case ROOK:

        return IsRookMoveLegal(
            startRow,
            startCol,
            endRow,
            endCol
        );

    case KNIGHT:

        return IsKnightMoveLegal(
            startRow,
            startCol,
            endRow,
            endCol
        );

    case BISHOP:

        return IsBishopMoveLegal(
            startRow,
            startCol,
            endRow,
            endCol
        );

    case QUEEN:

        return IsQueenMoveLegal(
            startRow,
            startCol,
            endRow,
            endCol
        );

    case KING:

        return IsKingMoveLegal(
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
    // CAPTURE
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
    // MOVE IT
    // ========================================================

    board[endRow][endCol] =
        movingPiece;

    board[endRow][endCol].hasMoved =
        true;

    board[startRow][startCol] = {};

    // ========================================================
    // PROMOTION
    // ========================================================

    if (board[endRow][endCol].type ==
        PAWN)
    {
        if (endRow == 0 ||
            endRow == 7)
        {
            board[endRow][endCol].type =
                QUEEN;

            lastMove +=
                " - promoted to Queen";
        }
    }

    // ========================================================
    // DID WE CAPTURE THE KING?
    // ========================================================

    if (capturedPiece.type == KING)
    {
        gameOver = true;
        winner = movingPiece.color;

        lastMove += " - VICTORY!";

        pieceSelected = false;
        selectedRow = -1;
        selectedCol = -1;

        legalMoves.clear();

        return;
    }

    // ========================================================
    // CHANGE TURN
    // ========================================================

    if (currentTurn == WHITE_SIDE)
    {
        currentTurn = BLACK_SIDE;
    }
    else
    {
        currentTurn = WHITE_SIDE;

        moveNumber++;
    }

    pieceSelected = false;

    selectedRow = -1;
    selectedCol = -1;

    legalMoves.clear();
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

void SetTexturePixelMode(
    Texture2D texture)
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
// PIECE LABELS
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

    char label =
        GetPieceLetter(
            piece.type
        );

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
            label
        ),
        x + 8,
        y + 5,
        16,
        RAYWHITE
    );
}

// ============================================================
// BOARD
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

            // Selected square
            if (pieceSelected &&
                row == selectedRow &&
                col == selectedCol)
            {
                DrawRectangleRec(
                    square,
                    selectedColor
                );
            }

            // Legal move
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
        }
    }

    // File letters
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

    // Rank numbers
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
    // TURN / WINNER
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

        if (currentTurn ==
            WHITE_SIDE)
        {
            DrawText(
                "WHITE",
                panelX,
                165,
                26,
                RAYWHITE
            );
        }
        else
        {
            DrawText(
                "BLACK",
                panelX,
                165,
                26,
                RED
            );
        }

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
    }
    else
    {
        DrawText(
            "WINNER",
            panelX,
            140,
            18,
            GRAY
        );

        Color winnerColor =
            winner == WHITE_SIDE
            ? RAYWHITE
            : RED;

        DrawText(
            GetColorName(winner),
            panelX,
            165,
            28,
            winnerColor
        );

        DrawText(
            "THE KING HAS FALLEN",
            panelX,
            200,
            16,
            GOLD
        );
    }

    // ========================================================
    // SELECTED
    // ========================================================

    DrawLine(
        panelX,
        230,
        SCREEN_WIDTH - 25,
        230,
        DARKGRAY
    );

    DrawText(
        "SELECTED",
        panelX,
        250,
        17,
        GRAY
    );

    if (pieceSelected)
    {
        Piece piece =
            board[selectedRow][selectedCol];

        std::string chessName =
            GetChessPieceName(
                piece.type
            );

        std::string underwearName =
            GetPieceName(
                piece.type
            );

        std::string square =
            GetSquareName(
                selectedRow,
                selectedCol
            );

        DrawText(
            chessName.c_str(),
            panelX,
            275,
            22,
            RAYWHITE
        );

        DrawText(
            underwearName.c_str(),
            panelX,
            302,
            17,
            GOLD
        );

        DrawText(
            square.c_str(),
            panelX,
            327,
            17,
            LIGHTGRAY
        );
    }
    else
    {
        DrawText(
            "None",
            panelX,
            280,
            20,
            DARKGRAY
        );
    }

    // ========================================================
    // LAST MOVE
    // ========================================================

    DrawLine(
        panelX,
        360,
        SCREEN_WIDTH - 25,
        360,
        DARKGRAY
    );

    DrawText(
        "LAST MOVE",
        panelX,
        380,
        17,
        GRAY
    );

    DrawText(
        lastMove.c_str(),
        panelX,
        405,
        15,
        LIGHTGRAY
    );

    // ========================================================
    // LEGEND
    // ========================================================

    DrawLine(
        panelX,
        445,
        SCREEN_WIDTH - 25,
        445,
        DARKGRAY
    );

    DrawText(
        "THE UNDERWEAR ARMY",
        panelX,
        465,
        18,
        RAYWHITE
    );

    DrawLegendLine(
        panelX,
        495,
        "P",
        "Briefs",
        "Pawn"
    );

    DrawLegendLine(
        panelX,
        535,
        "R",
        "Long Sock",
        "Rook"
    );

    DrawLegendLine(
        panelX,
        575,
        "N",
        "Short Sock",
        "Knight"
    );

    DrawLegendLine(
        panelX,
        615,
        "B",
        "Jock",
        "Bishop"
    );

    DrawLegendLine(
        panelX,
        655,
        "Q",
        "Boxers",
        "Queen"
    );

    DrawLegendLine(
        panelX,
        695,
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
// VICTORY SCREEN
// ============================================================

void DrawVictoryScreen()
{
    if (!gameOver)
    {
        return;
    }

    // Dark overlay
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

    const char* winnerText;

    if (winner == WHITE_SIDE)
    {
        winnerText =
            "WHITE WINS!";
    }
    else
    {
        winnerText =
            "BLACK WINS!";
    }

    int winnerFontSize = 58;

    int winnerWidth =
        MeasureText(
            winnerText,
            winnerFontSize
        );

    DrawText(
        winnerText,
        SCREEN_WIDTH / 2 -
        winnerWidth / 2,
        280,
        winnerFontSize,
        GOLD
    );

    const char* message =
        "THE BOXER BRIEFS HAVE FALLEN";

    int messageFontSize = 24;

    int messageWidth =
        MeasureText(
            message,
            messageFontSize
        );

    DrawText(
        message,
        SCREEN_WIDTH / 2 -
        messageWidth / 2,
        355,
        messageFontSize,
        RAYWHITE
    );

    const char* rematch =
        "Press R for a rematch";

    int rematchFontSize = 22;

    int rematchWidth =
        MeasureText(
            rematch,
            rematchFontSize
        );

    DrawText(
        rematch,
        SCREEN_WIDTH / 2 -
        rematchWidth / 2,
        410,
        rematchFontSize,
        LIGHTGRAY
    );
}

// ============================================================
// INPUT
// ============================================================

void HandleMouseInput()
{
    // No movement once somebody wins
    if (gameOver)
    {
        return;
    }

    // Right-click cancels selection
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

    if (!IsInsideBoard(
        row,
        col))
    {
        return;
    }

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
    // CHANGE SELECTION
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
    // PIXEL FILTERING
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
        HandleMouseInput();

        if (IsKeyPressed(KEY_R))
        {
            ResetGame();
        }

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

        DrawVictoryScreen();

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