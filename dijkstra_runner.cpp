#include <iostream>
#include <fstream>
#include <limits>
#include "Graph.h"
#include "DijkstraSolver.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <graph_file> <output_file> <source_node>" << std::endl;
        return 1;
    }

    std::string graphPath = argv[1];
    std::string outputPath = argv[2];
    int source = std::stoi(argv[3]);

    try {
        Graph g(1);
        // Uses loadFromFile which expects "num_nodes" then "u v w"
        // Ensure fuzzing_oracle exports in this format
        g.loadFromFile(graphPath);

        DijkstraSolver solver(g);
        solver.compute(source);

        std::ofstream outfile(outputPath);
        if (!outfile.is_open()) {
            std::cerr << "Error opening output file: " << outputPath << std::endl;
            return 1;
        }

        for (int i = 0; i < g.numVertices; ++i) {
            int d = solver.getDistance(i);
            int outDist = (d == std::numeric_limits<int>::max()) ? -1 : d;
            outfile << i << " " << outDist << "\n";
        }
        outfile.close();

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
