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

// GAME CONFIGURATION
#define SIDE 40  // Ukuran Maze
#define DELAY 50 // FPS (1000 / DELAY)
#define ENTITY_COUNT 3 // Jumlah musuh


#define BLK "\e[0;30m"
#define RED "\e[0;91m"
#define GRN "\e[0;92m"
#define YEL "\e[0;93m"
#define BLU "\e[0;94m"
#define PNK "\e[0;95m"
#define CYN "\e[0;96m"
#define WHT "\e[0;37m"
#define CRESET "\e[0m"

const int VERTEX = SIDE*SIDE;
int ENTITY_MOVEMENT_RANDOMNESS = 3; // Semakin high, semakin minimal kemungkinan musuh menggunakan algoritma dijkstra
int ENTITY_SPEED = 2; // Kecepatan musuh
int dist[SIDE*SIDE];
int source[ENTITY_COUNT], dest;
int totalCoin, collectedCoin, totalSideX, totalSideY;
long long int elapsedTime;

unsigned char maze[SIDE][SIDE] = {0};
char graph[SIDE*SIDE][SIDE*SIDE] = {0};

typedef struct Node{
    int data;
    struct Node *next;
} Node;
Node *head[ENTITY_COUNT][SIDE*SIDE] = {NULL};

typedef struct HighScoreNode {
    char name[50];
    int score;
    struct HighScoreNode *next;
} HighScoreNode;
HighScoreNode *highScores = NULL;


//void gotoxy(int x, int y) {
//	COORD coord;
//	coord.X = x-1;
//	coord.Y = y-1;
//	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
//}

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

void gameOver(int win) {	
	int i, j;
	for (i=0;i<totalSideY/2;i++) {
		for (j=0;j<totalSideX-1;j++) {
			if (i % 2 == 0) {
				gotoxy(j+1, i+1);
				putchar(176);
				gotoxy(totalSideX-j-1, totalSideY-i);
				putchar(176);
				Sleep(1);
			}
			else {
				gotoxy(j+1, i+1);
				putchar(176);
				gotoxy(totalSideX-j-1, totalSideY-i);
				putchar(176);
				
			}
		}
	}
	system("cls");
	if (!win) {
		for (i=0;i<10;i++) {
			system("cls");
			if (i % 2) {
				printf(YEL);
			}
			else {
				printf(RED);
			}
			puts("   _____                         ____                 ");
			puts("  / ____|                       / __ \\                ");
			puts(" | |  __  __ _ _ __ ___   ___  | |  | |_   _____ _ __ ");
			puts(" | | |_ |/ _` | '_ ` _ \\ / _ \\ | |  | \\ \\ / / _ \\ '__|");
			puts(" | |__| | (_| | | | | | |  __/ | |__| |\\ V /  __/ |   ");
			puts("  \\_____|\\__,_|_| |_| |_|\\___|  \\____/  \\_/ \\___|_|   ");
			Sleep(100);
		}
	}
	else {
		for (i=0;i<10;i++) {
			system("cls");
			if (i % 2) {
				printf(GRN);
			}
			else {
				printf(PNK);
			}
			puts(" ____  ____   ___   _____  _____   ____      ____  _____  ____  _____  ");
			puts("|_  _||_  _|.'   `.|_   _||_   _| |_  _|    |_  _||_   _||_   \\|_   _| ");
			puts("  \\ \\  / / /  .-.  \\ | |    | |     \\ \\  /\\  / /    | |    |   \\ | |   ");
			puts("   \\ \\/ /  | |   | | | '    ' |      \\ \\/  \\/ /     | |    | |\\ \\| |   ");
			puts("   _|  |_  \\  `-'  /  \\ \\__/ /        \\  /\\  /     _| |_  _| |_\\   |_  ");
			puts("  |______|  `.___.'    `.__.'          \\/  \\/     |_____||_____||____| ");
			Sleep(100);
		}
	}
	
	printf(CRESET);
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
	if (
		cell == '.' ||
		cell == ' ' ||
		cell == 'C' ||
		cell == 'E') {
		return true;
	}
	return false;
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

void readMapFromFile() {
	FILE *f = fopen("map.txt", "r");
	totalCoin = collectedCoin = elapsedTime = 0;
	int i, j, entityCount = 0;
	i = j = 0;
	
	while (!feof(f)) {	
		maze[i][j] = fgetc(f);
		if (maze[i][j] == '\n') {
			i++;
			j=0;
			continue;
		}
		j++;
	}
	totalSideX = j;
	totalSideY = i+1;
	buildGraph();
	
	for (i=0;i<totalSideY;i++) {
		for (j=0;j<totalSideX;j++) {
			if (maze[i][j] == '\n') {
				puts("");
				break;
			}
			if (maze[i][j] == 'C') {
				dest = i * SIDE + j;				
			}
			if (maze[i][j] == 'E') {
				source[entityCount] = i * SIDE + j;
				maze[i][j] = '.';
				entityCount++;
			}
			printf(BLU);
			switch (maze[i][j]) {
				case '{':
					putchar(185);
					break;
				case '|':
					putchar(186);
					break;
				case '\\':
					putchar(187);
					break;
				case 'l':
					putchar(188);
					break;
				case 'L':
					putchar(200);
					break;
				case '/':
					putchar(201);
					break;
				case '^':
					putchar(202);
					break;
				case 'T':
					putchar(203);
					break;
				case '}':
					putchar(204);
					break;
				case '-':
					putchar(205);
					break;
				case '.':
					printf(WHT);
					putchar(250);
					totalCoin++;
					break;
				case 'C':
					printf(CYN);
					putchar('C');
					break;
				case 'E':
					printf(YEL);
					putchar('E');
					break;	
				default:
					putchar(maze[i][j]);
			}
				
//			printf("%c", maze[i][j]);
		}
	}
	fclose(f);
	printf(CRESET);
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

bool stillAlive(int prevX[ENTITY_COUNT], int prevY[ENTITY_COUNT]) {
	int i;
	for (i=0;i<ENTITY_COUNT;i++) {
		if ((prevY[i] * SIDE) + prevX[i] == dest) {
			return false;
		} 
	}
	return true;
}

bool isCollideWithAnotherEntity(int pos, int prevX[ENTITY_COUNT], int prevY[ENTITY_COUNT]) {
	int i;
	for (i=0;i<ENTITY_COUNT;i++) {
		if (pos == (prevY[i]*SIDE) + prevX[i]) {
			return true;
		}
	}
	return false;
}

int gameExecution() {
	readMapFromFile();
	int x, y, j, i, prevY[ENTITY_COUNT], prevX[ENTITY_COUNT];
	Node *curr[ENTITY_COUNT];
	long long int screenRefreshCount = 0;
	char key, direction = ' ';
	for (i=0;i<ENTITY_COUNT;i++) {
		dijkstra(graph, i, source[i]);
		curr[i] = head[i][dest];
		prevY[i] = source[i] / SIDE;
		prevX[i] = source[i] % SIDE;
	}
	gotoxy(1, 20);
	system("pause");
	gotoxy(1, 20);
	printf("Points: %-30d", collectedCoin);
	gotoxy(1, 21);
	printf("Elapsed time: 0s");
	int startTime = time(0);

	while (stillAlive(prevX, prevY)) {
		for(i=0;i<ENTITY_COUNT;i++) {
			for (j=0;j<SIDE*SIDE;j++) {
				pop(i, j);
			}
			dijkstra(graph, i, (prevY[i] * SIDE) + prevX[i]);
			curr[i] = head[i][dest];
//			prevY[i] = curr[i]->data / SIDE;
//			prevX[i] = curr[i]->data % SIDE;
		}
		
		while (curr[0]) {
			direction = readKeyInput(direction);
			screenRefreshCount++;
			// move the enemy
			for (i=0;i<ENTITY_COUNT;i++) {
				if (screenRefreshCount % ENTITY_SPEED) {
					gotoxy(prevX[i] + 1, prevY[i] + 1);
					if (maze[prevY[i]][prevX[i]] == '.') {
						putchar(250);
					}
					else {
						putchar(' ');
					}
			         // clear previous cell
			        int randomMove = randint(0, ENTITY_MOVEMENT_RANDOMNESS);
					if (!randomMove || randomMove == 1) {
			            if (isCollideWithAnotherEntity(curr[i]->next->data, prevX, prevY)) { // pastikan 1 cell hanya diisi oleh 1 musuh
			            	continue;
						}
			            curr[i] = curr[i]->next;
			            y = curr[i]->data / SIDE;
			            x = curr[i]->data % SIDE;
						gotoxy(x + 1, y + 1);
			            printf("\033[1m\033[33mE\033[0;37m"); // write in new cell
			            prevY[i] = y;
			            prevX[i] = x;
					}
					else {
			            int randomPosX = prevX[i];
			            int randomPosY = prevY[i];
			            
						int directionVal = randint(0, 3);
						const int initialDirectionVal = directionVal;
						do {
							if (directionVal == 0 && isVoid(maze[randomPosY][randomPosX + 1])) {
								randomPosX += 1;
								break;
							}	
							else if (directionVal == 1 && isVoid(maze[randomPosY][randomPosX - 1])) {
								randomPosX -= 1;
								break;
							}
							else if (directionVal == 2 && isVoid(maze[randomPosY + 1][randomPosX])) {
								randomPosY += 1;
								break;
							}
							else if (directionVal == 3 && isVoid(maze[randomPosY - 1][randomPosX])) {
								randomPosY -= 1;
								break;
							}
							else {
								directionVal = (directionVal + 1) % 4;
							}
						} while (directionVal != initialDirectionVal);
							
						gotoxy(randomPosX + 1, randomPosY + 1);
						printf("\033[1m\033[33mE\033[0;37m"); // write in new cell
						prevY[i] = randomPosY;
						prevX[i] = randomPosX;
						dijkstra(graph, i, (prevY[i] * SIDE) + prevX[i]);
						curr[i] = head[i][dest];
					}
				}
			}
			
			gotoxy(9, 20);
			printf("%-5d", collectedCoin);
			gotoxy(15, 21);
			elapsedTime = time(0) - startTime;
			printf("%llds", elapsedTime);
			
			if (collectedCoin >= totalCoin) {
				gameOver(1); // win
				gotoxy(1, 12);
				return 1;
			}
			
			if (!stillAlive(prevX, prevY)) {
				gameOver(0); // lose
				gotoxy(1, 12);
				return 0;
			}
			usleep(DELAY*1000);
			
			//move the player
			if (direction == 'l') {
				j = (dest - 1) / SIDE;
				if (isVoid(maze[j][(dest - 1) % SIDE])) {
					if (maze[j][(dest - 1) % SIDE] == '.') {
						maze[j][(dest - 1) % SIDE] = ' ';
						collectedCoin++;
					}
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
				break;
			}
			else if (direction == 'r') {
				j = (dest + 1) / SIDE;
				if (isVoid(maze[j][(dest + 1) % SIDE])) {
					if (maze[j][(dest + 1) % SIDE] == '.') {
						maze[j][(dest + 1) % SIDE] = ' ';
						collectedCoin++;
					}
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
				break;
			}
			else if (direction == 'd') {
				j = (dest + SIDE) / SIDE;
				if (isVoid(maze[j][(dest + SIDE) % SIDE])) {
					if (maze[j][(dest + SIDE) % SIDE] == '.') {
						maze[j][(dest + SIDE) % SIDE] = ' ';
						collectedCoin++;
					}
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
				break;
			}
			else if (direction == 'u') {
				j = (dest - SIDE) / SIDE;
				if (isVoid(maze[j][(dest - SIDE) % SIDE])) {
					if (maze[j][(dest - SIDE) % SIDE] == '.') {
						maze[j][(dest - SIDE) % SIDE] = ' ';
						collectedCoin++;
					}
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
				break;
			}
		}
	}
	gameOver(0);
	gotoxy(1, 12);
	return 0;
}

void pressEnter() {
	char key;
	printf("Press enter to continue ...");
	do {
  		key = getch();
 	} while (key != '\r');
}

void addHighScore(char *name, int score) {
    HighScoreNode *current = highScores;
    HighScoreNode *previous = NULL;
    while (current != NULL && strcmp(current->name, name) != 0) {
        previous = current;
        current = current->next;
    }

    if (current != NULL) {
        if (current->score < score) {
            current->score = score;
        }
        return;
    }

    HighScoreNode *newNode = (HighScoreNode *)malloc(sizeof(HighScoreNode));
    strcpy(newNode->name, name);
    newNode->score = score;
    newNode->next = NULL;

    if (!highScores || score > highScores->score) {
        newNode->next = highScores;
        highScores = newNode;
    } else {
        HighScoreNode *current = highScores;
        while (current->next && current->next->score >= score) {
            current = current->next;
        }
        newNode->next = current->next;
        current->next = newNode;
    }

    FILE *file = fopen("highscore.txt", "a");
    if (file == NULL) {
        printf("Error opening file.\n");
        return;
    }

    current = highScores;
    while (current != NULL) {
        fprintf(file, "%s#%d\n", current->name, current->score);
        current = current->next;
    }

    fclose(file);
}

void displayHighScores() {
    FILE *file = fopen("highscore.txt", "r");
    if (file == NULL) {
        printf("Error opening file.\n");
        return;
    }

    HighScoreNode *head = NULL;

    // Read scores from the file and insert them into a linked list
    char name[50];
    int score;
    while (fscanf(file, "%[^#]#%d\n", name, &score) != EOF) {
        HighScoreNode *newNode = (HighScoreNode *)malloc(sizeof(HighScoreNode));
        strcpy(newNode->name, name);
        newNode->score = score;
        newNode->next = NULL;

        if (head == NULL) {
            head = newNode;
        } else {
            // Insert newNode into the linked list in descending order based on score
            HighScoreNode *prev = NULL;
            HighScoreNode *current = head;
            while (current != NULL && current->score >= newNode->score) {
                prev = current;
                current = current->next;
            }
            if (prev == NULL) {
                newNode->next = head;
                head = newNode;
            } else {
                prev->next = newNode;
                newNode->next = current;
            }
        }
    }

    fclose(file);

    // Display the sorted high scores
    printf("\n%sHigh Scores:%s\n", "\033[1;33m", "\033[0m");
    printf("%s------------%s\n", "\033[1;33m", "\033[0m");

    printf("+------+--------------------------+-------+\n");
    printf("| %-4s | %-24s | %-5s |\n", "Rank", "Name", "Score");
    printf("+------+--------------------------+-------+\n");

    int rank = 1;
    HighScoreNode *current = head;
    while (current != NULL) {
        printf("| %-4d | %-24s | %-5d |\n", rank++, current->name, current->score);
        current = current->next;
    }
    printf("+------+--------------------------+-------+\n");

    // Free allocated memory for the linked list
    while (head != NULL) {
        HighScoreNode *temp = head;
        head = head->next;
        free(temp);
    }

    pressEnter();
    system("cls");
}

void play(int difficulty){
	
	switch (difficulty) {
		case 0:
			ENTITY_MOVEMENT_RANDOMNESS = 6;
			ENTITY_SPEED = 2;
			break;
		case 1:
			ENTITY_MOVEMENT_RANDOMNESS = 3;
			ENTITY_SPEED = 3;
			break;
		case 2:
			ENTITY_MOVEMENT_RANDOMNESS = 2;
			ENTITY_SPEED = 10;
			break;
		case 3:
			ENTITY_MOVEMENT_RANDOMNESS = 0;
			ENTITY_SPEED = INT_MAX;
			break;			
	}
	char option = 'y';
	int win = 0;
	
	char name[100];
	do {
		system("cls");
		gotoxy(2, 2);
		printf(GRN);
		printf("Enter your name: ");
		printf(YEL);
    	scanf("%[^\n]", name); getchar();
	} while (strlen(name) < 1);
    system("cls");
	do {
		option = ' ';
		win = gameExecution();
		system("pause");
		scanf("%c", &option); getchar();
		system("cls");
	} while (option == 'y' || option == 'Y');
	addHighScore(name, collectedCoin);
}

void exitArt(){
	printf("  _______ _                 _          ______           _____  _             _             _ \n");
	Sleep(1);
    printf(" |__   __| |               | |        |  ____|         |  __ \\| |           (_)           | |\n");
    Sleep(1);
    printf("    | |  | |__   __ _ _ __ | | _____  | |__ ___  _ __  | |__) | | __ _ _   _ _ _ __   __ _| |\n");
    Sleep(1);
    printf("    | |  | '_ \\ / _` | '_ \\| |/ / __| |  __/ _ \\| '__| |  ___/| |/ _` | | | | | '_ \\ / _` | |\n");
    Sleep(1);
    printf("    | |  | | | | (_| | | | |   <\\__ \\ | | | (_) | |    | |    | | (_| | |_| | | | | | (_| |_|\n");
    Sleep(1);
    printf("    |_|  |_| |_|\\__,_|_| |_|_|\\_\\___/ |_|  \\___/|_|    |_|    |_|\\__,_|\\__, |_|_| |_|\\__, (_)\n");
    Sleep(1);
    printf("                                                                        __/ |         __/ |  \n");
    Sleep(1);
    printf("                                                                       |___/         |___/   \n");
	Sleep(1);
}


void splashArt(){
	system("cls");
	gotoxy(2, 2);
	printf(GRN);
	printf("/$$$$$$$$        /$$$$$$  /$$   /$$                                  \n");
    printf("| $$_____/       /$$__  $$| $$  /$$/                                  \n");
    printf("| $$    /$$$$$$ | $$  \\__/| $$ /$$/  /$$$$$$/$$$$   /$$$$$$  /$$$$$$$ \n");
    printf("| $$$$$|____  $$| $$      | $$$$$/  | $$_  $$_  $$ |____  $$| $$__  $$\n");
    printf("| $$__/ /$$$$$$$| $$      | $$  $$  | $$ \\ $$ \\ $$  /$$$$$$$| $$  \\ $$\n");
    printf("| $$   /$$__  $$| $$    $$| $$\\  $$ | $$ | $$ | $$ /$$__  $$| $$  | $$\n");
    printf("| $$  |  $$$$$$$|  $$$$$$/| $$ \\  $$| $$ | $$ | $$|  $$$$$$$| $$  | $$\n");
    printf("|__/   \\_______/ \\______/ |__/  \\__/|__/ |__/ |__/ \\_______/|__/  |__/\n");
}


void printMenu(char options[4][20], int index, int size) {
	int i;
	int len = 0;
	splashArt();
	
    for (i=0;i<size;i++) {
		if (i==index) {
			gotoxy(5 + (i*12) + len - 2, 15);
			
    		printf(BLU"< ");
    		printf(CYN"%s", options[i]);
    		printf(BLU" >");
		}
		else {
			gotoxy(5 + (i*12) + len, 15);
			printf(CYN"%s", options[i]);
		}
    	len += strlen(options[i]);
		gotoxy(0, 0);
	}
}

int selector(char options[4][20], int size) {
	int key;
	int index = 0;
	do {
		key = getch();
		if (key == 0 || key == 224) {
            key = getch(); 
            if (key == 77) {
				if (index < size - 1) {
					index++;
				}
				
			}
			else if (key == 75) {
				if (index > 0) {
					index--;
				}
			}
        } 
		printMenu(options, index, size);
	} while (key != '\r');
	return index;
}

int selectDifficulty(char difficulty[5][20]) {
	printMenu(difficulty, 0, 5);
	return selector(difficulty, 5);
}

int main() {
	// screen size, font, text size SETUP
	keybd_event(VK_F11, 0, 0, 0); // enter fullscreen mode
    keybd_event(VK_F11, 0, KEYEVENTF_KEYUP, 0);
    DWORD dwWidth = GetSystemMetrics(SM_CXSCREEN);
	DWORD dwHeight = GetSystemMetrics(SM_CYSCREEN);
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_FONT_INFOEX fontInfo = {0};
    fontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
    fontInfo.nFont = 0;
    fontInfo.dwFontSize.X = dwWidth/85;
    fontInfo.dwFontSize.Y = dwHeight/32; 
    fontInfo.FontFamily = FF_DONTCARE;
    fontInfo.FontWeight = FW_NORMAL;
    wcscpy_s(fontInfo.FaceName, sizeof(fontInfo.FaceName)/sizeof(wchar_t), L"Lucida Console");

    SetCurrentConsoleFontEx(consoleHandle, FALSE, &fontInfo); // change font family & size
    
	srand(time(0));
	int index = 0, diff = 0;
	char difficulty[5][20] = {"Easy", "Normal", "Hard", "Impossible", "Back"};
	char options[4][20] = {"PLAY", "HIGH SCORE", "EXIT"};
	do {
		printMenu(options, index, 3);
		index = selector(options, 3);
		switch (index) {
			case 0:
	            system("cls");
	            diff = selectDifficulty(difficulty);
	            if (diff == 4) {
	            	break;
				}
				else {
					play(diff);
				}
	            break;
	        case 1:
	            system("cls");
	            displayHighScores();
	            break;
	        case 2:
	        	system("cls");
    			exitArt();
				exit(0);
	            break;
	        default:
	            printf("Invalid choice. Please try again.\n");
		}
	} while (index != 2);	
//    do {
//    	splashArt();
//        printf("\nMenu:\n");
//        printf("1. Play\n");
//        printf("2. High Scores\n");
//        printf("3. Exit\n");
//        printf("Enter your choice: ");
//        scanf("%d", &choice); getchar();
//
//        switch (choice) {
//            
//        }
//    } while (choice != 3);
    
	return 0;
}
