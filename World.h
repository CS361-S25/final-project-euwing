#ifndef WORLD_H
#define WORLD_H

#include "Organism.h"
#include "emp/math/Random.hpp"
#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>

// Represents a single patch in the environment
struct Patch {
    std::vector<Organism*> occupants;  // All organisms currently in this patch
    double resource_level = 1.0;       // Amount of food or prey available
    double danger_level = 0.0;         // Risk level (used conceptually for prey)
};

class World {
private:
    std::vector<Patch> patches;           // All patches in the world
    emp::Random random;                   // Random number generator
    double mutation_rate = 0.05;          // Chance for mutation during reproduction
    double mutation_sd = 0.025;           // Standard deviation for mutation value
    double predator_death_rate = 0.1;     // Chance a predator dies each step

public:
    // Constructor: initializes the world with a fixed number of patches
    World(int num_patches)
        : patches(num_patches) {}

    // Add an organism to a specific patch by index
    void AddOrganism(Organism* org, int patch_index) {
        patches[patch_index].occupants.push_back(org);
    }

    // Run one full simulation step: move, reproduce, then cull
    void Step() {
        MoveOrganisms();
        Reproduce();
        CullDead();
    }

    // Move each organism to a potentially new patch based on scores
    void MoveOrganisms() {
        std::vector<std::vector<Organism*>> new_occupants(patches.size());

        for (size_t i = 0; i < patches.size(); ++i) {
            for (Organism* org : patches[i].occupants) {
                // Skip movement based on organism's move probability
                if (!random.P(org->GetMoveRate())) {
                    new_occupants[i].push_back(org);
                    continue;
                }

                // Calculate preference scores for all patches
                std::vector<double> patch_scores(patches.size());

                for (size_t j = 0; j < patches.size(); ++j) {
                    double resource = patches[j].resource_level;
                    double danger;

                    if (org->IsPrey()) {
                        int predator_count = std::count_if(
                            patches[j].occupants.begin(), patches[j].occupants.end(),
                            [](Organism* o) { return !o->IsPrey(); });
                        danger = static_cast<double>(predator_count);
                    } else {
                        int predator_count = std::count_if(
                            patches[j].occupants.begin(), patches[j].occupants.end(),
                            [](Organism* o) { return !o->IsPrey(); });
                        int prey_count = patches[j].occupants.size() - predator_count;

                        resource = static_cast<double>(prey_count);
                        danger = static_cast<double>(predator_count);
                    }

                    double alpha = org->GetAlpha();  // How sensitive to patch differences
                    double tau = org->GetTau();      // How much food is valued over safety

                    // Patch desirability score
                    patch_scores[j] = alpha * (tau * resource - (1 - tau) * danger);
                }

                // Normalize scores and select new patch
                double total_score = std::accumulate(patch_scores.begin(), patch_scores.end(), 0.0);
                size_t chosen_patch = 0;

                if (total_score <= 0.0) {
                    chosen_patch = random.GetUInt(patches.size());
                } else {
                    for (double& score : patch_scores) score /= total_score;

                    double rand_val = random.GetDouble();
                    double cumulative = 0.0;

                    for (size_t k = 0; k < patch_scores.size(); ++k) {
                        cumulative += patch_scores[k];
                        if (rand_val <= cumulative) {
                            chosen_patch = k;
                            break;
                        }
                    }
                }

                new_occupants[chosen_patch].push_back(org);
            }
        }

        // Update all patches with the new occupants
        for (size_t i = 0; i < patches.size(); ++i) {
            patches[i].occupants = new_occupants[i];
        }
    }

    // Allow organisms to reproduce with potential mutation
    void Reproduce() {
        std::vector<std::pair<Organism*, int>> babies;

        for (size_t i = 0; i < patches.size(); ++i) {
            double local_resource = patches[i].resource_level;

            for (Organism* org : patches[i].occupants) {
                double base_reproduction_chance = 0.25;
                double reproduction_chance = base_reproduction_chance;

                if (org->IsPrey()) {
                    reproduction_chance *= local_resource;

                    int max_babies = 1;
                    if (local_resource >= 0.66) max_babies = 3;
                    else if (local_resource >= 0.33) max_babies = 2;

                    for (int k = 0; k < max_babies; ++k) {
                        if (random.P(reproduction_chance)) {
                            Organism* baby = org->Clone();

                            // Possibly mutate alpha value
                            if (random.P(mutation_rate)) {
                                double new_alpha = baby->GetAlpha() + random.GetRandNormal(0, mutation_sd);
                                baby->SetAlpha(std::clamp(new_alpha, 0.0, 1.0));
                            }

                            babies.emplace_back(baby, i);
                        }
                    }
                } else {
                    // Predator can have one chance to reproduce
                    if (random.P(reproduction_chance)) {
                        Organism* baby = org->Clone();

                        if (random.P(mutation_rate)) {
                            double new_alpha = baby->GetAlpha() + random.GetRandNormal(0, mutation_sd);
                            baby->SetAlpha(std::clamp(new_alpha, 0.0, 1.0));
                        }

                        babies.emplace_back(baby, i);
                    }
                }
            }
        }

        // Add new babies to the patches
        for (auto& [baby, index] : babies) {
            patches[index].occupants.push_back(baby);
        }
    }

    // Remove predators that randomly die based on death rate
    void CullDead() {
        for (auto& patch : patches) {
            patch.occupants.erase(std::remove_if(
                patch.occupants.begin(), patch.occupants.end(),
                [&](Organism* org) {
                    if (!org->IsPrey() && random.P(predator_death_rate)) {
                        delete org;
                        return true;
                    }
                    return false;
                }),
                patch.occupants.end());
        }
    }

    // Get all patches (read-only)
    const std::vector<Patch>& GetPatches() const { return patches; }

    // Get all patches (modifiable)
    std::vector<Patch>& GetPatchesMutable() { return patches; }

    // Compute average alpha value for prey or predators
    double GetAverageAlpha(bool for_prey = true) const {
        std::vector<double> alphas;

        for (const auto& patch : patches) {
            for (Organism* org : patch.occupants) {
                if (org->IsPrey() == for_prey) {
                    alphas.push_back(org->GetAlpha());
                }
            }
        }

        if (alphas.empty()) return 0.0;

        double sum = std::accumulate(alphas.begin(), alphas.end(), 0.0);
        return sum / alphas.size();
    }
};

#endif









