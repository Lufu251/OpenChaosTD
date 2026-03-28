#pragma once

#include <core/grid2d.hpp>
#include <vector>
#include <iostream>
#include <queue>
#include <limits>
#include <algorithm>

// Type Alias
using Weight = int;

struct Edge{
    std::pair<int, int> target;
    Weight weight;

    Edge() : target(0, 0), weight(0){}
    Edge(std::pair<int, int> t, Weight w) : target(t), weight(w) {}
};

class Graph{
private:
    Grid2D<std::vector<Edge>> adjacency;

public:
    Graph() = default;
    Graph(int xSize, int ySize) { adjacency.Resize(xSize, ySize); }

    bool InBounds(std::pair<int, int> target){
        return adjacency.InBounds(target.first, target.second);;
    }

    // Add adjacency
    void AddEdge(std::pair<int, int> target, std::pair<int, int> neighbor, Weight w = 1) {
        adjacency.InBounds(target.first, target.second);
        adjacency.InBounds(neighbor.first, neighbor.second);

        // Add edge from u to v
        adjacency.Get(target.first, target.second).emplace_back(neighbor, w);
    }

    // Set the number of nodes in the graph
    void Resize(int xSize, int ySize){
        adjacency.Resize(xSize, ySize);
    }

    // Get all adjacency from a node
    const std::vector<Edge>& GetNeighbors(std::pair<int, int> target) const {
        adjacency.InBounds(target.first, target.second);
        return adjacency.Get(target.first, target.second);
    }

    int GetWidth() const { return adjacency.GetWidth(); }
    int GetHeight() const { return adjacency.GetHeight(); }
    int GetSize() const { return adjacency.GetWidth() * adjacency.GetHeight(); }
};


struct Node{
    int distance = std::numeric_limits<Weight>::max();
    std::pair<int, int> predecessor = {-1, -1};
};

/*
    Breadth-First Search algorithm
    Shortest Path
    Unweighted edges
    Ignores weights
*/
struct Bfs{
    Grid2D<Node> mesh;

    void solve(std::pair<int, int> startNode, Graph& graph){
        if(!graph.InBounds(startNode)){
            std::cerr << "Bfs tried to solve from a out of bounds starting nodes" << std::endl;
        }

        // Erase mesh and resize. Reset results to the starting values
        mesh.GetVector().clear();
        mesh.Resize(graph.GetWidth(), graph.GetHeight());

        std::queue<std::pair<int, int>> q;

        mesh.Get(startNode.first, startNode.second).distance = 0;
        mesh.Get(startNode.first, startNode.second).predecessor = startNode;
        q.push(startNode);

        while(!q.empty()){
            std::pair<int, int> u = q.front();
            q.pop();

            for(const Edge& edge : graph.GetNeighbors(u)){
                std::pair<int, int> v = edge.target;
                if(mesh.Get(v.first, v.second).distance == std::numeric_limits<int>::max()){
                    mesh.Get(v.first, v.second).distance = mesh.Get(u.first, u.second).distance + 1;
                    mesh.Get(v.first, v.second).predecessor = u;
                    q.push(v);
                }
            }
        }
    }
};
