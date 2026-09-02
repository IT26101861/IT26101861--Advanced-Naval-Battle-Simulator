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
#define MIN_RELOAD_TIME 0.10
#define MAX_RELOAD_TIME 60.00
#define MIN_PATH_POINTS 2
#define MAX_PATH_POINTS 50

#define GRAVITY 9.81
#define PI 3.14159265358979323846
#define ANGLE_STEP 0.01
#define DESTROYED_DAMAGE 1.0
#define MAX_IMPACT_RECORDS 5000

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
    int attackOrder;
    int bShotFired;
    double bFireTime;
    double bFireVelocity;
    double bFireAngle;
    double bImpactTime;

    int firesAtBattleship;
    double eFireVelocity;
    double eFireAngle;
    double eImpactTime;

    int alive;
    int destroyedIteration;
    int hasFired;
    int totalShotsFired;
    int stepShotsFired;

    int impactApplied;
    double damageAfterImpact;
} EscortShip;

typedef struct
{
    int escortIndex;
    int shotNumber;
    double impactTime;
    double damageAfterImpact;
} EscortImpactRecord;

typedef struct
{
    int completedIterations;
    int finalSinkerIndex;
    double finalSinkTime;
    double totalElapsedTime;
    int totalDestroyed;
    int battleshipSurvived;
    double cumulativeImpact;
} PathSimulationResult;

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
static int initialBattleshipX;
static int initialBattleshipY;

static int seedValue;

static double battleshipMinVelocity = 0.0;
static double battleshipMaxVelocity;
static double battleshipReloadTime;
static double escortReloadTimes[5];

static int battleshipMinAngle = 0;
static int battleshipMaxAngle = 90;

static double battleshipMinAttackRange = 0.0;
static double battleshipMaxAttackRange = 0.0;

static int battleshipAlive = 1;
static double battleshipDamage = 0.0;

static EscortShip escorts[MAX_ESCORTS];
static EscortImpactRecord impactRecords[MAX_IMPACT_RECORDS];
static int impactRecordCount = 0;

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

static double getDouble(const char *prompt, double min, double max)
{
    char buffer[100];
    char *end;
    double value;

    while (1)
    {
        printf("%s", prompt);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            printf("\n[!] Input error. Please try again.\n");
            continue;
        }

        if (strchr(buffer, '\n') == NULL)
        {
            discardLongInput();
            printf("\n[!] Input is too long.\n\n");
            continue;
        }

        errno = 0;
        value = strtod(buffer, &end);

        if (end == buffer || errno == ERANGE || !isfinite(value))
        {
            printf("\n[!] Enter a valid number from %.2f to %.2f.\n\n",
                   min, max);
            continue;
        }

        while (isspace((unsigned char)*end))
            end++;

        if (*end != '\0' || value < min || value > max)
        {
            printf("\n[!] Enter only a number from %.2f to %.2f.\n\n",
                   min, max);
            continue;
        }

        return value;
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

/* Chooses the valid trajectory with the shortest shell travel time. */
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

static void resetEscortStepData(void)
{
    int i;

    impactRecordCount = 0;

    for (i = 0; i < numberOfEscorts; i++)
    {
        escorts[i].targetedByBattleship = 0;
        escorts[i].attackOrder = 0;
        escorts[i].bShotFired = 0;
        escorts[i].bFireTime = 0.0;
        escorts[i].bFireVelocity = 0.0;
        escorts[i].bFireAngle = 0.0;
        escorts[i].bImpactTime = -1.0;

        escorts[i].firesAtBattleship = 0;
        escorts[i].eFireVelocity = 0.0;
        escorts[i].eFireAngle = 0.0;
        escorts[i].eImpactTime = -1.0;
        escorts[i].stepShotsFired = 0;

        escorts[i].impactApplied = 0;
        escorts[i].damageAfterImpact = battleshipDamage;
    }
}

static void resetScenarioState(void)
{
    int i;

    battleshipX = initialBattleshipX;
    battleshipY = initialBattleshipY;
    battleshipAlive = 1;
    battleshipDamage = 0.0;

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
        escorts[i].hasFired = 0;
        escorts[i].totalShotsFired = 0;
        escorts[i].stepShotsFired = 0;
    }

    resetEscortStepData();
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
        escorts[i].hasFired = 0;
        escorts[i].totalShotsFired = 0;
        escorts[i].stepShotsFired = 0;
        escorts[i].impactApplied = 0;
        escorts[i].damageAfterImpact = 0.0;
    }

    battleshipAlive = 1;
    battleshipDamage = 0.0;
    resetEscortStepData();
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
    printf("B Reload Time       : %.2f seconds\n",
           battleshipReloadTime);
    printf("Attack Strategy     : Threat-first\n");
    printf("Escort Fire Mode    : Continuous\n");
    printf("E Reload Times      : EA %.2f | EB %.2f | EC %.2f | ED %.2f | EE %.2f s\n",
           escortReloadTimes[0],
           escortReloadTimes[1],
           escortReloadTimes[2],
           escortReloadTimes[3],
           escortReloadTimes[4]);
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
    printf("%-6s %-4s %-11s %-7s %-7s %-6s %-6s %-8s %-8s %-7s\n",
           "ID",
           "TYPE",
           "POSITION",
           "Vmin",
           "Vmax",
           "Amin",
           "Amax",
           "Rmin",
           "Rmax",
           "IMPACT");

    printf("---------------------------------------------------------------------------------\n");

    for (i = 0; i < numberOfEscorts; i++)
    {
        char position[32];
        char impact[16];

        snprintf(
            position,
            sizeof(position),
            "(%d,%d)",
            escorts[i].x,
            escorts[i].y
        );

        snprintf(
            impact,
            sizeof(impact),
            "%.0f%%",
            escorts[i].type->impactPower * 100.0
        );

        printf(
            "E%03d   %-4s %-11s %-7.2f %-7.2f %-6d %-6d %-8.2f %-8.2f %-7s\n",
            escorts[i].id,
            escorts[i].type->notation,
            position,
            escorts[i].minVelocity,
            escorts[i].maxVelocity,
            escorts[i].minAngle,
            escorts[i].maxAngle,
            escorts[i].minAttackRange,
            escorts[i].maxAttackRange,
            impact
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
    fprintf(file, "Reload Time       : %.2f seconds\n", battleshipReloadTime);
    fprintf(file, "Attack Strategy   : Threat-first\n");
    fprintf(file, "Escort Fire Mode : Continuous\n");
    fprintf(file, "Minimum Angle     : %d degrees\n", battleshipMinAngle);
    fprintf(file, "Maximum Angle     : %d degrees\n", battleshipMaxAngle);
    fprintf(file, "Minimum Range     : %.4f\n", battleshipMinAttackRange);
    fprintf(file, "Maximum Range     : %.4f\n", battleshipMaxAttackRange);

    fprintf(file, "\n[ BATTLEFIELD ]\n");
    fprintf(file, "Lower Left        : (0, 0)\n");
    fprintf(file, "Upper Right       : (%d, %d)\n", canvasSize, canvasSize);
    fprintf(file, "Number of Escorts : %d\n", numberOfEscorts);
    fprintf(file, "Seed Value        : %d\n", seedValue);
    fprintf(file, "E Reload Times    : EA %.2f | EB %.2f | EC %.2f | ED %.2f | EE %.2f seconds\n",
            escortReloadTimes[0],
            escortReloadTimes[1],
            escortReloadTimes[2],
            escortReloadTimes[3],
            escortReloadTimes[4]);

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
        fprintf(file, "Impact Power      : %.2f (%.0f%%)\n",
                escorts[i].type->impactPower,
                escorts[i].type->impactPower * 100.0);
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
    fprintf(file, "B Reload Time     : %.2f seconds\n", battleshipReloadTime);
    fprintf(file, "Attack Strategy   : Threat-first\n");
    fprintf(file, "Escort Fire Mode  : Continuous\n");
    fprintf(file, "E Reload Times    : EA %.2f | EB %.2f | EC %.2f | ED %.2f | EE %.2f seconds\n",
            escortReloadTimes[0],
            escortReloadTimes[1],
            escortReloadTimes[2],
            escortReloadTimes[3],
            escortReloadTimes[4]);
    fprintf(file, "Canvas            : (0,0) to (%d,%d)\n",
            canvasSize,
            canvasSize);
    fprintf(file, "Escort Count      : %d\n", numberOfEscorts);
    fprintf(file, "Seed              : %d\n", seedValue);

    return file;
}

static double escortThreatScore(int index)
{
    double score = -escorts[index].bImpactTime;

    if (escorts[index].firesAtBattleship)
    {
        score += 100000.0;
        score += escorts[index].type->impactPower * 10000.0;
        score -= escorts[index].eImpactTime * 100.0;
    }

    return score;
}

static void scheduleBattleshipAttackOrder(void)
{
    int targets[MAX_ESCORTS];
    int targetCount = 0;
    int i;

    for (i = 0; i < numberOfEscorts; i++)
    {
        if (escorts[i].targetedByBattleship)
            targets[targetCount++] = i;
    }

    for (i = 0; i < targetCount; i++)
    {
        int best = i;
        int j;

        for (j = i + 1; j < targetCount; j++)
        {
            double candidateScore = escortThreatScore(targets[j]);
            double bestScore = escortThreatScore(targets[best]);

            if (candidateScore > bestScore + 0.000001 ||
                (fabs(candidateScore - bestScore) <= 0.000001 &&
                 escorts[targets[j]].id < escorts[targets[best]].id))
            {
                best = j;
            }
        }

        if (best != i)
        {
            int temporary = targets[i];
            targets[i] = targets[best];
            targets[best] = temporary;
        }
    }

    for (i = 0; i < targetCount; i++)
    {
        int index = targets[i];

        escorts[index].attackOrder = i + 1;
        escorts[index].bShotFired = 1;
        escorts[index].bFireTime = i * battleshipReloadTime;
        escorts[index].bImpactTime += escorts[index].bFireTime;
    }
}

static void determineShotsForCurrentPoint(int cumulativeMode)
{
    int i;

    (void)cumulativeMode;

    resetEscortStepData();

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

                escorts[i].hasFired = 1;
            }
        }
    }

    scheduleBattleshipAttackOrder();
}

static void destroyTargetedEscorts(
    int iteration,
    double sinkTime,
    int *destroyedThisIteration,
    double *lastBImpactTime
)
{
    int i;

    *destroyedThisIteration = 0;
    *lastBImpactTime = 0.0;

    for (i = 0; i < numberOfEscorts; i++)
    {
        if (!escorts[i].alive)
            continue;

        if (escorts[i].targetedByBattleship)
        {
            if (!battleshipAlive &&
                escorts[i].bFireTime > sinkTime + 0.000001)
            {
                escorts[i].bShotFired = 0;
                continue;
            }

            escorts[i].alive = 0;
            escorts[i].destroyedIteration = iteration;
            (*destroyedThisIteration)++;

            if (escorts[i].bImpactTime > *lastBImpactTime)
                *lastBImpactTime = escorts[i].bImpactTime;
        }
    }
}

static void resolveSingleHitStep(
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

    for (i = 0; i < numberOfEscorts; i++)
    {
        if (!escorts[i].alive)
            continue;

        if (escorts[i].firesAtBattleship)
        {
            escorts[i].stepShotsFired = 1;
            escorts[i].totalShotsFired++;

            if (escorts[i].eImpactTime < *sinkTime)
            {
                *sinkTime = escorts[i].eImpactTime;
                *sinkerIndex = i;
            }
        }
    }

    if (*sinkerIndex >= 0)
        battleshipAlive = 0;

    destroyTargetedEscorts(
        iteration,
        *sinkTime,
        destroyedThisIteration,
        lastBImpactTime
    );

    *iterationEndTime = *lastBImpactTime;

    if (*sinkerIndex >= 0 && *sinkTime > *iterationEndTime)
        *iterationEndTime = *sinkTime;

    if (*sinkerIndex < 0)
        *sinkTime = -1.0;
}

/* Escort impacts are applied in time order until cumulative damage reaches 100%. */
static void resolveCumulativeStep(
    int iteration,
    int *sinkerIndex,
    double *sinkTime,
    int *destroyedThisIteration,
    double *lastBImpactTime,
    double *iterationEndTime
)
{
    int nextShot[MAX_ESCORTS] = {0};
    int i;
    double lastEImpactTime = 0.0;

    *sinkerIndex = -1;
    *sinkTime = -1.0;

    while (1)
    {
        int selected = -1;
        int selectedShot = 0;
        double earliest = DBL_MAX;

        for (i = 0; i < numberOfEscorts; i++)
        {
            int typeIndex;
            double launchTime;
            double impactTime;

            if (!escorts[i].firesAtBattleship)
                continue;

            typeIndex = (int)(escorts[i].type - escortTypes);
            launchTime = nextShot[i] * escortReloadTimes[typeIndex];

            if (escorts[i].targetedByBattleship &&
                escorts[i].bShotFired &&
                launchTime > escorts[i].bImpactTime + 0.000001)
            {
                continue;
            }

            impactTime = launchTime + escorts[i].eImpactTime;

            if (impactTime < earliest)
            {
                earliest = impactTime;
                selected = i;
                selectedShot = nextShot[i] + 1;
            }
        }

        if (selected < 0)
            break;

        nextShot[selected]++;
        escorts[selected].stepShotsFired++;
        escorts[selected].totalShotsFired++;

        battleshipDamage += escorts[selected].type->impactPower;

        escorts[selected].impactApplied = 1;
        escorts[selected].damageAfterImpact = battleshipDamage;

        lastEImpactTime = earliest;

        if (impactRecordCount < MAX_IMPACT_RECORDS)
        {
            impactRecords[impactRecordCount].escortIndex = selected;
            impactRecords[impactRecordCount].shotNumber = selectedShot;
            impactRecords[impactRecordCount].impactTime = earliest;
            impactRecords[impactRecordCount].damageAfterImpact = battleshipDamage;
            impactRecordCount++;
        }

        if (battleshipDamage >= DESTROYED_DAMAGE - 0.0000001)
        {
            battleshipAlive = 0;
            *sinkerIndex = selected;
            *sinkTime = earliest;
            break;
        }
    }

    destroyTargetedEscorts(
        iteration,
        *sinkTime,
        destroyedThisIteration,
        lastBImpactTime
    );

    *iterationEndTime = *lastBImpactTime;

    if (lastEImpactTime > *iterationEndTime)
        *iterationEndTime = lastEImpactTime;

    if (*sinkerIndex >= 0 && *sinkTime > *iterationEndTime)
        *iterationEndTime = *sinkTime;
}

static void printBattleshipAttackOrder(void)
{
    int order;
    int found = 0;

    printf("\nBATTLESHIP ATTACK ORDER (THREAT-FIRST)\n");

    for (order = 1; order <= numberOfEscorts; order++)
    {
        int i;

        for (i = 0; i < numberOfEscorts; i++)
        {
            if (escorts[i].attackOrder == order)
            {
                printf(
                    "%2d. E%03d (%s) | Fire %.2f s | Impact %.4f s | %s%s\n",
                    order,
                    escorts[i].id,
                    escorts[i].type->notation,
                    escorts[i].bFireTime,
                    escorts[i].bImpactTime,
                    escorts[i].firesAtBattleship ? "THREAT" : "NON-THREAT",
                    escorts[i].bShotFired ? "" : " | CANCELLED - B SUNK"
                );
                found = 1;
                break;
            }
        }
    }

    if (!found)
        printf("No Escort Ship is inside B's attack range.\n");
}

static void printBattleshipImpacts(void)
{
    int i;
    int count = 0;

    printf("\nBATTLESHIP -> ESCORT\n");

    for (i = 0; i < numberOfEscorts; i++)
    {
        if (escorts[i].targetedByBattleship && escorts[i].bShotFired)
        {
            printf(
                "E%03d (%s) destroyed at %.4f s\n",
                escorts[i].id,
                escorts[i].type->notation,
                escorts[i].bImpactTime
            );

            count++;
        }
    }

    if (count == 0)
        printf("No Escort Ships destroyed in this iteration.\n");
}

static void printEscortImpacts(
    int cumulativeMode,
    int sinkerIndex
)
{
    int printed[MAX_ESCORTS] = {0};
    int count = 0;

    printf("\nESCORT -> BATTLESHIP\n");

    if (cumulativeMode)
    {
        int recordIndex;

        for (recordIndex = 0;
             recordIndex < impactRecordCount;
             recordIndex++)
        {
            EscortImpactRecord *record = &impactRecords[recordIndex];
            EscortShip *escort = &escorts[record->escortIndex];

            printf(
                "E%03d (%s) shot %d -> B at %.4f s | Impact %.0f%% | Cumulative %.0f%%%s\n",
                escort->id,
                escort->type->notation,
                record->shotNumber,
                record->impactTime,
                escort->type->impactPower * 100.0,
                record->damageAfterImpact * 100.0,
                (record->escortIndex == sinkerIndex &&
                 recordIndex == impactRecordCount - 1)
                    ? "  <-- DESTROYS B"
                    : ""
            );
        }

        if (impactRecordCount == 0)
            printf("No Escort Ship attacks B in this iteration.\n");

        return;
    }

    while (1)
    {
        int i;
        int selected = -1;
        double earliest = DBL_MAX;

        for (i = 0; i < numberOfEscorts; i++)
        {
            if (escorts[i].firesAtBattleship &&
                !printed[i] &&
                escorts[i].eImpactTime < earliest)
            {
                earliest = escorts[i].eImpactTime;
                selected = i;
            }
        }

        if (selected < 0)
            break;

        printed[selected] = 1;
        count++;

        if (!cumulativeMode)
        {
            printf(
                "E%03d (%s) -> B at %.4f s%s\n",
                escorts[selected].id,
                escorts[selected].type->notation,
                escorts[selected].eImpactTime,
                (selected == sinkerIndex)
                    ? "  <-- FIRST IMPACT / SINKS B"
                    : ""
            );
        }
        else
        {
            printf(
                "E%03d (%s) -> B at %.4f s | Impact %.0f%%",
                escorts[selected].id,
                escorts[selected].type->notation,
                escorts[selected].eImpactTime,
                escorts[selected].type->impactPower * 100.0
            );

            if (escorts[selected].impactApplied)
            {
                printf(
                    " | Cumulative %.0f%%",
                    escorts[selected].damageAfterImpact * 100.0
                );
            }
            else
            {
                printf(" | Not counted after B sank");
            }

            if (selected == sinkerIndex)
                printf("  <-- DESTROYS B");

            printf("\n");
        }
    }

    if (count == 0)
        printf("No Escort Ship attacks B in this iteration.\n");
}

static void printStepResult(
    int iteration,
    int cumulativeMode,
    int sinkerIndex,
    double sinkTime,
    int destroyedThisIteration,
    double iterationEndTime
)
{
    printSection("ITERATION RESULT");

    printf("\nIteration            : %d\n", iteration);
    printf("Battleship Position  : (%d, %d)\n",
           battleshipX,
           battleshipY);
    printf("B Vertical Angle     : %d - %d degrees\n",
           battleshipMinAngle,
           battleshipMaxAngle);

    if (cumulativeMode)
    {
        printf("B Cumulative Impact  : %.2f (%.0f%%)\n",
               battleshipDamage,
               battleshipDamage * 100.0);
    }

    printBattleshipAttackOrder();
    printBattleshipImpacts();
    printEscortImpacts(cumulativeMode, sinkerIndex);

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

        if (cumulativeMode)
        {
            printf("Cumulative impact                : %.0f%%\n",
                   battleshipDamage * 100.0);
        }
    }
}

static void writeStepHistory(
    FILE *file,
    int iteration,
    int cumulativeMode,
    int sinkerIndex,
    double sinkTime,
    int destroyedThisIteration,
    double iterationEndTime,
    double totalElapsedTime
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

    if (cumulativeMode)
    {
        fprintf(file, "B Impact          : %.2f (%.0f%%)\n",
                battleshipDamage,
                battleshipDamage * 100.0);
    }

    fprintf(file, "\nB ATTACK ORDER (THREAT-FIRST)\n");

    {
        int order;

        for (order = 1; order <= numberOfEscorts; order++)
        {
            for (i = 0; i < numberOfEscorts; i++)
            {
                if (escorts[i].attackOrder == order)
                {
                    fprintf(
                        file,
                        "%02d | E%03d | %s | Fire %.4f s | Impact %.4f s | %s%s\n",
                        order,
                        escorts[i].id,
                        escorts[i].type->notation,
                        escorts[i].bFireTime,
                        escorts[i].bImpactTime,
                        escorts[i].firesAtBattleship ? "THREAT" : "NON-THREAT",
                        escorts[i].bShotFired ? "" : " | CANCELLED - B SUNK"
                    );
                    break;
                }
            }
        }
    }

    fprintf(file, "\nB -> E IMPACTS\n");

    for (i = 0; i < numberOfEscorts; i++)
    {
        if (escorts[i].targetedByBattleship && escorts[i].bShotFired)
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

    if (cumulativeMode)
    {
        int recordIndex;

        for (recordIndex = 0;
             recordIndex < impactRecordCount;
             recordIndex++)
        {
            EscortImpactRecord *record = &impactRecords[recordIndex];
            EscortShip *escort = &escorts[record->escortIndex];

            fprintf(
                file,
                "E%03d | %s | Shot %d | Hit %.4f s | Impact %.0f%% | Cumulative %.0f%%%s\n",
                escort->id,
                escort->type->notation,
                record->shotNumber,
                record->impactTime,
                escort->type->impactPower * 100.0,
                record->damageAfterImpact * 100.0,
                (record->escortIndex == sinkerIndex &&
                 recordIndex == impactRecordCount - 1)
                    ? " | DESTROYS B"
                    : ""
            );
        }
    }
    else for (i = 0; i < numberOfEscorts; i++)
    {
        if (escorts[i].firesAtBattleship)
        {
            if (!cumulativeMode)
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
            else
            {
                fprintf(
                    file,
                    "E%03d | %s | Hit %.4f s | Impact %.0f%%",
                    escorts[i].id,
                    escorts[i].type->notation,
                    escorts[i].eImpactTime,
                    escorts[i].type->impactPower * 100.0
                );

                if (escorts[i].impactApplied)
                {
                    fprintf(
                        file,
                        " | Cumulative %.0f%%",
                        escorts[i].damageAfterImpact * 100.0
                    );
                }
                else
                {
                    fprintf(file, " | Not counted after B sank");
                }

                if (i == sinkerIndex)
                    fprintf(file, " | DESTROYS B");

                fprintf(file, "\n");
            }
        }
    }

    fprintf(file, "\nDestroyed in Step : %d\n",
            destroyedThisIteration);
    fprintf(file, "Step End Time     : %.4f seconds\n",
            iterationEndTime);
    fprintf(file, "Total Step Time   : %.4f seconds\n",
            totalElapsedTime);

    if (cumulativeMode)
    {
        fprintf(file, "Cumulative Impact : %.2f (%.0f%%)\n",
                battleshipDamage,
                battleshipDamage * 100.0);
    }

    if (sinkerIndex >= 0)
    {
        fprintf(file, "B Status          : SUNK\n");
        fprintf(file, "Sunk By           : E%03d\n",
                escorts[sinkerIndex].id);
        fprintf(file, "Sink Time         : %.4f seconds\n",
                sinkTime);
    }
    else
    {
        fprintf(file, "B Status          : ALIVE\n");
    }
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

static void saveFinalConditions(
    const char *mode,
    int cumulativeMode,
    int completedIterations,
    int sinkerIndex,
    double sinkTime,
    double totalElapsedTime
)
{
    FILE *file;
    int i;
    int totalDestroyed = countDestroyedEscorts();

    file = fopen("final_conditions.txt", "w");

    if (file == NULL)
    {
        printf("\n[!] Could not create final_conditions.txt\n");
        return;
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

    if (cumulativeMode)
    {
        fprintf(file, "Cumulative Impact : %.2f (%.0f%%)\n",
                battleshipDamage,
                battleshipDamage * 100.0);
    }

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

        fprintf(file, " | E Shots Fired: %d",
                escorts[i].totalShotsFired);

        fprintf(file, "\n");
    }

    fclose(file);
}

static void writeFinalHistoryState(
    FILE *file,
    int cumulativeMode,
    int completedIterations,
    int sinkerIndex,
    double sinkTime,
    double totalElapsedTime
)
{
    int i;
    int totalDestroyed = countDestroyedEscorts();

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

    if (cumulativeMode)
    {
        fprintf(file, "Cumulative Impact : %.2f (%.0f%%)\n",
                battleshipDamage,
                battleshipDamage * 100.0);
    }

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

        fprintf(file, " | E Shots Fired: %d",
                escorts[i].totalShotsFired);

        fprintf(file, "\n");
    }

    fprintf(file, "=============================================================\n");
}

static PathSimulationResult runPathScenario(
    const char *mode,
    int cumulativeMode,
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
    result.cumulativeImpact = 0.0;

    resetScenarioState();

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

        if (cumulativeMode)
        {
            fprintf(history, "Damage Model      : Escort impact power\n");
            fprintf(history, "Destroy B At      : 100%% cumulative impact\n");
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

        printf("\nPath Point %d of %d\n",
               iteration,
               pathCount);

        if (jamEnabled && iteration == jamAfterIteration + 1)
        {
            printf("Battleship gun is now JAMMED.\n");
            printf("New vertical angle range: %d - 90 degrees\n",
                   jamMinimumAngle);
        }

        determineShotsForCurrentPoint(cumulativeMode);

        if (cumulativeMode)
        {
            resolveCumulativeStep(
                iteration,
                &sinkerIndex,
                &sinkTime,
                &destroyedThisIteration,
                &lastBImpactTime,
                &iterationEndTime
            );
        }
        else
        {
            resolveSingleHitStep(
                iteration,
                &sinkerIndex,
                &sinkTime,
                &destroyedThisIteration,
                &lastBImpactTime,
                &iterationEndTime
            );
        }

        result.totalElapsedTime += iterationEndTime;
        result.completedIterations = iteration;

        printStepResult(
            iteration,
            cumulativeMode,
            sinkerIndex,
            sinkTime,
            destroyedThisIteration,
            iterationEndTime
        );

        if (history != NULL)
        {
            writeStepHistory(
                history,
                iteration,
                cumulativeMode,
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
    result.cumulativeImpact = battleshipDamage;

    printSection("PATH SIMULATION FINAL RESULT");

    printf("\nMode                 : %s\n", mode);
    printf("Iterations completed : %d of %d\n",
           result.completedIterations,
           pathCount);
    printf("Escorts destroyed    : %d\n",
           result.totalDestroyed);
    printf("Escorts remaining    : %d\n",
           numberOfEscorts - result.totalDestroyed);

    if (cumulativeMode)
    {
        printf("Cumulative B impact  : %.2f (%.0f%%)\n",
               result.cumulativeImpact,
               result.cumulativeImpact * 100.0);
    }

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
            cumulativeMode,
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
            cumulativeMode,
            result.completedIterations,
            result.finalSinkerIndex,
            result.finalSinkTime,
            result.totalElapsedTime
        );
    }

    return result;
}

static void runSinglePointScenario(
    const char *mode,
    int cumulativeMode
)
{
    int sinkerIndex;
    int destroyedThisIteration;
    double sinkTime;
    double lastBImpactTime;
    double iterationEndTime;

    FILE *history;
    int runNumber;

    pathCount = 0;
    resetScenarioState();

    saveInitialConditions(mode);

    determineShotsForCurrentPoint(cumulativeMode);

    if (cumulativeMode)
    {
        resolveCumulativeStep(
            1,
            &sinkerIndex,
            &sinkTime,
            &destroyedThisIteration,
            &lastBImpactTime,
            &iterationEndTime
        );
    }
    else
    {
        resolveSingleHitStep(
            1,
            &sinkerIndex,
            &sinkTime,
            &destroyedThisIteration,
            &lastBImpactTime,
            &iterationEndTime
        );
    }

    printStepResult(
        1,
        cumulativeMode,
        sinkerIndex,
        sinkTime,
        destroyedThisIteration,
        iterationEndTime
    );

    printSection("FINAL RESULT");

    printf("\nBattleship Status    : %s\n",
           battleshipAlive ? "ALIVE" : "SUNK");
    printf("Escorts destroyed    : %d\n",
           countDestroyedEscorts());

    if (cumulativeMode)
    {
        printf("Cumulative B impact  : %.2f (%.0f%%)\n",
               battleshipDamage,
               battleshipDamage * 100.0);
    }

    if (!battleshipAlive)
    {
        printf("Destroyed by         : E%03d\n",
               escorts[sinkerIndex].id);
        printf("Sink time            : %.4f seconds\n",
               sinkTime);
    }
    else
    {
        printf("Battle end time      : %.4f seconds\n",
               iterationEndTime);
    }

    history = openHistoryRun(mode, &runNumber);

    if (history != NULL)
    {
        writeStepHistory(
            history,
            1,
            cumulativeMode,
            sinkerIndex,
            sinkTime,
            destroyedThisIteration,
            iterationEndTime,
            iterationEndTime
        );

        writeFinalHistoryState(
            history,
            cumulativeMode,
            1,
            sinkerIndex,
            sinkTime,
            iterationEndTime
        );

        fclose(history);
    }

    saveFinalConditions(
        mode,
        cumulativeMode,
        1,
        sinkerIndex,
        sinkTime,
        iterationEndTime
    );

    printf("\nUpdated: simulation_history.txt\n");
    printf("Saved: final_conditions.txt\n");
}

static void printSimulationComparison(
    const char *title,
    PathSimulationResult simulation1,
    PathSimulationResult simulation2,
    int cumulativeMode,
    int jamAfterIteration,
    int jamMinimumAngle
)
{
    printSection(title);

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

    if (cumulativeMode)
    {
        char simulation1Impact[24];
        char simulation2Impact[24];

        snprintf(
            simulation1Impact,
            sizeof(simulation1Impact),
            "%.0f%%",
            simulation1.cumulativeImpact * 100.0
        );
        snprintf(
            simulation2Impact,
            sizeof(simulation2Impact),
            "%.0f%%",
            simulation2.cumulativeImpact * 100.0
        );

        printf("%-24s %-18s %-18s\n",
               "Cumulative Impact",
               simulation1Impact,
               simulation2Impact);
    }

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

static void runPart1BSimulation1(int cumulativeMode)
{
    const char *mode;

    mode = cumulativeMode
        ? "PART 2-B - PART 1-C / PART 1-B SIMULATION 1"
        : "PART 2-B - PART 1-B SIMULATION 1";

    pathCount = getInteger(
        "\nEnter number of path points k: ",
        MIN_PATH_POINTS,
        MAX_PATH_POINTS
    );

    generatePath();
    showPath();

    saveInitialConditions(mode);

    runPathScenario(
        mode,
        cumulativeMode,
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

static void runPart1BSimulation2(int cumulativeMode)
{
    int jamAfterIteration;
    int jamMinimumAngle;

    PathSimulationResult simulation1;
    PathSimulationResult simulation2;

    const char *baselineMode;
    const char *jammedMode;
    const char *initialMode;
    const char *comparisonTitle;

    baselineMode = cumulativeMode
        ? "PART 2-B - PART 1-C / PART 1-B SIMULATION 1 BASELINE"
        : "PART 2-B - PART 1-B SIMULATION 1 BASELINE";

    jammedMode = cumulativeMode
        ? "PART 2-B - PART 1-C / PART 1-B SIMULATION 2"
        : "PART 2-B - PART 1-B SIMULATION 2";

    initialMode = cumulativeMode
        ? "PART 2-B - PART 1-C / PART 1-B SIMULATION 2 COMPARISON"
        : "PART 2-B - PART 1-B SIMULATION 2 COMPARISON";

    comparisonTitle = cumulativeMode
        ? "PART 2-B / PART 1-C - SIMULATION COMPARISON"
        : "PART 2-B - SIMULATION COMPARISON";

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

    saveInitialConditions(initialMode);

    printSection("SIMULATION 1 BASELINE");
    printf("\nRunning Simulation 1 with the same initial conditions.\n");

    simulation1 = runPathScenario(
        baselineMode,
        cumulativeMode,
        0,
        0,
        0,
        0
    );

    printSection("RESETTING INITIAL CONDITIONS");
    printf("\nSame battlefield, path and Escort data will be reused.\n");

    simulation2 = runPathScenario(
        jammedMode,
        cumulativeMode,
        1,
        jamAfterIteration,
        jamMinimumAngle,
        1
    );

    printSimulationComparison(
        comparisonTitle,
        simulation1,
        simulation2,
        cumulativeMode,
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
    printSection("PART 2-B - SELECT SIMULATION");

    printf("\n");
    printf("B uses threat-first sequential fire; Escorts fire continuously.\n\n");
    printf("1. Redo Part 1-A\n");
    printf("2. Redo Part 1-B\n");
    printf("3. Redo Part 1-C\n");
    printf("4. Return to Main Menu\n");

    return getInteger(
        "\nEnter choice [1 - 4]: ",
        1,
        4
    );
}

static int selectPart1BMode(void)
{
    printSection("PART 1-B");

    printf("\n");
    printf("1. Simulation 1 - Battleship Path\n");
    printf("2. Simulation 2 - Jammed Gun\n");
    printf("3. Return\n");

    return getInteger(
        "\nEnter choice [1 - 3]: ",
        1,
        3
    );
}

static int selectPart1CMode(void)
{
    printSection("PART 1-C");

    printf("\n");
    printf("Escort impact powers:\n");
    printf("EA = 8%% | EB = 6%% | EC = 7%% | ED = 5%% | EE = 4%%\n\n");

    printf("1. Redo Part 1-A with cumulative impact\n");
    printf("2. Redo Part 1-B Simulation 1 with cumulative impact\n");
    printf("3. Redo Part 1-B Simulation 2 with cumulative impact\n");
    printf("4. Return\n");

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
    int partBChoice = 0;
    int partCChoice = 0;

    clearScreen();
    printBattleshipArt();

    mode = selectSimulationMode();

    if (mode == 4)
        return;

    if (mode == 2)
    {
        partBChoice = selectPart1BMode();

        if (partBChoice == 3)
            return;
    }
    else if (mode == 3)
    {
        partCChoice = selectPart1CMode();

        if (partCChoice == 4)
            return;
    }

    clearScreen();
    printBattleshipArt();

    showBattleshipMenu();

    battleshipChoice = getInteger(
        "\nEnter battleship number [1 - 4]: ",
        1,
        4
    );

    printSection("BATTLESHIP RELOAD TIME");

    printf("\nEnter T_B for %s [%c].\n",
           getBattleshipName(),
           getBattleshipNotation());
    printf("Allowed reload time: %.2f - %.2f seconds\n\n",
           MIN_RELOAD_TIME,
           MAX_RELOAD_TIME);

    battleshipReloadTime = getDouble(
        "Enter time between B gun firings: ",
        MIN_RELOAD_TIME,
        MAX_RELOAD_TIME
    );

    printSection("ESCORT RELOAD TIMES");

    printf("\nEnter T_E for all five Escort types.\n");

    {
        int typeIndex;

        for (typeIndex = 0; typeIndex < 5; typeIndex++)
        {
            char prompt[100];

            snprintf(
                prompt,
                sizeof(prompt),
                "Enter %s reload time [%.2f - %.2f seconds]: ",
                escortTypes[typeIndex].notation,
                MIN_RELOAD_TIME,
                MAX_RELOAD_TIME
            );

            escortReloadTimes[typeIndex] = getDouble(
                prompt,
                MIN_RELOAD_TIME,
                MAX_RELOAD_TIME
            );
        }
    }

    printSection("BATTLESHIP MAXIMUM SHELL VELOCITY");

    printf("\nBattleship minimum shell velocity is fixed at 0.\n");
    printf(
        "Allowed Battleship Vmax: %d - %d\n\n",
        MIN_BATTLESHIP_VMAX,
        MAX_BATTLESHIP_VMAX
    );

    battleshipMaxVelocity = (double)getInteger(
        "Enter Battleship Maximum Shell Velocity: ",
        MIN_BATTLESHIP_VMAX,
        MAX_BATTLESHIP_VMAX
    );

    printSection("BATTLEFIELD SIZE");

    printf("\nThe battlefield is a square from (0,0) to (D,D).\n");
    printf(
        "Allowed D: %d - %d\n\n",
        MIN_CANVAS,
        MAX_CANVAS
    );

    canvasSize = getInteger(
        "Enter Canvas Size D: ",
        MIN_CANVAS,
        MAX_CANVAS
    );

    printSection("NUMBER OF ESCORT SHIPS");

    printf(
        "\nAllowed number of Escort Ships: %d - %d\n\n",
        MIN_ESCORTS,
        MAX_ESCORTS
    );

    numberOfEscorts = getInteger(
        "Enter Number of Escort Ships: ",
        MIN_ESCORTS,
        MAX_ESCORTS
    );

    printSection("BATTLESHIP STARTING POSITION");

    printf(
        "\nValid coordinate range: 0 - %d\n\n",
        canvasSize
    );

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

    printf(
        "\nSeed range: %d - %d\n\n",
        MIN_SEED,
        MAX_SEED
    );

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

    if (mode == 1)
    {
        runSinglePointScenario(
            "PART 2-B - PART 1-A",
            0
        );
    }
    else if (mode == 2)
    {
        if (partBChoice == 1)
        {
            runPart1BSimulation1(0);
        }
        else if (partBChoice == 2)
        {
            runPart1BSimulation2(0);
        }
    }
    else if (mode == 3)
    {
        if (partCChoice == 1)
        {
            runSinglePointScenario(
                "PART 2-B - PART 1-C / PART 1-A",
                1
            );
        }
        else if (partCChoice == 2)
        {
            runPart1BSimulation1(1);
        }
        else if (partCChoice == 3)
        {
            runPart1BSimulation2(1);
        }
    }
}
