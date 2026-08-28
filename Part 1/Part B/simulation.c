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

#define MIN_PATH_POINTS 2
#define MAX_PATH_POINTS 50

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
    int x;
    int y;
} PathPoint;

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
    int destroyedIteration;
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
static int initialBattleshipX;
static int initialBattleshipY;
static int seedValue;

static double battleshipMinVelocity = 0.0;
static double battleshipMaxVelocity;

static int battleshipMinAngle = 0;
static int battleshipMaxAngle = 90;

static double battleshipMinAttackRange = 0.0;
static double battleshipMaxAttackRange = 0.0;

static int battleshipAlive = 1;

static EscortShip escorts[MAX_ESCORTS];

static PathPoint path[MAX_PATH_POINTS];
static int pathCount = 0;

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

static int escortPositionOccupied(int x, int y, int generatedCount)
{
    int i;

    if (x == initialBattleshipX && y == initialBattleshipY)
        return 1;

    for (i = 0; i < generatedCount; i++)
    {
        if (escorts[i].x == x && escorts[i].y == y)
            return 1;
    }

    return 0;
}

static int pathPointOccupied(int x, int y, int generatedCount)
{
    int i;

    for (i = 0; i < numberOfEscorts; i++)
    {
        if (escorts[i].x == x && escorts[i].y == y)
            return 1;
    }

    for (i = 0; i < generatedCount; i++)
    {
        if (path[i].x == x && path[i].y == y)
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

/* Selects the valid trajectory with the smallest shell travel time. */
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

static void resetEscortBattleData(void)
{
    int i;

    for (i = 0; i < numberOfEscorts; i++)
    {
        escorts[i].targetedByBattleship = 0;
        escorts[i].bFireVelocity = 0.0;
        escorts[i].bFireAngle = 0.0;
        escorts[i].bImpactTime = -1.0;

        escorts[i].firesAtBattleship = 0;
        escorts[i].eFireVelocity = 0.0;
        escorts[i].eFireAngle = 0.0;
        escorts[i].eImpactTime = -1.0;
    }
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
        while (escortPositionOccupied(x, y, i));

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

        escorts[i].alive = 1;
        escorts[i].destroyedIteration = 0;
    }

    battleshipAlive = 1;
    resetEscortBattleData();
}

static void showConfiguration(void)
{
    printSection("SIMULATION CONFIGURATION");

    printf("\n");
    printf("Battleship          : %s [%c]\n",
           getBattleshipName(),
           getBattleshipNotation());
    printf("Gun                 : %s\n", getBattleshipGunName());
    printf("Position            : (%d, %d)\n",
           initialBattleshipX,
           initialBattleshipY);
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
    printf("%-6s %-4s %-11s %-8s %-8s %-6s %-6s %-9s %-9s\n",
           "ID",
           "TYPE",
           "POSITION",
           "Vmin",
           "Vmax",
           "Amin",
           "Amax",
           "Rmin",
           "Rmax");

    printf("-----------------------------------------------------------------------------\n");

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
            "E%03d   %-4s %-11s %-8.2f %-8.2f %-6d %-6d %-9.2f %-9.2f\n",
            escorts[i].id,
            escorts[i].type->notation,
            position,
            escorts[i].minVelocity,
            escorts[i].maxVelocity,
            escorts[i].minAngle,
            escorts[i].maxAngle,
            escorts[i].minAttackRange,
            escorts[i].maxAttackRange
        );
    }

    printf(
        "\nBattleship Attack Disk : %.2f <= distance <= %.2f\n",
        battleshipMinAttackRange,
        battleshipMaxAttackRange
    );
}

static void generatePath(void)
{
    int i;

    path[0].x = initialBattleshipX;
    path[0].y = initialBattleshipY;

    for (i = 1; i < pathCount; i++)
    {
        int x;
        int y;

        do
        {
            x = randomInt(0, canvasSize);
            y = randomInt(0, canvasSize);
        }
        while (pathPointOccupied(x, y, i));

        path[i].x = x;
        path[i].y = y;
    }
}

static void showPath(void)
{
    int i;

    printSection("BATTLESHIP PATH");

    printf("\n");

    for (i = 0; i < pathCount; i++)
    {
        printf(
            "Point %2d : (%d, %d)%s\n",
            i + 1,
            path[i].x,
            path[i].y,
            (i == 0) ? "  [START]" : ""
        );
    }
}

static void saveInitialConditions(const char *mode)
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

    fprintf(file, "Simulation Mode   : %s\n\n", mode);

    fprintf(file, "[ BATTLESHIP ]\n");
    fprintf(file, "Type / Name       : %s\n", getBattleshipName());
    fprintf(file, "Notation          : %c\n", getBattleshipNotation());
    fprintf(file, "Gun               : %s\n", getBattleshipGunName());
    fprintf(file, "Initial Position  : (%d, %d)\n",
            initialBattleshipX,
            initialBattleshipY);
    fprintf(file, "Minimum Velocity  : %.2f\n", battleshipMinVelocity);
    fprintf(file, "Maximum Velocity  : %.2f\n", battleshipMaxVelocity);
    fprintf(file, "Minimum Angle     : %d degrees\n", battleshipMinAngle);
    fprintf(file, "Maximum Angle     : %d degrees\n", battleshipMaxAngle);
    fprintf(file, "Minimum Range     : %.4f\n", battleshipMinAttackRange);
    fprintf(file, "Maximum Range     : %.4f\n", battleshipMaxAttackRange);

    fprintf(file, "\n[ BATTLEFIELD ]\n");
    fprintf(file, "Lower Left        : (0, 0)\n");
    fprintf(file, "Upper Right       : (%d, %d)\n", canvasSize, canvasSize);
    fprintf(file, "Number of Escorts : %d\n", numberOfEscorts);
    fprintf(file, "Seed Value        : %d\n", seedValue);

    if (pathCount > 0)
    {
        fprintf(file, "\n[ BATTLESHIP PATH ]\n");
        fprintf(file, "Path Points       : %d\n", pathCount);

        for (i = 0; i < pathCount; i++)
        {
            fprintf(
                file,
                "Point %02d          : (%d, %d)\n",
                i + 1,
                path[i].x,
                path[i].y
            );
        }
    }

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
    }

    fclose(file);

    printf("\n[OK] Initial conditions saved to initial_conditions.txt\n");
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

static FILE *openHistoryRun(const char *mode, int *runNumber)
{
    FILE *file;

    *runNumber = nextHistoryRunNumber();

    file = fopen("simulation_history.txt", "a");

    if (file == NULL)
        return NULL;

    fprintf(file, "\n=============================================================\n");
    fprintf(file, "SIMULATION RUN %d\n", *runNumber);
    fprintf(file, "=============================================================\n");
    fprintf(file, "Mode              : %s\n", mode);
    fprintf(file, "Battleship        : %s [%c]\n",
            getBattleshipName(),
            getBattleshipNotation());
    fprintf(file, "Initial Position  : (%d, %d)\n",
            initialBattleshipX,
            initialBattleshipY);
    fprintf(file, "B Vmax            : %.2f\n", battleshipMaxVelocity);
    fprintf(file, "Canvas            : (0,0) to (%d,%d)\n",
            canvasSize,
            canvasSize);
    fprintf(file, "Escort Count      : %d\n", numberOfEscorts);
    fprintf(file, "Seed              : %d\n", seedValue);

    return file;
}

static void determineShotsForCurrentPoint(void)
{
    int i;

    resetEscortBattleData();

    for (i = 0; i < numberOfEscorts; i++)
    {
        double distance;
        double velocity;
        double angle;
        double time;

        if (!escorts[i].alive)
            continue;

        distance =
            distanceBetween(
                battleshipX,
                battleshipY,
                escorts[i].x,
                escorts[i].y
            );

        escorts[i].distanceFromBattleship = distance;

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

/* All valid shots at one point are fired simultaneously. */
static void resolveCurrentPoint(
    int iteration,
    int *sinkerIndex,
    double *sinkTime,
    int *destroyedThisIteration,
    double *lastBImpactTime,
    double *iterationEndTime
)
{
    int i;

    *sinkerIndex = -1;
    *sinkTime = DBL_MAX;
    *destroyedThisIteration = 0;
    *lastBImpactTime = 0.0;

    for (i = 0; i < numberOfEscorts; i++)
    {
        if (!escorts[i].alive)
            continue;

        if (escorts[i].firesAtBattleship &&
            escorts[i].eImpactTime < *sinkTime)
        {
            *sinkTime = escorts[i].eImpactTime;
            *sinkerIndex = i;
        }
    }

    if (*sinkerIndex >= 0)
        battleshipAlive = 0;

    for (i = 0; i < numberOfEscorts; i++)
    {
        if (!escorts[i].alive)
            continue;

        if (escorts[i].targetedByBattleship)
        {
            escorts[i].alive = 0;
            escorts[i].destroyedIteration = iteration;
            (*destroyedThisIteration)++;

            if (escorts[i].bImpactTime > *lastBImpactTime)
                *lastBImpactTime = escorts[i].bImpactTime;
        }
    }

    *iterationEndTime = *lastBImpactTime;

    if (*sinkerIndex >= 0 && *sinkTime > *iterationEndTime)
        *iterationEndTime = *sinkTime;

    if (*sinkerIndex < 0)
        *sinkTime = -1.0;
}

static void printCurrentPointResults(
    int iteration,
    int sinkerIndex,
    double sinkTime,
    int destroyedThisIteration,
    double iterationEndTime
)
{
    int i;

    printSection("ITERATION RESULT");

    printf("\nIteration            : %d\n", iteration);
    printf("Battleship Position  : (%d, %d)\n",
           battleshipX,
           battleshipY);
    printf("B Vertical Angle     : %d - %d degrees\n",
           battleshipMinAngle,
           battleshipMaxAngle);

    printf("\nBATTLESHIP -> ESCORT\n");

    for (i = 0; i < numberOfEscorts; i++)
    {
        if (escorts[i].targetedByBattleship)
        {
            printf(
                "E%03d (%s) destroyed at %.4f s\n",
                escorts[i].id,
                escorts[i].type->notation,
                escorts[i].bImpactTime
            );
        }
    }

    if (destroyedThisIteration == 0)
        printf("No Escort Ships destroyed in this iteration.\n");

    printf("\nESCORT -> BATTLESHIP\n");

    {
        int attackCount = 0;

        for (i = 0; i < numberOfEscorts; i++)
        {
            if (escorts[i].firesAtBattleship)
            {
                printf(
                    "E%03d (%s) -> B at %.4f s%s\n",
                    escorts[i].id,
                    escorts[i].type->notation,
                    escorts[i].eImpactTime,
                    (i == sinkerIndex)
                        ? "  <-- FIRST IMPACT / SINKS B"
                        : ""
                );

                attackCount++;
            }
        }

        if (attackCount == 0)
            printf("No Escort Ship can hit B from this point.\n");
    }

    printf("\nEscorts destroyed this iteration : %d\n",
           destroyedThisIteration);
    printf("Iteration end time               : %.4f seconds\n",
           iterationEndTime);

    if (sinkerIndex >= 0)
    {
        printf("Battleship Status                : SUNK\n");
        printf("Sunk by                          : E%03d\n",
               escorts[sinkerIndex].id);
        printf("Sink time                        : %.4f seconds\n",
               sinkTime);
    }
    else
    {
        printf("Battleship Status                : ALIVE\n");
    }
}

static void writeCurrentPointHistory(
    FILE *file,
    int iteration,
    int sinkerIndex,
    double sinkTime,
    int destroyedThisIteration,
    double iterationEndTime,
    double cumulativeTime
)
{
    int i;

    fprintf(file, "\n-------------------------------------------------------------\n");
    fprintf(file, "ITERATION %d\n", iteration);
    fprintf(file, "-------------------------------------------------------------\n");
    fprintf(file, "B Position        : (%d, %d)\n",
            battleshipX,
            battleshipY);
    fprintf(file, "B Angle Range     : %d - %d degrees\n",
            battleshipMinAngle,
            battleshipMaxAngle);

    fprintf(file, "\nB -> E IMPACTS\n");

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

    fprintf(file, "\nE -> B IMPACTS\n");

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

    fprintf(file, "\nDestroyed in Step : %d\n", destroyedThisIteration);
    fprintf(file, "Step End Time     : %.4f seconds\n", iterationEndTime);
    fprintf(file, "Cumulative Time   : %.4f seconds\n", cumulativeTime);

    if (sinkerIndex >= 0)
    {
        fprintf(file, "B Status          : SUNK\n");
        fprintf(file, "Sunk By           : E%03d\n",
                escorts[sinkerIndex].id);
        fprintf(file, "Sink Time         : %.4f seconds\n", sinkTime);
    }
    else
    {
        fprintf(file, "B Status          : ALIVE\n");
    }
}

static void saveFinalConditions(
    const char *mode,
    int completedIterations,
    int sinkerIndex,
    double sinkTime,
    double totalElapsedTime
)
{
    FILE *file;
    int i;
    int totalDestroyed = 0;

    file = fopen("final_conditions.txt", "w");

    if (file == NULL)
    {
        printf("\n[!] Could not create final_conditions.txt\n");
        return;
    }

    for (i = 0; i < numberOfEscorts; i++)
    {
        if (!escorts[i].alive)
            totalDestroyed++;
    }

    fprintf(file, "=============================================================\n");
    fprintf(file, "                    FINAL CONDITIONS\n");
    fprintf(file, "=============================================================\n\n");

    fprintf(file, "Simulation Mode   : %s\n", mode);
    fprintf(file, "Iterations Run    : %d\n", completedIterations);
    fprintf(file, "Battleship Status : %s\n",
            battleshipAlive ? "ALIVE" : "SUNK");
    fprintf(file, "Final B Position  : (%d, %d)\n",
            battleshipX,
            battleshipY);
    fprintf(file, "Escorts Destroyed : %d\n", totalDestroyed);
    fprintf(file, "Escorts Remaining : %d\n",
            numberOfEscorts - totalDestroyed);
    fprintf(file, "Total Step Time   : %.4f seconds\n",
            totalElapsedTime);

    if (!battleshipAlive && sinkerIndex >= 0)
    {
        fprintf(file, "Destroyed By      : E%03d\n",
                escorts[sinkerIndex].id);
        fprintf(file, "Destroyed At Step : %d\n",
                completedIterations);
        fprintf(file, "Sink Time In Step : %.4f seconds\n",
                sinkTime);
    }

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
        {
            fprintf(
                file,
                " in iteration %d",
                escorts[i].destroyedIteration
            );
        }

        fprintf(file, "\n");
    }

    fclose(file);
}

static void writeFinalHistoryState(
    FILE *file,
    int completedIterations,
    int sinkerIndex,
    double sinkTime,
    double totalElapsedTime
)
{
    int i;
    int totalDestroyed = 0;

    for (i = 0; i < numberOfEscorts; i++)
    {
        if (!escorts[i].alive)
            totalDestroyed++;
    }

    fprintf(file, "\n[ FINAL RESULT ]\n");
    fprintf(file, "Iterations Run    : %d\n", completedIterations);
    fprintf(file, "B Status          : %s\n",
            battleshipAlive ? "ALIVE" : "SUNK");
    fprintf(file, "Final B Position  : (%d, %d)\n",
            battleshipX,
            battleshipY);
    fprintf(file, "Escorts Destroyed : %d\n", totalDestroyed);
    fprintf(file, "Escorts Remaining : %d\n",
            numberOfEscorts - totalDestroyed);
    fprintf(file, "Total Step Time   : %.4f seconds\n",
            totalElapsedTime);

    if (!battleshipAlive && sinkerIndex >= 0)
    {
        fprintf(file, "Sunk By           : E%03d\n",
                escorts[sinkerIndex].id);
        fprintf(file, "Sink Iteration    : %d\n",
                completedIterations);
        fprintf(file, "Sink Time In Step : %.4f seconds\n",
                sinkTime);
    }

    fprintf(file, "\nFINAL ESCORT STATES\n");

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
            fprintf(file, " in iteration %d",
                    escorts[i].destroyedIteration);

        fprintf(file, "\n");
    }

    fprintf(file, "=============================================================\n");
}

static void runPart1A(void)
{
    int sinkerIndex;
    double sinkTime;
    int destroyedThisIteration;
    double lastBImpactTime;
    double iterationEndTime;
    FILE *history;
    int runNumber;

    battleshipX = initialBattleshipX;
    battleshipY = initialBattleshipY;

    pathCount = 0;
    saveInitialConditions("PART 1-A");

    determineShotsForCurrentPoint();

    resolveCurrentPoint(
        1,
        &sinkerIndex,
        &sinkTime,
        &destroyedThisIteration,
        &lastBImpactTime,
        &iterationEndTime
    );

    printCurrentPointResults(
        1,
        sinkerIndex,
        sinkTime,
        destroyedThisIteration,
        iterationEndTime
    );

    history = openHistoryRun("PART 1-A", &runNumber);

    if (history != NULL)
    {
        writeCurrentPointHistory(
            history,
            1,
            sinkerIndex,
            sinkTime,
            destroyedThisIteration,
            iterationEndTime,
            iterationEndTime
        );

        writeFinalHistoryState(
            history,
            1,
            sinkerIndex,
            sinkTime,
            iterationEndTime
        );

        fclose(history);
    }

    saveFinalConditions(
        "PART 1-A",
        1,
        sinkerIndex,
        sinkTime,
        iterationEndTime
    );

    printf("\nUpdated: simulation_history.txt\n");
    printf("Saved: final_conditions.txt\n");
}

typedef struct
{
    int completedIterations;
    int finalSinkerIndex;
    double finalSinkTime;
    double totalElapsedTime;
    int totalDestroyed;
    int battleshipSurvived;
} PathSimulationResult;

static void resetPathSimulationState(void)
{
    int i;

    battleshipX = initialBattleshipX;
    battleshipY = initialBattleshipY;
    battleshipAlive = 1;

    battleshipMinAngle = 0;
    battleshipMaxAngle = 90;

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
        escorts[i].alive = 1;
        escorts[i].destroyedIteration = 0;
    }

    resetEscortBattleData();
}

static int countDestroyedEscorts(void)
{
    int i;
    int count = 0;

    for (i = 0; i < numberOfEscorts; i++)
    {
        if (!escorts[i].alive)
            count++;
    }

    return count;
}

static PathSimulationResult runPathScenario(
    const char *mode,
    int jamEnabled,
    int jamAfterIteration,
    int jamMinimumAngle,
    int saveLatestFinal
)
{
    PathSimulationResult result;
    FILE *history;
    int runNumber;
    int iteration;

    result.completedIterations = 0;
    result.finalSinkerIndex = -1;
    result.finalSinkTime = -1.0;
    result.totalElapsedTime = 0.0;
    result.totalDestroyed = 0;
    result.battleshipSurvived = 1;

    resetPathSimulationState();

    history = openHistoryRun(mode, &runNumber);

    if (history != NULL)
    {
        int i;

        fprintf(history, "Path Points       : %d\n", pathCount);

        for (i = 0; i < pathCount; i++)
        {
            fprintf(
                history,
                "Path %02d          : (%d, %d)\n",
                i + 1,
                path[i].x,
                path[i].y
            );
        }

        if (jamEnabled)
        {
            fprintf(history, "Jam After         : %d iterations\n",
                    jamAfterIteration);
            fprintf(history, "Jammed B Angles   : %d - 90 degrees\n",
                    jamMinimumAngle);
        }
    }

    for (iteration = 1; iteration <= pathCount; iteration++)
    {
        int sinkerIndex;
        int destroyedThisIteration;
        double sinkTime;
        double lastBImpactTime;
        double iterationEndTime;

        battleshipX = path[iteration - 1].x;
        battleshipY = path[iteration - 1].y;

        if (jamEnabled && iteration > jamAfterIteration)
            battleshipMinAngle = jamMinimumAngle;
        else
            battleshipMinAngle = 0;

        battleshipMaxAngle = 90;

        calculateAttackRange(
            battleshipMinVelocity,
            battleshipMaxVelocity,
            battleshipMinAngle,
            battleshipMaxAngle,
            &battleshipMinAttackRange,
            &battleshipMaxAttackRange
        );

        printSection(mode);

        printf("\nPath Point %d of %d\n", iteration, pathCount);

        if (jamEnabled && iteration == jamAfterIteration + 1)
        {
            printf("Battleship gun is now JAMMED.\n");
            printf("New vertical angle range: %d - 90 degrees\n",
                   jamMinimumAngle);
        }

        determineShotsForCurrentPoint();

        resolveCurrentPoint(
            iteration,
            &sinkerIndex,
            &sinkTime,
            &destroyedThisIteration,
            &lastBImpactTime,
            &iterationEndTime
        );

        result.totalElapsedTime += iterationEndTime;
        result.completedIterations = iteration;

        printCurrentPointResults(
            iteration,
            sinkerIndex,
            sinkTime,
            destroyedThisIteration,
            iterationEndTime
        );

        if (history != NULL)
        {
            writeCurrentPointHistory(
                history,
                iteration,
                sinkerIndex,
                sinkTime,
                destroyedThisIteration,
                iterationEndTime,
                result.totalElapsedTime
            );
        }

        if (sinkerIndex >= 0)
        {
            result.finalSinkerIndex = sinkerIndex;
            result.finalSinkTime = sinkTime;
            result.battleshipSurvived = 0;
            break;
        }
    }

    result.totalDestroyed = countDestroyedEscorts();

    printSection("PATH SIMULATION FINAL RESULT");

    printf("\nMode                 : %s\n", mode);
    printf("Iterations completed : %d of %d\n",
           result.completedIterations,
           pathCount);
    printf("Escorts destroyed    : %d\n",
           result.totalDestroyed);
    printf("Escorts remaining    : %d\n",
           numberOfEscorts - result.totalDestroyed);

    if (result.battleshipSurvived)
    {
        printf("Battleship Status    : ALIVE\n");
    }
    else
    {
        printf("Battleship Status    : SUNK\n");
        printf("Destroyed at point   : %d\n",
               result.completedIterations);
        printf("Destroyed by         : E%03d\n",
               escorts[result.finalSinkerIndex].id);
        printf("Sink time in step    : %.4f seconds\n",
               result.finalSinkTime);
    }

    printf("Total step time      : %.4f seconds\n",
           result.totalElapsedTime);

    if (history != NULL)
    {
        writeFinalHistoryState(
            history,
            result.completedIterations,
            result.finalSinkerIndex,
            result.finalSinkTime,
            result.totalElapsedTime
        );

        fclose(history);
    }

    if (saveLatestFinal)
    {
        saveFinalConditions(
            mode,
            result.completedIterations,
            result.finalSinkerIndex,
            result.finalSinkTime,
            result.totalElapsedTime
        );
    }

    return result;
}

static void runPart1BSimulation1(void)
{
    pathCount = getInteger(
        "\nEnter number of path points k: ",
        MIN_PATH_POINTS,
        MAX_PATH_POINTS
    );

    generatePath();
    showPath();

    saveInitialConditions("PART 1-B - SIMULATION 1");

    runPathScenario(
        "PART 1-B - SIMULATION 1",
        0,
        0,
        0,
        1
    );

    battleshipMinAngle = 0;
    battleshipMaxAngle = 90;

    printf("\nUpdated: simulation_history.txt\n");
    printf("Saved: final_conditions.txt\n");
}

static void printSimulationComparison(
    PathSimulationResult simulation1,
    PathSimulationResult simulation2,
    int jamAfterIteration,
    int jamMinimumAngle
)
{
    printSection("SIMULATION 1 VS SIMULATION 2");

    printf("\n");
    printf("Same battlefield, Escort Ships and path are used.\n");
    printf("Simulation 2 jam starts after iteration %d.\n",
           jamAfterIteration);
    printf("Jammed B angle range: %d - 90 degrees\n\n",
           jamMinimumAngle);

    printf("%-24s %-18s %-18s\n",
           "",
           "SIMULATION 1",
           "SIMULATION 2");

    printf("%-24s %-18d %-18d\n",
           "Iterations",
           simulation1.completedIterations,
           simulation2.completedIterations);

    printf("%-24s %-18s %-18s\n",
           "Battleship",
           simulation1.battleshipSurvived ? "ALIVE" : "SUNK",
           simulation2.battleshipSurvived ? "ALIVE" : "SUNK");

    printf("%-24s %-18d %-18d\n",
           "Escorts Destroyed",
           simulation1.totalDestroyed,
           simulation2.totalDestroyed);

    printf("%-24s %-18.4f %-18.4f\n",
           "Total Step Time",
           simulation1.totalElapsedTime,
           simulation2.totalElapsedTime);

    if (!simulation1.battleshipSurvived)
    {
        printf("Simulation 1 sinker    : E%03d at iteration %d\n",
               escorts[simulation1.finalSinkerIndex].id,
               simulation1.completedIterations);
    }

    if (!simulation2.battleshipSurvived)
    {
        printf("Simulation 2 sinker    : E%03d at iteration %d\n",
               escorts[simulation2.finalSinkerIndex].id,
               simulation2.completedIterations);
    }
}

static void runPart1BSimulation2(void)
{
    int jamAfterIteration;
    int jamMinimumAngle;

    PathSimulationResult simulation1;
    PathSimulationResult simulation2;

    pathCount = getInteger(
        "\nEnter number of path points k: ",
        MIN_PATH_POINTS,
        MAX_PATH_POINTS
    );

    generatePath();
    showPath();

    jamAfterIteration = getInteger(
        "\nEnter t, the number of normal iterations before the gun jams: ",
        1,
        pathCount - 1
    );

    jamMinimumAngle = getInteger(
        "Enter jammed minimum vertical angle theta_min [1 - 29]: ",
        1,
        29
    );

    saveInitialConditions("PART 1-B - SIMULATION 2 COMPARISON");

    printSection("SIMULATION 1 BASELINE");

    printf("\nRunning Simulation 1 first using the same initial conditions.\n");

    simulation1 = runPathScenario(
        "PART 1-B - SIMULATION 1 BASELINE",
        0,
        0,
        0,
        0
    );

    printSection("RESETTING INITIAL CONDITIONS");

    printf("\n");
    printf("Escort states, Battleship position and firing angles reset.\n");
    printf("The same generated path and battlefield will be reused.\n");

    simulation2 = runPathScenario(
        "PART 1-B - SIMULATION 2",
        1,
        jamAfterIteration,
        jamMinimumAngle,
        1
    );

    printSimulationComparison(
        simulation1,
        simulation2,
        jamAfterIteration,
        jamMinimumAngle
    );

    battleshipMinAngle = 0;
    battleshipMaxAngle = 90;

    printf("\nUpdated: simulation_history.txt\n");
    printf("Saved: final_conditions.txt\n");
}

static int selectSimulationMode(void)
{
    printSection("SELECT SIMULATION");

    printf("\n");
    printf("1. Part 1-A\n");
    printf("2. Part 1-B - Simulation 1\n");
    printf("3. Part 1-B - Simulation 2\n");
    printf("4. Return to Main Menu\n");

    return getInteger(
        "\nEnter choice [1 - 4]: ",
        1,
        4
    );
}


void simulation(void)
{
    char generateChoice;
    char exitChoice;
    int mode;

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
    printf("Allowed D: %d - %d\n\n",
           MIN_CANVAS,
           MAX_CANVAS);

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

    printf("\nValid coordinate range: 0 - %d\n\n",
           canvasSize);

    initialBattleshipX = getInteger(
        "Enter Battleship X Coordinate: ",
        0,
        canvasSize
    );

    initialBattleshipY = getInteger(
        "Enter Battleship Y Coordinate: ",
        0,
        canvasSize
    );

    battleshipX = initialBattleshipX;
    battleshipY = initialBattleshipY;

    printSection("RANDOM SEED");

    printf("\nSeed range: %d - %d\n\n",
           MIN_SEED,
           MAX_SEED);

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
            "\nGenerate Battlefield? [Y/N]: "
        );

        if (generateChoice == 'Y')
        {
            generateEscortShips();
            showEscortShips();
            break;
        }

        exitChoice = getYesNo(
            "\nExit simulation setup? [Y/N]: "
        );

        if (exitChoice == 'Y')
            return;
    }

    mode = selectSimulationMode();

    if (mode == 1)
    {
        runPart1A();
    }
    else if (mode == 2)
    {
        runPart1BSimulation1();
    }
    else if (mode == 3)
    {
        runPart1BSimulation2();
    }
}
