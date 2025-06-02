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

// Run one full simulation experiment and save results
void RunExperiment(const std::string& label) {
    World world(9); //  total patches in this small experiment

    // Define patch resource levels (3 low, 2 medium, 3 high)
    std::vector<double> resource_levels = {
        0.1, 0.1, 0.1,    // Low
        0.5, 0.5,  0.5,       // Medium
        0.9, 0.9, 0.9     // High
    };

    for (size_t i = 0; i < 8; ++i) {
        world.GetPatchesMutable()[i].resource_level = resource_levels[i];
    }

    // Add 5 prey and 1 predator to every patch
    for (int i = 0; i < 8; ++i) {
        for (int p = 0; p < 5; ++p)
            world.AddOrganism(new Prey(0.5, 0.5, 0.5), i);
        world.AddOrganism(new Predator(0.5, 0.5, 0.5), i);
    }

    // Prepare CSV output
    std::filesystem::create_directory("Data");
    std::ofstream csv("Data/treatment_results.csv");
    csv << "Generation,AvgAlphaPrey,PreyLow,PreyMed,PreyHigh,PredLow,PredMed,PredHigh\n";

    // Terminal heading
    std::cout << "\nExperiment: " << label << "\n";
    std::cout << "Gen\tAlpha\tPreyLow\tPreyMed\tPreyHigh\tPredLow\tPredMed\tPredHigh\n";

    // Run for 25 generations
    for (int gen = 0; gen < 25; ++gen) {
        world.Step(); // Move, reproduce, and cull

        double avg_alpha = world.GetAverageAlpha(true); // For prey only

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

        // Save results
        csv << gen << "," << std::fixed << std::setprecision(4) << avg_alpha
            << "," << prey_low << "," << prey_med << "," << prey_high
            << "," << pred_low << "," << pred_med << "," << pred_high << "\n";

        std::cout << gen << "\t" << avg_alpha << "\t"
                  << prey_low << "\t" << prey_med << "\t" << prey_high << "\t"
                  << pred_low << "\t" << pred_med << "\t" << pred_high << "\n";
    }

    csv.close();
}

int main() {
    RunExperiment("Treatment (Alpha Evolves)");
    return 0;
}













