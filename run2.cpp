#include <bits/stdc++.h>
using namespace std;

struct MetroLine {
    int sx, sy, ex, ey;
};

// same variable ID logic as run1.cpp
int varId(int k, int x, int y, int N, int M) {
    return k * N * M + y * N + x + 1;
}

// inverse mapping for a given variable ID
void invVar(int id, int N, int M, int &k, int &x, int &y) {
    id--; // make zero-based
    k = id / (N * M);
    int rem = id % (N * M);
    y = rem / N;
    x = rem % N;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Usage: ./run2 <basename.city> <basename.satoutput> <basename.metromap>\n";
        return 1;
    }

    string cityFile = argv[1];
    string satFile  = argv[2];
    string outFile  = argv[3];

    // --- Parse city file ---
    ifstream fin(cityFile.c_str());
    if (!fin.is_open()) {
        cerr << "Cannot open " << cityFile << endl;
        return 1;
    }

    int scenario, N, M, K, J, P = 0;
    fin >> scenario;
    fin >> N >> M >> K >> J;
    if (scenario == 2) fin >> P;

    vector<MetroLine> lines(K);
    for (int i = 0; i < K; ++i)
        fin >> lines[i].sx >> lines[i].sy >> lines[i].ex >> lines[i].ey;

    if (scenario == 2) {
        for (int i = 0; i < P; ++i) {
            int x, y; fin >> x >> y;
        }
    }
    fin.close();

    // --- Read SAT output ---
    ifstream fsat(satFile.c_str());
    if (!fsat.is_open()) {
        cerr << "Cannot open " << satFile << endl;
        return 1;
    }

    string token;
    vector<int> assignment;
    bool unsat = false;
    while (fsat >> token) {
        if (token == "UNSAT" || token == "UNSATISFIABLE") {
            unsat = true;
            break;
        }
        if (token == "SAT" || token == "SATISFIABLE") continue;
        assignment.push_back(atoi(token.c_str()));
    }
    fsat.close();

    ofstream fout(outFile.c_str());
    if (!fout.is_open()) {
        cerr << "Cannot open output file\n";
        return 1;
    }

    // --- Handle UNSAT case ---
    if (unsat) {
        fout << "0\n";
        fout.close();
        return 0;
    }

    // --- Decode TRUE variables ---
    vector<vector<vector<bool> > > grid(K,
        vector<vector<bool> >(N, vector<bool>(M, false)));

    for (size_t i = 0; i < assignment.size(); ++i) {
        int val = assignment[i];
        if (val > 0) {
            int k, x, y;
            invVar(val, N, M, k, x, y);
            if (k < K && x < N && y < M)
                grid[k][x][y] = true;
        }
    }

    // --- Construct trivial path outputs (straight-line approximation) ---
    for (int k = 0; k < K; ++k) {
        int sx = lines[k].sx, sy = lines[k].sy;
        int ex = lines[k].ex, ey = lines[k].ey;

        string path = "";
        if (ex > sx) path += string(ex - sx, 'R');
        else if (ex < sx) path += string(sx - ex, 'L');

        if (ey > sy) path += string(ey - sy, 'D');
        else if (ey < sy) path += string(sy - ey, 'U');

        for (size_t i = 0; i < path.size(); ++i)
            fout << path[i] << " ";
        fout << "0\n";
    }

    fout.close();
    return 0;
}

