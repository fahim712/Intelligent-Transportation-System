/*
 * Intelligent Integrated Public Transportation & Traffic Control System
 * Educational C99 simulation project
 *
 * Build:
 *   gcc -std=c99 -Wall -Wextra -pedantic main.c -o its_simulation -lm
 *
 * Run:
 *   ./its_simulation
 *
 * This is a simplified decision-engine simulation, not a certified
 * real-world traffic signal controller.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_ROADS 5
#define MAX_BUSES 5
#define MAX_ROUTES 5

#define FIXED_GREEN 30.0
#define MIN_GREEN 15.0
#define MAX_GREEN 60.0
#define BUS_PRIORITY_THRESHOLD 60.0

typedef struct {
    int id;
    char name[50];
    int vehicle_count;
    double average_speed_kmh;
    int queue_length;
    int capacity;
    double density_ratio;
    double congestion_percent;
    double adaptive_green_seconds;
} RoadTraffic;

typedef struct {
    int id;
    int route_id;
    int passengers;
    int passenger_capacity;
    double delay_minutes;
    char location[50];
    double priority_score;
    int priority_granted;
} PublicBus;

typedef struct {
    int id;
    char name[50];
    double distance_km;
    double average_speed_kmh;
    double congestion_factor;
    double travel_time_minutes;
    double route_cost;
} RouteOption;

typedef struct {
    double average_wait_minutes;
    double average_bus_delay_minutes;
    double estimated_throughput;
    double efficiency_percent;
} SimulationKPI;

static RoadTraffic roads[MAX_ROADS];
static PublicBus buses[MAX_BUSES];
static RouteOption routes[MAX_ROUTES];
static int road_count, bus_count, route_count;

/* Read an integer from one complete input line and validate its range. */
static int read_int(const char *prompt, int min_value, int max_value)
{
    char line[128], extra;
    int value;

    for (;;) {
        printf("%s", prompt);
        if (fgets(line, sizeof line, stdin) == NULL) {
            puts("\nInput ended. Exiting.");
            exit(EXIT_SUCCESS);
        }
        if (sscanf(line, " %d %c", &value, &extra) == 1 &&
            value >= min_value && value <= max_value) {
            return value;
        }
        printf("Please enter a whole number from %d to %d.\n",
               min_value, max_value);
    }
}

/* Read a finite floating-point value from one complete input line. */
static double read_double(const char *prompt, double min_value,
                          double max_value)
{
    char line[128], extra;
    double value;

    for (;;) {
        printf("%s", prompt);
        if (fgets(line, sizeof line, stdin) == NULL) {
            puts("\nInput ended. Exiting.");
            exit(EXIT_SUCCESS);
        }
        if (sscanf(line, " %lf %c", &value, &extra) == 1 &&
            isfinite(value) && value >= min_value && value <= max_value) {
            return value;
        }
        printf("Please enter a number from %.2f to %.2f.\n",
               min_value, max_value);
    }
}

/* Read a short text field, removing the trailing newline if present. */
static void read_text(const char *prompt, char *out, size_t out_size)
{
    size_t length;

    printf("%s", prompt);
    if (fgets(out, (int)out_size, stdin) == NULL) {
        puts("\nInput ended. Exiting.");
        exit(EXIT_SUCCESS);
    }

    length = strlen(out);
    if (length > 0 && out[length - 1] == '\n') {
        out[length - 1] = '\0';
    } else {
        /* Discard the rest of an overlong input line. */
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {
            /* discard */
        }
    }

    if (out[0] == '\0') {
        strcpy(out, "Unnamed");
    }
}

/* Collect traffic measurements for each simulated road/intersection. */
static void input_traffic_data(void)
{
    int i;

    puts("\n=== TRAFFIC DATA INPUT ===");
    road_count = read_int("Number of roads/intersections (1-5): ",
                          1, MAX_ROADS);

    for (i = 0; i < road_count; ++i) {
        printf("\nRoad %d\n", i + 1);
        roads[i].id = i + 1;
        read_text("Road name: ", roads[i].name, sizeof roads[i].name);
        roads[i].vehicle_count =
            read_int("Vehicle count (0-10000): ", 0, 10000);
        roads[i].average_speed_kmh =
            read_double("Average speed in km/h (0.1-150): ", 0.1, 150.0);
        roads[i].queue_length =
            read_int("Queue length in vehicles (0-10000): ", 0, 10000);
        roads[i].capacity =
            read_int("Road capacity in vehicles (1-10000): ", 1, 10000);
    }
}

/* Collect passenger load, schedule delay and current location for buses. */
static void input_bus_data(void)
{
    int i;

    puts("\n=== PUBLIC BUS DATA INPUT ===");
    bus_count = read_int("Number of buses (1-5): ", 1, MAX_BUSES);

    for (i = 0; i < bus_count; ++i) {
        printf("\nBus %d\n", i + 1);
        buses[i].id = i + 1;
        buses[i].route_id = read_int("Bus route ID (1-1000): ", 1, 1000);
        buses[i].passenger_capacity =
            read_int("Passenger capacity (1-200): ", 1, 200);
        buses[i].passengers =
            read_int("Current passengers: ", 0,
                     buses[i].passenger_capacity);
        buses[i].delay_minutes =
            read_double("Schedule delay in minutes (0-180): ", 0.0, 180.0);
        read_text("Current bus location: ",
                  buses[i].location, sizeof buses[i].location);
    }
}

/* Collect alternative route data. Congestion factor is between 0 and 1. */
static void input_route_data(void)
{
    int i;

    puts("\n=== ROUTE DATA INPUT ===");
    route_count = read_int("Number of alternative routes (1-5): ",
                           1, MAX_ROUTES);

    for (i = 0; i < route_count; ++i) {
        printf("\nRoute %d\n", i + 1);
        routes[i].id = i + 1;
        read_text("Route name: ", routes[i].name, sizeof routes[i].name);
        routes[i].distance_km =
            read_double("Distance in km (0.1-500): ", 0.1, 500.0);
        routes[i].average_speed_kmh =
            read_double("Average speed in km/h (1-150): ", 1.0, 150.0);
        routes[i].congestion_factor =
            read_double("Congestion factor (0.0-1.0): ", 0.0, 1.0);
    }
}

/*
 * Density ratio = vehicle count / road capacity.
 * Congestion percentage is density ratio * 100, capped at 100 for display.
 */
static void calculate_congestion(void)
{
    int i;

    puts("\n=== CONGESTION ANALYSIS ===");
    for (i = 0; i < road_count; ++i) {
        roads[i].density_ratio =
            (double)roads[i].vehicle_count / roads[i].capacity;
        roads[i].congestion_percent = roads[i].density_ratio * 100.0;

        if (roads[i].congestion_percent > 100.0) {
            roads[i].congestion_percent = 100.0;
        }

        printf("\n%-20s density: %.2f | congestion: %.1f%% | ",
               roads[i].name, roads[i].density_ratio,
               roads[i].congestion_percent);

        if (roads[i].congestion_percent < 30.0) {
            puts("LOW");
        } else if (roads[i].congestion_percent < 70.0) {
            puts("MODERATE");
        } else {
            puts("HIGH");
        }
    }
}

/*
 * Passenger percentage = passengers / passenger capacity * 100.
 * Delay percentage = delay / 30 minutes * 100, capped at 100.
 * Priority score = 60% passenger percentage + 40% delay percentage.
 */
static void calculate_bus_priority(void)
{
    int i;

    puts("\n=== BUS PRIORITY ANALYSIS ===");
    for (i = 0; i < bus_count; ++i) {
        double passenger_percent =
            100.0 * buses[i].passengers / buses[i].passenger_capacity;
        double delay_percent = 100.0 * buses[i].delay_minutes / 30.0;

        if (delay_percent > 100.0) {
            delay_percent = 100.0;
        }

        buses[i].priority_score =
            0.60 * passenger_percent + 0.40 * delay_percent;
        buses[i].priority_granted =
            buses[i].priority_score >= BUS_PRIORITY_THRESHOLD;

        printf("\nBus %d | route %d | location: %s\n",
               buses[i].id, buses[i].route_id, buses[i].location);
        printf("Passengers: %d/%d | delay: %.1f min\n",
               buses[i].passengers, buses[i].passenger_capacity,
               buses[i].delay_minutes);
        printf("Priority score: %.1f/100 -> %s\n",
               buses[i].priority_score,
               buses[i].priority_granted ? "PRIORITY REQUEST" : "NORMAL");
    }
}

/*
 * Demand = 70% congestion + 30% queue ratio.
 * Green time = 20 + 0.35 * demand, with a 5-second bus extension
 * when a priority bus is mapped to the same road ID.
 * The result is clamped to the configured 15-60 second range.
 *
 * For this teaching example, bus route ID is treated as the road ID.
 * A real system would use GPS/map matching to identify the next signal.
 */
static void calculate_adaptive_signals(void)
{
    int i, j;

    puts("\n=== ADAPTIVE SIGNAL CONTROL ===");
    for (i = 0; i < road_count; ++i) {
        double queue_percent =
            100.0 * roads[i].queue_length / roads[i].capacity;
        double demand;
        int priority_bus_present = 0;

        if (queue_percent > 100.0) {
            queue_percent = 100.0;
        }

        demand = 0.70 * roads[i].congestion_percent +
                 0.30 * queue_percent;

        roads[i].adaptive_green_seconds = 20.0 + 0.35 * demand;

        for (j = 0; j < bus_count; ++j) {
            if (buses[j].route_id == roads[i].id &&
                buses[j].priority_granted) {
                priority_bus_present = 1;
            }
        }

        if (priority_bus_present) {
            roads[i].adaptive_green_seconds += 5.0;
        }

        if (roads[i].adaptive_green_seconds < MIN_GREEN) {
            roads[i].adaptive_green_seconds = MIN_GREEN;
        }
        if (roads[i].adaptive_green_seconds > MAX_GREEN) {
            roads[i].adaptive_green_seconds = MAX_GREEN;
        }

        printf("%-20s fixed: %5.1fs | adaptive: %5.1fs | ",
               roads[i].name, FIXED_GREEN,
               roads[i].adaptive_green_seconds);

        if (priority_bus_present) {
            puts("BUS PRIORITY EXTENSION");
        } else if (roads[i].adaptive_green_seconds > FIXED_GREEN) {
            puts("GREEN EXTENSION");
        } else if (roads[i].adaptive_green_seconds < FIXED_GREEN) {
            puts("REDUCED GREEN");
        } else {
            puts("UNCHANGED");
        }
    }
}

/*
 * Travel time (minutes) = distance / speed * 60.
 * Route cost = travel time * (1 + 2 * congestion factor).
 * The route with the smallest cost is recommended.
 */
static void calculate_route_costs(void)
{
    int i, best = 0;

    puts("\n=== TRAFFIC-AWARE ROUTE EVALUATION ===");
    for (i = 0; i < route_count; ++i) {
        routes[i].travel_time_minutes =
            routes[i].distance_km / routes[i].average_speed_kmh * 60.0;
        routes[i].route_cost =
            routes[i].travel_time_minutes *
            (1.0 + 2.0 * routes[i].congestion_factor);

        printf("%-22s distance: %5.1f km | time: %5.1f min | "
               "congestion: %.2f | cost: %6.2f\n",
               routes[i].name, routes[i].distance_km,
               routes[i].travel_time_minutes,
               routes[i].congestion_factor, routes[i].route_cost);

        if (routes[i].route_cost < routes[best].route_cost) {
            best = i;
        }
    }

    puts("\nRecommended route:");
    printf("  %s (estimated travel time %.1f minutes; cost %.2f)\n",
           routes[best].name, routes[best].travel_time_minutes,
           routes[best].route_cost);
}

/*
 * Estimate performance for either fixed-time (adaptive == 0) or
 * adaptive (adaptive != 0) control.
 *
 * This is a simplified comparative model:
 * base wait = 8 + 0.30*congestion + 0.70*queue percentage
 * green adjustment = 0.50*(green time - 30)
 * estimated wait = max(0, base wait - green adjustment)
 * estimated throughput = vehicle count * (1 - wait/200)
 *
 * It is an illustrative KPI estimate, not a traffic-flow measurement.
 */
static SimulationKPI calculate_kpi(int adaptive)
{
    SimulationKPI kpi = {0.0, 0.0, 0.0, 0.0};
    double weighted_wait = 0.0;
    double total_vehicles = 0.0;
    double total_bus_delay = 0.0;
    int i;

    for (i = 0; i < road_count; ++i) {
        double green = adaptive ? roads[i].adaptive_green_seconds
                                : FIXED_GREEN;
        double queue_percent =
            100.0 * roads[i].queue_length / roads[i].capacity;
        double base_wait, wait, throughput;

        if (queue_percent > 100.0) {
            queue_percent = 100.0;
        }

        base_wait = 8.0 +
                    0.30 * roads[i].congestion_percent +
                    0.70 * queue_percent;
        wait = base_wait - 0.50 * (green - FIXED_GREEN);

        if (wait < 0.0) {
            wait = 0.0;
        }

        weighted_wait += wait * roads[i].vehicle_count;
        total_vehicles += roads[i].vehicle_count;

        throughput = roads[i].vehicle_count * (1.0 - wait / 200.0);
        if (throughput < 0.0) {
            throughput = 0.0;
        }
        if (throughput > roads[i].vehicle_count) {
            throughput = roads[i].vehicle_count;
        }
        kpi.estimated_throughput += throughput;
    }

    if (total_vehicles > 0.0) {
        kpi.average_wait_minutes = weighted_wait / total_vehicles;
        kpi.efficiency_percent =
            100.0 * kpi.estimated_throughput / total_vehicles;
    }

    for (i = 0; i < bus_count; ++i) {
        double delay = buses[i].delay_minutes;

        /* Model a 25% delay reduction for priority buses under adaptive control. */
        if (adaptive && buses[i].priority_granted) {
            delay *= 0.75;
        }
        total_bus_delay += delay;
    }

    if (bus_count > 0) {
        kpi.average_bus_delay_minutes = total_bus_delay / bus_count;
    }

    return kpi;
}

/* Print a side-by-side comparison of fixed and adaptive KPI estimates. */
static void compare_kpis(void)
{
    SimulationKPI fixed = calculate_kpi(0);
    SimulationKPI adaptive = calculate_kpi(1);

    double wait_change = 0.0;
    double bus_change = 0.0;
    double throughput_change = 0.0;

    puts("\n=== PERFORMANCE COMPARISON ===");
    printf("%-30s %15s %15s\n",
           "KPI", "Fixed control", "Adaptive control");
    puts("----------------------------------------------------------------");

    printf("%-30s %15.2f %15.2f\n",
           "Average vehicle wait (min)",
           fixed.average_wait_minutes, adaptive.average_wait_minutes);
    printf("%-30s %15.2f %15.2f\n",
           "Average bus delay (min)",
           fixed.average_bus_delay_minutes,
           adaptive.average_bus_delay_minutes);
    printf("%-30s %15.2f %15.2f\n",
           "Estimated throughput",
           fixed.estimated_throughput, adaptive.estimated_throughput);
    printf("%-30s %14.2f%% %14.2f%%\n",
           "Efficiency estimate",
           fixed.efficiency_percent, adaptive.efficiency_percent);

    puts("----------------------------------------------------------------");

    if (fixed.average_wait_minutes > 0.0) {
        wait_change = 100.0 *
            (fixed.average_wait_minutes - adaptive.average_wait_minutes) /
            fixed.average_wait_minutes;
        printf("Vehicle waiting-time reduction: %+.2f%%\n", wait_change);
    } else {
        puts("Vehicle waiting-time reduction: N/A");
    }

    if (fixed.average_bus_delay_minutes > 0.0) {
        bus_change = 100.0 *
            (fixed.average_bus_delay_minutes -
             adaptive.average_bus_delay_minutes) /
            fixed.average_bus_delay_minutes;
        printf("Bus delay reduction:           %+.2f%%\n", bus_change);
    } else {
        puts("Bus delay reduction:           N/A");
    }

    if (fixed.estimated_throughput > 0.0) {
        throughput_change = 100.0 *
            (adaptive.estimated_throughput - fixed.estimated_throughput) /
            fixed.estimated_throughput;
        printf("Throughput change:              %+.2f%%\n",
               throughput_change);
    } else {
        puts("Throughput change:              N/A");
    }

    printf("Efficiency difference:          %+.2f percentage points\n",
           adaptive.efficiency_percent - fixed.efficiency_percent);
}

/* Display a compact summary of the calculated decisions. */
static void display_summary(void)
{
    int i;

    puts("\n=== FINAL SYSTEM SUMMARY ===");
    printf("Roads/intersections: %d | Buses: %d | Routes: %d\n",
           road_count, bus_count, route_count);

    puts("\nSignal timings:");
    for (i = 0; i < road_count; ++i) {
        printf("  %-20s %.1f seconds green\n",
               roads[i].name, roads[i].adaptive_green_seconds);
    }

    puts("\nBus priority:");
    for (i = 0; i < bus_count; ++i) {
        printf("  Bus %d: %s (score %.1f)\n",
               buses[i].id,
               buses[i].priority_granted ? "GRANTED" : "NORMAL",
               buses[i].priority_score);
    }
}

/* Load a repeatable illustrative scenario for a quick demonstration. */
static void load_demo_scenario(void)
{
    road_count = 4;

    roads[0] = (RoadTraffic){1, "Main Road", 85, 25.0, 45, 100, 0, 0, 0};
    roads[1] = (RoadTraffic){2, "University Road", 40, 35.0, 15, 100, 0, 0, 0};
    roads[2] = (RoadTraffic){3, "Market Road", 95, 15.0, 60, 100, 0, 0, 0};
    roads[3] = (RoadTraffic){4, "Station Road", 60, 30.0, 25, 100, 0, 0, 0};

    bus_count = 3;
    buses[0] = (PublicBus){1, 1, 45, 50, 18.0, "Main Road", 0, 0};
    buses[1] = (PublicBus){2, 3, 20, 50, 5.0, "Market Road", 0, 0};
    buses[2] = (PublicBus){3, 4, 48, 50, 25.0, "Station Road", 0, 0};

    route_count = 3;
    routes[0] = (RouteOption){1, "City Center Route", 12.0, 30.0, 0.80, 0, 0};
    routes[1] = (RouteOption){2, "University Bypass", 15.0, 45.0, 0.20, 0, 0};
    routes[2] = (RouteOption){3, "Market Road Route", 10.0, 20.0, 0.65, 0, 0};

    puts("\nPre-loaded demonstration scenario loaded.");
}

/* Run the modules in their logical processing order. */
static void run_simulation(void)
{
    puts("\n======================================================");
    puts("       INTELLIGENT TRANSPORTATION DECISION ENGINE");
    puts("======================================================");

    calculate_congestion();
    calculate_bus_priority();
    calculate_adaptive_signals();
    calculate_route_costs();
    compare_kpis();
    display_summary();

    puts("\nSimulation completed.");
}

int main(void)
{
    int choice;

    for (;;) {
        puts("\n======================================================");
        puts(" INTELLIGENT INTEGRATED PUBLIC TRANSPORTATION &");
        puts("             TRAFFIC CONTROL SYSTEM");
        puts("======================================================");
        puts("1. Run pre-loaded demo simulation");
        puts("2. Enter custom traffic, bus and route data");
        puts("3. Exit");

        choice = read_int("Select an option (1-3): ", 1, 3);

        if (choice == 1) {
            load_demo_scenario();
            run_simulation();
        } else if (choice == 2) {
            input_traffic_data();
            input_bus_data();
            input_route_data();
            run_simulation();
        } else {
            puts("Thank you for using the ITS simulation.");
            break;
        }
    }

    return 0;
}
