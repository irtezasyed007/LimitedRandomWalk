// LimitedRandomWalk.cpp : Defines the entry point for the console application.
//
#include <Windows.h>
#include <fstream>
#include <iostream>                  // for std::cout
#include <utility>                   // for std::pair
#include <algorithm>                 // for std::for_each
#include <memory>					 // for std::unique_ptr
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <cmath>					 // for std::pow
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graphviz.hpp>
#include <libs/graph/src/read_graphviz_new.cpp>
#include <boost/tuple/tuple.hpp>
#include <boost/property_map/dynamic_property_map.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/copy.hpp>
#include <Eigen/Core>
#include <Eigen/LU>

using namespace boost;

// define Vertex and Edge props
struct VertexProps {
	std::string name, color;
	Eigen::VectorXd featureVector;
};

struct EdgeProps {
	std::string label, color;
	double weight;
};

struct GraphProps {
	std::string splines, overlap;
};

// Define list graphs, matrix graphs, and other important typedefs
typedef adjacency_list<vecS, vecS, undirectedS, VertexProps, EdgeProps, GraphProps> ListGraph;
typedef adjacency_matrix<undirectedS, VertexProps, EdgeProps, GraphProps> MatrixGraph;
typedef graph_traits<ListGraph>::edge_iterator list_edge_iterator;
typedef graph_traits<ListGraph>::vertex_iterator list_vertex_iterator;

// PATH constants. Change as needed
const std::string PATH_TO_GRAPHVIZ_EXECUTABLE = "C:\\Program Files (x86)\\Graphviz2.38\\bin\\fdp.exe";
const std::string PATH_TO_GRAPH_DOT_FILE =
	"C:\\Users\\irtez\\Desktop\\Projects\\Macaulay Senior Thesis\\LimitedRandomWalk\\LimitedRandomWalk\\DOT_Input.dot";
const std::string PATH_TO_GRAPH_IMAGE_OUTPUT =
	"C:\\Users\\irtez\\Desktop\\Projects\\Macaulay Senior Thesis\\LimitedRandomWalk\\LimitedRandomWalk";

// Function prototypes

VOID generateGraphImage(ListGraph g, typename std::string outputFileName);
std::wstring s2ws(const std::string& s);
Eigen::MatrixXd getAdjacencyMatrix(ListGraph lg);
Eigen::MatrixXd getDegreeMatrix(ListGraph lg, int numTimesDegreesAreDefined = 1);

std::vector<Eigen::VectorXd> LRW(
	ListGraph lg,
	Eigen::MatrixXd adjMatrix,
	double inflationExponent = 2,
	int maxIterations = 10000,
	double toleranceEpsilon = 0.001,
	double convergenceDelta = 0.001,
	double thresholdTau = 0.5
);

Eigen::VectorXd inflation(Eigen::VectorXd vec, double exponent = 2.0);
Eigen::VectorXd normalization(Eigen::VectorXd vec);

// Edge and Vertex Property Structs

int main(int, char*[])
{

	ListGraph lg(0);
	dynamic_properties ldp(ignore_other_properties);
	ldp.property("node_id", get(&VertexProps::name, lg));
	ldp.property("color", get(&VertexProps::color, lg));
	ldp.property("color", get(&EdgeProps::color, lg));
	
	// Graph properties not attached to vertices or edges
	boost::ref_property_map<ListGraph *, std::string> lgSplines(get_property(lg, &GraphProps::splines));
	boost::ref_property_map<ListGraph *, std::string> lgOverlap(get_property(lg, &GraphProps::overlap));
	ldp.property("splines", lgSplines);
	ldp.property("overlap", lgOverlap);

	std::ifstream ifs("DOT_Input.dot");
	try {
		if (read_graphviz(ifs, lg, ldp)) {
			ifs.close();


			std::pair<list_vertex_iterator, list_vertex_iterator> vi;
			ListGraph::vertex_descriptor vd;
			for (size_t i = 0; i < num_vertices(lg); i++) {
				lg[i].color = "red";
			}

			list_edge_iterator iter, end;
			for (tie(iter, end) = edges(lg); iter != end; ++iter) {
				lg[*iter].color = "red";
			}

			for (vi = vertices(lg); vi.first != vi.second; ++vi.first) {
				std::cout << lg[*vi.first].name << ' ';
			}
			std::cout << std::endl;

			Eigen::MatrixXd adjMatrix = getAdjacencyMatrix(lg);
			Eigen::MatrixXd degMatrix = getDegreeMatrix(lg, 2);

			std::cout << adjMatrix << std::endl << std::endl << degMatrix << std::endl;
			auto featureVectors = LRW(lg, adjMatrix);


			std::ofstream ofs("DOT_Input.dot");
			ofs << "strict ";	// This is done to remove duplicate edges from showing up in GraphViz interpreter
			write_graphviz_dp(ofs, lg, ldp);
			generateGraphImage(lg, "graphImage.png");
		}
	}
	catch(bad_graphviz_syntax er){
		std::cout << er.errmsg;
	}


	return 0;
}

// This function calls a new thread to run a GraphViz executable to generate an image of the graph
VOID generateGraphImage(ListGraph g, typename std::string outputFileName = "graphOutput.png")
{
	// additional information
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// set the size of the structures
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));


	// Converting the cmd arguments to the right type
	std::string argsString = "\"" + PATH_TO_GRAPHVIZ_EXECUTABLE + "\" -Tpng \"" + PATH_TO_GRAPH_DOT_FILE + "\" -o \"" 
		+ PATH_TO_GRAPH_IMAGE_OUTPUT + "\\" + outputFileName + "\"";
	std::wstring wstringArgs = std::wstring(argsString.begin(), argsString.end());
	
	const wchar_t temp = *wstringArgs.c_str();
	LPTSTR args = const_cast<LPTSTR>(wstringArgs.c_str());


	// start the program up
	CreateProcess(NULL,   // the path
		args,        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
	);
	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

// Conversion function that goes from string to unicode-friendly wstring in a way that allows conversion to LCPWSTR... yeaah
// StackOverflow reference and source: https://stackoverflow.com/questions/27220/how-to-convert-stdstring-to-lpcwstr-in-c-unicode
std::wstring s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}

Eigen::MatrixXd getAdjacencyMatrix(const ListGraph lg) {
	int N = num_vertices(lg);
	Eigen::MatrixXd adjMatrix(N, N);
	adjMatrix.setZero();

	std::pair<list_edge_iterator, list_edge_iterator> ei = edges(lg);

	for (list_edge_iterator iter = ei.first; iter != ei.second; iter++) {
		int a = source(*iter, lg);
		int b = target(*iter, lg);
		adjMatrix(a, b) = 1;
		adjMatrix(b, a) = 1;
	}
	return adjMatrix;
}

Eigen::MatrixXd getDegreeMatrix(ListGraph lg, int numTimesDegreesAreDefined) {
	int N = num_vertices(lg);
	Eigen::MatrixXd degMatrix(N, N);
	degMatrix.setZero();

	auto i = lg.m_vertices;
	auto index = 0;
	for (auto i = lg.m_vertices.begin(); i != lg.m_vertices.end(); i++) {
		index = i - lg.m_vertices.begin();
		
		// Assumes edges are defined once. If defined twice per vertex, set optional parameter numTimesDegreesAreDefined to 2
		degMatrix(index, index) = lg.m_vertices[index].m_out_edges.size() / numTimesDegreesAreDefined;	
	}
	return degMatrix;
}

std::vector<Eigen::VectorXd> LRW(
	ListGraph lg,
	Eigen::MatrixXd adjMatrix,
	double inflationExponent,
	int maxIterations,
	double toleranceEpsilon,
	double convergenceDelta,
	double thresholdTau
){
	list_vertex_iterator iter, end;
	
	/*for (tie(iter, end) = vertices(lg); iter != end; iter++) {
		*iter
	}*/

	int N = num_vertices(lg);
	Eigen::MatrixXd degMatrix = getDegreeMatrix(lg, 2);
	Eigen::MatrixXd transitionMatrix =
		(Eigen::MatrixXd::Identity(N, N) + adjMatrix) * (Eigen::MatrixXd::Identity(N, N) + degMatrix).inverse();
	std::cout << transitionMatrix << std::endl;

	// Graph Exploring Phase, where feature vectors are generated
	std::vector<Eigen::VectorXd> featureVectors(N);
	Eigen::VectorXd probabilityVector, prevProbabilityVector;
	int counter;
	for (auto vertex = 0; vertex < num_vertices(lg); vertex++) {
		probabilityVector = Eigen::VectorXd::Zero(N);
		prevProbabilityVector = probabilityVector;
		probabilityVector(vertex) = 1;
		counter = 1;

		while (counter++ < maxIterations && (probabilityVector - prevProbabilityVector).norm() >= convergenceDelta) {
			prevProbabilityVector = probabilityVector;
			probabilityVector = transitionMatrix * prevProbabilityVector;
			for (auto i = 0; i < probabilityVector.size(); i++) {
				if (probabilityVector(i) < toleranceEpsilon) {
					probabilityVector(i) = 0;
				}
			}
			//std::cout << probabilityVector << "\n\n";
			probabilityVector = inflation(probabilityVector);
			//std::cout << probabilityVector << "\n\n";
			probabilityVector = normalization(probabilityVector);
		}
		featureVectors[vertex] = probabilityVector;
		std::cout << probabilityVector << "\n\n";
		std::cout << probabilityVector.sum() << "\n\n";
	}

	// Cluster Merging Phase, where clusters are formed by using the feature vectors
	double max;
	auto vertexIndex = 0;
	std::unique_ptr<std::unordered_set<int>[]> vecSet(new std::unordered_set<int>[N]);
	
	for (auto i = 0; i < featureVectors.size(); i++) {
		max = featureVectors[i].maxCoeff();
		vecSet[i].insert(i);
	}

	return featureVectors;
}

Eigen::VectorXd inflation(Eigen::VectorXd vec, double exponent) {
	for (auto i = 0; i < vec.size(); i++) {
		vec(i) = std::pow(vec(i), exponent);
	}
	return vec;
}

Eigen::VectorXd normalization(Eigen::VectorXd vec) {
	Eigen::RowVectorXd oneVector = Eigen::RowVectorXd::Ones(vec.size());
	/*std::cout << oneVector << "\n\n";
	std::cout << (oneVector * vec) << "\n\n";
	std::cout << (vec * oneVector)(0) << "\n\n";*/
	vec = vec / (oneVector * vec)(0);
	return vec;
}


//#include <boost/graph/adjacency_list.hpp>
//#include <boost/graph/graphviz.hpp>
//#include <boost/property_map/dynamic_property_map.hpp>
//#include <libs/graph/src/read_graphviz_new.cpp>
//#include <boost/graph/graph_utility.hpp>
//
//using namespace boost;
//
//struct Vertex {
//	std::string name, label, shape;
//};
//
//struct Edge {
//	std::string label;
//	double weight; // perhaps you need this later as well, just an example
//};
//
//typedef property<graph_name_t, std::string> graph_p;
//typedef adjacency_list<vecS, vecS, directedS, Vertex, Edge, graph_p> graph_t;
//
//int main() {
//	// Construct an empty graph and prepare the dynamic_property_maps.
//	graph_t graph(0);
//
//	dynamic_properties dp(ignore_other_properties);
//	dp.property("node_id", get(&Vertex::name, graph));
//	dp.property("label", get(&Vertex::label, graph));
//	dp.property("shape", get(&Vertex::shape, graph));
//	//dp.property("label", get(&Edge::label, graph));
//
//	// Use ref_property_map to turn a graph property into a property map
//	boost::ref_property_map<graph_t *, std::string> gname(get_property(graph, graph_name));
//	dp.property("name", gname);
//
//	std::ifstream dot("DOT_Graph_Test.dot");
//
//	if (read_graphviz(dot, graph, dp/*, "node_id"*/)) {
//		std::cout << "Graph name: '" << get_property(graph, graph_name) << "'\n";
//		get_property(graph, graph_name) = "Let's give it a name";
//		dot.close();
//		write_graphviz_dp(std::cout, graph, dp/*, "node_id"*/);
//	}
//	return 0;
//}
