#include <stdio.h>
#include <stdlib.h>
#include "simulation.h"


int choice, canvas_S, no_escort_ships, x_S, y_S, C_H, C_W, ran;
char gen_ship, status;


/* Generate random coordinates */
void randomccordinate(int random, int x, int y, int *x_ncordinate, int *y_ncordinate)
{
    srand(random);

    *x_ncordinate = rand() % (x + 1);
    *y_ncordinate = rand() % (y + 1);
}


/* Clear screen */
void clearScreen()
{
    // \e[1;1H moves the cursor to row 1, column 1
    // \e[2J clears the entire screen
    printf("\e[1;1H\e[2J");
}


/* Save initial conditions to TXT file */
void saveInitialConditions()
{
    FILE *file;

    file = fopen("initial_conditions.txt", "w");

    if (file == NULL)
    {
        printf("\nError: Could not create initial_conditions.txt\n");
        return;
    }

    fprintf(file, "===============================================\n");
    fprintf(file, "              INITIAL CONDITIONS\n");
    fprintf(file, "===============================================\n\n");

    /* Save battleship type */
    fprintf(file, "Battleship Type   : ");

    switch (choice)
    {
        case 1:
            fprintf(file, "USS Iowa BB-61 (U)\n");
            break;

        case 2:
            fprintf(file, "King George V (M)\n");
            break;

        case 3:
            fprintf(file, "Richelieu (R)\n");
            break;

        case 4:
            fprintf(file, "Sovetsky Soyuz-class (S)\n");
            break;

        default:
            fprintf(file, "Unknown\n");
            break;
    }

    fprintf(file, "Battleship Choice : %d\n", choice);
    fprintf(file, "Canvas Size       : %d\n", canvas_S);
    fprintf(file, "Number of Escorts : %d\n", no_escort_ships);
    fprintf(file, "Battleship X      : %d\n", x_S);
    fprintf(file, "Battleship Y      : %d\n", y_S);
    fprintf(file, "Seed Value        : %d\n", ran);

    fprintf(file, "\n===============================================\n");

    fclose(file);

    printf("\nInitial conditions saved to initial_conditions.txt\n");
}


/* Main simulation */
void simulation(void)
{
    printf("===============================================");
    printf("\n                 Intial-Setup\n");
    printf("===============================================\n");

    printf("\nBattleship Types:\n");

    printf("\n1. USS Iowa BB-61 (U)\n");
    printf("2. King George V (M)\n");
    printf("3. Richelieu (R)\n");
    printf("4. Sovetsky Soyuz-class (S)\n");

    printf("\nEnter choice:(Plesae Enter a number from 1 to 4) : ");
    scanf("%d", &choice);

    printf("\nCanvas Size:");
    scanf("%d", &canvas_S);

    /*
    printf("\nHeight:");
    scanf("%d", &C_H);

    printf("\nWidth:");
    scanf("%d", &C_W);
    */

    printf("\nNumber of Escort Ships:(Please enter a postive whole number) ");
    scanf("%d", &no_escort_ships);

    printf("\nBattleship Position:(Please eneter the character coordinates in x and y axis)");

    printf("\nX:");
    scanf("%d", &x_S);

    printf("\nY:");
    scanf("%d", &y_S);

    printf("\nSeed Value: ");
    scanf("%d", &ran);


    while (1)
    {
        printf("\nGenerate Battle Field? (Y/N) ");
        scanf(" %c", &gen_ship);


        if (gen_ship == 'n' || gen_ship == 'N')
        {
            printf("\nDo u want to exit the simulation? (Y/N) ");
            scanf(" %c", &status);

            if (status == 'Y' || status == 'y')
            {
                return;
            }
        }


        else if (gen_ship == 'y' || gen_ship == 'Y')
        {
            printf("\nGenerating Battle Field...");

            /* Save all initial conditions */
            saveInitialConditions();

            return;
        }


        else
        {
            printf("\nThe Entered input is wrong\n");
            continue;
        }
    }
}


/*
clearScreen();


printf("=");

for (int x = 0; x < C_W; x++)
{
    printf("=");
}

printf("=\n");

printf("            BATTLEFIELD\n");

printf("=");

for (int x = 0; x < C_W; x++)
{
    printf("=");
}

printf("=\n");


char canvas[C_H][C_W];
char escorts[gen_ship][gen_ship];

canvas[y_S][x_S] = 'B';


for (int y = 0; y < C_H; y++)
{
    for (int x = 0; x <= C_W; x++)
    {
        if (canvas[y][x] == 'B')
        {
            continue;
        }
        else
        {
            canvas[y][x] = ' ';
        }
    }
}


// Border creation

printf("\n+");

for (int x = 0; x < C_W; x++)
{
    printf("-");
}

printf("+\n");


for (int y = 1; y < C_H; y++)
{
    for (int x = 0; x <= C_W + 1; x++)
    {
        if (x == 0)
        {
            printf("|");
        }
        else if (x == (C_W + 1))
        {
            printf("|\n");
        }
        else
        {
            printf("%c", canvas[y][x]);
        }
    }
}


printf("+");

for (int x = 0; x < C_W; x++)
{
    printf("-");
}

printf("+\n");


printf("B = Battleship\n");
printf("E = Escort Ship\n");
*/