#include <bits/stdc++.h>
using namespace std;

struct MetroLine {
    int sx, sy, ex, ey;
};

int varId(int k, int x, int y, int N, int M) {
    return k * N * M + y * N + x + 1;
}

int Eid(int k, int x, int y, int d, int N, int M, int K) {
    return K * N * M + k * (N * M * 4) + (y * N + x) * 4 + d + 1;
}

int Rid(int k, int x, int y, int N, int M, int K) {
    return K * N * M + K * N * M * 4 + k * N * M + y * N + x + 1;
}

int Tid(int k, int x, int y, int N, int M, int K) {
    return K * N * M + K * N * M * 4 + K * N * M + k * N * M + y * N + x + 1;
}

bool inBounds(int x, int y, int N, int M) {
    return x >= 0 && x < N && y >= 0 && y < M;
}

int main(int argc, char* argv[]) {
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
    if (scenario == 2) fin >> P;

    vector<MetroLine> lines(K);
    for (int i = 0; i < K; ++i)
        fin >> lines[i].sx >> lines[i].sy >> lines[i].ex >> lines[i].ey;

    vector<pair<int,int>> popular(P);
    if (scenario == 2) {
        for (int i = 0; i < P; ++i) {
            int x, y; fin >> x >> y;
            popular[i] = {x, y};
        }
    }

    vector<vector<int>> clauses;

    int dx[4] = {1, 0, -1, 0}; // 0:R, 1:D, 2:L, 3:U
    int dy[4] = {0, 1, 0, -1};

    // Constraint 1: At most one metro per cell
    for (int x = 0; x < N; ++x) {
        for (int y = 0; y < M; ++y) {
            for (int a = 0; a < K; ++a) {
                for (int b = a + 1; b < K; ++b) {
                    clauses.push_back({-varId(a, x, y, N, M), -varId(b, x, y, N, M)});
                }
            }
        }
    }

    // Constraint 2: Start and end occupied
    for (int k = 0; k < K; ++k) {
        clauses.push_back({varId(k, lines[k].sx, lines[k].sy, N, M)});
        clauses.push_back({varId(k, lines[k].ex, lines[k].ey, N, M)});
    }

    // Path, connectivity, turns
    for (int k = 0; k < K; ++k) {
        int sx = lines[k].sx, sy = lines[k].sy;
        int ex = lines[k].ex, ey = lines[k].ey;

        // Edges and degrees
        for (int x = 0; x < N; ++x) {
            for (int y = 0; y < M; ++y) {
                bool is_start = (x == sx && y == sy);
                bool is_end = (x == ex && y == ey);

                // Outgoing edges
                vector<int> out_list;
                for (int d = 0; d < 4; ++d) {
                    int nx = x + dx[d];
                    int ny = y + dy[d];
                    if (inBounds(nx, ny, N, M)) {
                        int e = Eid(k, x, y, d, N, M, K);
                        out_list.push_back(e);
                        clauses.push_back({-e, varId(k, x, y, N, M)});
                        clauses.push_back({-e, varId(k, nx, ny, N, M)});
                    }
                }

                if (is_end) {
                    for (int e : out_list) clauses.push_back({-e});
                } else {
                    if (!out_list.empty()) {
                        clauses.push_back(out_list); // ALO
                        for (size_t i = 0; i < out_list.size(); ++i) {
                            for (size_t j = i + 1; j < out_list.size(); ++j) {
                                clauses.push_back({-out_list[i], -out_list[j]}); // AMO
                            }
                        }
                    }
                }

                // Incoming edges
                vector<int> in_list;
                for (int d = 0; d < 4; ++d) {
                    int px = x - dx[d];
                    int py = y - dy[d];
                    if (inBounds(px, py, N, M)) {
                        int e = Eid(k, px, py, d, N, M, K);
                        in_list.push_back(e);
                    }
                }

                if (is_start) {
                    for (int e : in_list) clauses.push_back({-e});
                } else {
                    if (!in_list.empty()) {
                        clauses.push_back(in_list); // ALO
                        for (size_t i = 0; i < in_list.size(); ++i) {
                            for (size_t j = i + 1; j < in_list.size(); ++j) {
                                clauses.push_back({-in_list[i], -in_list[j]}); // AMO
                            }
                        }
                    }
                }
            }
        }

        // Reachability
        clauses.push_back({Rid(k, sx, sy, N, M, K)});
        for (int x = 0; x < N; ++x) {
            for (int y = 0; y < M; ++y) {
                int r = Rid(k, x, y, N, M, K);
                int v = varId(k, x, y, N, M);
                clauses.push_back({-r, v});
                clauses.push_back({-v, r});
                for (int d = 0; d < 4; ++d) {
                    int nx = x + dx[d];
                    int ny = y + dy[d];
                    if (inBounds(nx, ny, N, M)) {
                        int e = Eid(k, x, y, d, N, M, K);
                        int nr = Rid(k, nx, ny, N, M, K);
                        clauses.push_back({-r, -e, nr});
                    }
                }
            }
        }

        // Force end is reached
        clauses.push_back({Rid(k, ex, ey, N, M, K)});

        // Turns
        vector<int> turn_vars;
        for (int x = 0; x < N; ++x) {
            for (int y = 0; y < M; ++y) {
                if ( (x == sx && y == sy) || (x == ex && y == ey) ) continue;
                int t = Tid(k, x, y, N, M, K);
                turn_vars.push_back(t);
                // Find possible in and out
                vector<pair<int, int>> in_e_d;
                for (int d = 0; d < 4; ++d) {
                    int px = x - dx[d];
                    int py = y - dy[d];
                    if (inBounds(px, py, N, M)) {
                        int e = Eid(k, px, py, d, N, M, K);
                        in_e_d.push_back({e, d});
                    }
                }

                vector<pair<int, int>> out_e_d;
                for (int d = 0; d < 4; ++d) {
                    int nx = x + dx[d];
                    int ny = y + dy[d];
                    if (inBounds(nx, ny, N, M)) {
                        int e = Eid(k, x, y, d, N, M, K);
                        out_e_d.push_back({e, d});
                    }
                }

                for (auto& ie : in_e_d) {
                    int ieid = ie.first;
                    int idir = ie.second;
                    for (auto& oe : out_e_d) {
                        int oeid = oe.first;
                        int odir = oe.second;
                        if (idir == odir) {
                            clauses.push_back({-ieid, -oeid, -t});
                        } else {
                            clauses.push_back({-ieid, -oeid, t});
                        }
                    }
                }
            }
        }

        // Sequential encoding for sum(turn_vars) <= J
        int n_turn = turn_vars.size();
        if (n_turn > 0) {
            int max_j = J + 1;
            int aux_base = Tid(k, N-1, M-1, N, M, K) + 1;
            auto sid = [&](int i, int j) {
                return aux_base + (i - 1) * (max_j + 1) + (j - 1);
            };

            // i=1
            if (n_turn >= 1) {
                int x1 = turn_vars[0];
                clauses.push_back({-sid(1,1), x1}); // s1,1 => x1
                clauses.push_back({-x1, sid(1,1)}); // x1 => s1,1
                for (int j=2; j<=max_j; ++j) {
                    clauses.push_back({-sid(1,j)}); // ~s1,j
                }
            }

            // i=2 to n_turn
            for (int i=2; i<=n_turn; ++i) {
                int xi = turn_vars[i-1];
                // j=1
                int sij = sid(i,1);
                int sim1j = sid(i-1,1);
                clauses.push_back({-sij, sim1j, xi}); // sij => sim1j v xi
                clauses.push_back({-sim1j, sij}); // sim1j => sij
                clauses.push_back({-xi, sij}); // xi => sij

                // j=2 to max_j
                for (int j=2; j<=max_j; ++j) {
                    sij = sid(i,j);
                    int sim1j = sid(i-1,j);
                    int sim1jm1 = sid(i-1,j-1);
                    clauses.push_back({-sij, sim1j, sim1jm1}); // sij => sim1j v sim1jm1
                    clauses.push_back({-sij, sim1j, xi}); // sij => sim1j v xi
                    clauses.push_back({-sim1j, sij}); // sim1j => sij
                    clauses.push_back({-sim1jm1, -xi, sij}); // sim1jm1 ^ xi => sij
                }

                // Overflow
                int sim1_max = sid(i-1, max_j);
                clauses.push_back({-sim1_max, -xi}); // sim1_max ^ xi => false
            }

            // sum <= J
            clauses.push_back({-sid(n_turn, max_j)});
        }
    }

    // Scenario 2: Popular cells
    if (scenario == 2) {
        for (auto& cell : popular) {
            int x = cell.first, y = cell.second;
            vector<int> ors;
            for (int k = 0; k < K; ++k) {
                ors.push_back(varId(k, x, y, N, M));
            }
            clauses.push_back(ors);
        }
    }

    // Total vars upper bound
    int numVars = K * N * M * (1 + 4 + 1 + 1) + K * (N * M * (J + 2));
    int numClauses = clauses.size();
    fout << "p cnf " << numVars << " " << numClauses << "\n";
    for (auto& cl : clauses) {
        for (int lit : cl) fout << lit << " ";
        fout << "0\n";
    }

    fin.close();
    fout.close();
    return 0;
}
