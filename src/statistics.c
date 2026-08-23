#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "statistics.h"


/* =========================================================
   SAFE MENU INPUT
   ========================================================= */

static int getStatisticsChoice(void)
{
    char buffer[100];
    char *end;
    long value;

    while (1)
    {
        printf("\nEnter choice [1 - 4]: ");

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            printf("\n[!] Input error. Please try again.\n");
            continue;
        }

        value = strtol(buffer, &end, 10);

        if (end == buffer)
        {
            printf("\n[!] Invalid input. Enter a number from 1 to 4.\n");
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

        if (value < 1 || value > 4)
        {
            printf("\n[!] Choice must be between 1 and 4.\n");
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
   DISPLAY CONTENTS OF A SAVED TEXT FILE
   ========================================================= */

static void displaySavedFile(
    const char *filename,
    const char *title
)
{
    FILE *file;
    char line[256];

    file = fopen(filename, "r");

    printf("\n");
    printf("+==================================================+\n");
    printf("| %-48s |\n", title);
    printf("+==================================================+\n");

    if (file == NULL)
    {
        printf("\n");
        printf("[!] No saved data was found.\n");
        printf("    Expected file: %s\n", filename);
        printf("\n");
        printf("Run the relevant simulation first and save its\n");
        printf("results before opening this option.\n");

        waitForEnter();
        return;
    }

    printf("\n");

    while (fgets(line, sizeof(line), file) != NULL)
    {
        printf("%s", line);
    }

    fclose(file);

    waitForEnter();
}


/* =========================================================
   STATISTICS MENU
   ========================================================= */

void statisticsMenu(void)
{
    int choice;

    while (1)
    {
        printf("\n");
        printf("====================================================\n");
        printf("              SIMULATION STATISTICS\n");
        printf("====================================================\n");

        printf("\n");
        printf("1. View Initial Conditions\n");
        printf("2. View Final Conditions\n");
        printf("3. View Battle Results\n");
        printf("4. Return to Main Menu\n");

        choice = getStatisticsChoice();

        switch (choice)
        {
            case 1:
                /*
                 * Your current simulation already creates
                 * initial_conditions.txt.
                 */
                displaySavedFile(
                    "initial_conditions.txt",
                    "INITIAL CONDITIONS"
                );
                break;

            case 2:
                /*
                 * Use this filename when you implement the
                 * assignment's final-condition saving stage.
                 */
                displaySavedFile(
                    "final_conditions.txt",
                    "FINAL CONDITIONS"
                );
                break;

            case 3:
                /*
                 * Use this filename for battle result details,
                 * hit information, attack order, etc.
                 */
                displaySavedFile(
                    "battle_results.txt",
                    "BATTLE RESULTS"
                );
                break;

            case 4:
                return;
        }
    }
}
