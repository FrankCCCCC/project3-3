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
#define get_tuple(t, i) std::get<i>(t)
#define M_DIR_NUM 4
#define REMOTE_RNG 2

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
    if (r < 0 || r >= G_B_SIZE || c < 0 || c >= G_B_SIZE){return false;}
    return true;
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
        return state[G_B_SIZE * r + c];
    }

    static inline bool set_spot(char *state, int r, int c, char value) {
        // if (r < 0 || r >= G_B_SIZE || c < 0 || c >= G_B_SIZE) return false;
        POS_CHECK(r, c, false)
        state[G_B_SIZE * r + c] = value;
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
    struct Measurement {
        // Number of pieces in a row
        char len; 
        // Number of ends blocked by edge or the other player (0-2)
        char block_cnt;
        // Number of spaces in the middle of pattern     
        char space_cnt;
    };
    // A single direction pattern
    struct Pattern {
        char min_occurrence;  // Minimum number of occurrences to match
        char len;          // Length of pattern (pieces in a row)
        char block_cnt;     // Number of ends blocked by edge or the other player (0-2)
        char space_cnt;     // Number of spaces in the middle of pattern (-1: Ignore value)
    };
    // An array of preset patterns
    static Pattern *preset_patterns;
    // Preset scores of each preset pattern
    static int *preset_scores;
    // Loads preset patterns into memory
    // preset_patterns_skip is the number of patterns to skip for a maximum
    // measured length in an all_direction_measurement (e.g. longest is 3 pieces
    // in an ADM, then skip first few patterns that require 4 pieces or more).
    static void gen_patterns(Pattern **preset_patterns,
                                       int **preset_scores,
                                       int *preset_patterns_size,
                                       int *preset_patterns_skip);

    // Evaluates an all-direction measurement
    static int eval_measures(Measurement *all_direction_measurement);

    // Tries to match a set of patterns with an all-direction measurement
    static int matchPattern(Measurement *all_direction_measurement,
                            Pattern *patterns);

    // Measures all 4 directions
    static void gen_measures(const char *state,
                                     int r,
                                     int c,
                                     int player,
                                     bool is_cont,
                                     Eval::Measurement *ms);

    // Measure a single direction
    static void gen_measure(const char *state,
                                 int r, int c,
                                 int dr, int dc,
                                 int player,
                                 bool is_cont,
                                 Eval::Measurement *result);
};

class Negamax {
    public:
    Negamax();
    ~Negamax();

    static void negamax(const char *state, int player, int depth, int time_limit, bool enable_ab_pruning,
                                 int *actual_depth, int *move_r, int *move_c);

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
    static bool search_act(const char *gs_string, int ai_player_id,
                             int search_depth, int time_limit, int num_threads,
                             int *actual_depth, int *move_r, int *move_c, int *winning_player,
                             unsigned int *node_count, unsigned int *eval_count, unsigned int *pm_count);

    // Convert a game state string to game state binary array
    static void convert_board(const char *gs_string, char *state);
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
Eval::Pattern *Eval::preset_patterns = nullptr;
int *Eval::preset_scores = nullptr;
int preset_patterns_size = 0;
int preset_patterns_skip[6] = {0};

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
    if (preset_patterns == nullptr) {
        gen_patterns(&preset_patterns, &preset_scores, &preset_patterns_size, preset_patterns_skip);
    }

    Measurement ms[M_DIR_NUM];

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

int Eval::eval_measures(Measurement *all_direction_measurement) {
    int sc = 0;
    int size = preset_patterns_size;

    // Add to sc by length on each direction
    // Find the maximum length in ADM and skip some patterns
    int max_measured_len = 0;
    for (int i = 0; i < M_DIR_NUM; i++) {
        int len = all_direction_measurement[i].len;
        max_measured_len = len > max_measured_len ? len : max_measured_len;
        sc += len - 1;
    }
    int start_pattern = preset_patterns_skip[max_measured_len];

    // Loop through and try to match all preset patterns
    for (int i = start_pattern; i < size; ++i) {
        sc += matchPattern(all_direction_measurement, &preset_patterns[2 * i]) * preset_scores[i];

        // Only match one threatening pattern
        if (sc >= kEvalThreateningScore) break;
    }

    return sc;
}

int Eval::matchPattern(Measurement *all_direction_measurement,
                              Pattern *patterns) {
    // Check arguments
    if (all_direction_measurement == nullptr) return -1;
    if (patterns == nullptr) return -1;

    // Increment PM count
    g_pattern_match_cnt++;

    // Initialize match_count to INT_MAX since minimum value will be output
    int match_count = INT_MAX, single_pattern_match = 0;

    // Currently allows maximum 2 patterns
    for (int i = 0; i < 2; ++i) {
        auto p = patterns[i];
        if (p.len == 0) break;

        // Initialize counter
        single_pattern_match = 0;

        // Loop through 4 directions
        for (int j = 0; j < M_DIR_NUM; ++j) {
            auto dm = all_direction_measurement[j];

            // Requires exact match
            if (dm.len == p.len &&
                (p.block_cnt == -1 || dm.block_cnt == p.block_cnt) &&
                (p.space_cnt == -1 || dm.space_cnt == p.space_cnt)) {
                single_pattern_match++;
            }
        }

        // Consider minimum number of occurrences
        single_pattern_match /= p.min_occurrence;

        // Take smaller value
        match_count = match_count >= single_pattern_match ? single_pattern_match : match_count;
    }
    return match_count;
}

void Eval::gen_measures(const char *state, int r, int c, int player, bool is_cont, Eval::Measurement *ms) {
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

void Eval::gen_measure(const char *state,
                                   int r, int c,
                                   int dr, int dc,
                                   int player,
                                   bool is_cont,
                                   Eval::Measurement *result) {
    // Check arguments
    // if (state == nullptr) return;
    // if (r < 0 || r >= G_B_SIZE || c < 0 || c >= G_B_SIZE) return;
    if (dr == 0 && dc == 0) return;
    ERR_NULL_CHECK(state,)
    ERR_POS_CHECK(r,c,)

    // Initialization
    int cr = r, cc = c;
    result->len = 1, result->block_cnt = 2, result->space_cnt = 0;

    int space_allowance = 1;
    if (is_cont) space_allowance = 0;

    for (bool reversed = false;; reversed = true) {
        while (true) {
            // Move
            cr += dr; cc += dc;

            // Validate position
            if (cr < 0 || cr >= G_B_SIZE || cc < 0 || cc >= G_B_SIZE) break;

            // Get spot value
            int spot = state[G_B_SIZE * cr + cc];

            // Empty spots
            if (spot == 0) {
                if (space_allowance > 0 && Util::get_spot(state, cr + dr, cc + dc) == player) {
                    space_allowance--; result->space_cnt++;
                    continue;
                } else {
                    result->block_cnt--;
                    break;
                }
            }

            // Another player
            if (spot != player) break;

            // Current player
            result->len++;
        }

        // Reverse direction and continue (just once)
        if (reversed) break;
        cr = r; cc = c;
        dr = -dr; dc = -dc;
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

void Eval::gen_patterns(Pattern **preset_patterns,
                                         int **preset_scores,
                                         int *preset_patterns_size,
                                         int *preset_patterns_skip) {
    const int pattern_num = 11;
    preset_patterns_skip[5] = 0;
    preset_patterns_skip[4] = 1;
    preset_patterns_skip[3] = 7;
    preset_patterns_skip[2] = 10;

    preset_patterns_skip[1] = pattern_num;
    preset_patterns_skip[0] = pattern_num;

    Pattern patterns[pattern_num * 2] = {
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

    int scores[pattern_num] = {
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

    *preset_patterns = new Pattern[pattern_num * 2];
    *preset_scores   = new int[pattern_num];

    memcpy(*preset_patterns, patterns, sizeof(Pattern) * pattern_num * 2);
    memcpy(*preset_scores, scores, sizeof(int) * pattern_num);

    *preset_patterns_size = pattern_num;
}

int Eval::win_player(const char *state) {
    if (state == nullptr) return 0;
    for (int r = 0; r < G_B_SIZE; ++r) {
        for (int c = 0; c < G_B_SIZE; ++c) {
            int spot = state[G_B_SIZE * r + c];
            if (spot == 0) continue;
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc <= 0) continue;
                    Measurement dm;
                    gen_measure(state, r, c, dr, dc, spot, 1, &dm);
                    if (dm.len >= 5) return spot;
                }
            }
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

void Negamax::negamax(const char *state, int player, int depth, int time_limit, bool enable_ab_pruning,
                                      int *actual_depth, int *move_r, int *move_c) {
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

bool API::search_act(const char *gs_string, int ai_player_id,
                            int search_depth, int time_limit, int num_threads,
                            int *actual_depth, int *move_r, int *move_c, int *winning_player,
                            unsigned int *node_count, unsigned int *eval_count, unsigned int *pm_count) {
    // Check input data
    if (strlen(gs_string) != G_B_AREA ||
        ai_player_id  < 1 || ai_player_id > 2 ||
        search_depth == 0 || search_depth > 10 ||
        time_limit < 0    ||
        num_threads  < 1) {
        return false;
    }

    // Copy game state
    char *state = new char[G_B_AREA];
    std::memcpy(state, gs_string, G_B_AREA);

    // Convert from string
    convert_board(gs_string, state);

    // Generate move
    Controller::search_act(state, ai_player_id, search_depth, time_limit, actual_depth,
                                    move_r, move_c, winning_player, node_count, eval_count, pm_count);

    // Release memory
    delete[] state;
    return true;
}

void API::convert_board(const char *gs_string, char *state) {
    if (strlen(gs_string) != G_B_AREA) return;
    for (int i = 0; i < static_cast<int>(G_B_AREA); i++) {
        state[i] = gs_string[i] - '0';
    }
}

int main(){
    string infile("state"), outfile("action");
    // string infile("input.txt"), outfile("output.txt");
    IO io(G_B_SIZE, infile, outfile);
    io.read_board();
    // std::cout << io.board << endl;

    bool is_first_hand = true;
    char gs_string[G_B_SIZE * G_B_SIZE] = {0};
    for(int i = 0; i < io.board.size(); i++){
        for(int j = 0; j < io.board.at(0).size(); j++){
            if(io.board[i][j] == OC){
                gs_string[i * G_B_SIZE + j] = '1';
                is_first_hand = false;
            }else if(io.board[i][j] == XC){
                gs_string[i * G_B_SIZE + j] = '2';
                is_first_hand = false;
            }else{
                gs_string[i * G_B_SIZE + j] = '0';
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
    bool success = API::search_act(gs_string, 1, -1, time_limit, 1, &actual_depth, &move_r, &move_c,
                                          &winning_player, &node_count, &eval_count, nullptr);

    io.write_valid_spot(Position(move_r, move_c));
    if(success){
        std::cout << "Success actual_depth: " << actual_depth << " Act: (" << move_r << ", " << move_c << ")" << " winning_player: " << winning_player << " node_count: " << node_count << " eval_count: " << eval_count << endl;
    }else{
        std::cout << "Fail actual_depth: " << actual_depth << " Act: (" << move_r << ", " << move_c << ")" << " winning_player: " << winning_player << " node_count: " << node_count << " eval_count: " << eval_count << endl;
    }
    
    return 0;
}