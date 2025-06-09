# Predator-Prey Evolution Simulation

This project simulates an evolving ecosystem composed of predators and two types of prey (Prey1 and Prey2) across a patch-based world. The simulation demonstrates how trait-driven behavior, resource distribution, and selection pressures influence population dynamics over generations.

## 🧬 Overview

There are three types of organisms:

- **Prey1**: Mobile prey that can move between patches. Their behavior is controlled by two traits:
  - `alpha`: Sensitivity to patch quality.
  - `tau`: Preference for food (resources) vs. safety (distance from predators).
- **Prey2**: Immobile prey (`move_rate = 0.0`), representing a different survival strategy.
- **Predators**: Hunt prey but are restricted to the resource zone they were born in. They can die over time based on a configurable death rate.

Each organism reproduces asexually with mutation, potentially altering `alpha` and `tau`. Evolutionary dynamics are driven by trait-based movement, survival, and reproduction in an environment with varied resource zones.

## 🗺️ Patch Zones

The world consists of patches assigned to one of three resource levels:

- High (green)
- Medium (orange)
- Low (red)

Predators are initialized only in medium and high resource zones. Prey are evenly split between Prey1 and Prey2 in all zones.

## 🔧 Files

| File         | Description |
|--------------|-------------|
| `Organism.h` | Abstract base class for all organisms |
| `Prey.h`     | Mobile prey definition (Prey1) |
| `Prey2.h`    | Immobile prey definition (Prey2) |
| `Predator.h` | Predator class |
| `World.h`    | Simulation environment, movement, reproduction, and death logic |
| `native.cpp` | Command-line interface to run simulation and log data to CSV |
| `web.cpp`    | Browser-based interactive visualization with configuration panel |