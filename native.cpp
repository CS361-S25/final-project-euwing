#include "World.h"
#include "Prey.h"
#include "Prey2.h"
#include "Predator.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>

// This function helps us group patches into zones: low, medium, high resource
int ClassifyZone(double resource_level) {
    if (resource_level < 0.33) return 0;       // Low
    else if (resource_level < 0.66) return 1;  // Medium
    else return 2;                             // High
}

// This function runs the main simulation experiment
void RunExperiment(double predator_death_rate) {
    const int width = 60;
    const int height = 60;
    const int total_patches = width * height;

    // Create the world and set how predators die
    World world(total_patches);
    world.SetPredatorDeathRate(predator_death_rate);

    // Tell the world how to clone organisms properly
    world.SetCloneFunction([](bool is_prey, double a, double t, double m) {
        if (!is_prey) return new Predator(a, t, m);
        if (t > 0.5) return new Prey(a, t, m);
        return new Prey2(a, t, m);
    });

    // Define rectangular zones in the world with different resources
    struct PatchZone {
        int x_start, y_start;
        double resource;
    };

    std::vector<PatchZone> zones = {
        {2, 2, 0.9}, {18, 3, 0.9}, {10, 18, 0.9},    // High-resource zones
        {35, 30, 0.5}, {50, 32, 0.5}, {42, 48, 0.5}, // Medium
        {25, 25, 0.1}, {8, 40, 0.1}, {22, 45, 0.1}   // Low
    };

    // Fill each zone with the correct resource level
    for (const auto& zone : zones) {
        for (int dy = 0; dy < 30; ++dy) {
            for (int dx = 0; dx < 15; ++dx) {
                int x = zone.x_start + dx;
                int y = zone.y_start + dy;
                if (x < width && y < height) {
                    int index = y * width + x;
                    world.GetPatchesMutable()[index].resource_level = zone.resource;
                }
            }
        }
    }

    // Add predators and prey to each zone
    emp::Random random;

    for (const auto& zone : zones) {
        std::vector<int> patch_indices;

        for (int dy = 0; dy < 30; ++dy) {
            for (int dx = 0; dx < 15; ++dx) {
                int x = zone.x_start + dx;
                int y = zone.y_start + dy;
                if (x < width && y < height) {
                    int index = y * width + x;
                    patch_indices.push_back(index);
                }
            }
        }

        // Find the center of the zone
        int center_x = zone.x_start + 7;
        int center_y = zone.y_start + 15;
        int center_index = center_y * width + center_x;

        // Add 3 predators to the center patch
        for (int i = 0; i < 3; ++i) {
            world.AddOrganism(new Predator(0.5, 0.0, 0.5), center_index);
        }

        // Add a mix of Prey and Prey2 to other patches
        for (int i = 0; i < 5; ++i) {
            int idx = patch_indices[random.GetUInt(patch_indices.size())];
            if (random.P(0.5))
                world.AddOrganism(new Prey(0.5, 1.0, 0.5), idx);
            else
                world.AddOrganism(new Prey2(0.5, 0.0, 0.5), idx);
        }
    }

    // Create a folder and CSV file to save results
    std::filesystem::create_directory("Data");
    std::ofstream csv("Data/Prey2_kp_0.02.csv");
    csv << "Generation,Alpha1,Tau1,Alpha2,Tau2,"
           "P1Low,P1Med,P1High,P2Low,P2Med,P2High,"
           "PredLow,PredMed,PredHigh\n";

    // Print header for console
    std::cout << "\n[Running Simulation] Predator Death Rate = " << predator_death_rate << "\n";
    std::cout << "Gen\tA1\tT1\tA2\tT2\tP1Low\tP1Med\tP1High\tP2Low\tP2Med\tP2High\tPredLow\tPredMed\tPredHigh\n";

    // Run the simulation for 15 generations
    for (int gen = 0; gen < 15; ++gen) {
        world.Step(); // Move, reproduce, and cull

        // Stats we'll track for this generation
        double alpha1 = 0, tau1 = 0, alpha2 = 0, tau2 = 0;
        int count1 = 0, count2 = 0;
        int p1low = 0, p1med = 0, p1high = 0;
        int p2low = 0, p2med = 0, p2high = 0;
        int predlow = 0, predmed = 0, predhigh = 0;

        // Go through all organisms and update stats
        for (const auto& patch : world.GetPatches()) {
            int zone = ClassifyZone(patch.resource_level);
            for (Organism* org : patch.occupants) {
                if (!org->IsPrey()) {
                    if (zone == 0) predlow++;
                    else if (zone == 1) predmed++;
                    else predhigh++;
                } else if (org->GetTau() > 0.5) {
                    alpha1 += org->GetAlpha();
                    tau1 += org->GetTau();
                    count1++;
                    if (zone == 0) p1low++;
                    else if (zone == 1) p1med++;
                    else p1high++;
                } else {
                    alpha2 += org->GetAlpha();
                    tau2 += org->GetTau();
                    count2++;
                    if (zone == 0) p2low++;
                    else if (zone == 1) p2med++;
                    else p2high++;
                }
            }
        }

        // Get averages for the traits
        alpha1 = count1 ? alpha1 / count1 : 0.0;
        tau1 = count1 ? tau1 / count1 : 0.0;
        alpha2 = count2 ? alpha2 / count2 : 0.0;
        tau2 = count2 ? tau2 / count2 : 0.0;

        // Save to CSV
        csv << gen << "," << alpha1 << "," << tau1 << "," << alpha2 << "," << tau2 << ","
            << p1low << "," << p1med << "," << p1high << ","
            << p2low << "," << p2med << "," << p2high << ","
            << predlow << "," << predmed << "," << predhigh << "\n";

        // Print to screen
        std::cout << gen << "\t" << alpha1 << "\t" << tau1 << "\t"
                  << alpha2 << "\t" << tau2 << "\t"
                  << p1low << "\t" << p1med << "\t" << p1high << "\t"
                  << p2low << "\t" << p2med << "\t" << p2high << "\t"
                  << predlow << "\t" << predmed << "\t" << predhigh << "\n";
    }

    csv.close(); // Save the file
}

// Main entry point
int main() {
    RunExperiment(0.02); // Run with death rate = 0.02
    return 0;
}

























