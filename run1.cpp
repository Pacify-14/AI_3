#include <bits/stdc++.h>
using namespace std;

struct MetroLine {
    int sx, sy, ex, ey;
};

int varId(int k, int x, int y, int N, int M) {
    // Variables are numbered from 1 to K*N*M
    return k * N * M + y * N + x + 1;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: ./run1 <input.city> <output.satinput>\n";
        return 1;
    }

    ifstream fin(argv[1]);
    ofstream fout(argv[2]);
    if (!fin.is_open()) { cerr << "Cannot open input file\n"; return 1; }

    int scenario;
    fin >> scenario;

    int N, M, K, J, P = 0;
    fin >> N >> M >> K >> J;
    if (scenario == 2) fin >> P;

    vector<MetroLine> lines(K);
    for (int i = 0; i < K; ++i)
        fin >> lines[i].sx >> lines[i].sy >> lines[i].ex >> lines[i].ey;

    vector<pair<int,int>> popular;
    if (scenario == 2) {
        for (int i = 0; i < P; ++i) {
            int x, y; fin >> x >> y;
            popular.push_back({x,y});
        }
    }

    vector<vector<int>> clauses;

    // --- Constraint 1: At most one metro per cell ---
    for (int x = 0; x < N; ++x)
        for (int y = 0; y < M; ++y)
            for (int a = 0; a < K; ++a)
                for (int b = a + 1; b < K; ++b)
                    clauses.push_back({-varId(a,x,y,N,M), -varId(b,x,y,N,M)});

    // --- Constraint 2: Start and End cells must be used ---
    for (int k = 0; k < K; ++k) {
        clauses.push_back({ varId(k, lines[k].sx, lines[k].sy, N, M) });
        clauses.push_back({ varId(k, lines[k].ex, lines[k].ey, N, M) });
    }

    // --- (Later) Constraint 3: Path connectivity ---
    // --- (Later) Constraint 4: Turn limit ---
    // --- (Later) Constraint 5: Popular cells ---

    int numVars = K * N * M;
    int numClauses = clauses.size();

    fout << "p cnf " << numVars << " " << numClauses << "\n";
    for (auto &cl : clauses) {
        for (int lit : cl) fout << lit << " ";
        fout << "0\n";
    }

    fin.close();
    fout.close();
    return 0;
}

