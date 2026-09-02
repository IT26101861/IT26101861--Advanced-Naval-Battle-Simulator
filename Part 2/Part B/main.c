#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "simulation.h"
#include "instructions.h"
#include "statistics.h"

static void printTitle(void)
{
    printf("\n");
    printf("=====================================================================\n");
    printf("\n");

    printf("███╗   ██╗ █████╗ ██╗   ██╗ █████╗ ██╗\n");
    printf("████╗  ██║██╔══██╗██║   ██║██╔══██╗██║\n");
    printf("██╔██╗ ██║███████║██║   ██║███████║██║\n");
    printf("██║╚██╗██║██╔══██║╚██╗ ██╔╝██╔══██║██║\n");
    printf("██║ ╚████║██║  ██║ ╚████╔╝ ██║  ██║██║\n");
    printf("╚═╝  ╚═══╝╚═╝  ╚═╝  ╚═══╝  ╚═╝  ╚═╝╚═╝\n");

    printf("\n");

    printf(" ███████╗██╗███╗   ███╗██╗   ██╗██╗      █████╗ ████████╗ ██████╗ ██████╗\n");
    printf(" ██╔════╝██║████╗ ████║██║   ██║██║     ██╔══██╗╚══██╔══╝██╔═══██╗██╔══██╗\n");
    printf(" ███████╗██║██╔████╔██║██║   ██║██║     ███████║   ██║   ██║   ██║██████╔╝\n");
    printf(" ╚════██║██║██║╚██╔╝██║██║   ██║██║     ██╔══██║   ██║   ██║   ██║██╔══██╗\n");
    printf(" ███████║██║██║ ╚═╝ ██║╚██████╔╝███████╗██║  ██║   ██║   ╚██████╔╝██║  ██║\n");
    printf(" ╚══════╝╚═╝╚═╝     ╚═╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝\n");

    printf("\n");
    printf("                         [ IT26101861 ]\n");
    printf("\n");
}




static void printMainMenu(void)
{
    printf("===============================================\n");
    printf("      NAVAL BATTLE SIMULATOR - IT26101861\n");
    printf("===============================================\n");

    printf("\n");
    printf("1. Start Simulation\n");
    printf("2. View Instructions\n");
    printf("3. Simulation Statistics\n");
    printf("4. Exit\n");
}



static int getMenuChoice(const char *prompt, int min, int max)
{
    char buffer[100];
    char *end;
    long value;

    while (1)
    {
        printf("%s", prompt);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            printf("\n[!] Input error. Please try again.\n");
            continue;
        }

        value = strtol(buffer, &end, 10);

        if (end == buffer)
        {
            printf("\n[!] Invalid input. Enter a whole number from %d to %d.\n\n",
                   min, max);
            continue;
        }

        while (isspace((unsigned char)*end))
        {
            end++;
        }

        if (*end != '\0')
        {
            printf("\n[!] Unnecessary characters detected.\n");
            printf("    Enter only a whole number from %d to %d.\n\n",
                   min, max);
            continue;
        }

        if (value < min || value > max)
        {
            printf("\n[!] Choice must be between %d and %d.\n\n",
                   min, max);
            continue;
        }

        return (int)value;
    }
}




static char getYesNo(const char *prompt)
{
    char buffer[100];
    int i;
    char answer;

    while (1)
    {
        printf("%s", prompt);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            printf("\n[!] Input error. Please try again.\n");
            continue;
        }

        i = 0;

        while (isspace((unsigned char)buffer[i]))
        {
            i++;
        }

        answer = buffer[i];

        if (answer != 'Y' &&
            answer != 'y' &&
            answer != 'N' &&
            answer != 'n')
        {
            printf("\n[!] Please enter only Y or N.\n\n");
            continue;
        }

        i++;

        while (isspace((unsigned char)buffer[i]))
        {
            i++;
        }

        if (buffer[i] != '\0')
        {
            printf("\n[!] Please enter only Y or N.\n\n");
            continue;
        }

        if (answer == 'Y' || answer == 'y')
        {
            return 'Y';
        }

        return 'N';
    }
}




int main(void)
{
    int choice;
    char exitChoice;

    printTitle();

    while (1)
    {
        printMainMenu();

        choice = getMenuChoice(
            "\nEnter choice [1 - 4]: ",
            1,
            4
        );

        switch (choice)
        {
           

            case 1:
                simulation();
                printf("\n");
                break;


            

            case 2:
                instructionsMenu();
                printf("\n");
                break;


      

            case 3:
                statisticsMenu();
                printf("\n");
                break;


            

            case 4:
                exitChoice = getYesNo(
                    "\nAre you sure you want to exit? [Y/N]: "
                );

                if (exitChoice == 'Y')
                {
                    printf("\n");
                    printf("+==================================================+\n");
                    printf("|        THANK YOU FOR USING NAVAL SIMULATOR       |\n");
                    printf("+==================================================+\n");

                    return 0;
                }

                printf("\nReturning to Main Menu...\n\n");
                break;
        }
    }
}
