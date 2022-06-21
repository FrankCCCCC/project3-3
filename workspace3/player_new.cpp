#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <ctime>
#include <algorithm>
#include <random>
// #include <renju_api.h>

using namespace std;

typedef std::string BoardRow;
typedef std::vector<BoardRow> Board;
typedef int Score;

#define G_B_SIZE 15
#define G_B_AREA 225
#define OC 'O'
#define XC 'X'
#define DC '.'
#define WIN_COND 5
#define get_tuple(t, i) std::get<i>(t)
#define M_DIR_NUM 4
#define REMOTE_RNG 2
#define PATTERN_PAIR_CNT 2

#define DEBUG 0
#define debug_cpp(s)\
if(DEBUG){\
    std::cout << s;\
}\

#define THROWERR 1
#define ERR(s)\
if(THROWERR){\
    std::cerr << "Error: "<< s << " (" << __FILE__ << ", Line " << __LINE__ << ")" << endl;\
    exit(1);\
}\

// #define NULL_CHECK(p) (p == nullptr? (THROWERR? ERR("Found nullptr") : true) : false)
#define ERR_NULL_CHECK(p, ret)\
if(p == nullptr){\
    ERR("Found nullptr");\
    return ret;\
}\

#define ERR_PLAYER_CHECK(p, ret)\
if(player != 1 && player != 2){\
    ERR("The player should be 1 or 2");\
    return ret;\
}\

#define ERR_POS_CHECK(r, c, ret)\
if (r < 0 || r >= G_B_SIZE || c < 0 || c >= G_B_SIZE){\
    ERR("Invalid position");\
    return ret;\
}\

#define NULL_CHECK(p, ret)\
if(p == nullptr){\
    return ret;\
}\

#define POS_CHECK(r, c, ret)\
if (r < 0 || r >= G_B_SIZE || c < 0 || c >= G_B_SIZE){\
    return ret;\
}\

inline bool pos_check(int r, int c){
    return (r < 0 || r >= G_B_SIZE || c < 0 || c >= G_B_SIZE);
}

inline int _2d_1d(int r, int c){
    return G_B_SIZE * r + c;
}

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
const int Position::BOARD_SIZE = G_B_SIZE;
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
    void rand_spot(Position& pos, Score& sc) {
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

// const int G_B_SIZE = 15;
// const unsigned int G_B_AREA = 225;
unsigned int g_node_cnt = 0;
unsigned int g_eval_cnt = 0;
unsigned int g_pattern_match_cnt = 0;
unsigned int g_cc_0 = 0;
unsigned int g_cc_1 = 0;

class Util {
    public:
    Util();
    ~Util();

    static inline char get_spot(const char *state, int r, int c) {
        // if (r < 0 || r >= G_B_SIZE || c < 0 || c >= G_B_SIZE) return -1;
        POS_CHECK(r, c, -1)
        // return state[G_B_SIZE * r + c];
           return state[_2d_1d(r, c)];
    }

    static inline bool set_spot(char *state, int r, int c, char value) {
        // if (r < 0 || r >= G_B_SIZE || c < 0 || c >= G_B_SIZE) return false;
        POS_CHECK(r, c, false)
        // state[G_B_SIZE * r + c] = value;
        state[_2d_1d(r, c)] = value;
        return true;
    }

    static bool remote_spot(const char *state, int r, int c);

    // Game state hashing
    static void zobristInit(int size, uint64_t *z1, uint64_t *z2);
    static uint64_t zobristHash(const char *state, uint64_t *z1, uint64_t *z2);
    // static inline void zobristToggle(uint64_t *state, uint64_t *z1, uint64_t *z2,
    //                                  int row_size, int r, int c, int player) {
    //     if (player == 1) {
    //         *state ^= z1[row_size * r + c];
    //     } else if (player == 2) {
    //         *state ^= z2[row_size * r + c];
    //     }
    // }
};

class Controller {
    public:
    Controller();
    ~Controller();
    static void search_act(const char *state, int player, int search_depth, int time_limit,
                             int *actual_depth, int *move_r, int *move_c, int *winning_player,
                             unsigned int *node_count, unsigned int *eval_count, unsigned int *pm_count);
};

#define kEvalWinningScore 10000
#define kEvalThreateningScore 300

class Eval {
    public:
    Eval();
    ~Eval();

    // Evaluate the entire game state as a player
    static int eval_state(const char *state, int player);
    // Evaluate one possible move as a player
    static int eval_pos(const char *state, int r, int c, int player);
    // Check if any player is winning based on a given state
    static int win_player(const char *state);
    // Result of a single direction measurement
    struct Measure {
        // Number of pieces in a row
        char len; 
        // Number of ends blocked by edge or the other player (0-2)
        char block_cnt;
        // Number of spaces in the middle of pattern     
        char space_cnt;
    };
    // A single direction pattern
    struct Pattern {
        // Minimum number of occurrences to match
        char min_occur;
        // Length of pattern (pieces in a row)
        char len;
        // Number of ends blocked by edge or the other player (0-2)
        char block_cnt;
        // Number of spaces in the middle of pattern (-1: Ignore value)
        char space_cnt;
    };
    const static int PATTERNS_NUM;
    const static int *SKIP_PATTERNS;
    // An array of preset patterns
    const static Pattern *PATTERNS;
    // Preset scores of each preset pattern
    const static int *PATTERN_SCORES;
    // Loads preset patterns into memory
    // SKIP_PATTERNS is the number of patterns to skip for a maximum
    // measured length in an measure_4d (e.g. longest is 3 pieces
    // in an ADM, then skip first few patterns that require 4 pieces or more).
    // static void gen_patterns(Pattern **PATTERNS, int **PATTERN_SCORES, int *PATTERNS_NUM, int *SKIP_PATTERNS);

    // Evaluates measures in 4 directions
    static int eval_measures(Measure *measure_4d);

    // Match the patterns to measures in 4 directions
    static int match_pattern(const Measure *measure_4d, const Pattern *patterns);

    // Measures all 4 directions
    static void gen_measures(const char *state, int r, int c, int player, bool is_cont, Eval::Measure *ms);

    // Measure a single direction
    static void gen_measure(const char *state, int r, int c, int dr, int dc, int player, bool is_cont, Eval::Measure *result);
};

class Negamax {
    public:
    Negamax();
    ~Negamax();

    static void negamax(const char *state, int player, int depth, int time_limit, bool enable_ab_pruning, int *actual_depth, int *move_r, int *move_c);

    private:
    // Preset search breadth
    // From root to leaf, each element is for 2 layers
    // e.g. {10, 5, 2} -> 10, 10, 5, 5, 2, 2, 2, ...
    static int presetSearchBreadth[5];

    // A move (candidate)
    struct Move {
        int r;
        int c;
        int heuristic_val;
        int actual_score;
        // Overloads < for sorting
        bool operator<(Move other) const {
            return heuristic_val > other.heuristic_val;
        }
    };

    static int search(char *state, int player, int initial_depth, int depth,
                                bool enable_ab_pruning, int alpha, int beta,
                                int *move_r, int *move_c);

    // Search possible moves based on a given state, sorted by heuristic values.
    static void searchMovesOrdered(const char *state, int player, std::vector<Move> *result);
};

class API {
    public:
    API();
    ~API();

    // Generate move based on a given game state
    static bool search_act(const char *b_raw_str, int ai_player_id,
                             int search_depth, int time_limit, int num_threads,
                             int *actual_depth, int *move_r, int *move_c, int *winning_player,
                             unsigned int *node_count, unsigned int *eval_count, unsigned int *pm_count);

    // Convert a game state string to game state binary array
    static void convert_board(const char *b_raw_str, char *state);
};

bool Util::remote_spot(const char *state, int r, int c) {
    // if (state == nullptr) return false;
    NULL_CHECK(state, false)
    for (int i = r - REMOTE_RNG; i <= r + REMOTE_RNG; i++) {
        if (i < 0 || i >= G_B_SIZE){continue;}
        for (int j = c - REMOTE_RNG; j <= c + REMOTE_RNG; j++) {
            if (j < 0 || j >= G_B_SIZE){continue;}
            if (state[G_B_SIZE * i + j] > 0){return false;}
        }
    }
    return true;
}

void Util::zobristInit(int size, uint64_t *z1, uint64_t *z2) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> d(0, UINT64_MAX);

    // Generate random values
    for (int i = 0; i < size; i++) {
        z1[i] = d(gen);
        z2[i] = d(gen);
    }
}

uint64_t Util::zobristHash(const char *state, uint64_t *z1, uint64_t *z2) {
    uint64_t hash = 0;
    for (int i = 0; i < G_B_AREA; i++) {
        if (state[i] == 1) {
            hash ^= z1[i];
        } else if (state[i] == 2) {
            hash ^= z2[i];
        }
    }
    return hash;
}


void Controller::search_act(const char *state, int player, int search_depth, int time_limit,
                           int *actual_depth, int *move_r, int *move_c, int *winning_player,
                           unsigned int *node_count, unsigned int *eval_count, unsigned int *pm_count) {
    // Check arguments
    if (state == nullptr ||
        player  < 1 || player > 2 ||
        search_depth == 0 || search_depth > 10 ||
        time_limit < 0 ||
        move_r == nullptr || move_c == nullptr) return;

    // Initialize counters
    g_eval_cnt = 0;
    g_pattern_match_cnt = 0;

    // Initialize data
    *move_r = -1;
    *move_c = -1;
    int _winning_player = 0;
    if (actual_depth != nullptr) *actual_depth = 0;

    // Check if anyone wins the game
    _winning_player = Eval::win_player(state);
    if (_winning_player != 0) {
        if (winning_player != nullptr) *winning_player = _winning_player;
        return;
    }

    // Copy game state
    char *_gs = new char[G_B_AREA];
    std::memcpy(_gs, state, G_B_AREA);

    // Run negamax
    Negamax::negamax(_gs, player, search_depth, time_limit, true, actual_depth, move_r, move_c);

    // Execute the move
    std::memcpy(_gs, state, G_B_AREA);
    Util::set_spot(_gs, *move_r, *move_c, static_cast<char>(player));

    // Check if anyone wins the game
    _winning_player = Eval::win_player(_gs);

    // Write output
    if (winning_player != nullptr) *winning_player = _winning_player;
    if (node_count != nullptr) *node_count = g_node_cnt;
    if (eval_count != nullptr) *eval_count = g_eval_cnt;
    if (pm_count != nullptr) *pm_count = g_pattern_match_cnt;

    delete[] _gs;
}

// Initialize global variables
// Eval::Pattern *Eval::PATTERNS = nullptr;
// int *Eval::PATTERN_SCORES = nullptr;
// int PATTERNS_NUM = 0;
// int SKIP_PATTERNS[6] = {0};

const int Eval::PATTERNS_NUM = 11;
const int *Eval::SKIP_PATTERNS = new int[6]{PATTERNS_NUM, PATTERNS_NUM, 10, 7, 1, 0};
const Eval::Pattern *Eval::PATTERNS = new Eval::Pattern[PATTERNS_NUM * 2]{
        {1, 5,  0,  0}, {0, 0,  0,  0},  // 10000
        {1, 4,  0,  0}, {0, 0,  0,  0},  // 700
        {2, 4,  1,  0}, {0, 0,  0,  0},  // 700
        {2, 4, -1,  1}, {0, 0,  0,  0},  // 700
        {1, 4,  1,  0}, {1, 4, -1,  1},  // 700
        {1, 4,  1,  0}, {1, 3,  0, -1},  // 500
        {1, 4, -1,  1}, {1, 3,  0, -1},  // 500
        {2, 3,  0, -1}, {0, 0,  0,  0},  // 300
        // {1, 4,  1,  0}, {0, 0,  0,  0},  // 1
        // {1, 4, -1,  1}, {0, 0,  0,  0},  // 1
        {3, 2,  0, -1}, {0, 0,  0,  0},  // 50
        {1, 3,  0, -1}, {0, 0,  0,  0},  // 20
        {1, 2,  0, -1}, {0, 0,  0,  0}   // 9
    };
const int *Eval::PATTERN_SCORES = new int[PATTERNS_NUM]{
        10000,
        700,
        700,
        700,
        700,
        500,
        500,
        300,
        // 1,
        // 1,
        50,
        20,
        9
};

int Eval::eval_state(const char *state, int player) {
    // if (state == nullptr || player < 1 || player > 2) return 0;
    ERR_NULL_CHECK(state, 0)
    ERR_PLAYER_CHECK(player, 0)

    // Evaluate all empty position
    Score sc = 0;
    for (int r = 0; r < G_B_SIZE; ++r) {
        for (int c = 0; c < G_B_SIZE; ++c) {
            sc += eval_pos(state, r, c, player);
        }
    }
    return sc;
}

int Eval::eval_pos(const char *state, int r, int c, int player) {
    // Check parameters
    // if (state == nullptr || player < 1 || player > 2) return 0;
    ERR_NULL_CHECK(state, 0)
    ERR_PLAYER_CHECK(player, 0)

    // Count evaluations
    ++g_eval_cnt;

    // Generate preset patterns structure in memory
    // if (PATTERNS == nullptr) {
    //     gen_patterns(&PATTERNS, &PATTERN_SCORES, &PATTERNS_NUM, SKIP_PATTERNS);
    // }

    Measure ms[M_DIR_NUM];

    // Measure continuous and non-continuous conditions
    gen_measures(state, r, c, player, false, ms);
    Score sc_non_conti = eval_measures(ms);
    gen_measures(state, r, c, player, true, ms);
    Score sc_conti = eval_measures(ms);
    return std::max(sc_non_conti, sc_conti);

    // Score max_score = 0;
    // for (bool is_cont = false;; is_cont = true) {
    //     // Execute measurement
    //     gen_measures(state, r, c, player, is_cont, ms);
    //     Score sc = eval_measures(ms);

    //     // Prefer continuous
    //     // if (!is_cont) sc *= 0.9;

    //     // Choose the better between continuous and non-continuous
    //     max_score = std::max(max_score, sc);

    //     if (is_cont) break;
    // }
    // return max_score;
}

int Eval::eval_measures(Measure *measure_4d) {
    int sc = 0;
    // int size = PATTERNS_NUM;

    // Add to score by length on each direction
    // Find the maximum length in measure_4d and skip some patterns
    int max_measure_len = 0;
    for (int i = 0; i < M_DIR_NUM; i++) {
        int len = measure_4d[i].len;
        // max_measure_len = len > max_measure_len ? len : max_measure_len;
        max_measure_len = max(len, max_measure_len);
        sc += (len - 1);
    }
    int start_pat_idx = SKIP_PATTERNS[max_measure_len];

    // Match specified patterns, ignore the patterns measures doesn't have
    for (int i = start_pat_idx; i < PATTERNS_NUM; i++) {
        sc += match_pattern(measure_4d, &PATTERNS[2 * i]) * PATTERN_SCORES[i];

        // Only match one threatening pattern
        if (sc >= kEvalThreateningScore){break;}
    }

    return sc;
}

int Eval::match_pattern(const Measure *measure_4d, const Pattern *patterns) {
    // Check arguments
    // if (measure_4d == nullptr) return -1;
    // if (patterns == nullptr) return -1;
    NULL_CHECK(measure_4d, -1);
    NULL_CHECK(patterns, -1);

    // Increment pattern match count
    g_pattern_match_cnt++;

    // Initialize res_match_cnt to INT_MAX since minimum value will be output
    int res_match_cnt = INT_MAX, pat_i_match_cnt = 0;

    // Currently allows maximum 2 patterns
    for (int i = 0; i < PATTERN_PAIR_CNT; i++) {
        Eval::Pattern p = patterns[i];
        if (p.len == 0) break;

        // Initialize counter
        pat_i_match_cnt = 0;

        // Loop through 4 directions
        for (int j = 0; j < M_DIR_NUM; j++) {
            auto m = measure_4d[j];

            // Exact match pattern
            if (m.len == p.len && (p.block_cnt == -1 || m.block_cnt == p.block_cnt) && (p.space_cnt == -1 || m.space_cnt == p.space_cnt)) {
                pat_i_match_cnt++;
            }
        }

        // Consider minimum number of occurrences
        pat_i_match_cnt /= p.min_occur;

        // Take smaller value
        // res_match_cnt = res_match_cnt >= pat_i_match_cnt ? pat_i_match_cnt : res_match_cnt;
        res_match_cnt = min(res_match_cnt, pat_i_match_cnt);
    }
    return res_match_cnt;
}

void Eval::gen_measures(const char *state, int r, int c, int player, bool is_cont, Eval::Measure *ms) {
    // Check arguments
    // if (state == nullptr) return;
    // if (r < 0 || r >= G_B_SIZE || c < 0 || c >= G_B_SIZE) return;
    ERR_NULL_CHECK(state,)
    ERR_POS_CHECK(r,c,)

    // Measure 4 directions
    gen_measure(state, r, c, 0,  1, player, is_cont, &ms[0]);
    gen_measure(state, r, c, 1,  1, player, is_cont, &ms[1]);
    gen_measure(state, r, c, 1,  0, player, is_cont, &ms[2]);
    gen_measure(state, r, c, 1, -1, player, is_cont, &ms[3]);
}

void Eval::gen_measure(const char *state, int r, int c, int dr, int dc, int player, bool is_cont, Eval::Measure *result) {
    // Check arguments
    // if (state == nullptr) return;
    // if (r < 0 || r >= G_B_SIZE || c < 0 || c >= G_B_SIZE) return;
    if (dr == 0 && dc == 0) return;
    ERR_NULL_CHECK(state,)
    ERR_POS_CHECK(r,c,)

    // Initialization
    int r_cnt = r, c_cnt = c;
    result->len = 1;
    result->block_cnt = 2;
    result->space_cnt = 0;

    int allowed_space = 1;
    if (is_cont) allowed_space = 0;

    // for (bool reversed = false;; reversed = true) {
    for (bool reversed: {false, true}) {
        while (true) {
            // Move
            r_cnt += dr; c_cnt += dc;

            // Validate position
            // if (r_cnt < 0 || r_cnt >= G_B_SIZE || c_cnt < 0 || c_cnt >= G_B_SIZE) break;
            if (pos_check(r_cnt, c_cnt)){break;}

            // Get spot value
            // int spot = state[G_B_SIZE * r_cnt + c_cnt];
            int spot = state[_2d_1d(r_cnt, c_cnt)];

            // Empty spots
            if (spot == 0) {
                if (allowed_space > 0 && Util::get_spot(state, r_cnt + dr, c_cnt + dc) == player) {
                    allowed_space--; 
                    result->space_cnt++;
                    continue;
                } else {
                    result->block_cnt--;
                    break;
                }
            }

            // Another player
            if (spot != player){break;}

            // Current player
            result->len++;
        }

        // Reverse direction and continue (just once)
        // if (reversed){break;}
        r_cnt = r; 
        c_cnt = c;
        dr = -dr; 
        dc = -dc;
    }

    // More than 5 pieces in a row is equivalent to 5 pieces
    if (result->len >= 5) {
        if (result->space_cnt == 0) {
            result->len = 5;
            result->block_cnt = 0;
        } else {
            result->len = 4;
            result->block_cnt = 1;
        }
    }
}

// void Eval::gen_patterns(Pattern **PATTERNS,
//                                          int **PATTERN_SCORES,
//                                          int *PATTERNS_NUM,
//                                          int *SKIP_PATTERNS) {
//     const int pattern_num = 11;
//     SKIP_PATTERNS[5] = 0;
//     SKIP_PATTERNS[4] = 1;
//     SKIP_PATTERNS[3] = 7;
//     SKIP_PATTERNS[2] = 10;

//     SKIP_PATTERNS[1] = pattern_num;
//     SKIP_PATTERNS[0] = pattern_num;

//     // Pattern = {min_occur, len, block_cnt, space_cnt}
//     Pattern patterns[pattern_num * 2] = {
//         {1, 5,  0,  0}, {0, 0,  0,  0},  // 10000
//         {1, 4,  0,  0}, {0, 0,  0,  0},  // 700
//         {2, 4,  1,  0}, {0, 0,  0,  0},  // 700
//         {2, 4, -1,  1}, {0, 0,  0,  0},  // 700
//         {1, 4,  1,  0}, {1, 4, -1,  1},  // 700
//         {1, 4,  1,  0}, {1, 3,  0, -1},  // 500
//         {1, 4, -1,  1}, {1, 3,  0, -1},  // 500
//         {2, 3,  0, -1}, {0, 0,  0,  0},  // 300
//         // {1, 4,  1,  0}, {0, 0,  0,  0},  // 1
//         // {1, 4, -1,  1}, {0, 0,  0,  0},  // 1
//         {3, 2,  0, -1}, {0, 0,  0,  0},  // 50
//         {1, 3,  0, -1}, {0, 0,  0,  0},  // 20
//         {1, 2,  0, -1}, {0, 0,  0,  0}   // 9
//     };

//     Score scores[pattern_num] = {
//         10000,
//         700,
//         700,
//         700,
//         700,
//         500,
//         500,
//         300,
//         // 1,
//         // 1,
//         50,
//         20,
//         9
//     };

//     *PATTERNS = new Pattern[pattern_num * 2];
//     *PATTERN_SCORES   = new int[pattern_num];

//     memcpy(*PATTERNS, patterns, sizeof(Pattern) * pattern_num * 2);
//     memcpy(*PATTERN_SCORES, scores, sizeof(int) * pattern_num);

//     *PATTERNS_NUM = pattern_num;
// }

int Eval::win_player(const char *state) {
    // if (state == nullptr) return 0;
    NULL_CHECK(state, 0)
    for (int r = 0; r < G_B_SIZE; r++) {
        for (int c = 0; c < G_B_SIZE; c++) {
            int spot = state[G_B_SIZE * r + c];
            if (spot == 0){continue;}
            // for (int dr = -1; dr <= 1; dr++) {
            //     for (int dc = -1; dc <= 1; dc++) {
            //         if (dr == 0 && dc <= 0){continue;}
            //         Measure dm;
            //         gen_measure(state, r, c, dr, dc, spot, 1, &dm);
            //         if (dm.len >= WIN_COND){return spot;}
            //     }
            // }
            // Check In 4 directions
            Measure m_h, m_v, m_lu, m_ru;
            gen_measure(state, r, c, 0, 1, spot, 1, &m_h);
            gen_measure(state, r, c, 1, 0, spot, 1, &m_v);
            gen_measure(state, r, c, 1, 1, spot, 1, &m_lu);
            gen_measure(state, r, c, 1, -1, spot, 1, &m_ru);
            if (m_h.len >= WIN_COND || m_v.len >= WIN_COND || m_lu.len >= WIN_COND || m_ru.len >= WIN_COND){return spot;}
        }
    }
    return 0;
}

// kSearchBreadth is used to control branching factor
// Different breadth configurations are possible:
// A lower breadth for a higher depth
// Or vice versa
int Negamax::presetSearchBreadth[5] = {17, 7, 5, 3, 3};

// Estimated average branching factor for iterative deepening
#define kAvgBranchingFactor 3

// Maximum depth for iterative deepening
#define kMaximumDepth 16

// kScoreDecayFactor decays score each layer so the algorithm
// prefers closer advantages
#define kScoreDecayFactor 0.95f

void Negamax::negamax(const char *state, int player, int depth, int time_limit, bool enable_ab_pruning, int *actual_depth, int *move_r, int *move_c) {
    // Check arguments
    if (state == nullptr ||
        player < 1 || player > 2 ||
        depth == 0 || depth < -1 ||
        time_limit < 0) return;

    // Copy game state
    char *_gs = new char[G_B_AREA];
    memcpy(_gs, state, G_B_AREA);

    // Speedup first move
    int _cnt = 0;
    for (int i = 0; i < static_cast<int>(G_B_AREA); i++)
        if (_gs[i] != 0) _cnt++;

    if (_cnt <= 2) depth = 6;

    // Fixed depth or iterative deepening
    if (depth > 0) {
        if (actual_depth != nullptr) *actual_depth = depth;
        search(_gs, player, depth, depth, enable_ab_pruning,
                         INT_MIN / 2, INT_MAX / 2, move_r, move_c);
    } else {
        // Iterative deepening
        std::clock_t c_start = std::clock();
        for (int d = 6;; d += 2) {
            std::clock_t c_iteration_start = std::clock();

            // Reset game state
            memcpy(_gs, state, G_B_AREA);

            // Execute negamax
            search(_gs, player, d, d, enable_ab_pruning,
                             INT_MIN / 2, INT_MAX / 2, move_r, move_c);

            // Times
            std::clock_t c_iteration = (std::clock() - c_iteration_start) * 1000 / CLOCKS_PER_SEC;
            std::clock_t c_elapsed = (std::clock() - c_start) * 1000 / CLOCKS_PER_SEC;

            if (c_elapsed + (c_iteration * kAvgBranchingFactor * kAvgBranchingFactor) > time_limit ||
                d >= kMaximumDepth) {
                if (actual_depth != nullptr) *actual_depth = d;
                break;
            }
        }
    }
    delete[] _gs;
}

int Negamax::search(char *state, int player, int initial_depth, int depth,
                                     bool enable_ab_pruning, int alpha, int beta,
                                     int *move_r, int *move_c) {
    // Count node
    ++g_node_cnt;

    int max_score = INT_MIN;
    int opponent = player == 1 ? 2 : 1;

    // Search and sort possible moves
    std::vector<Move> moves_player, moves_opponent, candidate_moves;
    searchMovesOrdered(state, player, &moves_player);
    searchMovesOrdered(state, opponent, &moves_opponent);

    // End if no move could be performed
    if (moves_player.size() == 0) return 0;

    // End directly if only one move or a winning move is found
    if (moves_player.size() == 1 || moves_player[0].heuristic_val >= kEvalWinningScore) {
        auto move = moves_player[0];
        if (move_r != nullptr) *move_r = move.r;
        if (move_c != nullptr) *move_c = move.c;
        return move.heuristic_val;
    }

    // If opponent has threatening moves, consider blocking them first
    bool block_opponent = false;
    int tmp_size = std::min(static_cast<int>(moves_opponent.size()), 2);
    if (moves_opponent[0].heuristic_val >= kEvalThreateningScore) {
        block_opponent = true;
        for (int i = 0; i < tmp_size; ++i) {
            auto move = moves_opponent[i];

            // Re-evaluate move as current player
            move.heuristic_val = Eval::eval_pos(state, move.r, move.c, player);

            // Add to candidate list
            candidate_moves.push_back(move);
        }
    }

    // Set breadth
    int breadth = (initial_depth >> 1) - ((depth + 1) >> 1);
    if (breadth > 4) breadth = presetSearchBreadth[4];
    else             breadth = presetSearchBreadth[breadth];

    // Copy moves for current player
    tmp_size = std::min(static_cast<int>(moves_player.size()), breadth);
    for (int i = 0; i < tmp_size; ++i)
        candidate_moves.push_back(moves_player[i]);

      // Print heuristic values for debugging
//    if (depth >= 8) {
//        for (int i = 0; i < moves_player.size(); ++i) {
//            auto move = moves_player[i];
//            std::cout << depth << " | " << move.r << ", " << move.c << ": " << move.heuristic_val << std::endl;
//        }
//    }

    // Loop through every move
    int size = static_cast<int>(candidate_moves.size());
    for (int i = 0; i < size; ++i) {
        auto move = candidate_moves[i];

        // Execute move
        Util::set_spot(state, move.r, move.c, static_cast<char>(player));

        // Run negamax recursively
        int sc = 0;
        if (depth > 1) sc = search(state,                 // Game state
                                                opponent,           // Change player
                                                initial_depth,      // Initial depth
                                                depth - 1,          // Reduce depth by 1
                                                enable_ab_pruning,  // Alpha-Beta
                                                -beta,              //
                                                -alpha + move.heuristic_val,
                                                nullptr,            // Result move not required
                                                nullptr);

        // Closer moves get more score
        if (sc >= 2) sc = static_cast<int>(sc * kScoreDecayFactor);

        // Calculate score difference
        move.actual_score = move.heuristic_val - sc;

        // Store back to candidate array
        candidate_moves[i].actual_score = move.actual_score;

        // Print actual scores for debugging
//        if (depth >= 8)
//            std::cout << depth << " | " << move.r << ", " << move.c << ": " << move.actual_score << std::endl;

        // Restore
        Util::set_spot(state, move.r, move.c, 0);

        // Update maximum sc
        if (move.actual_score > max_score) {
            max_score = move.actual_score;
            if (move_r != nullptr) *move_r = move.r;
            if (move_c != nullptr) *move_c = move.c;
        }

        // Alpha-beta
        int max_score_decayed = max_score;
        if (max_score >= 2) max_score_decayed = static_cast<int>(max_score_decayed * kScoreDecayFactor);
        if (max_score > alpha) alpha = max_score;
        if (enable_ab_pruning && max_score_decayed >= beta) break;
    }

    // If no moves that are much better than blocking threatening moves, block them.
    // This attempts blocking even winning is impossible if the opponent plays optimally.
    if (depth == initial_depth && block_opponent && max_score < 0) {
        auto blocking_move = candidate_moves[0];
        int b_score = blocking_move.actual_score;
        if (b_score == 0) b_score = 1;
        if ((max_score - b_score) / static_cast<float>(std::abs(b_score)) < 0.2) {
            if (move_r != nullptr) *move_r = blocking_move.r;
            if (move_c != nullptr) *move_c = blocking_move.c;
            max_score = blocking_move.actual_score;
        }
    }
    return max_score;
}

void Negamax::searchMovesOrdered(const char *state, int player, std::vector<Move> *result) {
    // Clear and previous result
    result->clear();

    // Find an extent to reduce unnecessary calls to Util::remote_spot
    int min_r = INT_MAX, min_c = INT_MAX, max_r = INT_MIN, max_c = INT_MIN;
    for (int r = 0; r < G_B_SIZE; ++r) {
        for (int c = 0; c < G_B_SIZE; ++c) {
            if (state[G_B_SIZE * r + c] != 0) {
                if (r < min_r) min_r = r;
                if (c < min_c) min_c = c;
                if (r > max_r) max_r = r;
                if (c > max_c) max_c = c;
            }
        }
    }

    if (min_r - 2 < 0) min_r = 2;
    if (min_c - 2 < 0) min_c = 2;
    if (max_r + 2 >= G_B_SIZE) max_r = G_B_SIZE - 3;
    if (max_c + 2 >= G_B_SIZE) max_c = G_B_SIZE - 3;

    // Loop through all cells
    for (int r = min_r - 2; r <= max_r + 2; ++r) {
        for (int c = min_c - 2; c <= max_c + 2; ++c) {
            // Consider only empty cells
            if (state[G_B_SIZE * r + c] != 0) continue;

            // Skip remote cells (no pieces within 2 cells)
            if (Util::remote_spot(state, r, c)) continue;

            Move m;
            m.r = r;
            m.c = c;

            // Evaluate move
            m.heuristic_val = Eval::eval_pos(state, r, c, player);

            // Add move
            result->push_back(m);
        }
    }
    std::sort(result->begin(), result->end());
}

bool API::search_act(const char *b_raw_str, int ai_player_id,
                            int search_depth, int time_limit, int num_threads,
                            int *actual_depth, int *move_r, int *move_c, int *winning_player,
                            unsigned int *node_count, unsigned int *eval_count, unsigned int *pm_count) {
    // Check input data
    if (strlen(b_raw_str) != G_B_AREA ||
        ai_player_id  < 1 || ai_player_id > 2 ||
        search_depth == 0 || search_depth > 10 ||
        time_limit < 0    ||
        num_threads  < 1) {
        return false;
    }

    // Copy game state
    char *state = new char[G_B_AREA];
    std::memcpy(state, b_raw_str, G_B_AREA);

    // Convert from string
    convert_board(b_raw_str, state);

    // Generate move
    Controller::search_act(state, ai_player_id, search_depth, time_limit, actual_depth,
                                    move_r, move_c, winning_player, node_count, eval_count, pm_count);

    // Release memory
    delete[] state;
    return true;
}

void API::convert_board(const char *b_raw_str, char *state) {
    if (strlen(b_raw_str) != G_B_AREA) return;
    for (int i = 0; i < static_cast<int>(G_B_AREA); i++) {
        state[i] = b_raw_str[i] - '0';
    }
}

int main(){
    string infile("state"), outfile("action");
    // string infile("input.txt"), outfile("output.txt");
    IO io(G_B_SIZE, infile, outfile);
    io.read_board();
    // std::cout << io.board << endl;

    bool is_first_hand = true;
    char b_raw_str[G_B_SIZE * G_B_SIZE] = {0};
    for(int i = 0; i < io.board.size(); i++){
        for(int j = 0; j < io.board.at(0).size(); j++){
            if(io.board[i][j] == OC){
                b_raw_str[i * G_B_SIZE + j] = '1';
                is_first_hand = false;
            }else if(io.board[i][j] == XC){
                b_raw_str[i * G_B_SIZE + j] = '2';
                is_first_hand = false;
            }else{
                b_raw_str[i * G_B_SIZE + j] = '0';
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
    bool success = API::search_act(b_raw_str, 1, -1, time_limit, 1, &actual_depth, &move_r, &move_c, &winning_player, &node_count, &eval_count, nullptr);

    io.write_valid_spot(Position(move_r, move_c));
    if(success){
        std::cout << "Success actual_depth: " << actual_depth << " Act: (" << move_r << ", " << move_c << ")" << " winning_player: " << winning_player << " node_count: " << node_count << " eval_count: " << eval_count << endl;
    }else{
        std::cout << "Fail actual_depth: " << actual_depth << " Act: (" << move_r << ", " << move_c << ")" << " winning_player: " << winning_player << " node_count: " << node_count << " eval_count: " << eval_count << endl;
    }
    
    return 0;
}