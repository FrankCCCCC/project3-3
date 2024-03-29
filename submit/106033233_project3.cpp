#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <ctime>
#include <algorithm>
#include <random>
// #include "hashmap.h"

using namespace std;

typedef std::string BoardRow;
typedef std::vector<BoardRow> Board;
typedef int Score;
typedef uint64_t ZbsHash;
typedef uint64_t Hash;
typedef unordered_map<Hash, Score> StateMap;
// typedef unordered_map<std::string, Score> StateMap;

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
#define INIT_DEPTH 6
#define INC_DEPTH 2
// -1 for all threats
#define WATCH_THREAT_CNT 2
#define ROLE_NUM 3
#define WIN_SCORE_LIMIT 10000
#define THRAT_SCORE_LIMIT 300
// Est. branch factor for iterative pruning
#define AVG_BRANCH_FACTOR 3
// Depth limit for iterative pruning
#define MAX_DEPTH 16
// SCORE_DECAY decays the score by the depth of the game tree, encourage to explore shorter path
#define SCORE_DECAY 0.95f
#define ENABLE_TIME_LIMIT false

#define SHOW_INFO false
#define INFO(s)\
if(SHOW_INFO){std::cout << s << endl;}\

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

#define PLAYER_CHECK(p, ret)\
if(player != 1 && player != 2){\
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
    for (int i = 0; i < static_cast<int>(b.size()); i++) {
        os << seq(i) << " ";
    }
    os << endl;

    for (int i = 0; i < static_cast<int>(b.size()); i++) {
        os << seq(i) << " ";
        for (int j = 0; j < static_cast<int>(b.at(0).size()); j++) {
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
    void write_spot(const Position &pos) {
        this->fout << pos.r << " " << pos.c << std::endl;
        // Remember to flush the output to ensure the last action is written to file.
        this->fout.flush();
    }
    void write_valid_spot(const Position& pos) {
        if (this->board[pos.r][pos.c] == DC) {
            this->write_spot(pos);
        }
    }
    void rand_spot(Position& pos, Score& sc) {
        sc = -1;
        srand(time(NULL));
        // Keep updating the output until getting killed.
        for (int i = 0; i < 1000; i++) {
            // Choose a random spot.
            pos.r = (rand() % this->b_size);
            pos.c = (rand() % this->b_size);
            write_valid_spot(pos);
        }
    }
};

unsigned int g_node_cnt = 0;
unsigned int g_eval_cnt = 0;
unsigned int g_pattern_match_cnt = 0;
unsigned int g_cc_0 = 0;
unsigned int g_cc_1 = 0;
unsigned int g_eval_missed = 0;
IO io(G_B_SIZE, "state", "action");

class Util {
    public:
    Util();

    static inline char& access_spot(char *state, int r, int c){
        return state[_2d_1d(r, c)];
    }

    static inline char get_spot(const char *state, int r, int c) {
        POS_CHECK(r, c, -1)
        return state[_2d_1d(r, c)];
    }

    static inline bool set_spot(char *state, int r, int c, char value) {
        POS_CHECK(r, c, false)
        state[_2d_1d(r, c)] = value;
        return true;
    }

    static bool remote_spot(const char *state, int r, int c);
};

class Controller {
    public:
    Controller();
    static void convert_board(const char *b_raw_str, char *state);
    static bool search_act(const char *state, int player, int search_depth, int time_limit, int &actual_depth, int &move_r, int &move_c, int &winning_player);
};

class ZobristHash {
private:
    static ZbsHash *HASH_O, *HASH_X, *HASH_R, *HASH_C, *HASH_ROLE;
    ZbsHash state_hash;
public:
    ZobristHash();
    ZobristHash(const char *state);
    static class ClassInit {
        public:
        ClassInit();
    } Initialize;

    static ZbsHash zobrist_hash(const char *state);
    static Hash hash(const char *state, int r, int c, int player);
    Hash yield(int r, int c, int player) const;
};

class Eval {
    public:
    Eval();
    const static char MEASURE_DIR_H = 0;
    const static char MEASURE_DIR_LU = 1;
    const static char MEASURE_DIR_V = 2;
    const static char MEASURE_DIR_RU = 3;

    // Evaluate the entire state as a player
    static int eval_state(const char *state, int player);
    // Evaluate one move as a player
    static int eval_pos(const char *state, int r, int c, int player);
    // Incorporate with ZobristHash, but profiling shows that it won't accelerate the speed of the program because the ``operator[]`` and ``find()`` take too much time.
    static int eval_pos(const char *state, int r, int c, int player, const ZobristHash &zb_hash);
    // Check whether any player is winning based on a given state
    static int win_player(const char *state);

    // Result of a measurement in a direction
    struct Measure {
        // Number of pieces in a row
        char len; 
        // Number of ends blocked by the other player (1 or 2) or the border of the board (-1: don't care the value)
        char block_cnt;
        // Number of empty spaces inside the row(number of pieces to separate the row,  -1: don't care the value)
        char space_cnt;
    };
    // A pattern in a direction
    struct Pattern {
        // Minimum count of occurrences
        char min_occur;
        // Length of a pattern (number of pieces in a row)
        char len;
        // Number of ends blocked by the other player (1 or 2) or the border of the board (-1: don't care the value)
        char block_cnt;
        // Number of empty spaces inside the row(number of pieces to separate the row,  -1: don't care the value)
        char space_cnt;
    };
    // static class ClassInit {
    //     public:
    //     ClassInit();
    // } Initialize;
    // static struct hashmap_s hashmap;
    // static StateMap state_map;
    const static int PATTERNS_NUM;
    const static int *SKIP_PATTERNS;
    // Patterns and corresponding score
    const static Pattern *PATTERNS;
    const static int *PATTERN_SCORES;
    // Evaluates measures in 4 directions
    static int eval_measures(const Measure *measure_4d);
    // Match the patterns to measures in 4 directions
    static int match_pattern(const Measure *measure_4d, const Pattern *patterns);
    // Generate measures in 4 directions
    static void gen_measures(const char *state, int r, int c, int player, bool is_cont, Eval::Measure *ms);
    // Measure a direction
    static void gen_measure(const char *state, int r, int c, char dir, int player, bool is_cont, Eval::Measure &res);
};

class Negamax {
    public:
    Negamax();

    static void search(const char *state, int player, int depth, int time_limit, bool enable_ab_pruning, int &actual_depth, int &move_r, int &move_c, int init_depth, int inc_depth);
    private:
    // search width
    static int search_width[5];
    struct Move {
        int r;
        int c;
        int score;
        int accum_score;
        // Overloads < for sorting
        bool operator<(Move other) const {
            return score > other.score;
        }
    };
    static int negamax(char *state, int player, int initial_depth, int depth, bool enable_ab_pruning, int alpha, int beta, int &move_r, int &move_c);
    // Search possible moves for a given state, sorted by evaluated value.
    static void search_sorted_cands(const char *state, int player, std::vector<Move> &result);
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

void Controller::convert_board(const char *b_raw_str, char *state) {
    // if (strlen(b_raw_str) != G_B_AREA) return;
    for (int i = 0; i < static_cast<int>(G_B_AREA); i++) {
        state[i] = b_raw_str[i] - '0';
    }
}
bool Controller::search_act(const char *b_raw_str, int player, int search_depth, int time_limit, int &actual_depth, int &move_r, int &move_c, int &winning_player) {
    // Check arguments
    if (search_depth == 0 || search_depth > 10 || time_limit < 0){return false;}
    NULL_CHECK(b_raw_str, false)
    ERR_PLAYER_CHECK(player, false)

    // Initialize counters
    g_eval_cnt = 0;
    g_pattern_match_cnt = 0;

    // Initialize data
    move_r = -1;
    move_c = -1;
    winning_player = 0;
    actual_depth = 0;

    // Copy game state
    char state[G_B_AREA] = {0};
    Controller::convert_board(b_raw_str, state);

    // Check if anyone wins the game
    winning_player = Eval::win_player(state);
    if (winning_player != 0) {
        winning_player = winning_player;

        // If there is a winning player, return a random move
        Position pos;
        Score sc;
        io.rand_spot(pos, sc);
        move_r = pos.r;
        move_c = pos.c;
        return true;
    }

    // Run negamax
    Negamax::search(state, player, search_depth, time_limit, true, actual_depth, move_r, move_c, INIT_DEPTH, INC_DEPTH);

    // Execute the move
    Util::set_spot(state, move_r, move_c, static_cast<char>(player));

    // Check if anyone wins the game
    winning_player = Eval::win_player(state);;
    return true;
}

ZobristHash::ZobristHash(const char *state) {
    this->state_hash = ZobristHash::zobrist_hash(state);
}
    
ZobristHash::ClassInit::ClassInit() {
    // Static constructor definition
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<ZbsHash> d(0, UINT64_MAX);

    ZobristHash::HASH_O = new ZbsHash[G_B_AREA];
    ZobristHash::HASH_X = new ZbsHash[G_B_AREA];
    ZobristHash::HASH_R = new ZbsHash[G_B_SIZE];
    ZobristHash::HASH_C = new ZbsHash[G_B_SIZE];
    ZobristHash::HASH_ROLE = new ZbsHash[ROLE_NUM];

    // Generate random values
    for (int i = 0; i < G_B_AREA; i++) {
        ZobristHash::HASH_O[i] = d(gen);
        ZobristHash::HASH_X[i] = d(gen);
    }
    for (int i = 0; i < G_B_SIZE; i++) {
        ZobristHash::HASH_R[i] = d(gen);
        ZobristHash::HASH_C[i] = d(gen);
    }
    for(int i = 0; i < ROLE_NUM; i++){
        ZobristHash::HASH_ROLE[i] = d(gen);
    }
}

ZbsHash ZobristHash::zobrist_hash(const char *state) {
    ZbsHash h = 0;
    for (int i = 0; i < G_B_AREA; i++) {
        if (state[i] == 1) { h ^= HASH_O[i]; }
        else if (state[i] == 2) { h ^= HASH_X[i]; }
    }
    return h;
}
Hash ZobristHash::hash(const char *state, int r, int c, int player) {
    Hash h = ZobristHash::zobrist_hash(state);
    h ^= (ZobristHash::HASH_R[r] ^ ZobristHash::HASH_C[c] ^ ZobristHash::HASH_ROLE[player]);
    return h;
}
Hash ZobristHash::yield(int r, int c, int player) const {
    return this->state_hash ^ (ZobristHash::HASH_R[r] ^ ZobristHash::HASH_C[c] ^ ZobristHash::HASH_ROLE[player]);
}
std::string hash_str(const char *state, int r, int c, int player) {
    char rc = (r + '0');
    char cc = (c + '0');
    char pc = (player + '0');
    return std::string(state) + rc + cc + pc;
}
// ZbsHash *ZobristHash::HASH_O, *ZobristHash::HASH_X;
ZbsHash *ZobristHash::HASH_O, *ZobristHash::HASH_X, *ZobristHash::HASH_R, *ZobristHash::HASH_C, *ZobristHash::HASH_ROLE;
ZobristHash::ClassInit ZobristHash::Initialize;

// typedef union ptr {
//     int *p;
//     int val;
// }Ptr;


// inline bool hashmap_init(int size){
//     if (0 != hashmap_create(size, &Eval::hashmap)) {return false;}
//     return true;
// }

// inline bool hashmap_put(const std::string &s,  int x){
//     Ptr p;
//     p.val = x;
//     if (0 != hashmap_put(&Eval::hashmap, s.c_str(), s.size(), p.p)) {return false;}
//     return true;
// }

// inline bool hashmap_get(const std::string &s, int &x){
//     Ptr ret;
//     ret.p = static_cast<int*>(hashmap_get(&Eval::hashmap, s.c_str(), s.size()));
//     x = ret.val;
//     if(ret.p == nullptr){return false;}
//     return true;
// }

// Eval::ClassInit::ClassInit() {
//     // Eval::state_map.reserve(65536);
//     // Eval::state_map.max_load_factor(0.25);

//     // hashmap.h
//     // hashmap_init(65536);
// }
// StateMap Eval::state_map;
// struct hashmap_s Eval::hashmap;
// Eval::ClassInit Eval::Initialize;
const int Eval::PATTERNS_NUM = 11;
const int *Eval::SKIP_PATTERNS = new int[6]{PATTERNS_NUM, PATTERNS_NUM, 10, 7, 1, 0};
// {Minimum number of occurrences to match, Length of pattern (pieces in a row), Number of ends blocked by edge or the other player (0-2), Number of spaces in the middle of pattern (-1: Ignore value)}
const Eval::Pattern *Eval::PATTERNS = new Eval::Pattern[PATTERNS_NUM * 2]{
        {1, 5,  0,  0}, {0, 0,  0,  0},  // 10000
        {1, 4,  0,  0}, {0, 0,  0,  0},  // 700
        // Threats-----------------------------
        {2, 4,  1,  0}, {0, 0,  0,  0},  // 700
        {2, 4, -1,  1}, {0, 0,  0,  0},  // 700
        {1, 4,  1,  0}, {1, 4, -1,  1},  // 700
        {1, 4,  1,  0}, {1, 3,  0, -1},  // 500
        {1, 4, -1,  1}, {1, 3,  0, -1},  // 500
        {2, 3,  0, -1}, {0, 0,  0,  0},  // 300
        // Threats-----------------------------
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
        50,
        20,
        9
};

int Eval::eval_state(const char *state, int player) {
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
    ERR_NULL_CHECK(state, 0)
    ERR_PLAYER_CHECK(player, 0)

    // Count evaluations
    ++g_eval_cnt;

    Measure ms[M_DIR_NUM];

    // Measure continuous and non-continuous conditions
    gen_measures(state, r, c, player, false, ms);
    Score sc_non_conti = eval_measures(ms);
    gen_measures(state, r, c, player, true, ms);
    Score sc_conti = eval_measures(ms);
    return std::max(sc_non_conti, sc_conti);
}

int Eval::eval_pos(const char *state, int r, int c, int player, const ZobristHash &zb_hash) {
    // Incorporate with ZobristHash, but profiling shows that it won't accelerate the speed of the program because the ``operator[]`` and ``find()`` take too much time.
    ERR_NULL_CHECK(state, 0);
    ERR_PLAYER_CHECK(player, 0);

    // Count evaluations
    ++g_eval_cnt;

    // StateMap
    // Hash h = zb_hash.yield(r, c, player);
    // // std::string h = hash_str(state, r, c, player);
    // StateMap::iterator f = Eval::state_map.find(h);
    // if(f == Eval::state_map.end()){
    //     g_eval_missed++;
    //     int res = Eval::eval_pos(state, r, c, player);
    //     Eval::state_map[h] = res;
    //     return res;
    // }
    // return f->second;

    // hashmap.h
    // std::string h = hash_str(state, r, c, player);
    // int x = 0;
    // bool f = hashmap_get(h, x);
    // if(!f){
    //     g_eval_missed++;
    //     int res = Eval::eval_pos(state, r, c, player);
    //     hashmap_put(h, res); 
    //     return res;
    // }
    // return x;
    return 0;
}

int Eval::eval_measures(const Measure *measure_4d) {
    int sc = 0;

    // Add to score by length on each direction
    // Find the maximum length in measure_4d and skip some patterns
    int max_measure_len = 0;
    for (int i = 0; i < M_DIR_NUM; i++) {
        int len = measure_4d[i].len;
        max_measure_len = max(len, max_measure_len);
        sc += (len - 1);
    }
    int start_pat_idx = SKIP_PATTERNS[max_measure_len];

    // Match specified patterns, ignore the patterns measures doesn't have
    for (int i = start_pat_idx; i < PATTERNS_NUM; i++) {
        sc += match_pattern(measure_4d, &PATTERNS[2 * i]) * PATTERN_SCORES[i];

        // Only match one threatening pattern
        if (sc >= THRAT_SCORE_LIMIT){break;}
    }

    return sc;
}

int Eval::match_pattern(const Measure *measure_4d, const Pattern *patterns) {
    NULL_CHECK(measure_4d, -1)
    NULL_CHECK(patterns, -1)

    // Increase pattern match count
    g_pattern_match_cnt++;

    // Initialize res_match_cnt to INT_MAX since minimum value will be output
    int res_match_cnt = INT_MAX, pat_i_match_cnt = 0;

    // Scan several patterns, the number of scanning patterns is defined at PATTERN_PAIR_CNT
    for (int i = 0; i < PATTERN_PAIR_CNT; i++) {
        Eval::Pattern p = patterns[i];
        if (p.len == 0) break;
        pat_i_match_cnt = 0;

        // Scan through 4 directions
        for (int j = 0; j < M_DIR_NUM; j++) {
            auto m = measure_4d[j];

            // Exact match pattern
            if (m.len == p.len && (p.block_cnt == -1 || m.block_cnt == p.block_cnt) && (p.space_cnt == -1 || m.space_cnt == p.space_cnt)) {
                pat_i_match_cnt++;
            }
        }

        // Consider smallest count of occurrences
        pat_i_match_cnt /= p.min_occur;

        // Choose smaller value
        res_match_cnt = min(res_match_cnt, pat_i_match_cnt);
    }
    return res_match_cnt;
}

void Eval::gen_measures(const char *state, int r, int c, int player, bool is_cont, Eval::Measure *ms) {
    ERR_NULL_CHECK(state,)
    ERR_POS_CHECK(r,c,)

    // Scan 4 directions
    gen_measure(state, r, c, Eval::MEASURE_DIR_H, player, is_cont, ms[0]);
    gen_measure(state, r, c, Eval::MEASURE_DIR_LU, player, is_cont, ms[1]);
    gen_measure(state, r, c, Eval::MEASURE_DIR_V, player, is_cont, ms[2]);
    gen_measure(state, r, c, Eval::MEASURE_DIR_RU, player, is_cont, ms[3]);
}

void Eval::gen_measure(const char *state, int r, int c, char dir, int player, bool is_cont, Eval::Measure &res) {
    if(dir < MEASURE_DIR_H || dir > MEASURE_DIR_RU){return;}
    ERR_NULL_CHECK(state,)
    ERR_POS_CHECK(r,c,)

    // Initialization
    int r_cnt = r, c_cnt = c;
    res.space_cnt = 0;
    res.block_cnt = 2;
    res.len = 1;

    int allowed_space = 1;
    if (is_cont) allowed_space = 0;

    // Set up direction
    int dr = 0, dc = 0;
    switch(dir){
        case(Eval::MEASURE_DIR_H):
            dr = 0, dc = 1;
            break;
        case(Eval::MEASURE_DIR_LU):
            dr = 1, dc = 1;
            break;
        case(Eval::MEASURE_DIR_V):
            dr = 1, dc = 0;
            break;
        case(Eval::MEASURE_DIR_RU):
            dr = 1, dc = -1;
            break;
    }

    for (int i = 0; i < 2; i++) {
        while (true) {
            // Shift
            r_cnt += dr; c_cnt += dc;

            // Validate position
            if (pos_check(r_cnt, c_cnt)){break;}

            // Get spot value
            int spot = state[_2d_1d(r_cnt, c_cnt)];

            // Empty spots
            if (spot == 0) {
                if (allowed_space > 0 && Util::get_spot(state, r_cnt + dr, c_cnt + dc) == player) {
                    allowed_space--; 
                    res.space_cnt++;
                    continue;
                } else {
                    res.block_cnt--;
                    break;
                }
            }

            // Another player
            if (spot != player){break;}

            // Current player
            res.len++;
        }

        // Reverse direction
        dr = -dr; 
        dc = -dc;
        r_cnt = r; 
        c_cnt = c;
    }

    // If there are more than 5 piece in a row, take it as 5 
    if (res.len >= 5) {
        if (res.space_cnt == 0) {
            res.block_cnt = 0;
            res.len = 5;
        } else {
            res.block_cnt = 1;
            res.len = 4;
        }
    }
}

int Eval::win_player(const char *state) {
    NULL_CHECK(state, 0)
    for (int r = 0; r < G_B_SIZE; r++) {
        for (int c = 0; c < G_B_SIZE; c++) {
            int spot = state[G_B_SIZE * r + c];
            if (spot == 0){continue;}
            
            // Check in 4 directions
            Measure m_h, m_v, m_lu, m_ru;
            gen_measure(state, r, c, Eval::MEASURE_DIR_H, spot, true, m_h);
            gen_measure(state, r, c, Eval::MEASURE_DIR_V, spot, true, m_v);
            gen_measure(state, r, c, Eval::MEASURE_DIR_LU, spot, true, m_lu);
            gen_measure(state, r, c, Eval::MEASURE_DIR_RU, spot, true, m_ru);
            if (m_h.len >= WIN_COND || m_v.len >= WIN_COND || m_lu.len >= WIN_COND || m_ru.len >= WIN_COND){return spot;}
        }
    }
    return 0;
}

// Search width hyperparameters for controlling the branching factor. Narrower width for deeper depth
int Negamax::search_width[5] = {17, 7, 5, 3, 3};

void Negamax::search(const char *state, int player, int depth, int time_limit, bool enable_ab_pruning, int &actual_depth, int &move_r, int &move_c, int init_depth=INIT_DEPTH, int inc_depth=INC_DEPTH) {
    ERR_NULL_CHECK(state,)
    ERR_PLAYER_CHECK(player,)
    if (depth == 0 || depth < -1 || time_limit < 0){return;}

    // Copy game state
    char ng_state[G_B_AREA] = {0};
    memcpy(ng_state, state, G_B_AREA);

    // Shallower tree for the first move
    int occupied_cnt = 0;
    for (int i = 0; i < G_B_AREA; i++){
        if (ng_state[i] != 0){occupied_cnt++;}
    }
    if (occupied_cnt <= 2){depth = INIT_DEPTH;}

    int alpha = INT_MIN / 2, beta = INT_MAX / 2;

    // Fixed depth or iterative deepening
    if (depth > 0) {
        actual_depth = depth;
        negamax(ng_state, player, depth, depth, enable_ab_pruning, alpha, beta, move_r, move_c);
    } else {
        // Iterative deepening
        std::clock_t clk_start = std::clock();
        for (int d = INIT_DEPTH;; d += INC_DEPTH) {
            std::clock_t clk_iter_start = std::clock();

            // Reset game state
            memcpy(ng_state, state, G_B_AREA);

            // Execute search
            negamax(ng_state, player, d, d, enable_ab_pruning, alpha, beta, move_r, move_c);
            actual_depth = d;
            io.write_valid_spot(Position(move_r, move_c));

            // Times
            std::clock_t clk_iter = (std::clock() - clk_iter_start) * 1000 / CLOCKS_PER_SEC;
            std::clock_t clk_elapse = (std::clock() - clk_start) * 1000 / CLOCKS_PER_SEC;

            INFO("Deepening - actual_depth: " << actual_depth << " | Act: (" << move_r << ", " << move_c << ")" << " | node_count: " << g_node_cnt << " | eval_count: " << g_eval_cnt << " | eval_missed: " << g_eval_missed << "(" << ((float)g_eval_missed / (float)g_eval_cnt) * 100 << "%)" << " | Iter ms: " << clk_iter)

            // if (ENABLE_TIME_LIMIT && (clk_elapse + (clk_iter * AVG_BRANCH_FACTOR * AVG_BRANCH_FACTOR) > time_limit || d >= MAX_DEPTH)) {
            //     actual_depth = d;
            //     break;
            // }
        }
    }
}
int Negamax::negamax(char *state, int player, int initial_depth, int depth, bool enable_ab_pruning, int alpha, int beta, int &move_r, int &move_c) {
    ++g_node_cnt;

    int max_sc = INT_MIN;
    int opn = player == 1 ? 2 : 1;

    // Sort valid, non-remote moves
    std::vector<Move> mvs_player, mvs_opn, cand_mvs;
    search_sorted_cands(state, player, mvs_player);
    search_sorted_cands(state, opn, mvs_opn);

    // exit if there is no more move can take
    if (mvs_player.size() == 0) return 0;

    // End directly if only one move or a winning move is found
    if (mvs_player.size() == 1 || (mvs_player.size() > 0 && mvs_player.at(0).score >= WIN_SCORE_LIMIT)) {
        auto move = mvs_player.at(0);
        move_r = move.r; move_c = move.c;
        return move.score;
    }

    // If opponent poses a threat, add to candidate moves first
    bool block_opn = false;
    int tmp_size = static_cast<int>(mvs_opn.size());
    if(WATCH_THREAT_CNT > -1){tmp_size = min(tmp_size, WATCH_THREAT_CNT);}
    // int tmp_size = std::min(static_cast<int>(mvs_opn.size()), 2);
    if (mvs_opn.at(0).score >= THRAT_SCORE_LIMIT) {
        block_opn = true;
        // ZobristHash zb(state);
        for (int i = 0; i < tmp_size; ++i) {
            Move move = mvs_opn.at(i);

            // Re-evaluate move as current player
            move.score = Eval::eval_pos(state, move.r, move.c, player);
            // move.score = Eval::eval_pos(state, move.r, move.c, player, zb);

            // Add to move candidates
            cand_mvs.push_back(move);
        }
    }

    // Set width
    int width = (initial_depth >> 1) - ((depth + 1) >> 1);
    if (width > 4) width = search_width[4];
    else width = search_width[width];

    // Copy moves for current player
    tmp_size = std::min(static_cast<int>(mvs_player.size()), width);
    for (int i = 0; i < tmp_size; ++i)
        cand_mvs.push_back(mvs_player.at(i));

    // Loop through each candidate move
    int cand_mvs_sz = static_cast<int>(cand_mvs.size());
    for (int i = 0; i < cand_mvs_sz; i++) {
        Move move = cand_mvs.at(i);

        Util::set_spot(state, move.r, move.c, static_cast<char>(player));

        int sc = 0;
        int dummy_r = 0, dummy_c = 0;
        // Negamax
        if (depth > 1) sc = negamax(state, opn, initial_depth, depth - 1, enable_ab_pruning, -beta, -alpha + move.score, dummy_r, dummy_c);

        // Decay longer moves
        if (sc >= 2) sc = static_cast<int>(sc * SCORE_DECAY);
        // sc = static_cast<int>(sc * SCORE_DECAY);

        // Calculate score difference
        move.accum_score = move.score - sc;

        // Store back to candidate array
        cand_mvs.at(i).accum_score = move.accum_score;

        // Restore the move
        Util::set_spot(state, move.r, move.c, 0);

        // Update maximum score
        if (move.accum_score > max_sc) {
            max_sc = move.accum_score;
            move_r = move.r; move_c = move.c;
        }

        // Alpha-beta
        int max_sc_decay = max_sc;
        if (max_sc >= 2) max_sc_decay = static_cast<int>(max_sc_decay * SCORE_DECAY);
        if (max_sc > alpha) alpha = max_sc;
        if (enable_ab_pruning && max_sc_decay >= beta) break;
        // if (enable_ab_pruning && alpha >= beta) break;

        // if (max_sc > alpha) alpha = max_sc;
        // if (enable_ab_pruning && alpha >= beta) break;
    }

    // If there is no move that is much better then the block threats(<0.2), choose the move that blocks the threat.
    if (depth == initial_depth && block_opn && max_sc < 0 && cand_mvs.size() > 0) {
        auto block_mv = cand_mvs.at(0);
        // double base = max(static_cast<double>(std::abs(block_mv.accum_score)), 0.00001);

        int block_sc = block_mv.accum_score;
        if (block_sc == 0) block_sc = 1;
        if ((max_sc - block_sc) / static_cast<float>(std::abs(block_sc)) < 0.2) {
        // if (((max_sc - block_mv.accum_score) / base) < 0.2) {
            move_r = block_mv.r; move_c = block_mv.c;
            max_sc = block_mv.accum_score;
        }
    }
    return max_sc;
}

void active_area(const char *state, int &l_r, int &l_c, int &r_r, int &u_c){
    // Find the box that is extended from the occupied spots in REMOTE_RNG
    l_r = INT_MAX, l_c = INT_MAX, r_r = INT_MIN, u_c = INT_MIN;
    for (int r = 0; r < G_B_SIZE; r++) {
        for (int c = 0; c < G_B_SIZE; c++) {
            if (state[G_B_SIZE * r + c] != 0) {
                if (r < l_r){l_r = r;}
                if (c < l_c){l_c = c;}
                if (r > r_r){r_r = r;}
                if (c > u_c){u_c = c;}
            }
        }
    }

    l_r = max(0, l_r - REMOTE_RNG);
    l_c = max(0, l_c - REMOTE_RNG);
    r_r = min(r_r + REMOTE_RNG, G_B_SIZE - 1);
    u_c = min(u_c + REMOTE_RNG, G_B_SIZE - 1);
}

void Negamax::search_sorted_cands(const char *state, int player, std::vector<Move> &result) {
    // Clear and previous result
    result.clear();
    ZobristHash zb(state);
    int l_r = 0, l_c = 0, r_r = 0, u_c = 0;
    active_area(state, l_r, l_c, r_r, u_c);

    // Loop through all cells
    for (int r = l_r; r <= r_r; r++) {
        for (int c = l_c; c <= u_c; c++) {
            // Consider only empty cells
            if (state[G_B_SIZE * r + c] != 0) continue;

            // Skip remote cells (no pieces within 2 cells)
            if (Util::remote_spot(state, r, c)) continue;

            Move m;
            m.r = r;
            m.c = c;

            // Evaluate move
            m.score = Eval::eval_pos(state, r, c, player);
            // m.score = Eval::eval_pos(state, r, c, player, zb);

            // Add move
            result.push_back(m);
        }
    }
    std::sort(result.begin(), result.end());
}

int main(){
    io.read_board();
    // std::cout << io.board << endl;

    bool is_first_hand = true;
    char b_raw_str[G_B_SIZE * G_B_SIZE] = {0};
    for(int i = 0; i < static_cast<int>(io.board.size()); i++){
        for(int j = 0; j < static_cast<int>(io.board.at(0).size()); j++){
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
    // unsigned int node_count, eval_count;
    bool success = Controller::search_act(b_raw_str, 1, -1, time_limit, actual_depth, move_r, move_c, winning_player);

    io.write_valid_spot(Position(move_r, move_c));
    if(success){
        INFO("Success actual_depth: " << actual_depth << " Act: (" << move_r << ", " << move_c << ")" << " winning_player: " << winning_player << " node_count: " << g_node_cnt << " eval_count: " << g_eval_cnt);
    }else{
        INFO("Fail actual_depth: " << actual_depth << " Act: (" << move_r << ", " << move_c << ")" << " winning_player: " << winning_player << " node_count: " << g_node_cnt << " eval_count: " << g_eval_cnt);
    }
    
    return 0;
}