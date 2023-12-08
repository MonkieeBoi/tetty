#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ncurses.h>

#define BOARD_HEIGHT 20
#define BOARD_WIDTH 10
#define SPAWN_X 4
#define SPAWN_Y 0

typedef struct Node {
    unsigned char row[BOARD_WIDTH];
    struct Node *next;
} Node;

struct piece {
    unsigned char x;
    unsigned char y;
    unsigned char type;
    unsigned char rotation;
};

// TODO: figure out better way to store this
// Defined by offset from the piece center
// 7 pieces, 4 rotations, 3 coordinate pairs
const int pieces[7][4][3][2] = {
    // I
    {
        {{-1, 0}, {1, 0}, {2, 0}},
         // []<>[][]
        {{0, 1}, {0, -1}, {0, -2}},
        // []
        // <>
        // []
        // []
        {{-2, 0}, {-1, 0}, {1, 0}},
         // [][]<>[]
        {{0, 2}, {0, 1}, {0, -1}},
        // []
        // []
        // <>
        // []
    },
    // J
    {
        {{-1, 1}, {-1, 0}, {1, 0}},
         // []
         // []<>[]
        {{0, 1}, {1, 1}, {0, -1}},
         // [][]
         // <>
         // []
        {{-1, 0}, {1, 0}, {1, -1}},
         // []<>[]
         //     []
        {{0, 1}, {-1, -1}, {0, -1}},
         //   []
         //   <>
         // [][]
    },
    // L
    {
        {{1, 1}, {-1, 0}, {1, 0}},
        //     []
        // []<>[]
        {{0, 1}, {0, -1}, {1, -1}},
        // []
        // <>
        // [][]
        {{-1, 0}, {1, 0}, {-1, -1}},
        // []<>[]
        // []
        {{-1, 1}, {0, 1}, {0, -1}},
        // [][]
        //   <>
        //   []
    },
    // O
    {
        {{0, 1}, {1, 1}, {1, 0}},
        // [][]
        // <>[]
        {{1, 0}, {0, -1}, {1, -1}},
        // <>[]
        // [][]
        {{-1, 0}, {-1, -1}, {0, -1}},
        // []<>
        // [][]
        {{-1, 1}, {0, 1}, {-1, 0}}
        // [][]
        // []<>
    },
    // S
    {
        {{0, 1}, {1, 1}, {-1, 0}},
        //   [][]
        // []<>
        {{0, 1}, {1, 0}, {1, -1}},
        // []
        // <>[]
        //   []
        {{1, 0}, {-1, -1}, {0, -1}},
        //   <>[]
        // [][]
        {{-1, 1}, {-1, 0}, {0, -1}}
        // []
        // []<>
        //   []
    },
    // T
    {
        {{0, 1}, {-1, 0}, {1, 0}},
        //   []
        // []<>[]
        {{0, 1}, {1, 0}, {0, -1}},
        // []
        // <>[]
        // []
        {{-1, 0}, {1, 0}, {0, -1}},
        // []<>[]
        //   []
        {{0, 1}, {-1, 0}, {0, -1}}
        //   []
        // []<>
        //   []
    },
    // Z
    {
        {{-1, 1}, {0, 1}, {1, 0}},
        // [][]
        //   <>[]
        {{1, 1}, {1, 0}, {0, -1}},
        //   []
        // <>[]
        // []
        {{-1, 0}, {0, -1}, {1, -1}},
        // []<>
        //   [][]
        {{0, 1}, {-1, 0}, {-1, -1}}
        //   []
        // []<>
        // []
    },
};

int count_nodes(Node *n) {
    int length = 0;
    while (n != NULL) {
        length++;
        n = n->next;
    }
    return length;
}

void draw_board(Node *n, struct piece *p) {
    int height = count_nodes(n);
    printf("\n\n");
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        printf("%02d ┃", i);
        for (int j = 0; j < BOARD_WIDTH; j++) {
            unsigned char found = 0;
            for (int k = 0; k < 3; k++) {
                if (p->x + pieces[p->type][p->rotation][k][0] == j &&
                    p->y - pieces[p->type][p->rotation][k][1] == i) {
                    printf("[]");
                    found = 1;
                    break;
                }
            }
            if (found)
                continue;
            if (BOARD_HEIGHT - height <= i && n != NULL && n->row[j]) {
                printf("[]");
                n = n->next;
            } else if (p->x == j && p->y == i)
                printf("()");
            else
                printf("  ");
        }
        printf("┃\n");
    }
    printf("   ┗━━━━━━━━━━━━━━━━━━━━┛\n");
}

void print_pieces() {
    for (int i = 0; i < 7; i++) {
        printf("━━━━━ ");
        switch (i) {
            case 1: printf("I"); break;
            case 2: printf("J"); break;
            case 3: printf("L"); break;
            case 4: printf("O"); break;
            case 5: printf("S"); break;
            case 6: printf("T"); break;
            case 7: printf("Z"); break;
        }
        printf(" ━━━━━\n");

        for (int j = 0; j < 4; j++) {
            for (int y = 4; y != -1; y--) {
                for (int x = 0; x < 5; x++) {
                    if (x == 2 && y == 2) {
                        printf("<>");
                        continue;
                    }
                    int found = 0;
                    for (int k = 0; k < 3; k++) {
                        if (2 + pieces[i][j][k][0] == x && 2 + pieces[i][j][k][1] == y) {
                            printf("[]");
                            found = 1;
                        }
                    }
                    if (!found)
                        printf("  ");
                }
                printf("\n");
            }
        }
    }
}

void queue_pop(struct piece *p, int queue[], int queue_pos) {
    p->type = queue[queue_pos];
    p->x = SPAWN_X;
    p->y = SPAWN_Y;
    p->rotation = 0;
    int rand = random() % 7;

    while (queue_pos != 0) {
        int dup = 0;
        rand = random() % 7;
        for (int i = 0; i < queue_pos; i++) {
            if (rand == queue[i])
                dup = 1;
        }
        if (!dup)
            break;
    }

    queue[queue_pos] = rand;
}

void queue_init (int queue[]) {
    for (int i = 0; i < 7; i++) {
        int rand = random() % 7;
        while (i != 0) {
            int dup = 0;
            rand = random() % 7;
            for (int j = 0; j < i; j++) {
                if (rand == queue[j])
                    dup = 1;
            }
            if (!dup)
                break;
        }
        queue[i] = rand;
    }
}

int main() {
    Node *board = malloc(sizeof(Node));
    board->next = NULL;
    struct piece *curr = malloc(sizeof(struct piece));

    srandom(time(NULL));

    int queue[7];
    queue_init(queue);

    int queue_pos = 1;
    queue_pop(curr, queue, 0);

    // Game Loop

    draw_board(board, curr);
    while (1) {
        printf("\e[1;1H\e[2J");
        draw_board(board, curr);
        usleep(500 * 1000);
        curr->y = (curr->y + 1) % BOARD_HEIGHT;
    }

    return 0;
}
