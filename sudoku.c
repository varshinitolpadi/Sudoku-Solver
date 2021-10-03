#include <stdio.h>
#include <malloc.h>
#include <stdbool.h>

#include "graph.h"
bool array_contains(int *array, int count, int value)
{
	int i = 0;
	for (i = 0; i < count; i++)
		if (array[i] == value)
			return true;
	
	return false;
}


// determine is a vertex is constrained to only a single given color
bool vertex_must_be(struct vertex *vertex, int color)
{
	if (vertex->num_possible == 1 && vertex->current_value == color)
		return true;

	return false;
}


// determine whether a vertex's edges would disallow a given color to be chosen
bool edges_disallow_color(struct vertex *vertex, int color)
{
	bool in_edge = false;
	struct edge *last_edge = vertex->edges;

	while (last_edge != NULL) {
		if (last_edge->connects_to != NULL) {
			struct vertex *connected = last_edge->connects_to;
			in_edge = vertex_must_be(connected, color);
		}

		if (in_edge == true)
			return true;

		last_edge = last_edge->next;
	}

	return in_edge;
}


// See if a color is valid for a cell, not contained in any neighbors and not been removed
int is_valid_color(struct vertex *vertex, int *removed, int removed_count, int color)
{
	if (vertex->num_possible == 1 && vertex->current_value == color)
		return true;
	else
		if (!array_contains(removed, removed_count, color))
			return !edges_disallow_color(vertex, color);
	
	return false;
}


// Recursive backtracking algorithm
bool color_graph(struct vertex *vertex, int num_colors)
{
	if (vertex == NULL)
		return true;

	int removed_index = 0;
	int removed_colors[num_colors];
	
	int original_value = vertex->current_value;
	int original_num = vertex->num_possible;

	if (original_num == 1)
		return color_graph(vertex->next, num_colors);	

	int i = 0;
	for (i = 1; i <= original_num; i++) {
		if (!is_valid_color(vertex, removed_colors, removed_index, i))
			continue;

		vertex->current_value = i;
		vertex->num_possible = 1;

		bool success = color_graph(vertex->next, num_colors);

		if (success == false) {
			removed_colors[removed_index] = i;
			removed_index++;
			vertex->num_possible = original_num;
			vertex->current_value = original_value;
		}
		else {
			return true;
		}
	}

	return false;
}


// check if each of the vertices has only a single color possibility, meaning it's colored
bool graph_colored(struct graph *graph)
{
	struct vertex *next_vertex = graph->vertices;
	while (next_vertex != NULL) {
		if (next_vertex->num_possible > 1)
			return false;

		next_vertex = next_vertex->next;
	}

	return true;
}
struct graph *create_graph()
{
	struct graph *created =(struct graph *) malloc(sizeof(struct graph));
	created->vertices = NULL;
	created->vertex_index = 1;

	return created;
}


void free_graph(struct graph *graph)
{
	struct vertex *next_vertex = graph->vertices;

	while (next_vertex != NULL) {
		struct vertex *current = next_vertex;
		next_vertex = next_vertex->next;

		struct edge *next_edge = current->edges;
		while (next_edge != NULL) {
			struct edge *current_edge = next_edge;
			next_edge = next_edge->next;

			free(current_edge);
		}

		free(current);
	}

	free(graph);
}


struct vertex *add_vertex(struct graph *graph, int num_possible, int value)
{
	struct vertex *next = (struct vertex*)malloc(sizeof(struct vertex));
	next->index = graph->vertex_index++;
	next->current_value = value;
	next->num_possible = num_possible;
	next->edges = NULL;
	next->next = graph->vertices;
	graph->vertices = next;

	return next;
}


void add_edge(struct vertex *vertex1, struct vertex *vertex2)
{
	struct edge *next = (struct edge*)malloc(sizeof(struct edge));
	next->connects_to = vertex2;
	next->next = vertex1->edges;
	vertex1->edges = next;
}


void print_graph(struct graph *graph)
{
	int edge_count = 0;
	struct vertex *last_vertex = graph->vertices;
	
	while (last_vertex != NULL) {
		printf("Vertex %d[%d] connects to vertices: ", last_vertex->index, last_vertex->current_value);
		struct edge *last_edge = last_vertex->edges;

		while (last_edge != NULL) {
			if (last_edge->connects_to != NULL) {
				edge_count++;
				printf("%d[%d], ", last_edge->connects_to->index,
					last_edge->connects_to->current_value);
			}

			last_edge = last_edge->next;
		}

		printf("\n");

		last_vertex = last_vertex->next;
	}

	printf("Total edge count: %d\n", edge_count);
}


void print_sudoku(struct graph *graph, int size)
{
	int *output = (int*)malloc(sizeof(int) * (size * size));

	struct vertex *last_vertex = graph->vertices;

	int index = 0;
	while (last_vertex != NULL) {
		output[index] = last_vertex->current_value;
		
		last_vertex = last_vertex->next;
		index++;
	}

	index--;
	for (; index >= 0; index--)
		printf("%d", output[index]);

	printf("\n");

	free(output);
}

// first 3 3x3 groups, in a row
int initial_groups[3][9] = {
	{0,1,2, 9,10,11, 18,19,20},
	{27,28,29, 36,37,38, 45,46,47},
	{54,55,56, 63,64,65, 72,73,74}
};


// check if a given index is in a (usually) 9x9 group
bool index_in_group(int *group, int size, int index)
{
	int i = 0;
	for (i = 0; i < size; i++)
		if (group[i] == index)
			return true;
	
	return false;
}


// link a vertex at index with every other vertex in the group
void link_vertex_with_group(int *group, int size, int index, struct vertex **vertices)
{
	struct vertex *to_link = vertices[index];

	int i = 0;
	for (i = 0; i < size; i++)
		if (group[i] != index)
			add_edge(to_link, vertices[group[i]]);
}


// link 3x3 groups in the list of vertices
void link_three_groups(struct vertex **vertices)
{
	// link 3x3 groups from initial data
	int i = 0;
	int j = 0;
	int current_group[9];

	for (i = 0; i < 9; i++) {
		int factor = 0;
		if (i > 2 && i < 6)
			factor = 1;
		else if (i > 5)
			factor = 2;

		for (j = 0; j < 9; j++)
			current_group[j] = initial_groups[i % 3][j] + (factor * 3); 

		// search through vertex indices and link vertexes in a group together
		for (j = 0; j < 81; j++)
			if (index_in_group(current_group, 9, j))
				link_vertex_with_group(current_group, 9, j, vertices);

	}
}


// link rows and columns together
void link_cols_and_rows(struct vertex **vertices)
{
	int current_row[9];
	int current_col[9];
	int i = 0;
	int j = 0;
	for (i = 0; i < 9; i++) {
		for (j = 0; j < 9; j++) {
			current_row[j] = (i * 9) + j;
			current_col[j] = (j * 9) + i;
		}

		// search through vertex indices and link vertexes in a group together
		for (j = 0; j < 81; j++) 
			if (index_in_group(current_row, 9, j))
				link_vertex_with_group(current_row, 9, j, vertices);

		for (j = 0; j < 81; j++) 
			if (index_in_group(current_col, 9, j))
				link_vertex_with_group(current_col, 9, j, vertices);
	}
}

		
// create graph, load up vertices and link them together in the sudoku pattern
struct graph *load_initial()
{
	// load sudoku puzzle in from stdin
	int puzzle_size = 9;
	int num_chars = (puzzle_size * puzzle_size) + 1;
	int num_boxes = puzzle_size * puzzle_size;

	char *buffer = (char*)malloc(sizeof(char) * num_chars);
	fgets(buffer, num_chars, stdin);

	struct graph *graph = create_graph();
	struct vertex **vertices = (struct vertex**)malloc(sizeof(void *) * num_boxes);

	// set up the graph with the vertices
	int i = 0;
	for (i = 0; i < num_boxes; i++) { 
		// add all 9 color possibilities if we get a 0, which is an unfilled cell
		if (buffer[i] == '0') {
			vertices[i] = add_vertex(graph, 9, 0);
		}
		else {
			// add a single possibility if we get a pre-filled cell
			vertices[i] = add_vertex(graph, 1, buffer[i] - '0');
		}
	}

	link_three_groups(vertices);
	link_cols_and_rows(vertices);

	free(buffer);
	free(vertices);

	return graph;
}


int main(int argc, char **argv)
{
	printf("hello");
	struct graph *graph = load_initial();

	color_graph(graph->vertices, 9);

	if (graph_colored(graph))
		print_sudoku(graph, 9);
	else
		printf("Unable to color sudoku graph.\n");

	free_graph(graph);
	
	return 0;
}
