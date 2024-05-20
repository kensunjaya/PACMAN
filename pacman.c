#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <windows.h>
#include <conio.h>
#include <unistd.h>
#include <time.h>

#define gotoxy(x,y) printf("\033[%d;%dH", y, x)

// USER CONFIG
#define SIDE 40  // UKURAN MAZE (SESUAIKAN DENGAN MAP DI TXT)
#define DELAY 75 // FPS (1000 / DELAY)
#define ENTITY_COUNT 3 // JUMLAH MUSUH (SESUAIKAN DENGAN MAP DI TXT)
#define ENTITY_SPEED 5 // MAKIN TINGGI MAKIN LINCAH PULA MUSUHNYA
#define ENTITY_MOVEMENT_RANDOMNESS 3 // MAKIN GEDE MAKIN KECIL KEMUNGKINAN PAKE DIJKSTRANYA

const int text_x = SIDE*2 + 12;
const int text_y = SIDE/2 + 2;
const int VERTEX = SIDE*SIDE;

int dist[SIDE*SIDE];
int source[ENTITY_COUNT], dest;

char maze[SIDE][SIDE] = {0};
char graph[SIDE*SIDE][SIDE*SIDE] = {0};

typedef struct Node{
    int data;
    struct Node *next;
} Node;
Node *head[ENTITY_COUNT][SIDE*SIDE] = {NULL};

int randint(int a, int b) { // inclusive
    return ((rand() % (b - a + 1) + a));
}

Node *createNewNode(int x) {
    Node *newNode = (Node*)malloc(sizeof(Node));
    newNode->data = x;
    newNode->next = NULL;
    return newNode;
}

Node *insert_node(Node *curr, int x) {
    if (curr == NULL) {
        return createNewNode(x);
    }
    
    curr->next = insert_node(curr->next, x);
    return curr;
}

void pop(int entityIndex, int index) { 
    if (head[entityIndex][index] == NULL) {
        return;
    }
    Node *nodeToDelete = head[entityIndex][index];
    head[entityIndex][index] = head[entityIndex][index]->next;
    free(nodeToDelete);
    nodeToDelete = NULL;
    pop(entityIndex, index);
}

bool isVoid(char cell) {
	if (cell == ' ' ||
		cell == 'C' ||
		cell == 'E') {
		return true;	
	}
	return false;
}

void buildGraph() {
    int i, j, baris = 0;
    
    for (i=0;i<SIDE;i++) {
        for (j=0;j<SIDE;j++) {
            if (isVoid(maze[i][j+1])) {
                graph[baris][baris+1] = 1;
            }
            if (isVoid(maze[i][j-1])) {
                graph[baris][baris-1] = 1;
            }
            if (isVoid(maze[i+1][j])) {
                graph[baris][baris + SIDE] = 1;
            }
            if (isVoid(maze[i-1][j])) {
                graph[baris][baris - SIDE] = 1;
            }
            
            baris++;
        }
        
    }
}

int minDistance(int dist[], bool sptSet[]) {
    int min = INT_MAX, min_index, v;
    for (v=0;v<VERTEX;v++) {
        if (!sptSet[v] && dist[v] <= min) {
            min = dist[v];
            min_index = v;
        }
    }
    return min_index;
}

void dijkstra(char graph[VERTEX][VERTEX], int entityIndex, int src) {  // find the shortest path
    int i, count, v;
    bool sptSet[VERTEX]; 
    for (i=0;i<VERTEX;i++) {
        dist[i] = INT_MAX;
        sptSet[i] = false; 
        head[entityIndex][i] = insert_node(head[entityIndex][i], src);
    }
	
    dist[src] = 0;

    for (count=0;count<VERTEX-1;count++) {
        int u = minDistance(dist, sptSet);
        sptSet[u] = true;
        for (v=0;v<VERTEX;v++) {
            if (!sptSet[v] && graph[u][v] && dist[u] != INT_MAX && dist[u] + graph[u][v] < dist[v]) {
                pop(entityIndex, v);
                Node *curr = head[entityIndex][u];
                while (curr) { // replace head[v] dengan head[u]
                    head[entityIndex][v] = insert_node(head[entityIndex][v], curr->data);
                    curr = curr->next;
                }
                head[entityIndex][v] = insert_node(head[entityIndex][v], v);
                dist[v] = dist[u] + graph[u][v];
            }
        }    
    }
}

void readMapFromFile() {
	FILE *f = fopen("map.txt", "r");
	int i = 0, j = 0;
	int entityCount = 0;
	
	while (!feof(f)) {	
		fscanf(f, "%c", &maze[i][j]);
		if (maze[i][j] == '\n') {
			i++;
			j=0;
			continue;
		}
		j++;
	}
	buildGraph();
	
	for (i=0;i<SIDE;i++) {
		for (j=0;j<SIDE;j++) {
			if (maze[i][j] == '\n') {
				puts("");
				break;
			}
			if (maze[i][j] == 'C') {
				dest = i * SIDE + j;				
			}
			if (maze[i][j] == 'E') {
				source[entityCount] = i * SIDE + j;
				entityCount++;
				
			}
			printf("%c", maze[i][j]);
		}
	}
	fclose(f);
}

char readKeyInput(char defaultKey) {
	if (GetAsyncKeyState(VK_LEFT)) {
		return 'l';
	};
	if (GetAsyncKeyState(VK_RIGHT)) {
		return 'r';
	}
	if (GetAsyncKeyState(VK_UP)) {
		return 'u';
	}
	if (GetAsyncKeyState(VK_DOWN)) {
		return 'd';
	}			
	return defaultKey;			
}

bool stillAlive(Node *node[ENTITY_COUNT]) {
	int i;
	for (i=0;i<ENTITY_COUNT;i++) {
		if (node[i]->data == dest) {
			return false;
		} 
	}
	return true;
}

bool isCollideWithAnotherEntity(int pos, Node *entity[ENTITY_COUNT]) {
	int i;
	for (i=0;i<ENTITY_COUNT;i++) {
		if (pos == entity[i]->data) {
			return true;
		}
	}
	return false;
}

int gameExecution() {
	readMapFromFile();
	int y, j, i, prevY[ENTITY_COUNT];
	Node *curr[ENTITY_COUNT];
	long long int screenRefreshCount = 0;
	char key, direction = ' ';
	for (i=0;i<ENTITY_COUNT;i++) {
		dijkstra(graph, i, source[i]);
		curr[i] = head[i][dest];
	}
	gotoxy(1, 25);
	system("pause");
	while (stillAlive(curr)) {
		for(i=0;i<ENTITY_COUNT;i++) {
			for (j=0;j<SIDE*SIDE;j++) {
				pop(i, j);
			}
			dijkstra(graph, i, source[i]);
			curr[i] = head[i][dest];
			prevY[i] = curr[i]->data / SIDE;
		}
		
		while (curr[0]) {
			direction = readKeyInput(direction);
			screenRefreshCount++;
			
			// move the enemy
			for (i=0;i<ENTITY_COUNT;i++) {
				if (screenRefreshCount % ENTITY_SPEED) {
					if (!randint(0, ENTITY_MOVEMENT_RANDOMNESS)) {
						gotoxy((curr[i]->data % SIDE) + 1, prevY[i] + 1);
			            printf(" "); // clear previous cell
			            if (isCollideWithAnotherEntity(curr[i]->next->data, curr)) { // pastikan 1 cell hanya diisi oleh 1 musuh
			            	continue;
						}
			            curr[i] = curr[i]->next;
			            y = curr[i]->data / SIDE;
						gotoxy((curr[i]->data % SIDE) + 1, y + 1);
			            printf("\033[1m\033[33mE\033[0;37m"); // write in new cell
			            prevY[i] = y;
					}
				}
			}
			
			if (!stillAlive(curr)) {
				gotoxy(1, 26);
				return 0;
			}
			usleep(DELAY*1000);
			
			//move the player
			if (direction == 'l') {
				j = (dest - 1) / SIDE;
				if (isVoid(maze[j][(dest - 1) % SIDE])) {
					y = dest / SIDE;
					gotoxy((dest % SIDE) + 1, y + 1);
					printf(" ");
					gotoxy(((dest - 1) % SIDE) + 1, j + 1);
					printf("\033[1m\033[36mC\033[0;37m");
					dest -= 1;
					for (i=0;i<ENTITY_COUNT;i++) {
						source[i] = curr[i]->data;
					}
					break;
				}
			}
			else if (direction == 'r') {
				j = (dest + 1) / SIDE;
				if (isVoid(maze[j][(dest + 1) % SIDE])) {
					y = dest / SIDE;
					gotoxy((dest % SIDE) + 1, y + 1);
					printf(" ");
					gotoxy(((dest + 1) % SIDE) + 1, j + 1);
					printf("\033[1m\033[36mC\033[0;37m");
					dest += 1;
					for (i=0;i<ENTITY_COUNT;i++) {
						source[i] = curr[i]->data;
					}
					break;
				}
			}
			else if (direction == 'd') {
				j = (dest + SIDE) / SIDE;
				if (isVoid(maze[j][(dest + SIDE) % SIDE])) {
					y = dest / SIDE;
					gotoxy((dest % SIDE) + 1, y + 1);
					printf(" ");
					gotoxy(((dest + SIDE) % SIDE) + 1, j + 1);
					printf("\033[1m\033[36mC\033[0;37m");
					dest += SIDE;
					for (i=0;i<ENTITY_COUNT;i++) {					
						source[i] = curr[i]->data;
					}
					break;
				}
			}
			else if (direction == 'u') {
				j = (dest - SIDE) / SIDE;
				if (isVoid(maze[j][(dest - SIDE) % SIDE])) {
					y = dest / SIDE;
					gotoxy((dest % SIDE) + 1, y + 1);
					printf(" ");
					gotoxy(((dest - SIDE) % SIDE) + 1, j + 1);
					printf("\033[1m\033[36mC\033[0;37m");
					dest -= SIDE;
					for (i=0;i<ENTITY_COUNT;i++) {
						source[i] = curr[i]->data;
					}
					break;
				}
			}
			gotoxy(1, 25);
            
		}
	}
	gotoxy(1, 26);
}

int main() {
	char option = 'y';
	do {
		gameExecution();
		printf("Do you want to try again (y/n)? ");
		scanf("%c", &option); getchar();
		system("cls");
	} while (option == 'y' || option == 'Y');
	printf("Thanks for playing");
	return 0;
}
