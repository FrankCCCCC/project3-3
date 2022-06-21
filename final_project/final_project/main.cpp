// Some accelerate command, but you should be careful to use it, sometimes it will affect the execution of the program
// ![40 Lines Acceleration](https://www.cnblogs.com/Yuhuger/p/9280598.html)

#include <iostream>
#include <climits>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <set>
#include <tuple>
#include <algorithm>
#include <random>
#include <sys/types.h>

using namespace std;

enum SPOT_STATE {
    EMPTY = 0,
    BLACK = 1,
    WHITE = 2
};

// Board Size
#define B_SIZE 15
#define DEBUG 0
#define THROWERR 0
#define ERR(s)\
if(THROWERR){\
    std::cerr << "Error: "<< s << " (" << __FILE__ << ", Line " << __LINE__ << ")" << endl;\
    exit(1);\
}\

#define debug_cpp(s)\
if(DEBUG){\
    std::cout << s;\
}\
// Unify coordinate expression
#define SEQ_LEN 17
#define SEQ_TAIL 'Z'
const char SEQ[SEQ_LEN] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'Z' };
inline char seq(int x) {
    if (x < SEQ_LEN) return SEQ[x];
    return SEQ_TAIL;
}
// Coordinate
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

#define get_tuple(t, i) std::get<i>(t)

typedef std::string BoardRow;
typedef std::vector<BoardRow> Board;
typedef int Score;
typedef pair<string, Score> Pattern;
typedef map<Position, Score> MAP;
typedef pair<Position, Score> Candidate;

#define SCORE_MIN INT_MIN
#define SCORE_MAX INT_MAX
#define OC 'O'
#define XC 'X'
#define DC '.'
#define WC '#'
#define O 1
#define X 0
#define D 2
// Normal patterns
const vector<Pattern> PATTERNS({ {"XO.",1}, {".O.",10}, {"XOO.",10}, {".OO.",100}, {"XOOO.",100}, {"XO.OO.",100}, {".OOOO.",100000}, {"OOOOO",1000000} });
// If you don't defense, you will lose
const vector<Pattern> THREATS({ {".OOO.",10000}, {"XOOOO.",100000}, {".O.OO.",10000}, {"OOO.O",100000}, {"XOO.OOX",100000}, {"XOO.OO.",100000}, {".OO.OO.",100000} });
const string TERMINATE_PAT("OOOOO");
#define POS_M_H 'h'
#define POS_M_V 'v'
#define POS_M_LU 'l'
#define POS_M_RU 'r'

inline Position h_adder(const Position& pos, int shift) { return Position(pos.r, pos.c + shift); }
inline Position v_adder(const Position& pos, int shift) { return Position(pos.r + shift, pos.c); }
inline Position lu_adder(const Position& pos, int shift) { return Position(pos.r + shift, pos.c + shift); }
inline Position ru_adder(const Position& pos, int shift) { return Position(pos.r + shift, pos.c - shift); }

inline void insert_cand(const string& pat, Score val, const Position& pos, int shift, MAP* cands, Position(*f)(const Position&, int)) {
    for (int i = 0; i < pat.size(); i++) {
        // if(pat.at(i) == DC) (*cands)[f(pos, shift + i)] += val;
        if (pat.at(i) == DC) {
            (*cands)[f(pos, shift + i)] += val;
            Position ip = f(pos, shift + i);
            //debug_cpp("Insert Cand - Shift: " << shift << ", Pos: " << ip << ", Step Score: " << (*cands)[f(pos, shift + i)] << endl);
        }
    }
}

inline void add_cand(const string& pat, Score val, const Position& s_pos, int shift, char mode, MAP* cands) {
    if (cands == nullptr) return;
    switch (mode) {
    case(POS_M_H):
        // Horizontal
        // cands->insert(make_cand(s_pos.r, s_pos.c + shift, val));
        insert_cand(pat, val, s_pos, shift, cands, h_adder);
        // debug_cpp("Insert H_Cands" << endl);
        break;
    case(POS_M_V):
        // Vertical
        // cands->insert(make_cand(s_pos.r + shift, s_pos.c, val));
        insert_cand(pat, val, s_pos, shift, cands, v_adder);
        break;
    case(POS_M_LU):
        // Left Up
        // cands->insert(make_cand(s_pos.r + shift, s_pos.c + shift, val));
        insert_cand(pat, val, s_pos, shift, cands, lu_adder);
        break;
    case(POS_M_RU):
        // Right Up
        // cands->insert(make_cand(s_pos.r + shift, s_pos.c - shift, val));
        insert_cand(pat, val, s_pos, shift, cands, ru_adder);
        break;
    }
}

inline void computeLPSArray(const string &pat, int M, int* lps) {
    // Length of the previous longest
    // prefix suffix
    int len = 0;
    int i = 1;
    lps[0] = 0; // lps[0] is always 0

    // The loop calculates lps[i] for
    // i = 1 to M-1
    while (i < M) {
        if (pat[i] == pat[len]) {
            len++;
            lps[i] = len;
            i++;
        }
        else {
            // (pat[i] != pat[len])
            // This is tricky. Consider the example.
            // AAACAAAA and i = 7. The idea is similar
            // to search step.
            if (len != 0) {
                len = lps[len - 1];
                // Also, note that we do not
                // increment i here
            }
            else {
                // if (len == 0)
                lps[i] = len;
                i++;
            }
        }
    }
}

inline int KMPSearch(const char *txt, int N, const string &pat, int s_cnt, Score val, const Position& s_pos, char mode, MAP* cands) {
    // KMP Algorithm
    const int M = pat.length();
    //const int N = txt.length();
    if (M > N) return 0;

    // Create lps[] that will hold the longest prefix suffix values for pattern
    int lps[50];
    int j = 0; // index for pat[]

    // Preprocess the pattern (calculate lps[] array)
    computeLPSArray(pat, M, lps);

    int i = 0; // index for txt[]
    int res = 0;
    // int next_i = 0;

    while (i < N) {
        if (pat[j] == txt[i]) {
            j++;
            i++;
        }
        if (j == M) {
            // When we find pattern first time, we iterate again to check if there exists more pattern
            j = lps[j - 1];
            add_cand(pat, val, s_pos, i - M - s_cnt, mode, cands);
            res++;
        }
        else if (i < N && pat[j] != txt[i]) {
            // Mismatch after j matches Do not match lps[0..lps[j-1]] characters, they will match anyway
            if (j != 0)
                j = lps[j - 1];
            else
                i = i + 1;
        }
    }
    return res;
}

typedef uint64_t Hash;

class ZobristHash {
private:
    static Hash* HASH_O, * HASH_X;
public:
    static const int BOARD_SIZE;
    ZobristHash() {}
    static class ClassInit {
        public:
        ClassInit() {
            // Static constructor definition
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<Hash> d(0, UINT64_MAX);
            int tbl_sz = ZobristHash::BOARD_SIZE * ZobristHash::BOARD_SIZE;

            ZobristHash::HASH_O = new Hash[tbl_sz];
            ZobristHash::HASH_X = new Hash[tbl_sz];

            // Generate random values
            for (int i = 0; i < tbl_sz; i++) {
                ZobristHash::HASH_O[i] = d(gen);
                ZobristHash::HASH_X[i] = d(gen);
            }
        }
    } Initialize;

    static Hash hash(const Board &b) {
        Hash h = 0;
        for (int r = 0; r < ZobristHash::BOARD_SIZE; r++) {
            for (int c = 0; c < ZobristHash::BOARD_SIZE; c++) {
                if (b[r][c] == OC) { h ^= HASH_O[r * ZobristHash::BOARD_SIZE + c]; }
                else if (b[r][c] == XC) { h ^= HASH_X[r * ZobristHash::BOARD_SIZE + c]; }
            }
        }
        return h;
    }
    static Hash yield(Hash h, const Position & pos, char player) {
        if (player == O){h ^= HASH_O[pos.r * ZobristHash::BOARD_SIZE + pos.c]; }
        else if (player == X) { h ^= HASH_X[pos.r * ZobristHash::BOARD_SIZE + pos.c]; }
        else { ERR("No such role: " << player); }
        return h;
    }
};
Hash *ZobristHash::HASH_O, *ZobristHash::HASH_X;
const int ZobristHash::BOARD_SIZE = B_SIZE;
ZobristHash::ClassInit ZobristHash::Initialize;

class State {
    friend ostream& operator<<(ostream&, const State&);
    friend bool operator==(const State& a, const State& b);
    friend class ClasInit;
    typedef unordered_map<Hash, State> StateMap;

private:
    Board b;
    Score o_score = 0, x_score = 0, o_threat = 0, x_threat = 0, terminate = 0;
    MAP o_cands, o_threat_cands, x_cands, x_threat_cands;
    Hash hash = 0;
    static MAP dummy_cands;
    static StateMap state_map;

    char buff[50] = { XC };
    int buff_i = 1;
    inline void reset_buff() {
        this->buff_i = 1;
    }
    inline void insert_buff(char c) {
        this->buff[this->buff_i] = c;
        this->buff_i++;
    }
    inline void finalize_buff() {
        this->buff[this->buff_i] = XC;
    }

    inline void update_score(char player, Score d_score, Score d_threat) {
        if (player == O) {
            this->o_score += d_score;
            this->o_threat += d_threat;
        }
        else if (player == X) {
            this->x_score += d_score;
            this->x_threat += d_threat;
        }
    }
    inline Score count_val(char *s, const vector<Pattern>& patterns, const Position& s_pos, char mode, MAP* cands, int* terminate = nullptr) {
        Score d_score = 0;
        for (const Pattern& p : patterns) {
            d_score += p.second * KMPSearch(this->buff, this->buff_i + 1, p.first, 1, p.second, s_pos, mode, cands);
        }
        if (terminate != nullptr) { *terminate += KMPSearch(this->buff, this->buff_i + 1, TERMINATE_PAT, 1, 0, s_pos, mode, nullptr); }
        return d_score;
    }
    inline void count_vals(char *s, const vector<Pattern>& patterns, const vector<Pattern>& threats, const Position& s_pos, char mode, Score& score, Score& threat, MAP* cands, MAP* threat_cands, int* terminate = nullptr) {
        score += count_val(s, patterns, s_pos, mode, cands, terminate);
        threat += count_val(s, threats, s_pos, mode, threat_cands, terminate);
    }
    /**
     * @brief Get the row string
     *
     * @param b
     * @param pos
     * @param player
     * @param n_c
     * @param radius
     * @return string
     */
    inline void get_row(const Position& pos, char player, int n_c, int radius, Position& start) {
        this->reset_buff();
        string s;
        int j = max(pos.c - radius, 0);
        start = Position(pos.r, j);
        for (; j < min(pos.c + radius, n_c); j++) {
            //s += this->convert(player, b[pos.r][j]); 
            this->insert_buff(this->convert(player, b[pos.r][j]));
        }
        this->finalize_buff();
        //return concat_wall(s);
    }
    /**
     * @brief Get the col string
     *
     * @param b
     * @param pos
     * @param player
     * @param n_r
     * @param radius
     * @return string
     */
    inline void get_col(const Position& pos, char player, int n_r, int radius, Position& start) {
        this->reset_buff();
        string s;
        int j = max(pos.r - radius, 0);
        start = Position(j, pos.c);
        for (; j < min(pos.r + radius, n_r); j++) {
            //s += this->convert(player, b[j][pos.c]);
            this->insert_buff(this->convert(player, b[j][pos.c]));
        }
        this->finalize_buff();
        //return concat_wall(s);
    }
    inline void get_lu_diag(const Position& pos, char player, int n_r, int n_c, int radius, Position& start) {
        this->reset_buff();
        string s;
        int dir_shift = min(pos.r, pos.c);
        int shift = min(dir_shift, radius);
        int i = pos.r - shift, j = pos.c - shift;
        start = Position(i, j);
        for (; i < n_r && j < n_c; i++, j++) {
            //s += this->convert(player, this->b[i][j]); 
            this->insert_buff(this->convert(player, this->b[i][j]));
        }
        this->finalize_buff();
        //return concat_wall(s);
    }
    inline void get_ru_diag(const Position& pos, char player, int n_r, int n_c, int radius, Position& start) {
        this->reset_buff();
        string s;
        int dir_shift = min(pos.r, n_c - pos.c - 1);
        int shift = min(dir_shift, radius);
        int i = pos.r - shift, j = pos.c + shift;
        start = Position(i, j);
        // debug_cpp("Start: " << start << " | dir_shift: " << dir_shift << " | shift: " << shift << endl);
        for (; i < n_r && j >= 0; i++, j--) { 
            //s += this->convert(player, this->b[i][j]); 
            this->insert_buff(this->convert(player, this->b[i][j]));
        }
        this->finalize_buff();
        //return concat_wall(s);
    }
    inline char convert(char player, char x) {
        if (player == O) {
            if (x == OC) return OC;
            else if (x == XC) return XC;
            else return DC;
        }
        else if (player == X) {
            if (x == OC) return XC;
            else if (x == XC) return OC;
            else return DC;
        }
        ERR("No such role: " << player);
        return DC;
        // return !(player ^ x);
    }

    inline void update_val_role(char player, Score d_score, Score d_threat) {
        if (player == O) {
            this->o_score += d_score;
            this->o_threat += d_threat;
        }
        else if (player == X) {
            this->x_score += d_score;
            this->x_threat += d_threat;
        }
    }
    void build_state_role(char player) {
        if (player != O && player != X) { ERR("No such role"); return; }

        int n_r = b.size(), n_c = b.at(0).size();
        int radius = max(n_c, n_r);

        MAP* cands = nullptr, * threat_cands = nullptr;
        if (player == O) { cands = &this->o_cands; threat_cands = &this->o_threat_cands; }
        else if (player == X) { cands = &this->x_cands; threat_cands = &this->x_threat_cands; }

        Position dummy_pos;
        Score d_score = 0, d_threat = 0;
        // Horizontal
        for (int i = 0; i < n_r; i++) {
            Position s_pos(i, 0);
            this->get_row(s_pos, player, n_c, radius, dummy_pos);
            // debug_cpp("Get Row [" << i << "]: " << s << endl);
            this->count_vals(this->buff, State::patterns, State::threats, s_pos, POS_M_H, d_score, d_threat, cands, threat_cands, &this->terminate);
        }
        // Vertical
        for (int i = 0; i < n_c; i++) {
            Position s_pos(0, i);
            this->get_col(s_pos, player, n_r, radius, dummy_pos);
            this->count_vals(this->buff, State::patterns, State::threats, s_pos, POS_M_V, d_score, d_threat, cands, threat_cands, &this->terminate);
        }
        // Left Up Diagonal
        for (int i = 0; i < n_c; i++) {
            Position s_pos(0, i);
            this->get_lu_diag(s_pos, player, n_r, n_c, radius, dummy_pos);
            this->count_vals(this->buff, State::patterns, State::threats, s_pos, POS_M_LU, d_score, d_threat, cands, threat_cands, &this->terminate);
        }
        for (int i = 1; i < n_r; i++) {
            Position s_pos(i, 0);
            this->get_lu_diag(s_pos, player, n_r, n_c, radius, dummy_pos);
            this->count_vals(this->buff, State::patterns, State::threats, s_pos, POS_M_LU, d_score, d_threat, cands, threat_cands, &this->terminate);
        }
        // Right Up Diagonal
        for (int i = 0; i < n_c; i++) {
            Position s_pos(0, i);
            this->get_ru_diag(s_pos, player, n_r, n_c, radius, dummy_pos);
            this->count_vals(this->buff, State::patterns, State::threats, s_pos, POS_M_RU, d_score, d_threat, cands, threat_cands, &this->terminate);
        }
        for (int i = 1; i < n_r; i++) {
            Position s_pos(i, n_c - 1);
            this->get_ru_diag(s_pos, player, n_r, n_c, radius, dummy_pos);
            this->count_vals(this->buff, State::patterns, State::threats, s_pos, POS_M_RU, d_score, d_threat, cands, threat_cands, &this->terminate);
        }

        // Search Pattern
        // this->count_vals(s, State::patterns, State::threats, d_score, d_threat, nullptr);
        this->update_val_role(player, d_score, d_threat);
    }
    void build_state() {
        this->build_state_role(O);
        this->build_state_role(X);
    }

    void pos_score_threat(const Position& pos, char player, Score& score, Score& threat, MAP* cands = nullptr, MAP* threat_cands = nullptr) {
        int n_r = this->b.size(), n_c = this->b.at(0).size();
        if (pos.r >= n_r || pos.r < 0 || pos.c >= n_c || pos.c < 0) { ERR("Position Out of boundary"); return; }
        if (player != O && player != X) { ERR("No such role"); return; }

        string s;
        Position s_pos;
        if (cands == nullptr && threat_cands == nullptr) {
            if (player == O) { cands = &this->o_cands; threat_cands = &this->o_threat_cands; }
            else if (player == X) { cands = &this->x_cands; threat_cands = &this->x_threat_cands; }
        }

        // Score d_score = 0, d_threat = 0;
        // Horizontal
        this->get_row(pos, player, n_c, State::SCAN_RADIUS, s_pos);
        count_vals(this->buff, State::patterns, State::threats, s_pos, POS_M_H, score, threat, cands, threat_cands);
        // Vertical
        this->get_col(pos, player, n_r, State::SCAN_RADIUS, s_pos);
        count_vals(this->buff, State::patterns, State::threats, s_pos, POS_M_V, score, threat, cands, threat_cands);
        // Left Up
        this->get_lu_diag(pos, player, n_r, n_c, State::SCAN_RADIUS, s_pos);
        count_vals(this->buff, State::patterns, State::threats, s_pos, POS_M_LU, score, threat, cands, threat_cands);
        // Right Up
        this->get_ru_diag(pos, player, n_r, n_c, State::SCAN_RADIUS, s_pos);
        // debug_cpp("update_score_threat - RU: pos: " << pos << " | s_pos: " << s_pos << " -> " << s << endl);
        count_vals(this->buff, State::patterns, State::threats, s_pos, POS_M_RU, score, threat, cands, threat_cands);

        // this->update_val_role(player, d_score, d_threat);
    }

    State& update(const Position& p, char player) {
        this->hash = ZobristHash::yield(this->hash, p, player);
        Score o_score0 = 0, o_threat0 = 0, o_score1 = 0, o_threat1 = 0;
        Score x_score0 = 0, x_threat0 = 0, x_score1 = 0, x_threat1 = 0;

        const int cand_n = 4;
        // cands0[4] = {o_cands0, o_threat_cands0, x_cands0, x_threat_cands0}
        MAP cands0[cand_n];
        // cands1[4] = {o_cands1, o_threat_cands1, x_cands1, x_threat_cands1}
        MAP cands1[cand_n];
        MAP* cands_p[cand_n] = { &(this->o_cands), &(this->o_threat_cands), &(this->x_cands), &(this->x_threat_cands) };

        // Current score
        pos_score_threat(p, O, o_score0, o_threat0, &(cands0[0]), &(cands0[1]));
        pos_score_threat(p, X, x_score0, x_threat0, &(cands0[2]), &(cands0[3]));
        // Update board
        if (player == O) {
            this->b[p.r][p.c] = OC;
        }
        else if (player == X) {
            this->b[p.r][p.c] = XC;
        }
        // Score after new chess
        pos_score_threat(p, O, o_score1, o_threat1, &(cands1[0]), &(cands1[1]));
        pos_score_threat(p, X, x_score1, x_threat1, &(cands1[2]), &(cands1[3]));

        this->update_val_role(O, o_score1 - o_score0, o_threat1 - o_threat0);
        this->update_val_role(X, x_score1 - x_score0, x_threat1 - x_threat0);

        for (int i = 0; i < cand_n; i++) {
            // Remove the occupied position
            cands_p[i]->erase(p);
            for (Candidate cand : cands1[i]) {
                // Compute the difference of the value of each entry and, then update
                ((*cands_p[i])[cand.first]) += (cands1[i][cand.first] - cands0[i][cand.first]);
            }
        }
        return *this;
    }

public:
    const static string SEP;
    const static int SCAN_RADIUS;
    static vector<Pattern> patterns;
    static vector<Pattern> threats;

    static class ClassInit {
    public:
        ClassInit() {
            // Static constructor definition
            State::patterns = PATTERNS;
            State::threats = THREATS;
            // Reverse patterns
            for (const Pattern& pat : PATTERNS) {
                Pattern pat_r(pat);
                reverse(pat_r.first.begin(), pat_r.first.end());
                //cout << "<" << pat_r.first << ", " << pat_r.second << "> - " << State::patterns.back().first << endl;
                if (pat.first != pat_r.first) { State::patterns.push_back(pat_r); }
            }
            for (const Pattern& thr : THREATS) {
                Pattern thr_r(thr);
                reverse(thr_r.first.begin(), thr_r.first.end());
                //cout << "<" << thr_r.first << ", " << thr_r.second << "> - " << State::threats.back().first << endl;
                if (thr.first != thr_r.first) { State::threats.push_back(thr_r); }
            }
        }
    } Initialize;

    State() {
        this->o_score = this->o_threat = this->o_threat = this->x_threat = 0;
    }
    State(const Board &b) {
        this->b = b;
        this->o_score = this->o_threat = this->o_threat = this->x_threat = 0;
        this->build_state();
        State::state_map[ZobristHash::hash(b)] = *this;
        this->hash = ZobristHash::hash(this->b);
    }
    inline Score get_score(char player) const {
        if (player == O) return this->o_score;
        else if (player == X) return this->x_score;
        ERR("No such role");
        return -1;
    }
    inline Score get_threat(char player) const {
        if (player == O) return this->o_threat;
        else if (player == X) return this->x_threat;
        ERR("No such role");
        return -1;
    }
    inline Score get_enemy_score(char player) const {
        if (player == O) return this->x_score;
        else if (player == X) return this->o_score;
        ERR("No such role");
        return -1;
    }
    inline Score get_enemy_threat(char player) const {
        if (player == O) return this->x_threat;
        else if (player == X) return this->o_threat;
        ERR("No such role");
        return -1;
    }
    inline const MAP& get_cands(char player) const {
        if (player == O) return this->o_cands;
        else if (player == X) return this->x_cands;
        ERR("No such role");
        return State::dummy_cands;
    }
    inline const MAP& get_enemy_cands(char player) const {
        if (player == O) return this->x_cands;
        else if (player == X) return this->o_cands;
        ERR("No such role");
        return State::dummy_cands;
    }
    inline const MAP& get_threat_cands(char player) const {
        if (player == O) return this->o_threat_cands;
        else if (player == X) return this->x_threat_cands;
        ERR("No such role");
        return State::dummy_cands;
    }
    inline const MAP& get_enemy_threat_cands(char player) const {
        if (player == O) return this->x_threat_cands;
        else if (player == X) return this->o_threat_cands;
        ERR("No such role");
        return State::dummy_cands;
    }
    inline const Board& board() const { return this->b; }
    inline Score is_terminate() const { return this->terminate; }

    static State* yield(const State& s, const Position& p, char player) {
        /*State& new_s = State(s).update(p, player);
        State::state_map[new_s.hash] = new_s;
        return &(State::state_map.at(new_s.hash));*/

        Hash yield_h = ZobristHash::yield(s.hash, p, player);

        StateMap::iterator f = State::state_map.find(yield_h);
        if (f == State::state_map.end()) {
            State::state_map[yield_h] = State(s).update(p, player);
            return &(State::state_map[yield_h]);
        }
        // Debug to check whether the state is the smae or not.
        /*State new_s = State(s).update(p, player);
        if (!State::is_equal(f->second, new_s)) {
            ERR("Updated state is different from the one in the map");
        }*/
        return &(f->second);
    }
    static bool is_equal(const State& a, const State& b) {
        /*Board b;
        Score o_score = 0, x_score = 0, o_threat = 0, x_threat = 0, terminate = 0;
        MAP o_cands, o_threat_cands, x_cands, x_threat_cands;*/
        if (a.b != b.b) {
            ERR("Boards are different");
            return false;
        }
        if (a.o_score != b.o_score) {
            ERR("o_score are different");
            return false;
        }
        if (a.x_score != b.x_score) {
            ERR("x_score are different");
            return false;
        }
        if (a.o_threat != b.o_threat) {
            ERR("o_threat are different");
            return false;
        }
        if (a.x_threat != b.x_threat) {
            ERR("x_threat are different");
            return false;
        }
        /*if (a.o_cands != b.o_cands) {
            ERR("o_cands are different");
            return false;
        }
        if (a.x_cands != b.x_cands) {
            ERR("x_cands are different");
            return false;
        }
        if (a.o_threat_cands != b.o_threat_cands) {
            ERR("o_threat_cands are different");
            return false;
        }
        if (a.x_threat_cands != b.x_threat_cands) {
            ERR("x_threat_cands are different");
            return false;
        }*/
        return true;
    }
};
State::StateMap State::state_map;

ostream& operator<<(ostream& os, const MAP& cands) {
    os << "Candidates: ";
    for (const Candidate& cand : cands) {
        os << "<" << cand.first << ", " << cand.second << "> ";
    }
    os << endl;
    return os;
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

ostream& operator<<(ostream& os, const State& s) {
    Board b = s.b;
    for (const pair<Position, int>& p : s.o_threat_cands) {
        if (b[p.first.r][p.first.c] == DC) { b[p.first.r][p.first.c] = 'B'; }
    }
    for (const pair<Position, int>& p : s.x_threat_cands) {
        if (b[p.first.r][p.first.c] == DC) { b[p.first.r][p.first.c] = 'D'; }
    }
    for (const pair<Position, int>& p : s.o_cands) {
        if (b[p.first.r][p.first.c] == DC) { b[p.first.r][p.first.c] = 'A'; }
    }
    for (const pair<Position, int>& p : s.x_cands) {
        if (b[p.first.r][p.first.c] == DC) { b[p.first.r][p.first.c] = 'C'; }
    }
    os << b;
    os << "Is_Terminate: " << s.is_terminate() << endl;
    os << "O_Score: " << s.o_score << ", O_Threat: " << s.o_threat << ", X_Score: " << s.x_score << ", X_Threat: " << s.x_threat << endl;
    os << "O_Cands: " << s.o_cands.size() << ", O_Threat_Cands: " << s.o_threat_cands.size() << ", X_Cands: " << s.x_cands.size() << ", X_Threat_Cands: " << s.x_threat_cands.size() << endl;
    os << "-> O_Cands - " << s.o_cands;
    os << "-> O_Threat_Cands - " << s.o_threat_cands;
    os << "-> X_Cands - " << s.x_cands;
    os << "-> X_Threat_Cands - " << s.x_threat_cands;
    os << "=============================" << endl;
    return os;
}

MAP  State::dummy_cands;
const string State::SEP = string("|");
const int State::SCAN_RADIUS = 5;
vector<Pattern> State::patterns;
vector<Pattern> State::threats;
State::ClassInit State::Initialize = State::ClassInit();

class IO {
protected:
    int b_size = 0;
    std::ifstream fin;
    std::ofstream fout;
    // char player = 0;

public:
    Board board;

    IO(int b_size, const string& infile, const string& outfile) {
        this->b_size = b_size;
        this->board = Board(b_size, BoardRow(b_size, 0));
        this->fin = std::ifstream(infile);
        this->fout = std::ofstream(outfile);
    }
    // inline int get_player(){return this->player;}

    void read_board() {
        char player = 0;
        this->fin >> player;
        for (int i = 0; i < b_size; i++) {
            for (int j = 0; j < b_size; j++) {
                char c = 0;
                this->fin >> c;
                if (player == '1') {
                    if (c == '1') { this->board[i][j] = OC; }
                    else if (c == '2') { this->board[i][j] = XC; }
                    else { this->board[i][j] = DC; }
                }
                else if (player == '2') {
                    if (c == '2') { this->board[i][j] = OC; }
                    else if (c == '1') { this->board[i][j] = XC; }
                    else { this->board[i][j] = DC; }
                }
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
        // while(true) {
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
// PosCandMap: <Position of next step, <Score of step, <Next State, Score of next state>>>
// typedef pair<Position, pair<State, Score>> PosCand;

#define PC_I 0
#define ST_I 1
#define STC_I 2
#define P_I 3
//typedef map<Board, State> StateMap;
typedef tuple<Score, State*, Score, Position> PosCandMapElem;
// PosCandMap: map<Position of next step, tuple<Score of step, Next State, Score of next state, Position of next step>>
typedef unordered_map<int, PosCandMapElem> PosCandMap;
typedef vector<PosCandMapElem*> PosCandSorted;

ostream& operator<<(ostream& os, const PosCandMapElem& pc) {
    os << "<" << get_tuple(pc, P_I) << ", <Pos Sc: " << get_tuple(pc, PC_I) << ", St Sc: " << get_tuple(pc, STC_I) << ">>";
    return os;
}
ostream& operator<<(ostream& os, const PosCandMap& map) {
    os << "PosCandMap - ";
    for (const pair<int, PosCandMapElem>& p : map) {
        os << p.second << " ";
    }
    os << endl;
    return os;
}
ostream& operator<<(ostream& os, const PosCandSorted& v) {
    os << "PosCandSorted - ";
    for (const PosCandMapElem* e : v) {
        os << *e << " ";
    }
    os << endl;
    return os;
}

inline Score eval_state(const State& s, char player) {
    // const State &s = *get_tuple(a, 1);
    return s.get_score(player) + s.get_threat(player) - s.get_enemy_score(player) + s.get_enemy_threat(player);
}

inline bool pos_cand_comp(const PosCandMapElem* a, const PosCandMapElem* b) {
    // Sorted by state score
    return get_tuple(*a, STC_I) < get_tuple(*b, STC_I);
}

inline Board& insert_pos(Board& b, const Position& pos, char player) {
    if (player == O) { b[pos.r][pos.c] = OC; }
    else if (player == X) { b[pos.r][pos.c] = XC; }
    else { ERR("No such role"); }
    return b;
}

class Strategy : IO {
private:
    char player;
    int cnt = 0;
    // PosCandMap cands;
    // PosCandSorted sorted_cands;

    inline static State* update_state(const State& s, const Position& pos, char player) {
        return State::yield(s, pos, player);
        /*State s_new(s);
        s_new.update(pos, player);
        pair<StateMap::iterator, bool> p = Strategy::state_map.insert({ s_new.board(), s_new });
        if (!p.second) { p.first->second = s_new; }
        return &(p.first->second);*/

        // State s_new(s);
        // s_new.update(pos, player);
        // return Strategy::state_map[s_new.board()] = s_new;
    }
public:
    //static StateMap state_map;
    Strategy(int b_size, char player, const string& infile, const string& outfile) : IO(b_size, infile, outfile) {
        if (player != O && player != X) { ERR("No such role"); return; }
        this->player = player;
    }
    // If there is any threat posed by enemy, defense immediately
    bool quick_react(const State& s, char player, const PosCandMap& cands, const PosCandSorted& sorted_cands, Position& next_pos, Score& next_s_score) {
        Score next_p_score = SCORE_MIN;
        if (s.get_enemy_threat_cands(player).size() > 0) {
            for (const Candidate& cand : s.get_enemy_threat_cands(player)) {
                // Compare with position score, not state score
                if (cand.second > next_p_score) {
                    // Update position score
                    next_p_score = cand.second;
                    // Update next_pos(next position) and next_s_score(next state score)
                    next_pos = cand.first;
                    next_s_score = get_tuple(cands.at(next_pos.idx), STC_I);
                }
            }
            return true;
        }
        return false;
    }
    // Construct Position-Candidate map and sort the candidate by the state score.
    void create_cands(const State& s, char player, PosCandMap& cands, PosCandSorted& sorted_cands) {
        // Add candidate positions and the position score of the candidates
        for (const Candidate& cand : s.get_cands(player)) {
            PosCandMapElem &e = cands[cand.first.idx];
            get_tuple(e, PC_I) += cand.second;
            get_tuple(e, P_I) = cand.first;
        }
        for (const Candidate& cand : s.get_threat_cands(player)) {
            PosCandMapElem& e = cands[cand.first.idx];
            get_tuple(e, PC_I) += cand.second;
            get_tuple(e, P_I) = cand.first;
        }
        for (const Candidate& cand : s.get_enemy_cands(player)) {
            PosCandMapElem& e = cands[cand.first.idx];
            get_tuple(e, PC_I) += cand.second;
            get_tuple(e, P_I) = cand.first;
        }
        for (const Candidate& cand : s.get_enemy_threat_cands(player)) {
            PosCandMapElem& e = cands[cand.first.idx];
            get_tuple(e, PC_I) += cand.second;
            get_tuple(e, P_I) = cand.first;
        }
        // TODO: Add all valid candidate step in order to handle the first hand
        for (PosCandMap::iterator pc = cands.begin(); pc != cands.end(); pc++) {
            // Compute next state
            State* next_s = Strategy::update_state(s, get_tuple(pc->second, P_I), player);
            get_tuple(pc->second, ST_I) = next_s;
            // Compute score of the state, in the your own view
            Score s_sc = eval_state(*next_s, this->player);
            get_tuple(pc->second, STC_I) = s_sc;

            // debug_cpp(*next_s);
            // debug_cpp(pc->first << " - S_SC: " << s_sc << endl);
            // Add position info to the tuple
            //get_tuple(cands[pc->first], P_I) = pc->first;
            sorted_cands.push_back(&(pc->second));
        }
        sort(sorted_cands.begin(), sorted_cands.end(), pos_cand_comp);

        debug_cpp(cands);
        debug_cpp(sorted_cands);
    }
    tuple<Position, Score> expand(const State& s, char player, int depth, bool q_react, Score a, Score b) {
        if (player != O && player != X) { ERR("No such role"); return tuple<Position, Score>(Position(0, 0), 0); }
        if (depth < 0 || s.is_terminate()) {
            // No expansion, depth = 0 means only expand your own 1 step
            return tuple<Position, Score>(Position(0, 0), eval_state(s, player));
        }
        debug_cpp("Expanding..." << endl);
        debug_cpp(s);
        
        char op = player == O ? X : O;

        PosCandMap cands;
        PosCandSorted sorted_cands;
        debug_cpp("Creating Candidates..." << endl);
        this->create_cands(s, player, cands, sorted_cands);
        debug_cpp("Created Candidates..." << endl);

        // Detect threat
        if (q_react) {
            debug_cpp("Quick Reacting..." << endl);
            tuple<Position, Score> tpl;
            if (this->quick_react(s, player, cands, sorted_cands, get_tuple(tpl, 0), get_tuple(tpl, 1))) {
                return tpl;
            }
            debug_cpp("Quick Reacted..." << endl);
        }

        // Expand next step
        Score v = SCORE_MIN;
        tuple<Position, Score> res;
        for (PosCandMapElem* scand : sorted_cands) {
            debug_cpp("Searching... " << get_tuple(*scand, P_I) << "..." << endl);
            Position act;
            Score sc = 0;
            std::tie(act, sc) = this->expand(*get_tuple(*scand, ST_I), op, depth - 1, false, -b, -a);
            sc = -sc;
            if (sc > v) {
                v = sc;
                res = tuple<Position, Score>(get_tuple(*scand, P_I), v);
            }
            //if (v >= b) { return res; }
            a = max(a, v);
        }
        return res;
    }

    pair<Position, Score> search_act() {
        Position act;
        Score sc = 0;
        bool is_first_hand = true;
        this->read_board();

        for (int r = 0; r < this->board.size(); r++) {
            for (int c = 0; c < this->board.at(0).size(); c++) {
                if (this->board[r][c] != DC) {
                    is_first_hand = false;
                }
            }
        }

        if (is_first_hand) {
            // this->rand_spot(act, sc); 
            act = Position(11, 12);
            // act = Position(11, 10);
            sc = 0;
            write_valid_spot(act);
        }
        else {
            std::tie(act, sc) = this->expand(State(this->board), this->player, 2, false, SCORE_MIN, SCORE_MAX);
            sc = -sc;
            write_valid_spot(act);
        }
        return make_pair(act, sc);
    }
};

//StateMap Strategy::state_map = StateMap();

int main() {
    string infile("state"), outfile("action");
    // string infile("input.txt"), outfile("output.txt");
    /*IO io(B_SIZE, infile, outfile);
    io.read_board();
    std::cout << io.board << endl;
    State s(io.board);
    std::cout << s << endl;*/

    // s.update(Position(7,12), O);
    // std::cout << s << endl;

    // s.update(Position(7,8), X);
    // s.update(Position(8,8), X);
    // s.update(Position(9,8), X);
    // s.update(Position(8,6), O);
    // std::cout << s << endl;

    Strategy stg(B_SIZE, O, infile, outfile);
    pair<Position, Score> act = stg.search_act();
    std::cout << "Next Pos: " << act.first << " | State Score: " << act.second << endl;

    return 0;
}