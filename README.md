# Intelligent Integrated Public Transportation & Traffic Control System

A beginner-friendly C99 console simulation for an Intelligent Transportation Systems (ITS) project. It combines basic road traffic analysis, public bus priority, adaptive green-time calculations, route cost evaluation, and estimated KPI comparison.

## Features

- Pre-loaded demonstration scenario
- Custom input for roads, buses, and alternative routes
- Congestion and density calculation
- Passenger-load and schedule-delay-based bus priority score
- Adaptive green-time recommendation
- Traffic-aware route cost and route recommendation
- Fixed-time versus adaptive KPI comparison

## Requirements

- GCC or another C99-compatible C compiler
- Standard C library and math library

## Compile

Linux / macOS / MinGW terminal:

```bash
gcc -std=c99 -Wall -Wextra -pedantic main.c -o its_simulation -lm
```

## Run

Linux / macOS:

```bash
./its_simulation
```

Windows Command Prompt:

```bat
its_simulation.exe
```

## How to use

1. Run the program.
2. Choose `1` for the pre-loaded demo or `2` to enter your own data.
3. For custom mode, enter the road, bus, and route information when prompted.
4. Review the congestion analysis, bus priority decisions, signal timings, route recommendation, and KPI comparison.
5. Choose `3` to exit.

## Main formulas

### Density and congestion

```text
Density ratio = vehicle count / road capacity
Congestion (%) = density ratio * 100
```

The displayed congestion percentage is capped at 100%.

### Bus priority score

```text
Passenger percentage = passengers / passenger capacity * 100
Delay percentage = schedule delay / 30 * 100 (capped at 100)
Priority score = 0.60 * passenger percentage
               + 0.40 * delay percentage
```

A score of 60 or higher requests priority.

### Adaptive green time

```text
Queue percentage = queue length / road capacity * 100 (capped at 100)
Demand = 0.70 * congestion percentage + 0.30 * queue percentage
Green time = 20 + 0.35 * demand
```

A priority bus mapped to that road adds 5 seconds. The final value is clamped between 15 and 60 seconds.

### Route cost

```text
Travel time (minutes) = distance (km) / speed (km/h) * 60
Route cost = travel time * (1 + 2 * congestion factor)
```

The route with the lowest calculated cost is recommended.

## Important limitations

This project is an educational decision-engine simulation. It does not move individual vehicles through a network or simulate signal phases over time. The KPI values are estimates produced by simplified equations, not measured real-world traffic results.

For the demonstration, a bus `route_id` is mapped to a road `id` to determine whether it can receive a green extension at that road. A real ITS deployment would need GPS/map matching and the next-intersection position.

A real traffic controller must also coordinate conflicting approaches, yellow/all-red clearance intervals, pedestrian crossings, minimum green times, and cycle lengths. Do not connect this educational program directly to real traffic lights.

## Suggested future improvements

- Add time-stepped vehicle arrivals, departures, and queue updates.
- Add multiple approaches per intersection and safe signal-phase scheduling.
- Add GPS-based bus-to-intersection mapping.
- Add file-based input and CSV KPI export.
- Connect sensor data through an IoT gateway in a later project phase.

## Repository structure

```text
Intelligent-Transportation-System/
├── main.c
├── README.md
└── .gitignore
```

## License

This project is provided for educational and academic demonstration. You may adapt it for your coursework; follow your institution's rules for attribution and collaboration.
