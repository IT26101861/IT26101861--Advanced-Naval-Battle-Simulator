#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#include "simulation.h"

#define MIN_CANVAS 10
#define MAX_CANVAS 100
#define MIN_ESCORTS 1
#define MAX_ESCORTS 50
#define MIN_SEED 1
#define MAX_SEED 999999
#define MIN_BATTLESHIP_VMAX 1
#define MAX_BATTLESHIP_VMAX 1000

typedef struct
{
    const char *notation;
    const char *name;
    const char *gunName;
    double impactPower;
    int angleRange;
} EscortTypeInfo;

typedef struct
{
    int id;
    const EscortTypeInfo *type;
    int x;
    int y;
    double minVelocity;
    double maxVelocity;
    int minAngle;
    int maxAngle;
} EscortShip;

static const EscortTypeInfo escortTypes[5] =
{
    {"EA", "1936A-class Destroyer", "SK C/34 naval gun", 0.08, 20},
    {"EB", "Gabbiano-class Corvette", "L/47 dual-purpose gun", 0.06, 30},
    {"EC", "Matsu-class Destroyer", "Type 89 dual-purpose gun", 0.07, 25},
    {"ED", "F-class Escort Ship", "SK C/32 naval gun", 0.05, 50},
    {"EE", "Japanese Kaibokan", "(4.7 inch) naval guns", 0.04, 70}
};

static int battleshipChoice;
static int canvasSize;
static int numberOfEscorts;
static int battleshipX;
static int battleshipY;
static int seedValue;

static double battleshipMinVelocity = 0.0;
static double battleshipMaxVelocity;

static int battleshipMinAngle = 0;
static int battleshipMaxAngle = 90;

static EscortShip escorts[MAX_ESCORTS];

static char generateChoice;
static char exitChoice;

static void clearScreen(void)
{
    printf("\033[1;1H\033[2J");
}

static void printBattleshipArt(void)
{
    printf("\n");
    printf("                    |\\\n");
    printf("                    | \\\n");
    printf("              _____/|__\\_____\n");
    printf("        _____/______________  \\____\n");
    printf("   ____/___________________________\\____\n");
    printf("  |_____________________________________|\n");
    printf("    \\_________________________________/\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("          ADVANCED NAVAL SIMULATOR\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
}

static void printSection(const char *title)
{
    printf("\n");
    printf("+------------------------------------------------------------+\n");
    printf("| %-58s |\n", title);
    printf("+------------------------------------------------------------+\n");
}

static void discardLongInput(void)
{
    char discard[100];

    while (fgets(discard, sizeof(discard), stdin) != NULL)
    {
        int i = 0;

        while (discard[i] != '\0')
        {
            if (discard[i] == '\n')
            {
                return;
            }

            i++;
        }
    }
}

static void integerError(int min, int max)
{
    printf("\n");
    printf("  [!] INVALID INPUT\n");
    printf("  ----------------------------------------------------------\n");
    printf("  Enter a whole number between %d and %d.\n", min, max);
    printf("  Do not enter letters, decimals, or symbols.\n");
    printf("  ----------------------------------------------------------\n\n");
}

static int getInteger(const char *message, int min, int max)
{
    char buffer[100];
    char *end;
    long number;

    while (1)
    {
        int hasNewline = 0;
        int i = 0;

        printf("%s", message);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            printf("\n[!] Input error. Please try again.\n");
            continue;
        }

        while (buffer[i] != '\0')
        {
            if (buffer[i] == '\n')
            {
                hasNewline = 1;
                break;
            }

            i++;
        }

        if (!hasNewline)
        {
            discardLongInput();
            printf("\n[!] Input is too long.\n");
            integerError(min, max);
            continue;
        }

        errno = 0;
        number = strtol(buffer, &end, 10);

        if (end == buffer)
        {
            integerError(min, max);
            continue;
        }

        if (errno == ERANGE || number < INT_MIN || number > INT_MAX)
        {
            printf("\n[!] Number is too large.\n");
            integerError(min, max);
            continue;
        }

        while (isspace((unsigned char)*end))
        {
            end++;
        }

        if (*end != '\0')
        {
            printf("\n[!] Unnecessary characters detected.\n");
            integerError(min, max);
            continue;
        }

        if (number < min || number > max)
        {
            printf("\n[!] Value must be between %d and %d.\n", min, max);
            continue;
        }

        return (int)number;
    }
}

static char getYesNo(const char *message)
{
    char buffer[100];

    while (1)
    {
        char answer;
        int i = 0;
        int hasNewline = 0;

        printf("%s", message);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            printf("\n[!] Input error. Please try again.\n");
            continue;
        }

        while (buffer[i] != '\0')
        {
            if (buffer[i] == '\n')
            {
                hasNewline = 1;
                break;
            }

            i++;
        }

        if (!hasNewline)
        {
            discardLongInput();
            printf("\n[!] Input is too long. Enter only Y or N.\n");
            continue;
        }

        i = 0;

        while (isspace((unsigned char)buffer[i]))
        {
            i++;
        }

        answer = buffer[i];

        if (answer != 'Y' && answer != 'y' &&
            answer != 'N' && answer != 'n')
        {
            printf("\n[!] Please enter only Y or N.\n");
            continue;
        }

        i++;

        while (isspace((unsigned char)buffer[i]))
        {
            i++;
        }

        if (buffer[i] != '\0')
        {
            printf("\n[!] Please enter only Y or N.\n");
            continue;
        }

        if (answer == 'Y' || answer == 'y')
        {
            return 'Y';
        }

        return 'N';
    }
}

static const char *getBattleshipName(void)
{
    switch (battleshipChoice)
    {
        case 1: return "USS Iowa (BB-61)";
        case 2: return "MS King George V";
        case 3: return "Richelieu";
        case 4: return "Sovetsky Soyuz-class";
        default: return "Unknown";
    }
}

static char getBattleshipNotation(void)
{
    switch (battleshipChoice)
    {
        case 1: return 'U';
        case 2: return 'M';
        case 3: return 'R';
        case 4: return 'S';
        default: return '?';
    }
}

static const char *getBattleshipGunName(void)
{
    switch (battleshipChoice)
    {
        case 1: return "50-caliber Mark 7 gun";
        case 2: return "(356 mm) Mark VII gun";
        case 3: return "(15 inch) Mle 1935 gun";
        case 4: return "(16 inch) B-37 gun";
        default: return "Unknown";
    }
}

static void showBattleshipMenu(void)
{
    printSection("SELECT BATTLESHIP");

    printf("\n");
    printf("   ID   NOTATION   BATTLESHIP\n");
    printf("  --------------------------------------------------------\n");
    printf("   1       U       USS Iowa (BB-61)\n");
    printf("   2       M       MS King George V\n");
    printf("   3       R       Richelieu\n");
    printf("   4       S       Sovetsky Soyuz-class\n");
    printf("  --------------------------------------------------------\n");
}

static int randomInt(int min, int max)
{
    if (max <= min)
    {
        return min;
    }

    return min + rand() % (max - min + 1);
}

static int positionIsOccupied(int x, int y, int generatedCount)
{
    int i;

    if (x == battleshipX && y == battleshipY)
    {
        return 1;
    }

    for (i = 0; i < generatedCount; i++)
    {
        if (escorts[i].x == x && escorts[i].y == y)
        {
            return 1;
        }
    }

    return 0;
}

static void generateEscortShips(void)
{
    int i;

    srand((unsigned int)seedValue);

    for (i = 0; i < numberOfEscorts; i++)
    {
        int typeIndex;
        int x;
        int y;
        double maxV;
        double minV;
        int minAngle;
        int maxAngle;

        escorts[i].id = i + 1;

        typeIndex = randomInt(0, 4);
        escorts[i].type = &escortTypes[typeIndex];

        do
        {
            x = randomInt(0, canvasSize);
            y = randomInt(0, canvasSize);

        } while (positionIsOccupied(x, y, i));

        escorts[i].x = x;
        escorts[i].y = y;

        if (typeIndex == 0)
        {
            maxV = 1.2 * battleshipMaxVelocity;
        }
        else
        {
            int upper = (int)battleshipMaxVelocity - 1;

            if (upper < 1)
            {
                upper = 1;
            }

            maxV = (double)randomInt(1, upper);
        }

        if (maxV <= 1.0)
        {
            minV = 0.0;
        }
        else
        {
            minV = (double)randomInt(0, (int)maxV - 1);
        }

        escorts[i].minVelocity = minV;
        escorts[i].maxVelocity = maxV;

        minAngle = randomInt(0, 90 - escorts[i].type->angleRange);
        maxAngle = minAngle + escorts[i].type->angleRange;

        escorts[i].minAngle = minAngle;
        escorts[i].maxAngle = maxAngle;
    }
}

static void showConfiguration(void)
{
    printSection("SIMULATION CONFIGURATION");

    printf("\n");
    printf("  Battleship            : %s [%c]\n",
           getBattleshipName(),
           getBattleshipNotation());

    printf("  Battleship Gun        : %s\n", getBattleshipGunName());
    printf("  Battleship Position   : (%d, %d)\n", battleshipX, battleshipY);
    printf("  Battleship Vmin       : %.2f m/s\n", battleshipMinVelocity);
    printf("  Battleship Vmax       : %.2f m/s\n", battleshipMaxVelocity);
    printf("  Battleship Angle      : %d - %d degrees\n",
           battleshipMinAngle,
           battleshipMaxAngle);

    printf("\n");
    printf("  Canvas                : (0,0) to (%d,%d)\n",
           canvasSize,
           canvasSize);

    printf("  Number of Escorts     : %d\n", numberOfEscorts);
    printf("  Random Seed           : %d\n", seedValue);
}

static void showEscortShips(void)
{
    int i;

    printSection("GENERATED ESCORT SHIPS");

    printf("\n");
    printf("%-6s %-5s %-12s %-10s %-10s %-9s %-9s %-8s\n",
           "ID", "TYPE", "POSITION", "Vmin", "Vmax",
           "ANG-MIN", "ANG-MAX", "IMPACT");

    printf("--------------------------------------------------------------------------\n");

    for (i = 0; i < numberOfEscorts; i++)
    {
        char idText[16];
        char positionText[32];

        snprintf(idText, sizeof(idText), "E%03d", escorts[i].id);
        snprintf(positionText, sizeof(positionText), "(%d,%d)",
                 escorts[i].x, escorts[i].y);

        printf("%-6s %-5s %-12s %-10.2f %-10.2f %-9d %-9d %-8.2f\n",
               idText,
               escorts[i].type->notation,
               positionText,
               escorts[i].minVelocity,
               escorts[i].maxVelocity,
               escorts[i].minAngle,
               escorts[i].maxAngle,
               escorts[i].type->impactPower);
    }

    printf("\n");
    printf("Escort type names:\n");
    printf(" EA = 1936A-class Destroyer\n");
    printf(" EB = Gabbiano-class Corvette\n");
    printf(" EC = Matsu-class Destroyer\n");
    printf(" ED = F-class Escort Ship\n");
    printf(" EE = Japanese Kaibokan\n");
}

static void saveInitialConditions(void)
{
    FILE *file;
    int i;

    file = fopen("initial_conditions.txt", "w");

    if (file == NULL)
    {
        printf("\n[!] Could not create initial_conditions.txt\n");
        return;
    }

    fprintf(file, "=============================================================\n");
    fprintf(file, "                    INITIAL CONDITIONS\n");
    fprintf(file, "=============================================================\n\n");

    fprintf(file, "[ BATTLESHIP ]\n");
    fprintf(file, "-------------------------------------------------------------\n");
    fprintf(file, "Type / Name       : %s\n", getBattleshipName());
    fprintf(file, "Notation          : %c\n", getBattleshipNotation());
    fprintf(file, "Gun               : %s\n", getBattleshipGunName());
    fprintf(file, "Position          : (%d, %d)\n", battleshipX, battleshipY);
    fprintf(file, "Minimum Velocity  : %.2f m/s\n", battleshipMinVelocity);
    fprintf(file, "Maximum Velocity  : %.2f m/s\n", battleshipMaxVelocity);
    fprintf(file, "Minimum Angle     : %d degrees\n", battleshipMinAngle);
    fprintf(file, "Maximum Angle     : %d degrees\n", battleshipMaxAngle);

    fprintf(file, "\n[ BATTLEFIELD ]\n");
    fprintf(file, "-------------------------------------------------------------\n");
    fprintf(file, "Lower-left        : (0, 0)\n");
    fprintf(file, "Upper-right       : (%d, %d)\n", canvasSize, canvasSize);
    fprintf(file, "Canvas D          : %d\n", canvasSize);
    fprintf(file, "Number of Escorts : %d\n", numberOfEscorts);
    fprintf(file, "Seed Value        : %d\n", seedValue);

    fprintf(file, "\n[ ESCORT SHIPS ]\n");
    fprintf(file, "-------------------------------------------------------------\n");

    for (i = 0; i < numberOfEscorts; i++)
    {
        fprintf(file, "\nEscort ID         : E%03d\n", escorts[i].id);
        fprintf(file, "Type / Notation   : %s\n", escorts[i].type->notation);
        fprintf(file, "Type Name         : %s\n", escorts[i].type->name);
        fprintf(file, "Gun               : %s\n", escorts[i].type->gunName);
        fprintf(file, "Position          : (%d, %d)\n", escorts[i].x, escorts[i].y);
        fprintf(file, "Minimum Velocity  : %.2f m/s\n", escorts[i].minVelocity);
        fprintf(file, "Maximum Velocity  : %.2f m/s\n", escorts[i].maxVelocity);
        fprintf(file, "Minimum Angle     : %d degrees\n", escorts[i].minAngle);
        fprintf(file, "Maximum Angle     : %d degrees\n", escorts[i].maxAngle);
        fprintf(file, "Angle Range       : %d degrees\n", escorts[i].type->angleRange);
        fprintf(file, "Impact Power      : %.2f\n", escorts[i].type->impactPower);
        fprintf(file, "-------------------------------------------------------------\n");
    }

    fclose(file);

    printf("\n[OK] Complete initial conditions saved to initial_conditions.txt\n");
}

void simulation(void)
{
    clearScreen();
    printBattleshipArt();

    showBattleshipMenu();

    battleshipChoice = getInteger(
        "\nEnter battleship number [1 - 4]: ",
        1,
        4
    );

    printf("\n[OK] Selected: %s [%c]\n",
           getBattleshipName(),
           getBattleshipNotation());

    printSection("BATTLESHIP SHELL VELOCITY");

    printf("\nBattleship minimum shell velocity is fixed at 0 m/s.\n");
    printf("Choose Battleship maximum shell velocity between %d and %d m/s.\n\n",
           MIN_BATTLESHIP_VMAX,
           MAX_BATTLESHIP_VMAX);

    battleshipMaxVelocity = (double)getInteger(
        "Enter Battleship Maximum Shell Velocity: ",
        MIN_BATTLESHIP_VMAX,
        MAX_BATTLESHIP_VMAX
    );

    printf("\n[OK] Battleship shell velocity range: %.2f - %.2f m/s\n",
           battleshipMinVelocity,
           battleshipMaxVelocity);

    printSection("BATTLEFIELD SIZE");

    printf("\nThe battlefield is a square from (0,0) to (D,D).\n");
    printf("Allowed D value: %d - %d\n\n",
           MIN_CANVAS,
           MAX_CANVAS);

    canvasSize = getInteger(
        "Enter Canvas Size D: ",
        MIN_CANVAS,
        MAX_CANVAS
    );

    printf("\n[OK] Battlefield: (0,0) to (%d,%d)\n",
           canvasSize,
           canvasSize);

    printSection("ESCORT SHIPS");

    printf("\nAllowed number of escorts: %d - %d\n\n",
           MIN_ESCORTS,
           MAX_ESCORTS);

    numberOfEscorts = getInteger(
        "Enter Number of Escort Ships: ",
        MIN_ESCORTS,
        MAX_ESCORTS
    );

    printf("\n[OK] Number of Escort Ships: %d\n", numberOfEscorts);

    printSection("BATTLESHIP STARTING POSITION");

    printf("\nValid X coordinates: 0 - %d\n", canvasSize);
    printf("Valid Y coordinates: 0 - %d\n\n", canvasSize);

    battleshipX = getInteger(
        "Enter Battleship X Coordinate: ",
        0,
        canvasSize
    );

    battleshipY = getInteger(
        "Enter Battleship Y Coordinate: ",
        0,
        canvasSize
    );

    printf("\n[OK] Battleship position: (%d, %d)\n",
           battleshipX,
           battleshipY);

    printSection("RANDOM SEED");

    printf("\nSeed range: %d - %d\n", MIN_SEED, MAX_SEED);
    printf("The seed controls random Escort types, positions,\n");
    printf("velocities and firing-angle limits.\n\n");

    seedValue = getInteger(
        "Enter Seed Value: ",
        MIN_SEED,
        MAX_SEED
    );

    printf("\n[OK] Seed accepted: %d\n", seedValue);

    clearScreen();
    printBattleshipArt();
    showConfiguration();

    while (1)
    {
        generateChoice = getYesNo(
            "\nGenerate Battle Field? [Y/N]: "
        );

        if (generateChoice == 'Y')
        {
            printf("\n");
            printf("=============================================================\n");
            printf("                 GENERATING BATTLEFIELD\n");
            printf("=============================================================\n");

            printf("\n");
            printf("   Battleship configuration........ [OK]\n");
            printf("   Battlefield dimensions.......... [OK]\n");
            printf("   Escort count.................... [OK]\n");
            printf("   Random seed..................... [OK]\n");

            generateEscortShips();

            printf("   Escort identifiers.............. [OK]\n");
            printf("   Escort types.................... [OK]\n");
            printf("   Escort coordinates.............. [OK]\n");
            printf("   Escort velocity ranges.......... [OK]\n");
            printf("   Escort firing angles............ [OK]\n");

            showEscortShips();
            saveInitialConditions();

            printf("\n");
            printf("=============================================================\n");
            printf("             BATTLEFIELD READY FOR SIMULATION\n");
            printf("=============================================================\n");

            return;
        }

        exitChoice = getYesNo(
            "\nExit simulation setup? [Y/N]: "
        );

        if (exitChoice == 'Y')
        {
            printf("\n");
            printf("+-----------------------------------------------------------+\n");
            printf("|                 SIMULATION CANCELLED                      |\n");
            printf("+-----------------------------------------------------------+\n");

            return;
        }

        printf("\nReturning to battlefield generation confirmation...\n");
    }
}
