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
        {2, 2, 0.9}, {22, 2, 0.9}, {42, 2, 0.9},
        {2, 22, 0.5}, {22, 22, 0.5}, {42, 22, 0.5},
        {2, 42, 0.1}, {22, 42, 0.1}, {42, 42, 0.1}
    };

    // Initialize patches with resources and add organisms
    for (const auto& zone_info : zones) {
        for (int y = zone_info.y_start; y < zone_info.y_start + 16; ++y) {
            for (int x = zone_info.x_start; x < zone_info.x_start + 16; ++x) {
                int index = y * width + x;
                world.GetPatchesMutable()[index].resource_level = zone_info.resource;
            }
        }
    }

    // Add initial organisms to center of each zone (simplified distribution)
    std::vector<int> zone_centers = {
        (7*width + 7), (27*width + 7), (47*width + 7),
        (7*width + 27), (27*width + 27), (47*width + 27),
        (7*width + 47), (27*width + 47), (47*width + 47)
    };

    for(size_t i = 0; i < zones.size(); ++i) {
        int center_idx = zone_centers[i];
        int zone_type = ClassifyZone(zones[i].resource);

        // Add predators
        int num_predators = (zone_type == 2) ? 6 : (zone_type == 1 ? 3 : 0);
        for (int j = 0; j < num_predators; ++j) {
            world.AddOrganism(new Predator(0.5, 0.8, 0.5), center_idx);
        }

        // Add prey (half Prey, half Prey2)
        for (int j = 0; j < 10; ++j) { // 10 prey per zone center
            if (j % 2 == 0) // Prey with higher tau (blue)
                world.AddOrganism(new Prey(0.5, 1.0, 0.5), center_idx);
            else // Prey2 with lower tau (cyan)
                world.AddOrganism(new Prey2(0.5, 0.0, 0.5), center_idx);
        }
    }

    // Set up CSV file for output
    std::string filename = "evolution_data_deathrate_" + std::to_string(static_cast<int>(predator_death_rate * 100000)) + ".csv";
    std::ofstream csv(filename);
    csv << "Generation,AvgAlphaPrey1,AvgTauPrey1,AvgAlphaPrey2,AvgTauPrey2,"
        << "Prey1Low,Prey1Med,Prey1High,Prey2Low,Prey2Med,Prey2High,"
        << "PredatorLow,PredatorMed,PredatorHigh\n";

    // Run the simulation
    for (int gen = 0; gen <= 1000; ++gen) { // Run for 1000 generations
        world.Step();

        // Collect stats
        double alpha1 = 0.0, tau1 = 0.0;
        double alpha2 = 0.0, tau2 = 0.0;
        int count1 = 0, count2 = 0;
        int p1low = 0, p1med = 0, p1high = 0;
        int p2low = 0, p2med = 0, p2high = 0;
        int predlow = 0, predmed = 0, predhigh = 0;

        for (const auto& patch : world.GetPatches()) {
            int zone = ClassifyZone(patch.resource_level);
            for (Organism* org : patch.occupants) {
                if (!org->IsPrey()) {
                    if (zone == 0) predlow++;
                    else if (zone == 1) predmed++;
                    else predhigh++;
                } else if (org->GetTau() > 0.5) { // Prey1 (blue)
                    alpha1 += org->GetAlpha();
                    tau1 += org->GetTau();
                    count1++;
                    if (zone == 0) p1low++;
                    else if (zone == 1) p1med++;
                    else p1high++;
                } else { // Prey2 (cyan)
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
                  << predlow << "\t" << predmed << "\t" << predhigh << std::endl;
    }

    csv.close();
}

int main() {
    std::cout << std::fixed << std::setprecision(5);

    std::cout << "Running experiment with low predator death rate (0.02):" << std::endl;
    RunExperiment(0.02);

    return 0;
}

























