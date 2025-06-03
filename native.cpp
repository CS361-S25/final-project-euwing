#include "World.h"
#include "Prey.h"
#include "Predator.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>

// Classify resource zone: 0 = Low, 1 = Medium, 2 = High
int ClassifyZone(double resource_level) {
    if (resource_level < 0.33) return 0;
    else if (resource_level < 0.66) return 1;
    else return 2;
}

// Run one full simulation for ideal result only
void RunIdealPhaseShiftExperiment(double predator_death_rate) {
    const int num_patches = 60 * 60;
    World world(num_patches);
    world.SetPredatorDeathRate(predator_death_rate);

    world.SetCloneFunction([](bool is_prey, double a, double t, double m) {
        return is_prey ? static_cast<Organism*>(new Prey(a, t, m))
                       : static_cast<Organism*>(new Predator(a, t, m));
    });

    // Define 9 patch zones: 3 per resource level
    struct PatchZone {
        int x_start, y_start;
        double resource;
    };

    const int num_columns = 60;
    std::vector<PatchZone> zones = {
        // High
        {2, 2, 0.9}, {18, 3, 0.9}, {10, 18, 0.9},

        // Medium
        {35, 30, 0.5}, {50, 32, 0.5}, {42, 48, 0.5},

        // Low
        {25, 25, 0.1}, {8, 40, 0.1}, {22, 45, 0.1}
    };

    for (const auto& zone : zones) {
        for (int dy = 0; dy < 30; ++dy) {
            for (int dx = 0; dx < 15; ++dx) {
                int x = zone.x_start + dx;
                int y = zone.y_start + dy;
                if (x < num_columns && y < num_columns) {
                    int idx = y * num_columns + x;
                    world.GetPatchesMutable()[idx].resource_level = zone.resource;
                }
            }
        }
    }

    emp::Random random;

    for (const auto& zone : zones) {
        std::vector<int> patch_indices;

        for (int dy = 0; dy < 30; ++dy) {
            for (int dx = 0; dx < 15; ++dx) {
                int x = zone.x_start + dx;
                int y = zone.y_start + dy;
                if (x < num_columns && y < num_columns) {
                    int idx = y * num_columns + x;
                    patch_indices.push_back(idx);
                }
            }
        }

        int center_x = zone.x_start + 7;
        int center_y = zone.y_start + 15;
        if (center_x < num_columns && center_y < num_columns) {
            int center_idx = center_y * num_columns + center_x;

            for (int p = 0; p < 3; ++p)
                world.AddOrganism(new Predator(0.5, 0.0, 0.5), center_idx);

            std::vector<int> prey_indices;
            for (int idx : patch_indices) {
                if (idx != center_idx) prey_indices.push_back(idx);
            }

            for (int i = 0; i < 5; ++i) {
                int idx = prey_indices[random.GetUInt(prey_indices.size())];
                world.AddOrganism(new Prey(0.5, 1.0, 0.5), idx);
            }
        }
    }

    std::filesystem::create_directory("Data");
    std::ofstream csv("Data/ideal_shift_kp_0.14.csv");
    csv << "Generation,AvgAlphaPrey,AvgTauPrey,PreyLow,PreyMed,PreyHigh,PredLow,PredMed,PredHigh\n";

    std::cout << "\n[Ideal Result] kP = " << predator_death_rate << "\n";
    std::cout << "Gen\tAlpha\tTau\tPreyLow\tPreyMed\tPreyHigh\tPredLow\tPredMed\tPredHigh\n";

    for (int gen = 0; gen < 15; ++gen) {
        world.Step();

        double avg_alpha = world.GetAverageAlpha(true);
        double avg_tau = world.GetAverageTau(true);

        int prey_low = 0, prey_med = 0, prey_high = 0;
        int pred_low = 0, pred_med = 0, pred_high = 0;

        const auto& patches = world.GetPatches();
        for (const auto& patch : patches) {
            int zone = ClassifyZone(patch.resource_level);
            for (Organism* org : patch.occupants) {
                if (org->IsPrey()) {
                    if (zone == 0) prey_low++;
                    else if (zone == 1) prey_med++;
                    else prey_high++;
                } else {
                    if (zone == 0) pred_low++;
                    else if (zone == 1) pred_med++;
                    else pred_high++;
                }
            }
        }

        csv << gen << "," << avg_alpha << "," << avg_tau << ","
            << prey_low << "," << prey_med << "," << prey_high << ","
            << pred_low << "," << pred_med << "," << pred_high << "\n";

        std::cout << gen << "\t" << avg_alpha << "\t" << avg_tau << "\t"
                  << prey_low << "\t" << prey_med << "\t" << prey_high << "\t"
                  << pred_low << "\t" << pred_med << "\t" << pred_high << "\n";
    }

    csv.close();
}

int main() {
    RunIdealPhaseShiftExperiment(0.14); // Adjust kP value as needed
    return 0;
}




















