// run1.cpp
// Revised CNF builder for Metro Map Planning (DIMACS output)
// - contiguous variable blocks
// - correct reachability propagation from start
// - exact in/out degree for intermediate cells
// - combinatorial at-most-(J) for turns (forbid any J+1)
// Note: Designed to follow assignment I/O exactly.

#include <bits/stdc++.h>
using namespace std;

struct MetroLine { int sx, sy, ex, ey; };

// Block sizes computed from parameters
// We will assign contiguous blocks:
//  block 0: varId (K * N * M)
//  block 1: Eid  (4 * K * N * M)
//  block 2: Rid  (K * N * M)
//  block 3: Tid  (K * N * M)
// total vars = (1 + 4 + 1 + 1) * K * N * M = 7 * K * N * M

inline bool inBounds(int x, int y, int N, int M) {
    return x >= 0 && x < N && y >= 0 && y < M;
}

int Eid_base(int K, int N, int M) { return K * N * M; }
int Rid_base(int K, int N, int M) { return Eid_base(K,N,M) + 4 * K * N * M; }
int Tid_base(int K, int N, int M) { return Rid_base(K,N,M)/*5 * K * N * M */ + K * N * M; }
 
// constant buffer of y * N only, not x * M
int varId(int k, int x, int y, int N, int M) {
    return k * N * M + y * N + x + 1;
}
int Eid(int k, int x, int y, int d, int N, int M, int K) {
    // base made to avoid collision with the varID variables...
    int base = Eid_base(K,N,M);
    // d in [0..3]; layout: block per direction: d * (K*N*M)
    return base + d * (K * N * M) + k * N * M + y * N + x + 1;
}
int Rid(int k, int x, int y, int N, int M, int K) {
    int base = Rid_base(K,N,M);
    return base + k * N * M + y * N + x + 1;
}
int Tid(int k, int x, int y, int N, int M, int K) {
    int base = Tid_base(K,N,M); // 5 * M * N * k
    return base + k * N * M + y * N + x + 1;
}

// Add pairwise at-most-one (simple, quadratic)
void add_amo_pairwise(const vector<int>& vars, vector<vector<int>>& clauses) {
    int n = vars.size();
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
            clauses.push_back({-vars[i], -vars[j]});
}

// Add combinatorial "at-most-K" by forbidding any (K+1) subset
// This is exponential in K but K (turn limit) is small in assignment.
void add_at_most_k_combinatorial(const vector<int>& vars, int Klimit, vector<vector<int>>& clauses) {
    int n = vars.size();
    if (n <= Klimit) return;
    int r = Klimit + 1; // forbid any r-set
    // generate all combinations of r indices out of n
    vector<int> comb(r); // avoiding any set of r variables being simultaneously true
    for (int i = 0; i < r; ++i) comb[i] = i;
    while (true) {
        vector<int> clause;
        for (int i = 0; i < r; ++i) clause.push_back(-vars[comb[i]]);
        // adds the constraint that not all of  
        clauses.push_back(move(clause));
        // next combination
        int t = r - 1;
        while (t >= 0 && comb[t] == n - (r - t)) --t;
        if (t < 0) break;
        ++comb[t];
        for (int j = t + 1; j < r; ++j) comb[j] = comb[j - 1] + 1;
    }
}

int main(int argc, char* argv[]) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc < 3) {
        cerr << "Usage: ./run1 <input.city> <output.satinput>\n";
        return 1;
    }
    ifstream fin(argv[1]);
    ofstream fout(argv[2]);
    if (!fin.is_open()) { cerr << "Cannot open input file\n"; return 1; }
    if (!fout.is_open()) { cerr << "Cannot open output file\n"; return 1; }

    int scenario;
    fin >> scenario;
    int N, M, K, J, P = 0;
    fin >> N >> M >> K >> J;
    // if (scenario == 2) fin >> P;

    vector<MetroLine> lines(K);
    for (int i = 0; i < K; ++i) fin >> lines[i].sx >> lines[i].sy >> lines[i].ex >> lines[i].ey;
    
    vector<pair<int,int>> popular;
    if (scenario == 2) {
        popular.resize(P);
        for (int i = 0; i < P; ++i) fin >> popular[i].first >> popular[i].second;
    }
    
    vector<vector<int>> clauses;
    int dx[4] = {1, 0, -1, 0}; // 0:R,1:D,2:L,3:U
    int dy[4] = {0, 1, 0, -1};

    // Constraint 1: At most one metro per cell (across k)
    for (int x = 0; x < N; ++x) {
        for (int y = 0; y < M; ++y) {
            for (int a = 0; a < K; ++a) {
                for (int b = a + 1; b < K; ++b) {
                    clauses.push_back({-varId(a, x, y, N, M), -varId(b, x, y, N, M)});
                }
            }
        }
    }

    // Constraint 2: start and end occupied
    for (int k = 0; k < K; ++k) {
        clauses.push_back({varId(k, lines[k].sx, lines[k].sy, N, M)});
        clauses.push_back({varId(k, lines[k].ex, lines[k].ey, N, M)});
    }

    // For each line, build path, edges, reachability, and turns
for (int k = 0; k < K; ++k) {
    int sx = lines[k].sx, sy = lines[k].sy;
    int ex = lines[k].ex, ey = lines[k].ey;
    /*
    for (int x = 0; x < N; ++x) {
        for (int y = 0; y < M; ++y) {
            int v = varId(k, x, y, N, M); // ✅ fixed missing K
            bool is_start = (x == sx && y == sy);
            bool is_end   = (x == ex && y == ey);

            vector<int> out_list, in_list;

            // Outgoing edges
            for (int d = 0; d < 4; ++d) {
                int nx = x + dx[d], ny = y + dy[d];
                if (!inBounds(nx, ny, N, M)) continue;
                int e = Eid(k, x, y, d, N, M, K);
                out_list.push_back(e);

                clauses.push_back({-e, v}); // e ⇒ source occupied
                clauses.push_back({-e, varId(k, nx, ny, N, M)}); // e ⇒ dest occupied
                clauses.push_back({v, -e}); // ¬v ⇒ ¬e ✅ correct fix
            }

            // Incoming edges
            for (int d = 0; d < 4; ++d) {
                int px = x - dx[d], py = y - dy[d];
                if (!inBounds(px, py, N, M)) continue;
                int e = Eid(k, px, py, d, N, M, K);
                in_list.push_back(e);

                clauses.push_back({-v, -e}); // ¬v ⇒ ¬incoming edge
                clauses.push_back({-e, v});  // e ⇒ v ✅ added for symmetry
            }

            add_amo_pairwise(in_list, clauses);
            add_amo_pairwise(out_list, clauses);

            if (is_start) {
                for (int e : in_list) clauses.push_back({-e});
                if (!out_list.empty()) clauses.push_back(out_list); // OR(out_list)
            } else if (is_end) {
                for (int e : out_list) clauses.push_back({-e});
                if (!in_list.empty()) clauses.push_back(in_list); // OR(in_list)
            } else {
                if (!in_list.empty()) {
                    vector<int> atleast_in = {-v};
                    atleast_in.insert(atleast_in.end(), in_list.begin(), in_list.end());
                    clauses.push_back(atleast_in); // v ⇒ OR(in_list)
                }
                if (!out_list.empty()) {
                    vector<int> atleast_out = {-v};
                    atleast_out.insert(atleast_out.end(), out_list.begin(), out_list.end());
                    clauses.push_back(atleast_out); // v ⇒ OR(out_list)
                }
            }
        }
    } */

    // Reachability
    // Rid(..) represents the start cell
    clauses.push_back({Rid(k, sx, sy, N, M, K)}); // start reachable

    for (int x = 0; x < N; ++x) {
        for (int y = 0; y < M; ++y) {
            int r = Rid(k, x, y, N, M, K);
            int v = varId(k, x, y, N, M);

            // clauses.push_back({-r, v}); // r ⇒ v
            clauses.push_back({-v, r}); // v ⇒ r

            for (int d = 0; d < 4; ++d) {
                int nx = x + dx[d], ny = y + dy[d];
                if (!inBounds(nx, ny, N, M)) continue;
                int e = Eid(k, x, y, d, N, M, K);
                int nr = Rid(k, nx, ny, N, M, K);
                clauses.push_back({-r, -e, nr}); // r & e ⇒ r_next
            }
        }
    }

    clauses.push_back({Rid(k, ex, ey, N, M, K)}); // end must be reachable

    // Turns
    // enforcing atmost J turns
    vector<int> turn_vars;
    for (int x = 0; x < N; ++x) {
        for (int y = 0; y < M; ++y) {
            // inside, iterating through each object of metroline 
            if ((x == sx && y == sy) || (x == ex && y == ey)) continue;
            // t -> a turn varible, activate t iff direction change occurs
            int t = Tid(k, x, y, N, M, K);
            turn_vars.push_back(t); // possible turn variable push for later use
                
            // merely possible prev and next cells...
            vector<pair<int,int>> in_e_d, out_e_d;
            for (int d = 0; d < 4; ++d) {
                int px = x - dx[d], py = y - dy[d];
                if (inBounds(px, py, N, M))
                    // takes the possible previous coordinate , not necessarily guaranteeing that was the previous path... it is just a possibility
                    in_e_d.push_back({Eid(k, px, py, d, N, M, K), d});
                int nx = x + dx[d], ny = y + dy[d];
                if (inBounds(nx, ny, N, M))
                    out_e_d.push_back({Eid(k, x, y, d, N, M, K), d});
            }
            // in_e_d and out_e_d represent same coordis, but different edge variables actually... cuz of mapping with the same x, y but different px, py vs nx, ny

            for (auto &ie : in_e_d) {
                for (auto &oe : out_e_d) {
                    int ieid = ie.first, idir = ie.second;
                    int oeid = oe.first, odir = oe.second;
                    if ((idir) == (odir)) {
                        clauses.push_back({-ieid, -oeid, -t}); // same axis ⇒ no turn
                    } else {
                        clauses.push_back({-ieid, -oeid, t}); // different axis ⇒ turn
                    }
                }
            }
        }
    }

    add_at_most_k_combinatorial(turn_vars, J, clauses); // enforce turn limit
}
      

    // Scenario 2: popular cells must be covered by at least one line
    if (scenario == 2) {
        for (auto &cell : popular) {
            int x = cell.first, y = cell.second;
            vector<int> ors;
            for (int k = 0; k < K; ++k) ors.push_back(varId(k, x, y, N, M));
            clauses.push_back(ors);
        }
    }

    // Compute total variables (upper bound)
    long long totalVars = (long long)7 * K * N * M; // 7 blocks of size K*N*M
    long long numClauses = clauses.size();

    fout << "p cnf " << totalVars << " " << numClauses << "\n";
    for (auto &cl : clauses) {
        for (int lit : cl) fout << lit << " ";
        fout << "0\n";
    }

    fin.close();
    fout.close();
    return 0;
}

