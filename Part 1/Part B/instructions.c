#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "instructions.h"


/* =========================================================
   SAFE MENU INPUT
   ========================================================= */

static int getInstructionChoice(void)
{
    char buffer[100];
    char *end;
    long value;

    while (1)
    {
        printf("\nEnter choice [1 - 5]: ");

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            printf("\n[!] Input error. Please try again.\n");
            continue;
        }

        value = strtol(buffer, &end, 10);

        if (end == buffer)
        {
            printf("\n[!] Invalid input. Enter a number from 1 to 5.\n");
            continue;
        }

        while (isspace((unsigned char)*end))
        {
            end++;
        }

        if (*end != '\0')
        {
            printf("\n[!] Enter numbers only.\n");
            continue;
        }

        if (value < 1 || value > 5)
        {
            printf("\n[!] Choice must be between 1 and 5.\n");
            continue;
        }

        return (int)value;
    }
}


/* =========================================================
   WAIT FOR ENTER
   ========================================================= */

static void waitForEnter(void)
{
    char buffer[100];

    printf("\nPress ENTER to continue...");
    fgets(buffer, sizeof(buffer), stdin);
}


/* =========================================================
   OBJECTIVE
   ========================================================= */

static void showObjective(void)
{
    printf("\n");
    printf("+==================================================+\n");
    printf("|               SIMULATOR OBJECTIVE                |\n");
    printf("+==================================================+\n");

    printf("\n");
    printf("The simulation contains one Battleship (B) and a\n");
    printf("number of Escort Ships (E) on a square battlefield.\n");

    printf("\n");
    printf("The Battleship belongs to the Allied side while the\n");
    printf("Escort Ships belong to the Axis side.\n");

    printf("\n");
    printf("The main objective of the Battleship is to destroy\n");
    printf("as many Escort Ships as possible while minimizing\n");
    printf("the damage received from enemy attacks.\n");

    waitForEnter();
}


/* =========================================================
   SETUP HELP
   ========================================================= */

static void showSetupHelp(void)
{
    printf("\n");
    printf("+==================================================+\n");
    printf("|                 SIMULATION SETUP                 |\n");
    printf("+==================================================+\n");

    printf("\n");
    printf("When Start Simulation is selected, the program asks\n");
    printf("for the initial battlefield conditions.\n");

    printf("\n");
    printf("You will configure:\n");
    printf("  1. Battleship type\n");
    printf("  2. Battlefield / canvas size\n");
    printf("  3. Number of Escort Ships\n");
    printf("  4. Battleship X coordinate\n");
    printf("  5. Battleship Y coordinate\n");
    printf("  6. Random number generator seed\n");

    printf("\n");
    printf("The battlefield is square and begins at coordinate\n");
    printf("(0, 0). The selected X and Y coordinates must remain\n");
    printf("inside the battlefield.\n");

    printf("\n");
    printf("Escort Ship positions and types may be randomly\n");
    printf("generated during battlefield creation.\n");

    waitForEnter();
}


/* =========================================================
   SHIP INFORMATION
   ========================================================= */

static void showShipInformation(void)
{
    printf("\n");
    printf("+==================================================+\n");
    printf("|                  SHIP TYPES                      |\n");
    printf("+==================================================+\n");

    printf("\n");
    printf("BATTLESHIPS\n");
    printf("----------------------------------------------------\n");
    printf("U  - USS Iowa (BB-61)\n");
    printf("M  - MS King George V\n");
    printf("R  - Richelieu\n");
    printf("S  - Sovetsky Soyuz-class\n");

    printf("\n");
    printf("ESCORT SHIPS\n");
    printf("----------------------------------------------------\n");
    printf("EA - 1936A-class Destroyer\n");
    printf("EB - Gabbiano-class Corvette\n");
    printf("EC - Matsu-class Destroyer\n");
    printf("ED - F-class Escort Ship\n");
    printf("EE - Japanese Kaibokan\n");

    printf("\n");
    printf("Each Escort Ship is assigned its own identifier so\n");
    printf("that it can be tracked during the simulation.\n");

    waitForEnter();
}


/* =========================================================
   INPUT HELP
   ========================================================= */

static void showInputHelp(void)
{
    printf("\n");
    printf("+==================================================+\n");
    printf("|                  INPUT HELP                      |\n");
    printf("+==================================================+\n");

    printf("\n");
    printf("All numeric menu inputs must be whole numbers.\n");

    printf("\n");
    printf("Examples:\n");
    printf("  Valid   : 1, 2, 25, 100\n");
    printf("  Invalid : abc, 2.5, 20abc, @#$\n");

    printf("\n");
    printf("For confirmation questions, enter only:\n");
    printf("  Y - Yes\n");
    printf("  N - No\n");

    printf("\n");
    printf("If an invalid value is entered, the program will\n");
    printf("display an error and request the value again.\n");

    waitForEnter();
}


/* =========================================================
   INSTRUCTIONS MENU
   ========================================================= */

void instructionsMenu(void)
{
    int choice;

    while (1)
    {
        printf("\n");
        printf("====================================================\n");
        printf("                 INSTRUCTIONS\n");
        printf("====================================================\n");

        printf("\n");
        printf("1. Simulator Objective\n");
        printf("2. Simulation Setup Guide\n");
        printf("3. Battleship and Escort Ship Types\n");
        printf("4. Input and Control Help\n");
        printf("5. Return to Main Menu\n");

        choice = getInstructionChoice();

        switch (choice)
        {
            case 1:
                showObjective();
                break;

            case 2:
                showSetupHelp();
                break;

            case 3:
                showShipInformation();
                break;

            case 4:
                showInputHelp();
                break;

            case 5:
                return;
        }
    }
}
