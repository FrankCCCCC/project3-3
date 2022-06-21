#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <renju_api.h>

using namespace std;

typedef std::string BoardRow;
typedef std::vector<BoardRow> Board;
typedef int Score;

#define B_SIZE 15
#define OC 'O'
#define XC 'X'
#define DC '.'
#define get_tuple(t, i) std::get<i>(t)

// Unify coordinate expression
#define SEQ_LEN 17
#define SEQ_TAIL 'Z'
const char SEQ[SEQ_LEN] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'Z' };
inline char seq(int x) {
    if (x < SEQ_LEN) return SEQ[x];
    return SEQ_TAIL;
}
class Position {
    friend ostream& operator<<(ostream&, const Position&);
    friend bool operator<(const Position&, const Position&);
    friend Position operator+(const Position&, const Position&);

public:
    static const int BOARD_SIZE;
    int r = 0, c = 0, idx = 0;
    Position() { this->r = this->c = 0; }
    Position(int r, int c) {
        this->r = r;
        this->c = c;
        this->idx = r * Position::BOARD_SIZE + c;
    }
};
const int Position::BOARD_SIZE = B_SIZE;
ostream& operator<<(ostream& os, const Position& pos) {
    os << "P(" << seq(pos.r) << ", " << seq(pos.c) << ")";
    return os;
}
bool operator<(const Position& a, const Position& b) {
    if (a.r != b.r) return a.r < b.r;
    return a.c < b.c;
}
Position operator+(const Position& a, const Position& b) {
    return Position(a.r + b.r, a.c + b.c);
}

ostream& operator<<(ostream& os, const Board& b) {
    string bar("===============================");
    os << bar << endl;
    os << "+ ";
    for (int i = 0; i < b.size(); i++) {
        os << seq(i) << " ";
    }
    os << endl;

    for (int i = 0; i < b.size(); i++) {
        os << seq(i) << " ";
        for (int j = 0; j < b.at(0).size(); j++) {
            os << b[i][j] << " ";
        }
        os << endl;
    }
    os << bar << endl;
    return os;
}

class IO {
protected:
    int b_size = 0;
    std::ifstream fin;
    std::ofstream fout;
    char player = 0;

public:
    Board board;

    IO(int b_size, const string& infile, const string& outfile) {
        this->b_size = b_size;
        this->board = Board(b_size, BoardRow(b_size, 0));
        this->fin = std::ifstream(infile);
        this->fout = std::ofstream(outfile);
    }
    inline char get_player(){return this->player;}

    void read_board() {
        this->fin >> this->player;
        for (int i = 0; i < b_size; i++) {
            for (int j = 0; j < b_size; j++) {
                char c = 0;
                // this->fin >> this->board[i][j];
                this->fin >> c;
                if (this->player == c) {
                    // Denote the AI player as 'O'
                    this->board[i][j] = OC;
                }else if (c == '0') { 
                    // Denote the space as '.'
                    this->board[i][j] = DC; 
                }else{this->board[i][j] = XC; }
            }
        }
    }
    void write_valid_spot(const Position& pos) {
        if (this->board[pos.r][pos.c] == DC) {
            this->fout << pos.r << " " << pos.c << std::endl;
            // Remember to flush the output to ensure the last action is written to file.
            this->fout.flush();
        }
    }
    void rand_spot(Position& pos, Score& score) {
        srand(time(NULL));
        int x, y;
        // Keep updating the output until getting killed.
        for (int i = 0; i < 30; i++) {
            // Choose a random spot.
            int x = (rand() % this->b_size);
            int y = (rand() % this->b_size);
            if (this->board[x][y] == DC) {
                pos.r = x;
                pos.c = y;
                this->fout << x << " " << y << std::endl;
                // Remember to flush the output to ensure the last action is written to file.
                this->fout.flush();
            }
        }
    }
};

int main(){
    string infile("state"), outfile("action");
    // string infile("input.txt"), outfile("output.txt");
    IO io(B_SIZE, infile, outfile);
    io.read_board();
    // std::cout << io.board << endl;

    bool is_first_hand = true;
    char gs_string[B_SIZE * B_SIZE] = {0};
    for(int i = 0; i < io.board.size(); i++){
        for(int j = 0; j < io.board.at(0).size(); j++){
            if(io.board[i][j] == OC){
                gs_string[i * B_SIZE + j] = '1';
                is_first_hand = false;
            }else if(io.board[i][j] == XC){
                gs_string[i * B_SIZE + j] = '2';
                is_first_hand = false;
            }else{
                gs_string[i * B_SIZE + j] = '0';
            }
        }
    }
    if(is_first_hand){
        Position p;
        Score sc;
        io.rand_spot(p, sc);
        return 0;
    }

    int time_limit = 10000;
    int move_r, move_c, winning_player, actual_depth;
    unsigned int node_count, eval_count;
    bool success = RenjuAPI::generateMove(gs_string, 1, -1, time_limit, 1, &actual_depth, &move_r, &move_c,
                                          &winning_player, &node_count, &eval_count, nullptr);

    io.write_valid_spot(Position(move_r, move_c));
    if(success){
        std::cout << "Success actual_depth: " << actual_depth << " Act: (" << move_r << ", " << move_c << ")" << " winning_player: " << winning_player << " node_count: " << node_count << " eval_count: " << eval_count << endl;
    }else{
        std::cout << "Fail actual_depth: " << actual_depth << " Act: (" << move_r << ", " << move_c << ")" << " winning_player: " << winning_player << " node_count: " << node_count << " eval_count: " << eval_count << endl;
    }
    
    return 0;
}