#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <string.h>

#include "simulation.h"

#define MIN_CANVAS 10
#define MAX_CANVAS 100
#define MIN_ESCORTS 1
#define MAX_ESCORTS 50
#define MIN_SEED 1
#define MAX_SEED 999999
#define MIN_BATTLESHIP_VMAX 10
#define MAX_BATTLESHIP_VMAX 1000

#define GRAVITY 9.81
#define PI 3.14159265358979323846
#define ANGLE_STEP 0.01

typedef struct
{
    const char *notation;
    const char *name;
    const char *gunName;
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

    double minAttackRange;
    double maxAttackRange;
    double distanceFromBattleship;

    int targetedByBattleship;
    double bFireVelocity;
    double bFireAngle;
    double bImpactTime;

    int firesAtBattleship;
    double eFireVelocity;
    double eFireAngle;
    double eImpactTime;

    int alive;
} EscortShip;

static const EscortTypeInfo escortTypes[5] =
{
    {"EA", "1936A-class Destroyer", "SK C/34 naval gun", 20},
    {"EB", "Gabbiano-class Corvette", "L/47 dual-purpose gun", 30},
    {"EC", "Matsu-class Destroyer", "Type 89 dual-purpose gun", 25},
    {"ED", "F-class Escort Ship", "SK C/32 naval gun", 50},
    {"EE", "Japanese Kaibokan", "(4.7 inch) naval guns", 70}
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

static double battleshipMinAttackRange = 0.0;
static double battleshipMaxAttackRange = 0.0;

static int battleshipAlive = 1;

static EscortShip escorts[MAX_ESCORTS];

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
    printf("               PART 1-A\n");
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
    char buffer[100];

    while (fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        int i = 0;

        while (buffer[i] != '\0')
        {
            if (buffer[i] == '\n')
                return;

            i++;
        }
    }
}

static int getInteger(const char *prompt, int min, int max)
{
    char buffer[100];
    char *end;
    long value;

    while (1)
    {
        int i = 0;
        int hasNewline = 0;

        printf("%s", prompt);

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
            continue;
        }

        errno = 0;
        value = strtol(buffer, &end, 10);

        if (end == buffer || errno == ERANGE)
        {
            printf("\n[!] Enter a whole number between %d and %d.\n", min, max);
            continue;
        }

        while (isspace((unsigned char)*end))
            end++;

        if (*end != '\0')
        {
            printf("\n[!] Enter only a whole number between %d and %d.\n", min, max);
            continue;
        }

        if (value < min || value > max)
        {
            printf("\n[!] Value must be between %d and %d.\n", min, max);
            continue;
        }

        return (int)value;
    }
}

static char getYesNo(const char *prompt)
{
    char buffer[100];

    while (1)
    {
        int i = 0;
        char answer;

        printf("%s", prompt);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            printf("\n[!] Input error. Please try again.\n");
            continue;
        }

        while (isspace((unsigned char)buffer[i]))
            i++;

        answer = buffer[i];

        if (answer != 'Y' && answer != 'y' &&
            answer != 'N' && answer != 'n')
        {
            printf("\n[!] Enter only Y or N.\n");
            continue;
        }

        i++;

        while (isspace((unsigned char)buffer[i]))
            i++;

        if (buffer[i] != '\0')
        {
            printf("\n[!] Enter only Y or N.\n");
            continue;
        }

        return (answer == 'Y' || answer == 'y') ? 'Y' : 'N';
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
    printf("1. USS Iowa (BB-61)          [U]\n");
    printf("2. MS King George V          [M]\n");
    printf("3. Richelieu                 [R]\n");
    printf("4. Sovetsky Soyuz-class      [S]\n");
}

static int randomInt(int min, int max)
{
    if (max <= min)
        return min;

    return min + rand() % (max - min + 1);
}

static int positionIsOccupied(int x, int y, int generatedCount)
{
    int i;

    if (x == battleshipX && y == battleshipY)
        return 1;

    for (i = 0; i < generatedCount; i++)
    {
        if (escorts[i].x == x && escorts[i].y == y)
            return 1;
    }

    return 0;
}

static double degreesToRadians(double angle)
{
    return angle * PI / 180.0;
}

static double projectileRange(double velocity, double angle)
{
    double radians = degreesToRadians(angle);

    return (velocity * velocity * sin(2.0 * radians)) / GRAVITY;
}

static double projectileFlightTime(double velocity, double angle)
{
    double radians = degreesToRadians(angle);

    return (2.0 * velocity * sin(radians)) / GRAVITY;
}

static double distanceBetween(double x1, double y1, double x2, double y2)
{
    double dx = x2 - x1;
    double dy = y2 - y1;

    return sqrt(dx * dx + dy * dy);
}

static double angleForMaximumRange(double minAngle, double maxAngle)
{
    if (minAngle <= 45.0 && maxAngle >= 45.0)
        return 45.0;

    if (maxAngle < 45.0)
        return maxAngle;

    return minAngle;
}

static double angleForMinimumRange(double minAngle, double maxAngle)
{
    double a = sin(2.0 * degreesToRadians(minAngle));
    double b = sin(2.0 * degreesToRadians(maxAngle));

    return (a <= b) ? minAngle : maxAngle;
}

static void calculateAttackRange(
    double minVelocity,
    double maxVelocity,
    double minAngle,
    double maxAngle,
    double *minRange,
    double *maxRange
)
{
    double innerAngle = angleForMinimumRange(minAngle, maxAngle);
    double outerAngle = angleForMaximumRange(minAngle, maxAngle);

    *minRange = projectileRange(minVelocity, innerAngle);
    *maxRange = projectileRange(maxVelocity, outerAngle);

    if (*minRange < 0.0)
        *minRange = 0.0;
}

/* Finds the shortest valid projectile travel time to the target. */
static int findFastestShot(
    double distance,
    double minVelocity,
    double maxVelocity,
    double minAngle,
    double maxAngle,
    double *selectedVelocity,
    double *selectedAngle,
    double *timeToHit
)
{
    double angle;
    double bestTime = DBL_MAX;
    double bestVelocity = 0.0;
    double bestAngle = 0.0;
    int found = 0;

    for (angle = minAngle;
         angle <= maxAngle + 0.000001;
         angle += ANGLE_STEP)
    {
        double radians = degreesToRadians(angle);
        double factor = sin(2.0 * radians);
        double requiredVelocity;
        double currentTime;

        if (factor <= 0.0000001)
            continue;

        requiredVelocity = sqrt((distance * GRAVITY) / factor);

        if (requiredVelocity < minVelocity - 0.000001 ||
            requiredVelocity > maxVelocity + 0.000001)
        {
            continue;
        }

        currentTime = projectileFlightTime(requiredVelocity, angle);

        if (currentTime < bestTime)
        {
            bestTime = currentTime;
            bestVelocity = requiredVelocity;
            bestAngle = angle;
            found = 1;
        }
    }

    if (!found)
        return 0;

    *selectedVelocity = bestVelocity;
    *selectedAngle = bestAngle;
    *timeToHit = bestTime;

    return 1;
}

static void generateEscortShips(void)
{
    int i;

    srand((unsigned int)seedValue);

    calculateAttackRange(
        battleshipMinVelocity,
        battleshipMaxVelocity,
        battleshipMinAngle,
        battleshipMaxAngle,
        &battleshipMinAttackRange,
        &battleshipMaxAttackRange
    );

    for (i = 0; i < numberOfEscorts; i++)
    {
        int typeIndex;
        int x;
        int y;
        int upperVelocity;
        double maxVelocity;
        double minVelocity;

        escorts[i].id = i + 1;

        typeIndex = randomInt(0, 4);
        escorts[i].type = &escortTypes[typeIndex];

        do
        {
            x = randomInt(0, canvasSize);
            y = randomInt(0, canvasSize);
        }
        while (positionIsOccupied(x, y, i));

        escorts[i].x = x;
        escorts[i].y = y;

        if (typeIndex == 0)
        {
            maxVelocity = 1.2 * battleshipMaxVelocity;
        }
        else
        {
            upperVelocity = (int)battleshipMaxVelocity - 1;

            if (upperVelocity < 2)
                upperVelocity = 2;

            maxVelocity = (double)randomInt(2, upperVelocity);
        }

        minVelocity = (double)randomInt(1, (int)maxVelocity - 1);

        escorts[i].minVelocity = minVelocity;
        escorts[i].maxVelocity = maxVelocity;

        escorts[i].minAngle =
            randomInt(1, 89 - escorts[i].type->angleRange);

        escorts[i].maxAngle =
            escorts[i].minAngle + escorts[i].type->angleRange;

        calculateAttackRange(
            escorts[i].minVelocity,
            escorts[i].maxVelocity,
            escorts[i].minAngle,
            escorts[i].maxAngle,
            &escorts[i].minAttackRange,
            &escorts[i].maxAttackRange
        );

        escorts[i].distanceFromBattleship =
            distanceBetween(
                battleshipX,
                battleshipY,
                escorts[i].x,
                escorts[i].y
            );

        escorts[i].targetedByBattleship = 0;
        escorts[i].bFireVelocity = 0.0;
        escorts[i].bFireAngle = 0.0;
        escorts[i].bImpactTime = -1.0;

        escorts[i].firesAtBattleship = 0;
        escorts[i].eFireVelocity = 0.0;
        escorts[i].eFireAngle = 0.0;
        escorts[i].eImpactTime = -1.0;

        escorts[i].alive = 1;
    }

    battleshipAlive = 1;
}

static void showConfiguration(void)
{
    printSection("SIMULATION CONFIGURATION");

    printf("\n");
    printf("Battleship          : %s [%c]\n",
           getBattleshipName(),
           getBattleshipNotation());
    printf("Gun                 : %s\n", getBattleshipGunName());
    printf("Position            : (%d, %d)\n", battleshipX, battleshipY);
    printf("Shell Velocity      : %.2f - %.2f\n",
           battleshipMinVelocity,
           battleshipMaxVelocity);
    printf("Vertical Angle      : %d - %d degrees\n",
           battleshipMinAngle,
           battleshipMaxAngle);
    printf("Canvas              : (0,0) to (%d,%d)\n",
           canvasSize,
           canvasSize);
    printf("Escort Ships        : %d\n", numberOfEscorts);
    printf("Random Seed         : %d\n", seedValue);
}

static void showEscortShips(void)
{
    int i;

    printSection("GENERATED ESCORT SHIPS");

    printf("\n");
    printf("%-6s %-4s %-11s %-8s %-8s %-6s %-6s %-9s %-9s %-9s\n",
           "ID",
           "TYPE",
           "POSITION",
           "Vmin",
           "Vmax",
           "Amin",
           "Amax",
           "Rmin",
           "Rmax",
           "DIST-B");

    printf("--------------------------------------------------------------------------------------\n");

    for (i = 0; i < numberOfEscorts; i++)
    {
        char position[32];

        snprintf(
            position,
            sizeof(position),
            "(%d,%d)",
            escorts[i].x,
            escorts[i].y
        );

        printf(
            "E%03d   %-4s %-11s %-8.2f %-8.2f %-6d %-6d %-9.2f %-9.2f %-9.2f\n",
            escorts[i].id,
            escorts[i].type->notation,
            position,
            escorts[i].minVelocity,
            escorts[i].maxVelocity,
            escorts[i].minAngle,
            escorts[i].maxAngle,
            escorts[i].minAttackRange,
            escorts[i].maxAttackRange,
            escorts[i].distanceFromBattleship
        );
    }

    printf(
        "\nBattleship Attack Disk : %.2f <= distance <= %.2f\n",
        battleshipMinAttackRange,
        battleshipMaxAttackRange
    );
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
    fprintf(file, "                       PART 1-A\n");
    fprintf(file, "=============================================================\n\n");

    fprintf(file, "[ BATTLESHIP ]\n");
    fprintf(file, "Type / Name       : %s\n", getBattleshipName());
    fprintf(file, "Notation          : %c\n", getBattleshipNotation());
    fprintf(file, "Gun               : %s\n", getBattleshipGunName());
    fprintf(file, "Position          : (%d, %d)\n", battleshipX, battleshipY);
    fprintf(file, "Minimum Velocity  : %.2f\n", battleshipMinVelocity);
    fprintf(file, "Maximum Velocity  : %.2f\n", battleshipMaxVelocity);
    fprintf(file, "Minimum Angle     : %d degrees\n", battleshipMinAngle);
    fprintf(file, "Maximum Angle     : %d degrees\n", battleshipMaxAngle);
    fprintf(file, "Minimum Range     : %.4f\n", battleshipMinAttackRange);
    fprintf(file, "Maximum Range     : %.4f\n", battleshipMaxAttackRange);

    fprintf(file, "\n[ BATTLEFIELD ]\n");
    fprintf(file, "Lower Left        : (0, 0)\n");
    fprintf(file, "Upper Right       : (%d, %d)\n", canvasSize, canvasSize);
    fprintf(file, "Canvas D          : %d\n", canvasSize);
    fprintf(file, "Number of Escorts : %d\n", numberOfEscorts);
    fprintf(file, "Seed Value        : %d\n", seedValue);

    fprintf(file, "\n[ ESCORT SHIPS ]\n");

    for (i = 0; i < numberOfEscorts; i++)
    {
        fprintf(file, "\nEscort ID         : E%03d\n", escorts[i].id);
        fprintf(file, "Type / Notation   : %s\n", escorts[i].type->notation);
        fprintf(file, "Type Name         : %s\n", escorts[i].type->name);
        fprintf(file, "Gun               : %s\n", escorts[i].type->gunName);
        fprintf(file, "Position          : (%d, %d)\n",
                escorts[i].x,
                escorts[i].y);
        fprintf(file, "Minimum Velocity  : %.2f\n", escorts[i].minVelocity);
        fprintf(file, "Maximum Velocity  : %.2f\n", escorts[i].maxVelocity);
        fprintf(file, "Minimum Angle     : %d degrees\n", escorts[i].minAngle);
        fprintf(file, "Maximum Angle     : %d degrees\n", escorts[i].maxAngle);
        fprintf(file, "Angle Range       : %d degrees\n",
                escorts[i].type->angleRange);
        fprintf(file, "Minimum Range     : %.4f\n", escorts[i].minAttackRange);
        fprintf(file, "Maximum Range     : %.4f\n", escorts[i].maxAttackRange);
        fprintf(file, "Distance from B   : %.4f\n",
                escorts[i].distanceFromBattleship);
    }

    fclose(file);

    printf("\n[OK] Initial conditions saved to initial_conditions.txt\n");
}

static void determineAllShotsAtTimeZero(void)
{
    int i;

    for (i = 0; i < numberOfEscorts; i++)
    {
        double distance = escorts[i].distanceFromBattleship;
        double velocity;
        double angle;
        double time;

        if (distance >= battleshipMinAttackRange - 0.000001 &&
            distance <= battleshipMaxAttackRange + 0.000001)
        {
            if (findFastestShot(
                    distance,
                    battleshipMinVelocity,
                    battleshipMaxVelocity,
                    battleshipMinAngle,
                    battleshipMaxAngle,
                    &velocity,
                    &angle,
                    &time))
            {
                escorts[i].targetedByBattleship = 1;
                escorts[i].bFireVelocity = velocity;
                escorts[i].bFireAngle = angle;
                escorts[i].bImpactTime = time;
            }
        }

        if (distance >= escorts[i].minAttackRange - 0.000001 &&
            distance <= escorts[i].maxAttackRange + 0.000001)
        {
            if (findFastestShot(
                    distance,
                    escorts[i].minVelocity,
                    escorts[i].maxVelocity,
                    escorts[i].minAngle,
                    escorts[i].maxAngle,
                    &velocity,
                    &angle,
                    &time))
            {
                escorts[i].firesAtBattleship = 1;
                escorts[i].eFireVelocity = velocity;
                escorts[i].eFireAngle = angle;
                escorts[i].eImpactTime = time;
            }
        }
    }
}

/* All valid shots are fired at t = 0. Fired shells are not cancelled later. */
static void resolvePart1A(
    int *sinkerIndex,
    double *sinkTime,
    int *destroyedEscortCount,
    double *lastBImpactTime,
    double *allShellsFinishedTime
)
{
    int i;

    *sinkerIndex = -1;
    *sinkTime = DBL_MAX;
    *destroyedEscortCount = 0;
    *lastBImpactTime = 0.0;

    for (i = 0; i < numberOfEscorts; i++)
    {
        if (escorts[i].firesAtBattleship &&
            escorts[i].eImpactTime < *sinkTime)
        {
            *sinkTime = escorts[i].eImpactTime;
            *sinkerIndex = i;
        }
    }

    if (*sinkerIndex >= 0)
    {
        battleshipAlive = 0;
    }
    else
    {
        battleshipAlive = 1;
        *sinkTime = -1.0;
    }

    for (i = 0; i < numberOfEscorts; i++)
    {
        if (escorts[i].targetedByBattleship)
        {
            escorts[i].alive = 0;
            (*destroyedEscortCount)++;

            if (escorts[i].bImpactTime > *lastBImpactTime)
                *lastBImpactTime = escorts[i].bImpactTime;
        }
        else
        {
            escorts[i].alive = 1;
        }
    }

    *allShellsFinishedTime = *lastBImpactTime;

    if (!battleshipAlive && *sinkTime > *allShellsFinishedTime)
        *allShellsFinishedTime = *sinkTime;
}

static void printBattleshipImpactTimeline(void)
{
    int printed[MAX_ESCORTS] = {0};
    int eventNumber = 1;

    while (1)
    {
        int i;
        int selectedIndex = -1;
        double earliestTime = DBL_MAX;

        for (i = 0; i < numberOfEscorts; i++)
        {
            if (escorts[i].targetedByBattleship &&
                !printed[i] &&
                escorts[i].bImpactTime < earliestTime)
            {
                earliestTime = escorts[i].bImpactTime;
                selectedIndex = i;
            }
        }

        if (selectedIndex < 0)
            break;

        printed[selectedIndex] = 1;

        printf(
            "%2d. E%03d (%s) destroyed at t = %.4f seconds\n",
            eventNumber,
            escorts[selectedIndex].id,
            escorts[selectedIndex].type->notation,
            escorts[selectedIndex].bImpactTime
        );

        printf(
            "    Distance = %.4f | Angle = %.2f | Velocity = %.4f\n",
            escorts[selectedIndex].distanceFromBattleship,
            escorts[selectedIndex].bFireAngle,
            escorts[selectedIndex].bFireVelocity
        );

        eventNumber++;
    }

    if (eventNumber == 1)
        printf("No Escort Ship is inside the Battleship attack range.\n");
}

static void printEscortImpactTimeline(int sinkerIndex)
{
    int printed[MAX_ESCORTS] = {0};
    int eventNumber = 1;

    while (1)
    {
        int i;
        int selectedIndex = -1;
        double earliestTime = DBL_MAX;

        for (i = 0; i < numberOfEscorts; i++)
        {
            if (escorts[i].firesAtBattleship &&
                !printed[i] &&
                escorts[i].eImpactTime < earliestTime)
            {
                earliestTime = escorts[i].eImpactTime;
                selectedIndex = i;
            }
        }

        if (selectedIndex < 0)
            break;

        printed[selectedIndex] = 1;

        printf(
            "%2d. E%03d (%s) shell reaches B at t = %.4f seconds",
            eventNumber,
            escorts[selectedIndex].id,
            escorts[selectedIndex].type->notation,
            escorts[selectedIndex].eImpactTime
        );

        if (selectedIndex == sinkerIndex)
            printf("  <-- FIRST IMPACT / SINKS B");

        printf("\n");

        printf(
            "    Distance = %.4f | Angle = %.2f | Velocity = %.4f\n",
            escorts[selectedIndex].distanceFromBattleship,
            escorts[selectedIndex].eFireAngle,
            escorts[selectedIndex].eFireVelocity
        );

        eventNumber++;
    }

    if (eventNumber == 1)
        printf("No Escort Ship has B inside its attack range.\n");
}

static int nextHistoryRunNumber(void)
{
    FILE *file;
    char line[200];
    int count = 0;

    file = fopen("simulation_history.txt", "r");

    if (file == NULL)
        return 1;

    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (strncmp(line, "SIMULATION RUN ", 15) == 0)
            count++;
    }

    fclose(file);

    return count + 1;
}

static void saveSimulationHistory(
    int sinkerIndex,
    double sinkTime,
    int destroyedEscortCount,
    double lastBImpactTime,
    double allShellsFinishedTime
)
{
    FILE *file;
    int i;
    int runNumber;

    runNumber = nextHistoryRunNumber();

    file = fopen("simulation_history.txt", "a");

    if (file == NULL)
    {
        printf("\n[!] Could not open simulation_history.txt\n");
        return;
    }

    fprintf(file, "\n=============================================================\n");
    fprintf(file, "SIMULATION RUN %d\n", runNumber);
    fprintf(file, "=============================================================\n");

    fprintf(file, "\n[ INITIAL SETUP ]\n");
    fprintf(file, "Battleship        : %s [%c]\n",
            getBattleshipName(),
            getBattleshipNotation());
    fprintf(file, "B Position        : (%d, %d)\n",
            battleshipX,
            battleshipY);
    fprintf(file, "B Vmin            : %.2f\n", battleshipMinVelocity);
    fprintf(file, "B Vmax            : %.2f\n", battleshipMaxVelocity);
    fprintf(file, "B Angle Range     : %d - %d degrees\n",
            battleshipMinAngle,
            battleshipMaxAngle);
    fprintf(file, "B Attack Range    : %.4f - %.4f\n",
            battleshipMinAttackRange,
            battleshipMaxAttackRange);
    fprintf(file, "Canvas            : (0,0) to (%d,%d)\n",
            canvasSize,
            canvasSize);
    fprintf(file, "Escort Count      : %d\n", numberOfEscorts);
    fprintf(file, "Seed              : %d\n", seedValue);

    fprintf(file, "\n[ ESCORT INITIAL CONDITIONS ]\n");

    for (i = 0; i < numberOfEscorts; i++)
    {
        fprintf(
            file,
            "E%03d | %s | (%d,%d) | V %.2f-%.2f | Angle %d-%d | Range %.4f-%.4f | Dist B %.4f\n",
            escorts[i].id,
            escorts[i].type->notation,
            escorts[i].x,
            escorts[i].y,
            escorts[i].minVelocity,
            escorts[i].maxVelocity,
            escorts[i].minAngle,
            escorts[i].maxAngle,
            escorts[i].minAttackRange,
            escorts[i].maxAttackRange,
            escorts[i].distanceFromBattleship
        );
    }

    fprintf(file, "\n[ BATTLESHIP -> ESCORT IMPACTS ]\n");

    for (i = 0; i < numberOfEscorts; i++)
    {
        if (escorts[i].targetedByBattleship)
        {
            fprintf(
                file,
                "E%03d | %s | Hit %.4f s | Distance %.4f | Angle %.2f | Velocity %.4f\n",
                escorts[i].id,
                escorts[i].type->notation,
                escorts[i].bImpactTime,
                escorts[i].distanceFromBattleship,
                escorts[i].bFireAngle,
                escorts[i].bFireVelocity
            );
        }
    }

    fprintf(file, "\n[ ESCORT -> BATTLESHIP IMPACTS ]\n");

    for (i = 0; i < numberOfEscorts; i++)
    {
        if (escorts[i].firesAtBattleship)
        {
            fprintf(
                file,
                "E%03d | %s | Hit %.4f s | Distance %.4f | Angle %.2f | Velocity %.4f%s\n",
                escorts[i].id,
                escorts[i].type->notation,
                escorts[i].eImpactTime,
                escorts[i].distanceFromBattleship,
                escorts[i].eFireAngle,
                escorts[i].eFireVelocity,
                (i == sinkerIndex) ? " | SINKS B" : ""
            );
        }
    }

    fprintf(file, "\n[ RESULT ]\n");
    fprintf(file, "Battleship Status : %s\n",
            battleshipAlive ? "ALIVE" : "SUNK");
    fprintf(file, "Escorts Hit by B  : %d\n", destroyedEscortCount);

    if (battleshipAlive)
    {
        fprintf(file, "Battle End Time   : %.4f seconds\n", lastBImpactTime);
    }
    else
    {
        fprintf(file, "Sunk By Escort    : E%03d\n",
                escorts[sinkerIndex].id);
        fprintf(file, "Sink Time         : %.4f seconds\n", sinkTime);
        fprintf(file, "Last B -> E Hit   : %.4f seconds\n", lastBImpactTime);
        fprintf(file, "All Shells End    : %.4f seconds\n", allShellsFinishedTime);
    }

    fprintf(file, "\n[ FINAL ESCORT STATES ]\n");

    for (i = 0; i < numberOfEscorts; i++)
    {
        fprintf(
            file,
            "E%03d | %s | %s",
            escorts[i].id,
            escorts[i].type->notation,
            escorts[i].alive ? "ALIVE" : "DESTROYED"
        );

        if (!escorts[i].alive)
            fprintf(file, " at %.4f seconds", escorts[i].bImpactTime);

        fprintf(file, "\n");
    }

    fprintf(file, "=============================================================\n");

    fclose(file);
}

static void saveFinalConditions(
    int sinkerIndex,
    double sinkTime,
    int destroyedEscortCount,
    double lastBImpactTime,
    double allShellsFinishedTime
)
{
    FILE *file;
    int i;

    file = fopen("final_conditions.txt", "w");

    if (file == NULL)
    {
        printf("\n[!] Could not create final_conditions.txt\n");
        return;
    }

    fprintf(file, "=============================================================\n");
    fprintf(file, "                    FINAL CONDITIONS\n");
    fprintf(file, "                       PART 1-A\n");
    fprintf(file, "=============================================================\n\n");

    fprintf(file, "[ BATTLESHIP ]\n");
    fprintf(file, "Type              : %s [%c]\n",
            getBattleshipName(),
            getBattleshipNotation());
    fprintf(file, "Position          : (%d, %d)\n",
            battleshipX,
            battleshipY);
    fprintf(file, "Status            : %s\n",
            battleshipAlive ? "ALIVE" : "SUNK");

    if (!battleshipAlive)
    {
        fprintf(file, "Destroyed By      : E%03d\n",
                escorts[sinkerIndex].id);
        fprintf(file, "Destroyed At      : %.4f seconds\n",
                sinkTime);
    }

    fprintf(file, "\n[ BATTLE SUMMARY ]\n");
    fprintf(file, "Escorts Destroyed : %d\n", destroyedEscortCount);
    fprintf(file, "Last B -> E Hit   : %.4f seconds\n", lastBImpactTime);
    fprintf(file, "All Shells End    : %.4f seconds\n", allShellsFinishedTime);

    fprintf(file, "\n[ ESCORT FINAL STATES ]\n");

    for (i = 0; i < numberOfEscorts; i++)
    {
        fprintf(
            file,
            "E%03d | %-3s | Position (%d,%d) | %s",
            escorts[i].id,
            escorts[i].type->notation,
            escorts[i].x,
            escorts[i].y,
            escorts[i].alive ? "ALIVE" : "DESTROYED"
        );

        if (!escorts[i].alive)
            fprintf(file, " at %.4f seconds", escorts[i].bImpactTime);

        fprintf(file, "\n");
    }

    fclose(file);
}

static void runPart1A(void)
{
    int sinkerIndex;
    double sinkTime;
    int destroyedEscortCount;
    double lastBImpactTime;
    double allShellsFinishedTime;

    determineAllShotsAtTimeZero();

    resolvePart1A(
        &sinkerIndex,
        &sinkTime,
        &destroyedEscortCount,
        &lastBImpactTime,
        &allShellsFinishedTime
    );

    printSection("BATTLESHIP -> ESCORT IMPACTS");
    printf("\n");
    printBattleshipImpactTimeline();

    printSection("ESCORT -> BATTLESHIP IMPACTS");
    printf("\n");
    printEscortImpactTimeline(sinkerIndex);

    printSection("PART 1-A FINAL RESULT");
    printf("\n");

    if (battleshipAlive)
    {
        printf("[SURVIVED] Battleship does not sink.\n");
        printf("Escort Ships hit by B : %d\n", destroyedEscortCount);
        printf("Time to end battle     : %.4f seconds\n", lastBImpactTime);
    }
    else
    {
        printf("[SUNK] Battleship is destroyed.\n");
        printf("Escort that sank B     : E%03d\n",
               escorts[sinkerIndex].id);
        printf("Escort type            : %s - %s\n",
               escorts[sinkerIndex].type->notation,
               escorts[sinkerIndex].type->name);
        printf("First E -> B impact    : %.4f seconds\n", sinkTime);
        printf("Escorts hit by B       : %d\n", destroyedEscortCount);
        printf("Last B -> E impact     : %.4f seconds\n", lastBImpactTime);
    }

    saveSimulationHistory(
        sinkerIndex,
        sinkTime,
        destroyedEscortCount,
        lastBImpactTime,
        allShellsFinishedTime
    );

    saveFinalConditions(
        sinkerIndex,
        sinkTime,
        destroyedEscortCount,
        lastBImpactTime,
        allShellsFinishedTime
    );

    printf("\nSaved: initial_conditions.txt\n");
    printf("Updated: simulation_history.txt\n");
    printf("Saved: final_conditions.txt\n");
}

void simulation(void)
{
    char generateChoice;
    char exitChoice;

    clearScreen();
    printBattleshipArt();

    showBattleshipMenu();

    battleshipChoice = getInteger(
        "\nEnter battleship number [1 - 4]: ",
        1,
        4
    );

    printSection("BATTLESHIP MAXIMUM SHELL VELOCITY");

    printf("\nBattleship minimum shell velocity is fixed at 0.\n");
    printf("Allowed Battleship Vmax: %d - %d\n\n",
           MIN_BATTLESHIP_VMAX,
           MAX_BATTLESHIP_VMAX);

    battleshipMaxVelocity = (double)getInteger(
        "Enter Battleship Maximum Shell Velocity: ",
        MIN_BATTLESHIP_VMAX,
        MAX_BATTLESHIP_VMAX
    );

    printSection("BATTLEFIELD SIZE");

    printf("\nThe battlefield is a square from (0,0) to (D,D).\n");
    printf("Allowed D: %d - %d\n\n", MIN_CANVAS, MAX_CANVAS);

    canvasSize = getInteger(
        "Enter Canvas Size D: ",
        MIN_CANVAS,
        MAX_CANVAS
    );

    printSection("NUMBER OF ESCORT SHIPS");

    printf("\nAllowed number of Escort Ships: %d - %d\n\n",
           MIN_ESCORTS,
           MAX_ESCORTS);

    numberOfEscorts = getInteger(
        "Enter Number of Escort Ships: ",
        MIN_ESCORTS,
        MAX_ESCORTS
    );

    printSection("BATTLESHIP STARTING POSITION");

    printf("\nValid coordinate range: 0 - %d\n\n", canvasSize);

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

    printSection("RANDOM SEED");

    printf("\nSeed range: %d - %d\n\n", MIN_SEED, MAX_SEED);

    seedValue = getInteger(
        "Enter Seed Value: ",
        MIN_SEED,
        MAX_SEED
    );

    clearScreen();
    printBattleshipArt();
    showConfiguration();

    while (1)
    {
        generateChoice = getYesNo(
            "\nGenerate Battlefield and run Part 1-A? [Y/N]: "
        );

        if (generateChoice == 'Y')
        {
            generateEscortShips();
            showEscortShips();
            saveInitialConditions();
            runPart1A();

            printf("\n");
            printf("=============================================================\n");
            printf("                    PART 1-A COMPLETE\n");
            printf("=============================================================\n");

            return;
        }

        exitChoice = getYesNo(
            "\nExit simulation setup? [Y/N]: "
        );

        if (exitChoice == 'Y')
            return;

        printf("\nReturning to generation confirmation...\n");
    }
}