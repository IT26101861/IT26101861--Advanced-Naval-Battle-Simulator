#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "statistics.h"

static int getChoice(void)
{
    char buffer[100];
    char *end;
    long value;

    while (1)
    {
        printf("\nEnter choice [1 - 4]: ");

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            printf("\n[!] Input error.\n");
            continue;
        }

        value = strtol(buffer, &end, 10);

        if (end == buffer)
        {
            printf("\n[!] Enter a number from 1 to 4.\n");
            continue;
        }

        while (isspace((unsigned char)*end))
            end++;

        if (*end != '\0' || value < 1 || value > 4)
        {
            printf("\n[!] Enter a number from 1 to 4.\n");
            continue;
        }

        return (int)value;
    }
}

static void waitForEnter(void)
{
    char buffer[10];

    printf("\nPress ENTER to continue...");
    fgets(buffer, sizeof(buffer), stdin);
}

static void displayFile(const char *filename, const char *title)
{
    FILE *file;
    char line[300];

    printf("\n============================================================\n");
    printf("                     %s\n", title);
    printf("============================================================\n\n");

    file = fopen(filename, "r");

    if (file == NULL)
    {
        printf("No saved data found in %s\n", filename);
        waitForEnter();
        return;
    }

    while (fgets(line, sizeof(line), file) != NULL)
        printf("%s", line);

    fclose(file);
    waitForEnter();
}

void statisticsMenu(void)
{
    int choice;

    while (1)
    {
        printf("\n");
        printf("============================================================\n");
        printf("                  SIMULATION STATISTICS\n");
        printf("============================================================\n");
        printf("1. View Simulation History\n");
        printf("2. View Latest Initial Conditions\n");
        printf("3. View Latest Final Conditions\n");
        printf("4. Return to Main Menu\n");

        choice = getChoice();

        switch (choice)
        {
            case 1:
                displayFile(
                    "simulation_history.txt",
                    "SIMULATION HISTORY"
                );
                break;

            case 2:
                displayFile(
                    "initial_conditions.txt",
                    "LATEST INITIAL CONDITIONS"
                );
                break;

            case 3:
                displayFile(
                    "final_conditions.txt",
                    "LATEST FINAL CONDITIONS"
                );
                break;

            case 4:
                return;
        }
    }
}
