// LimitedRandomWalk.cpp : Defines the entry point for the console application.
//
#include <Windows.h>
#include <fstream>
#include <iostream>                  // for std::cout
#include <utility>                   // for std::pair
#include <algorithm>                 // for std::for_each
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graphviz.hpp>
using namespace boost;

// Global typedefs
typedef adjacency_list<vecS, vecS, undirectedS> Graph;

// PATH constants. Change as needed
const std::string PATH_TO_GRAPHVIZ_EXECUTABLE = "C:\\Program Files (x86)\\Graphviz2.38\\bin\\fdp.exe";
const std::string PATH_TO_GRAPH_DOT_FILE =
	"C:\\Users\\irtez\\Desktop\\Projects\\Macaulay Senior Thesis\\LimitedRandomWalk\\LimitedRandomWalk\\DOT_Graph_Test.dot";
const std::string PATH_TO_GRAPH_IMAGE_OUTPUT =
	"C:\\Users\\irtez\\Desktop\\Projects\\Macaulay Senior Thesis\\LimitedRandomWalk\\LimitedRandomWalk";

// Function prototypes

VOID generateGraphImage(Graph g, typename std::string outputFileName);
std::wstring s2ws(const std::string& s);

int main(int, char*[])
{
	// create a typedef for the Graph type

	// Make convenient labels for the vertices
	enum { A, B, C, D, E, N };
	const int num_vertices = N;
	const char* name = "ABCDE";

	// writing out the edges in the graph
	typedef std::pair<int, int> Edge;
	Edge edge_array[] =
	{ Edge(A,B), Edge(A,D), Edge(C,A), Edge(D,C),
		Edge(C,E), Edge(B,D), Edge(D,E) };
	const int num_edges = sizeof(edge_array) / sizeof(edge_array[0]);

	// declare a graph object
	Graph g(num_vertices);

	// add the edges to the graph object
	for (int i = 0; i < num_edges; ++i)
		add_edge(edge_array[i].first, edge_array[i].second, g);

	typedef graph_traits<Graph>::vertex_descriptor Vertex;

	// get the property map for vertex indices
	typedef property_map<Graph, vertex_index_t>::type IndexMap;
	IndexMap index = get(vertex_index, g);

	std::cout << "vertices(g) = ";
	typedef graph_traits<Graph>::vertex_iterator vertex_iter;
	std::pair<vertex_iter, vertex_iter> vp;
	for (vp = vertices(g); vp.first != vp.second; ++vp.first) {
		Vertex v = *vp.first;
		std::cout << index[v] << " ";
	}
	std::cout << std::endl;

	std::cout << "edges(g) = ";
	graph_traits<Graph>::edge_iterator ei, ei_end;
	for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
		std::cout << "(" << index[source(*ei, g)]
		<< "," << index[target(*ei, g)] << ") ";
	std::cout << std::endl;

	std::fstream ofs;
	ofs.open("DOT_Graph_Test.dot", std::basic_ios<char, std::char_traits<char> >::trunc 
		| std::basic_ios<char, std::char_traits<char> >::out);
	write_graphviz(ofs, g);
	ofs.close();

	generateGraphImage(g, "graphImage.png");

	return 0;
}

// This function calls a new thread to run a GraphViz executable to generate an image of the graph
VOID generateGraphImage(Graph g, typename std::string outputFileName = "graphOutput.png")
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