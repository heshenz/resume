#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "ai.h"
#include "gate.h"
#include "radix.h"
#include "utils.h"

#define DEBUG 0

#define UP 'u'
#define DOWN 'd'
#define LEFT 'l'
#define RIGHT 'r'
char directions[] = {UP, DOWN, LEFT, RIGHT};
char invertedDirections[] = {DOWN, UP, RIGHT, LEFT};
char pieceNames[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

static int solver_algorithm = 3;

void set_solver_algorithm(int algorithm) {
	if (algorithm >= 1 && algorithm <= 3) {
		solver_algorithm = algorithm;
	}
}

static bool is_valid_direction(char direction) {
	return direction == UP || direction == DOWN || direction == LEFT || direction == RIGHT;
}

static bool find_piece_coordinates(gate_t *state, char pieceChar) {
	int pieceIdx = pieceChar - '0';
	if (pieceIdx < 0 || pieceIdx >= MAX_PIECES) {
		return false;
	}

	state->piece_x[pieceIdx] = -1;
	state->piece_y[pieceIdx] = -1;
	for (int i = 0; i < state->lines; i++) {
		for (int j = 0; state->map[i][j] != '\0'; j++) {
			*state = check_if_piece(*state, i, j, pieceChar);
			if (state->piece_x[pieceIdx] != -1) {
				return true;
			}
		}
	}
	return false;
}

static bool apply_move_in_place(gate_t *state, char pieceChar, char direction) {
	if (!state || !is_valid_direction(direction)) {
		return false;
	}

	int pieceIdx = pieceChar - '0';
	if (pieceIdx < 0 || pieceIdx >= state->num_pieces) {
		return false;
	}

	char letterPiece = pieceChar - '0' + 'H';

	if (!find_piece_coordinates(state, pieceChar)) {
		return false;
	}

	bool can_move = true;
	for (int i = 0; i < state->lines && can_move; i++) {
		for (int j = 0; state->map[i][j] != '\0'; j++) {
			char cell = state->map[i][j];
			if ((cell == pieceChar || cell == letterPiece) && !part_can_move(*state, i, j, direction)) {
				can_move = false;
				break;
			}
		}
	}

	if (!can_move) {
		return false;
	}

	for (int i = 0; i < state->lines; i++) {
		strcpy(state->map_save[i], state->map[i]);
	}

	for (int i = 0; i < state->lines; i++) {
		for (int j = 0; state->map[i][j] != '\0'; j++) {
			char savedCell = state->map_save[i][j];
			if (savedCell == pieceChar || savedCell == letterPiece) {
				int targetY = i;
				int targetX = j;
				if (direction == UP) {
					targetY = i - 1;
				} else if (direction == DOWN) {
					targetY = i + 1;
				} else if (direction == LEFT) {
					targetX = j - 1;
				} else if (direction == RIGHT) {
					targetX = j + 1;
				}

				char destinationCell = state->map_save[targetY][targetX];
				if (destinationCell == 'G' || destinationCell == letterPiece) {
					state->map[targetY][targetX] = letterPiece;
				} else {
					state->map[targetY][targetX] = pieceChar;
				}

				bool occupiedAfterMove = false;
				if (direction == UP && (state->map_save[i + 1][j] == pieceChar || state->map_save[i + 1][j] == letterPiece)) {
					occupiedAfterMove = true;
				}
				if (direction == DOWN && (state->map_save[i - 1][j] == pieceChar || state->map_save[i - 1][j] == letterPiece)) {
					occupiedAfterMove = true;
				}
				if (direction == LEFT && (state->map_save[i][j + 1] == pieceChar || state->map_save[i][j + 1] == letterPiece)) {
					occupiedAfterMove = true;
				}
				if (direction == RIGHT && (state->map_save[i][j - 1] == pieceChar || state->map_save[i][j - 1] == letterPiece)) {
					occupiedAfterMove = true;
				}

				if (savedCell == letterPiece && !occupiedAfterMove) {
					state->map[i][j] = 'G';
				} else if (savedCell == pieceChar && !occupiedAfterMove) {
					state->map[i][j] = ' ';
				}
			}
		}
	}

	find_piece_coordinates(state, pieceChar);
	return true;
}

// Node structure for priority queue
typedef struct search_node {
    gate_t* state;
    struct search_node* parent;
    int depth;
    int priority;
    char piece;
    char direction;
} search_node_t;

// Priority queue structure (binary min-heap on node priority)
typedef struct priority_queue {
	search_node_t **nodes;
	int size;
	int capacity;
} priority_queue_t;

typedef struct {
	bool solved;
	char *solution;
	gate_t *final_state;
	int expanded;
	int generated;
	int duplicated;
} search_run_result_t;

static bool next_combination(int *indices, int size, int totalPieces);
static void pack_subset(unsigned char *dest, int destBytes, const unsigned char *src, int atomBits,
	const int *indices, int count);
static bool all_combinations_present(struct radixTree *tree, const unsigned char *packedMap, int numPieces,
	int size, int atomBits, unsigned char *buffer, int bufferBytes);
static void insert_all_combinations(struct radixTree *tree, const unsigned char *packedMap, int numPieces,
	int size, int atomBits, unsigned char *buffer, int bufferBytes);

// Forward declarations
gate_t* duplicate_state(gate_t* gate);
void free_state(gate_t* stateToFree, gate_t *init_data);
void free_initial_state(gate_t *init_data);
static void run_search(gate_t *init_data, int width_limit, int packedBytes, search_run_result_t *result);
static void free_search_node(search_node_t* node);

/**
 * Priority queue functions for Uniform Cost Search
 */

#define PQ_INITIAL_CAPACITY 64

static void pq_swap(search_node_t **a, search_node_t **b) {
	search_node_t *tmp = *a;
	*a = *b;
	*b = tmp;
}

static void pq_bubble_up(priority_queue_t *pq, int idx) {
	while (idx > 0) {
		int parent = (idx - 1) / 2;
		if (pq->nodes[idx]->priority >= pq->nodes[parent]->priority) {
			break;
		}
		pq_swap(&pq->nodes[idx], &pq->nodes[parent]);
		idx = parent;
	}
}

static void pq_bubble_down(priority_queue_t *pq, int idx) {
	while (true) {
		int left = idx * 2 + 1;
		int right = left + 1;
		int smallest = idx;
		if (left < pq->size && pq->nodes[left]->priority < pq->nodes[smallest]->priority) {
			smallest = left;
		}
		if (right < pq->size && pq->nodes[right]->priority < pq->nodes[smallest]->priority) {
			smallest = right;
		}
		if (smallest == idx) {
			break;
		}
		pq_swap(&pq->nodes[idx], &pq->nodes[smallest]);
		idx = smallest;
	}
}

// Initialize priority queue
priority_queue_t* init_priority_queue() {
	priority_queue_t* pq = (priority_queue_t*)malloc(sizeof(priority_queue_t));
	if (!pq) {
		return NULL;
	}
	pq->capacity = PQ_INITIAL_CAPACITY;
	pq->size = 0;
	pq->nodes = (search_node_t **)malloc(sizeof(search_node_t*) * pq->capacity);
	if (!pq->nodes) {
		free(pq);
		return NULL;
	}
	return pq;
}

// Create a new search node
search_node_t* create_search_node(gate_t* state, search_node_t* parent, int depth, char piece, char direction) {
    search_node_t* node = (search_node_t*)malloc(sizeof(search_node_t));
    node->state = state;
    node->parent = parent;
    node->depth = depth;
    node->priority = depth; // For UCS, priority equals depth
    node->piece = piece;
    node->direction = direction;
    return node;
}

static bool pq_reserve(priority_queue_t *pq) {
	if (pq->size < pq->capacity) {
		return true;
	}
	int newCapacity = pq->capacity * 2;
	search_node_t **newNodes = (search_node_t **)realloc(pq->nodes, sizeof(search_node_t*) * newCapacity);
	if (!newNodes) {
		return false;
	}
	pq->nodes = newNodes;
	pq->capacity = newCapacity;
	return true;
}

// Enqueue node in priority queue (ordered by priority/depth)
bool pq_enqueue(priority_queue_t* pq, search_node_t* node) {
	if (!pq || !node) {
		return false;
	}
	if (!pq_reserve(pq)) {
		return false;
	}
	pq->nodes[pq->size] = node;
	int idx = pq->size;
	pq->size++;
	pq_bubble_up(pq, idx);
	return true;
}

// Dequeue node from priority queue
search_node_t* pq_dequeue(priority_queue_t* pq) {
	if (!pq || pq->size == 0) {
		return NULL;
	}
	search_node_t* node = pq->nodes[0];
	pq->size--;
	if (pq->size > 0) {
		pq->nodes[0] = pq->nodes[pq->size];
		pq_bubble_down(pq, 0);
	}
	pq->nodes[pq->size] = NULL;
	return node;
}

// Check if priority queue is empty
bool pq_is_empty(priority_queue_t* pq) {
	return !pq || pq->size == 0;
}

// Free priority queue
void free_priority_queue(priority_queue_t* pq) {
	if (!pq) {
		return;
	}
	for (int i = 0; i < pq->size; i++) {
		if (pq->nodes[i]) {
			free_search_node(pq->nodes[i]);
		}
	}
	free(pq->nodes);
	free(pq);
}

// Free search node
static void free_search_node(search_node_t* node) {
    if (node) {
        if (node->state) {
            free_state(node->state, NULL);
        }
        free(node);
    }
}

// Apply action to create new state
gate_t* apply_action(gate_t* current_state, char piece, char direction) {
	gate_t* new_state = duplicate_state(current_state);
	if (!new_state) {
		return NULL;
	}

	if (!apply_move_in_place(new_state, piece, direction)) {
		free_state(new_state, NULL);
		return NULL;
	}

	size_t old_len = strlen(new_state->soln);
	char *extended = realloc(new_state->soln, (old_len + 3) * sizeof(char));
	if (!extended) {
		free_state(new_state, NULL);
		return NULL;
	}

	extended[old_len] = piece;
	extended[old_len + 1] = direction;
	extended[old_len + 2] = '\0';
	new_state->soln = extended;

	return new_state;
}
/**
 * Given a game state, work out the number of bytes required to store the state.
*/
int getPackedSize(gate_t *gate);

/**
 * Store state of puzzle in map.
*/
void packMap(gate_t *gate, unsigned char *packedMap);

/**
 * Check if the given state is in a won state.
 */
bool winning_state(gate_t gate);

gate_t* duplicate_state(gate_t* gate) {
	if (!gate) return NULL;
	
	gate_t* duplicate = (gate_t*)malloc(sizeof(gate_t));
	if (!duplicate) return NULL;
	
	// Copy basic properties
	*duplicate = *gate; // Copy all basic fields
	duplicate->map = NULL;
	duplicate->map_save = NULL;
	duplicate->soln = NULL;
	
	// Duplicate map
	if (gate->lines > 0) {
		duplicate->map = (char**)malloc(gate->lines * sizeof(char*));
		if (!duplicate->map) {
			free(duplicate);
			return NULL;
		}
		for (int i = 0; i < gate->lines; i++) {
			duplicate->map[i] = NULL;
		}
		for(int i = 0; i < gate->lines; i++) {
			if(gate->map && gate->map[i]) {
				int len = strlen(gate->map[i]);
				duplicate->map[i] = (char*)malloc((len + 1) * sizeof(char));
				if (!duplicate->map[i]) {
					goto duplicate_fail;
				}
				strcpy(duplicate->map[i], gate->map[i]);
			}
		}
	}
	
	// Duplicate map_save
	if (gate->lines > 0) {
		duplicate->map_save = (char**)malloc(gate->lines * sizeof(char*));
		if (!duplicate->map_save) {
			goto duplicate_fail;
		}
		for (int i = 0; i < gate->lines; i++) {
			duplicate->map_save[i] = NULL;
		}
		for(int i = 0; i < gate->lines; i++) {
			if(gate->map_save && gate->map_save[i]) {
				int len = strlen(gate->map_save[i]);
				duplicate->map_save[i] = (char*)malloc((len + 1) * sizeof(char));
				if (!duplicate->map_save[i]) {
					goto duplicate_fail;
				}
				strcpy(duplicate->map_save[i], gate->map_save[i]);
			}
		}
	}
	
	// Initialize solution string if not present
	if(gate->soln && strlen(gate->soln) > 0) {
		int len = strlen(gate->soln);
		duplicate->soln = (char*)malloc((len + 1) * sizeof(char));
		if (!duplicate->soln) {
			goto duplicate_fail;
		}
		strcpy(duplicate->soln, gate->soln);
	} else {
		duplicate->soln = (char*)malloc(sizeof(char));
		if (!duplicate->soln) {
			goto duplicate_fail;
		}
		duplicate->soln[0] = '\0';
	}
	
	return duplicate;

duplicate_fail:
	if (duplicate->map) {
		for (int i = 0; i < gate->lines; i++) {
			if (duplicate->map[i]) {
				free(duplicate->map[i]);
			}
		}
		free(duplicate->map);
	}
	if (duplicate->map_save) {
		for (int i = 0; i < gate->lines; i++) {
			if (duplicate->map_save[i]) {
				free(duplicate->map_save[i]);
			}
		}
		free(duplicate->map_save);
	}
	if (duplicate->soln) {
		free(duplicate->soln);
	}
	free(duplicate);
	return NULL;
}

/**
 * Without lightweight states, the second argument may not be required.
 * Its use is up to your decision.
 */
void free_state(gate_t* stateToFree, gate_t *init_data) {
	(void)init_data; // Suppress unused parameter warning
	if(!stateToFree) return;
	
	// Free map
	if(stateToFree->map) {
		for(int i = 0; i < stateToFree->lines; i++) {
			if(stateToFree->map[i]) {
				free(stateToFree->map[i]);
			}
		}
		free(stateToFree->map);
	}
	
	// Free map_save
	if(stateToFree->map_save) {
		for(int i = 0; i < stateToFree->lines; i++) {
			if(stateToFree->map_save[i]) {
				free(stateToFree->map_save[i]);
			}
		}
		free(stateToFree->map_save);
	}
	
	// Free solution string
	if(stateToFree->soln) {
		free(stateToFree->soln);
	}
	
	// Free the state itself
	free(stateToFree);
}

void free_initial_state(gate_t *init_data) {
	/* Frees dynamic elements of initial state data - including 
		unchanging state. */
	if(!init_data) return;
	
	if(init_data->buffer) {
		free(init_data->buffer);
		init_data->buffer = NULL;
	}

	// Free map_save
	if(init_data->map_save) {
		for(int i = 0; i < init_data->lines; i++) {
			if(init_data->map_save[i]) {
				free(init_data->map_save[i]);
			}
		}
		free(init_data->map_save);
		init_data->map_save = NULL;
	}
	
	// Free map
	if(init_data->map) {
		for(int i = 0; i < init_data->lines; i++) {
			if(init_data->map[i]) {
				free(init_data->map[i]);
			}
		}
		free(init_data->map);
		init_data->map = NULL;
	}
	
	// Free solution string if we allocated it
	if(init_data->soln) {
		free(init_data->soln);
		init_data->soln = NULL;
	}
}

static bool next_combination(int *indices, int size, int totalPieces) {
	for (int i = size - 1; i >= 0; i--) {
		if (indices[i] < totalPieces - (size - i)) {
			indices[i]++;
			for (int j = i + 1; j < size; j++) {
				indices[j] = indices[j - 1] + 1;
			}
			return true;
		}
	}
	return false;
}

static void pack_subset(unsigned char *dest, int destBytes, const unsigned char *src, int atomBits,
	const int *indices, int count) {
	memset(dest, 0, destBytes);
	int destBit = 0;
	for (int k = 0; k < count; k++) {
		int pieceIndex = indices[k];
		int srcStart = pieceIndex * atomBits;
		for (int bit = 0; bit < atomBits; bit++, destBit++) {
			if (getBit((unsigned char *)src, srcStart + bit)) {
				bitOn(dest, destBit);
			}
		}
	}
}

static bool all_combinations_present(struct radixTree *tree, const unsigned char *packedMap, int numPieces,
	int size, int atomBits, unsigned char *buffer, int bufferBytes) {
	if (!tree || !packedMap || size <= 0 || size > numPieces) {
		return true;
	}

	int indices[MAX_PIECES];
	for (int i = 0; i < size; i++) {
		indices[i] = i;
	}

	do {
		pack_subset(buffer, bufferBytes, packedMap, atomBits, indices, size);
		if (checkPresent(tree, buffer, size) == NOTPRESENT) {
			return false;
		}
	} while (next_combination(indices, size, numPieces));

	return true;
}

static void insert_all_combinations(struct radixTree *tree, const unsigned char *packedMap, int numPieces,
	int size, int atomBits, unsigned char *buffer, int bufferBytes) {
	if (!tree || !packedMap || size <= 0 || size > numPieces) {
		return;
	}

	int indices[MAX_PIECES];
	for (int i = 0; i < size; i++) {
		indices[i] = i;
	}

	do {
		pack_subset(buffer, bufferBytes, packedMap, atomBits, indices, size);
		insertRadixTree(tree, buffer, size);
	} while (next_combination(indices, size, numPieces));
}

static void run_search(gate_t *init_data, int width_limit, int packedBytes, search_run_result_t *result) {
	if (!result) {
		return;
	}

	result->solved = false;
	result->solution = NULL;
	result->final_state = NULL;
	result->expanded = 0;
	result->generated = 0;
	result->duplicated = 0;

	if (!init_data) {
		return;
	}

	if (packedBytes <= 0) {
		packedBytes = 1;
	}

	unsigned char *packedMap = (unsigned char *)calloc(packedBytes, sizeof(unsigned char));
	unsigned char *candidatePacked = (unsigned char *)calloc(packedBytes, sizeof(unsigned char));
	if (!packedMap || !candidatePacked) {
		goto teardown;
	}

	priority_queue_t *pq = init_priority_queue();
	struct radixTree *expandedStates = NULL;
	struct radixTree **partialStates = NULL;
	int noveltyLimit = 0;
	bool searchError = false;
	unsigned char **subsetBuffers = NULL;
	int *subsetBytes = NULL;
	int atomBits = 0;

	if (!pq) {
		goto teardown;
	}

	expandedStates = getNewRadixTree(init_data->num_pieces, init_data->lines,
		init_data->num_chars_map / init_data->lines);
	if (!expandedStates) {
		goto teardown;
	}

	noveltyLimit = width_limit;
	if (noveltyLimit > init_data->num_pieces) {
		noveltyLimit = init_data->num_pieces;
	}
	if (noveltyLimit < 0) {
		noveltyLimit = 0;
	}

	int pBits = calcBits(init_data->num_pieces);
	int hBits = calcBits(init_data->lines);
	int wBits = calcBits(init_data->num_chars_map / init_data->lines);
	atomBits = pBits + hBits + wBits;

	if (noveltyLimit > 0) {
		partialStates = (struct radixTree **)malloc(noveltyLimit * sizeof(struct radixTree *));
		if (!partialStates) {
			goto teardown;
		}
		for (int i = 0; i < noveltyLimit; i++) {
			partialStates[i] = NULL;
			partialStates[i] = getNewRadixTree(init_data->num_pieces, init_data->lines,
				init_data->num_chars_map / init_data->lines);
		}

		subsetBuffers = (unsigned char **)malloc(noveltyLimit * sizeof(unsigned char *));
		subsetBytes = (int *)malloc(noveltyLimit * sizeof(int));
		if (!subsetBuffers || !subsetBytes) {
			goto teardown;
		}
		for (int i = 0; i < noveltyLimit; i++) {
			subsetBuffers[i] = NULL;
			subsetBytes[i] = 0;
		}
		for (int i = 0; i < noveltyLimit; i++) {
			int subsetBits = atomBits * (i + 1);
			int bytes = (subsetBits + 7) / 8;
			if (bytes <= 0) {
				bytes = 1;
			}
			subsetBuffers[i] = (unsigned char *)malloc(sizeof(unsigned char) * bytes);
			if (!subsetBuffers[i]) {
				goto teardown;
			}
			subsetBytes[i] = bytes;
		}
	}

	gate_t *initial_state = duplicate_state(init_data);
	if (!initial_state) {
		goto teardown;
	}

	search_node_t *root = create_search_node(initial_state, NULL, 0, '\0', '\0');
	if (!root) {
		free_state(initial_state, NULL);
		goto teardown;
	}
	if (!pq_enqueue(pq, root)) {
		free_search_node(root);
		goto teardown;
	}
	result->generated++;

	while (!pq_is_empty(pq)) {
		search_node_t *current = pq_dequeue(pq);
		result->expanded++;
		gate_t *current_state = current->state;

		if (winning_state(*current_state)) {
			const char *srcSoln = current_state->soln ? current_state->soln : "";
			size_t solnLen = strlen(srcSoln);
			result->solution = (char *)malloc((solnLen + 1) * sizeof(char));
			if (result->solution) {
				strcpy(result->solution, srcSoln);
			}
			result->final_state = current_state;
			result->solved = true;
			current->state = NULL;
			free(current);
			break;
		}

		memset(packedMap, 0, packedBytes);
		packMap(current_state, packedMap);

		if (checkPresent(expandedStates, packedMap, current_state->num_pieces)) {
			result->duplicated++;
			free_search_node(current);
			continue;
		}

		insertRadixTree(expandedStates, packedMap, current_state->num_pieces);

		if (noveltyLimit > 0) {
			int currentLimit = current_state->num_pieces < noveltyLimit ? current_state->num_pieces : noveltyLimit;
			for (int size = 1; size <= currentLimit; size++) {
				insert_all_combinations(partialStates[size - 1], packedMap, current_state->num_pieces,
					size, atomBits, subsetBuffers[size - 1], subsetBytes[size - 1]);
			}
		}

		for (int piece = 0; piece < current_state->num_pieces; piece++) {
			char piece_char = pieceNames[piece];
			for (int dir = 0; dir < 4; dir++) {
				char direction = directions[dir];
				gate_t *next_state = apply_action(current_state, piece_char, direction);
				if (!next_state) {
					continue;
				}

				memset(candidatePacked, 0, packedBytes);
				packMap(next_state, candidatePacked);

				bool skip = false;
				if (checkPresent(expandedStates, candidatePacked, next_state->num_pieces)) {
					skip = true;
				} else if (noveltyLimit > 0) {
					int candidateLimit = next_state->num_pieces < noveltyLimit ? next_state->num_pieces : noveltyLimit;
					for (int size = 1; size <= candidateLimit; size++) {
						if (all_combinations_present(partialStates[size - 1], candidatePacked, next_state->num_pieces,
							size, atomBits, subsetBuffers[size - 1], subsetBytes[size - 1])) {
							skip = true;
							break;
						}
					}
				}

				if (skip) {
					result->duplicated++;
					free_state(next_state, NULL);
					continue;
				}

				search_node_t *child = create_search_node(next_state, NULL, current->depth + 1, piece_char, direction);
				if (!child) {
					free_state(next_state, NULL);
					searchError = true;
					break;
				}
				if (!pq_enqueue(pq, child)) {
					free_search_node(child);
					searchError = true;
					break;
				}
				result->generated++;
			}
			if (searchError) {
				break;
			}
		}

		if (searchError) {
			free_search_node(current);
			break;
		}

		free_search_node(current);
	}

	if (searchError) {
		if (result->solution) {
			free(result->solution);
			result->solution = NULL;
		}
		if (result->final_state) {
			free_state(result->final_state, NULL);
			result->final_state = NULL;
		}
		result->solved = false;
	}

teardown:
	if (pq) {
		while (!pq_is_empty(pq)) {
			search_node_t *node = pq_dequeue(pq);
			free_search_node(node);
		}
		free(pq->nodes);
		free(pq);
	}

	if (subsetBuffers) {
		for (int i = 0; i < noveltyLimit; i++) {
			if (subsetBuffers[i]) {
				free(subsetBuffers[i]);
			}
		}
		free(subsetBuffers);
	}

	if (subsetBytes) {
		free(subsetBytes);
	}

	if (partialStates) {
		for (int i = 0; i < noveltyLimit; i++) {
			if (partialStates[i]) {
				freeRadixTree(partialStates[i]);
			}
		}
		free(partialStates);
	}

	if (expandedStates) {
		freeRadixTree(expandedStates);
	}

	if (packedMap) {
		free(packedMap);
	}
	if (candidatePacked) {
		free(candidatePacked);
	}

	if (!result->solved) {
		if (result->solution) {
			free(result->solution);
			result->solution = NULL;
		}
		if (result->final_state) {
			free_state(result->final_state, NULL);
			result->final_state = NULL;
		}
	}
}

/**
 * Find a solution by exploring all possible paths
 */
static void report_results(const char *solnStr, double elapsed, int expanded, int generated,
	int duplicated, int memoryUsage, gate_t *winning_state_ptr, int num_pieces, int solvingWidth,
	bool usedFallback, bool has_won, int algorithm);

void find_solution(gate_t* init_data, int algorithm) {
	int packedBits = getPackedSize(init_data);
	int packedBytes = (packedBits + 7) / 8;
	if (packedBytes <= 0) {
		packedBytes = 1;
	}

	bool has_won = false;
	double start = now();
	double elapsed = 0.0;
	char *soln = NULL;
	gate_t *winning_state_ptr = NULL;
	int totalExpanded = 0;
	int totalGenerated = 0;
	int totalDuplicated = 0;
	int solvingWidth = -1;
	bool usedFallback = false;

	if (algorithm == 1) {
		int width = init_data->num_pieces + 1;
		search_run_result_t runResult;
		run_search(init_data, width, packedBytes, &runResult);
		totalExpanded = runResult.expanded;
		totalGenerated = runResult.generated;
		totalDuplicated = runResult.duplicated;
		has_won = runResult.solved;
		soln = runResult.solution;
		winning_state_ptr = runResult.final_state;
		solvingWidth = width;
	} else if (algorithm == 2) {
		search_run_result_t runResult;
		run_search(init_data, 0, packedBytes, &runResult);
		totalExpanded = runResult.expanded;
		totalGenerated = runResult.generated;
		totalDuplicated = runResult.duplicated;
		has_won = runResult.solved;
		soln = runResult.solution;
		winning_state_ptr = runResult.final_state;
		solvingWidth = 0;
	} else {
		int maxWidth = init_data->num_pieces > 0 ? init_data->num_pieces : 0;
		for (int width = 1; width <= maxWidth && !has_won; width++) {
			search_run_result_t runResult;
			run_search(init_data, width, packedBytes, &runResult);
			totalExpanded += runResult.expanded;
			totalGenerated += runResult.generated;
			totalDuplicated += runResult.duplicated;
			if (runResult.solved) {
				has_won = true;
				soln = runResult.solution;
				winning_state_ptr = runResult.final_state;
				solvingWidth = width;
			} else {
				if (runResult.solution) {
					free(runResult.solution);
				}
				if (runResult.final_state) {
					free_state(runResult.final_state, NULL);
				}
			}
		}

		if (!has_won) {
			search_run_result_t fallbackResult;
			run_search(init_data, 0, packedBytes, &fallbackResult);
			totalExpanded += fallbackResult.expanded;
			totalGenerated += fallbackResult.generated;
			totalDuplicated += fallbackResult.duplicated;
			usedFallback = true;
			if (fallbackResult.solved) {
				has_won = true;
				soln = fallbackResult.solution;
				winning_state_ptr = fallbackResult.final_state;
				solvingWidth = 0;
			} else {
				if (fallbackResult.solution) {
					free(fallbackResult.solution);
				}
				if (fallbackResult.final_state) {
					free_state(fallbackResult.final_state, NULL);
				}
			}
		}
	}

	if (!soln) {
		soln = (char *)malloc(sizeof(char));
		if (soln) {
			soln[0] = '\0';
		}
	}

	elapsed = now() - start;
	const char *solnStr = soln ? soln : "";
	int memoryUsage = 0;
	report_results(solnStr, elapsed, totalExpanded, totalGenerated, totalDuplicated, memoryUsage,
		winning_state_ptr, init_data->num_pieces, solvingWidth, usedFallback, has_won, algorithm);

	if (winning_state_ptr) {
		free_state(winning_state_ptr, NULL);
	}
	if (soln) {
		free(soln);
	}

	free_initial_state(init_data);
}

/**
 * Given a game state, work out the number of bytes required to store the state.
*/
int getPackedSize(gate_t *gate) {
	int pBits = calcBits(gate->num_pieces);
    int hBits = calcBits(gate->lines);
    int wBits = calcBits(gate->num_chars_map / gate->lines);
    int atomSize = pBits + hBits + wBits;
	int bitCount = atomSize * gate->num_pieces;
	return bitCount;
}

/**
 * Store state of puzzle in map.
*/
void packMap(gate_t *gate, unsigned char *packedMap) {
	int pBits = calcBits(gate->num_pieces);
    int hBits = calcBits(gate->lines);
    int wBits = calcBits(gate->num_chars_map / gate->lines);
	int bitIdx = 0;
	for(int i = 0; i < gate->num_pieces; i++) {
		for(int j = 0; j < pBits; j++) {
			if(((i >> j) & 1) == 1) {
				bitOn( packedMap, bitIdx );
			} else {
				bitOff( packedMap, bitIdx );
			}
			bitIdx++;
		}
		for(int j = 0; j < hBits; j++) {
			if(((gate->piece_y[i] >> j) & 1) == 1) {
				bitOn( packedMap, bitIdx );
			} else {
				bitOff( packedMap, bitIdx );
			}
			bitIdx++;
		}
		for(int j = 0; j < wBits; j++) {
			if(((gate->piece_x[i] >> j) & 1) == 1) {
				bitOn( packedMap, bitIdx );
			} else {
				bitOff( packedMap, bitIdx );
			}
			bitIdx++;
		}
	}
}

/**
 * Check if the given state is in a won state.
 */
bool winning_state(gate_t gate) {
	for (int i = 0; i < gate.lines; i++) {
		for (int j = 0; gate.map_save[i][j] != '\0'; j++) {
			if (gate.map[i][j] == 'G' || (gate.map[i][j] >= 'I' && gate.map[i][j] <= 'Q')) {
				return false;
			}
		}
	}
	return true;
}

void solve(char const *path)
{
	/**
	 * Load Map
	*/
	gate_t gate = make_map(path, gate);
	
	/**
	 * Verify map is valid
	*/
	map_check(gate);

	/**
	 * Locate player x, y position
	*/
	gate = find_player(gate);

	/**
	 * Locate each piece.
	*/
	gate = find_pieces(gate);
	
	gate.base_path = path;
	
	// Initialize solution string
	gate.soln = (char*)malloc(sizeof(char));
	if (gate.soln) {
		gate.soln[0] = '\0';
	}

	find_solution(&gate, solver_algorithm);

}

static void report_results(const char *solnStr, double elapsed, int expanded, int generated,
	int duplicated, int memoryUsage, gate_t *winning_state_ptr, int num_pieces, int solvingWidth,
	bool usedFallback, bool has_won, int algorithm) {
	printf("Solution path: ");
	printf("%s\n", solnStr);
	printf("Execution time: %lf\n", elapsed);
	printf("Expanded nodes: %d\n", expanded);
	printf("Generated nodes: %d\n", generated);
	printf("Duplicated nodes: %d\n", duplicated);
	printf("Auxiliary memory usage (bytes): %d\n", memoryUsage);
	printf("Number of pieces in the puzzle: %d\n", num_pieces);
	printf("Number of steps in solution: %ld\n", (long)(strlen(solnStr) / 2));
	int emptySpaces = 0;
	if (winning_state_ptr) {
		for (int i = 0; i < winning_state_ptr->lines; i++) {
			for (int j = 0; winning_state_ptr->map[i][j] != '\0'; j++) {
				if (winning_state_ptr->map[i][j] == ' ') {
					emptySpaces++;
				}
			}
		}
	}
	printf("Number of empty spaces: %d\n", emptySpaces);
	char solvedBy[64];
	if (algorithm == 1) {
		if (has_won) {
			snprintf(solvedBy, sizeof(solvedBy), "Algorithm1-IW(%d)", solvingWidth);
		} else {
			snprintf(solvedBy, sizeof(solvedBy), "Algorithm1-IW(%d) (no solution)", solvingWidth);
		}
	} else if (algorithm == 2) {
		if (has_won) {
			snprintf(solvedBy, sizeof(solvedBy), "Algorithm2-UCS");
		} else {
			snprintf(solvedBy, sizeof(solvedBy), "Algorithm2-UCS (no solution)");
		}
	} else {
		if (has_won) {
			if (solvingWidth > 0) {
				snprintf(solvedBy, sizeof(solvedBy), "Algorithm3-IW(%d)", solvingWidth);
			} else {
				snprintf(solvedBy, sizeof(solvedBy), "Algorithm3-UCS");
			}
		} else {
			if (usedFallback) {
				snprintf(solvedBy, sizeof(solvedBy), "Algorithm3-UCS (no solution)");
			} else {
				snprintf(solvedBy, sizeof(solvedBy), "Algorithm3-IW(no solution)");
			}
		}
	}
	printf("Solved by %s\n", solvedBy);
	printf("Number of nodes expanded per second: %lf\n", (expanded + 1) / (elapsed > 0 ? elapsed : 1));
}
