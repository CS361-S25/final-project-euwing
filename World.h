#ifndef WORLD_H
#define WORLD_H

#include "Organism.h"
#include "emp/math/Random.hpp"
#include <vector>
#include <numeric>
#include <algorithm> // Required for std::clamp, std::shuffle
#include <iostream>
#include <functional>
#include <cmath>
#include <random> 

// NEW INCLUDES: Required for ResetOrganisms to know about these types
#include "Prey.h"
#include "Prey2.h"
#include "Predator.h"

struct Patch {
    std::vector<Organism*> occupants;
    double resource_level = 1.0;
    double danger_level = 0.0;
};

class World {
private:
    std::vector<Patch> patches;
    emp::Random random; 
    std::mt19937 std_random; 
    double mutation_rate = 0.05;
    double mutation_sd = 0.025;
    double predator_death_rate = 0.00001;
    std::function<Organism*(bool, double, double, double)> clone_func;

public:
    World(int num_patches) : patches(num_patches) {
        std::random_device rd; // Obtain a random number from hardware
        std_random.seed(rd()); // Seed the standard random engine
    }

    void SetCloneFunction(std::function<Organism*(bool, double, double, double)> func) {
        clone_func = func;
    }

    void SetPredatorDeathRate(double rate) {
        predator_death_rate = rate;
    }

    void SetMutationRate(double rate) {
        mutation_rate = rate;
    }

    void SetMutationSD(double sd) {
        mutation_sd = sd;
    }

    void AddOrganism(Organism* org, int patch_index) {
        if (patches[patch_index].occupants.empty()) {
            patches[patch_index].occupants.push_back(org);
            org->SetBirthZone(ClassifyZone(patches[patch_index].resource_level));
        } else {
            delete org;
        }
    }

    void Step() {
        MoveOrganisms();
        Reproduce();
        CullDead();
    }

    int ClassifyZone(double r) {
        if (r < 0.33) return 0;
        if (r < 0.66) return 1;
        return 2;
    }

    void MoveOrganisms() {
        std::vector<std::vector<Organism*>> new_occupants(patches.size());

        for (size_t i = 0; i < patches.size(); ++i) {
            for (Organism* org : patches[i].occupants) {
                if (!random.P(org->GetMoveRate())) {
                    if (new_occupants[i].empty()) new_occupants[i].push_back(org);
                    continue;
                }

                std::vector<double> patch_scores(patches.size());
                for (size_t j = 0; j < patches.size(); ++j) {
                    if (!org->IsPrey() &&
                        org->GetBirthZone() != ClassifyZone(patches[j].resource_level)) {
                        patch_scores[j] = 0;
                        continue;
                    }

                    double resource_val;
                    double danger_val;

                    if (org->IsPrey()) {
                        int predators_in_patch = std::count_if(patches[j].occupants.begin(),
                                                      patches[j].occupants.end(),
                                                      [](Organism* o) { return !o->IsPrey(); });
                        resource_val = patches[j].resource_level;
                        danger_val = static_cast<double>(predators_in_patch);

                        double a = org->GetAlpha();
                        double t = org->GetTau();
                        patch_scores[j] = a * (t * resource_val - (1 - t) * danger_val);
                    } else {
                        int predators_in_patch = std::count_if(patches[j].occupants.begin(),
                                                      patches[j].occupants.end(),
                                                      [](Organism* o) { return !o->IsPrey(); });
                        int prey_count = patches[j].occupants.size() - predators_in_patch;
                        resource_val = static_cast<double>(prey_count);
                        danger_val = static_cast<double>(predators_in_patch);

                        double a_predator_behavior = 0.5;
                        double t_predator_behavior = 0.9;

                        patch_scores[j] = a_predator_behavior * (t_predator_behavior * resource_val - (1 - t_predator_behavior) * danger_val);
                    }
                }

                double total_score = std::accumulate(patch_scores.begin(), patch_scores.end(), 0.0);
                size_t chosen_patch = i;
                if (total_score > 0.0) {
                    for (double& score : patch_scores) score /= total_score;
                    double r_val = random.GetDouble();
                    double running_total = 0.0;
                    for (size_t k = 0; k < patch_scores.size(); ++k) {
                        running_total += patch_scores[k];
                        if (r_val <= running_total) {
                            chosen_patch = k;
                            break;
                        }
                    }
                }

                if (new_occupants[chosen_patch].empty()) {
                    new_occupants[chosen_patch].push_back(org);
                } else {
                    new_occupants[i].push_back(org);
                }
            }
        }

        for (size_t i = 0; i < patches.size(); ++i) {
            patches[i].occupants = std::move(new_occupants[i]);
        }
    }

    void Reproduce() {
        std::vector<std::pair<Organism*, int>> babies;

        for (size_t i = 0; i < patches.size(); ++i) {
            double resources = patches[i].resource_level;
            int zone = ClassifyZone(resources);

            for (Organism* org : patches[i].occupants) {
                if (!org->IsPrey() && org->GetBirthZone() != zone) continue;

                double chance = 1.0;

                if (org->IsPrey()) {
                    chance *= resources;
                    // Modified to make preys reproduce "so much faster"
                    int max_babies = (resources >= 0.66) ? 10 : (resources >= 0.33 ? 7 : 4); // Significantly increased baby count

                    for (int b = 0; b < max_babies; ++b) {
                        if (random.P(chance)) {
                            Organism* baby = org->Clone();
                            baby->SetBirthZone(zone);

                            double a = baby->GetAlpha();
                            double t = baby->GetTau();
                            double m = baby->GetMoveRate();
                            bool is_prey = baby->IsPrey();

                            if (random.P(mutation_rate)) a = std::clamp(a + random.GetRandNormal(0, mutation_sd), 0.0, 1.0);
                            if (random.P(mutation_rate)) t = std::clamp(t + random.GetRandNormal(0, mutation_sd), 0.0, 1.0);

                            if (clone_func) {
                                delete baby;
                                baby = clone_func(is_prey, a, t, m);
                                baby->SetBirthZone(zone);
                            }

                            babies.emplace_back(baby, i);
                        }
                    }
                } else {
                    if (random.P(chance)) {
                        Organism* baby = org->Clone();
                        baby->SetBirthZone(zone);

                        double a = baby->GetAlpha();
                        double t = baby->GetTau();
                        double m = baby->GetMoveRate();
                        bool is_prey = baby->IsPrey();

                        if (random.P(mutation_rate)) a = std::clamp(a + random.GetRandNormal(0, mutation_sd), 0.0, 1.0);
                        if (random.P(mutation_rate)) t = std::clamp(t + random.GetRandNormal(0, mutation_sd), 0.0, 1.0);

                        if (clone_func) {
                            delete baby;
                            baby = clone_func(is_prey, a, t, m);
                            baby->SetBirthZone(zone);
                        }

                        babies.emplace_back(baby, i);
                    }
                }
            }
        }

        for (auto& [baby, index] : babies) {
            if (patches[index].occupants.empty()) {
                patches[index].occupants.push_back(baby);
            } else {
                delete baby;
            }
        }
    }

    void CullDead() {
        for (auto& patch : patches) {
            patch.occupants.erase(std::remove_if(
                patch.occupants.begin(), patch.occupants.end(),
                [&](Organism* org) {
                    if (!org->IsPrey() && random.P(predator_death_rate)) {
                        delete org;
                        return true;
                    }
                    if (org->IsDead()) {
                        delete org;
                        return true;
                    }
                    return false;
                }),
                patch.occupants.end());
        }
    }

    const std::vector<Patch>& GetPatches() const { return patches; }
    std::vector<Patch>& GetPatchesMutable() { return patches; }

    double GetAveragePreyAlpha(bool is_prey1) const {
        double total_alpha = 0.0;
        int count = 0;
        for (const auto& patch : patches) {
            for (Organism* org : patch.occupants) {
                if (org->IsPrey()) {
                    if ((is_prey1 && org->GetTau() > 0.5) || (!is_prey1 && org->GetTau() <= 0.5)) {
                        total_alpha += org->GetAlpha();
                        count++;
                    }
                }
            }
        }
        return count > 0 ? total_alpha / count : 0.0;
    }

    double GetAveragePreyTau(bool is_prey1) const {
        double total_tau = 0.0;
        int count = 0;
        for (const auto& patch : patches) {
            for (Organism* org : patch.occupants) {
                if (org->IsPrey()) {
                    if ((is_prey1 && org->GetTau() > 0.5) || (!is_prey1 && org->GetTau() <= 0.5)) {
                        total_tau += org->GetTau();
                        count++;
                    }
                }
            }
        }
        return count > 0 ? total_tau / count : 0.0;
    }

    int GetPrey1Count() const {
        int count = 0;
        for (const auto& patch : patches) {
            for (Organism* org : patch.occupants) {
                if (org->IsPrey() && org->GetTau() > 0.5) {
                    count++;
                }
            }
        }
        return count;
    }

    int GetPrey2Count() const {
        int count = 0;
        for (const auto& patch : patches) {
            for (Organism* org : patch.occupants) {
                if (org->IsPrey() && org->GetTau() <= 0.5) {
                    count++;
                }
            }
        }
        return count;
    }

    int GetPredatorCount() const {
        int count = 0;
        for (const auto& patch : patches) {
            for (Organism* org : patch.occupants) {
                if (!org->IsPrey()) {
                    count++;
                }
            }
        }
        return count;
    }

    int GetTotalOrganismCount() const {
        int count = 0;
        for (const auto& patch : patches) {
            count += patch.occupants.size();
        }
        return count;
    }

    void ResetOrganisms(
        int initial_prey1, int initial_prey2,
        int initial_predators_low_resource,
        int initial_predators_medium_resource,
        int initial_predators_high_resource
    ) {
        for (auto& patch : patches) {
            for (Organism* org : patch.occupants) {
                delete org;
            }
            patch.occupants.clear();
        }

        std::vector<int> low_resource_patches;
        std::vector<int> medium_resource_patches;
        std::vector<int> high_resource_patches;
        std::vector<int> all_prey_patches;

        for (size_t i = 0; i < patches.size(); ++i) {
            int zone = ClassifyZone(patches[i].resource_level);
            if (zone == 0) low_resource_patches.push_back(i);
            else if (zone == 1) medium_resource_patches.push_back(i);
            else high_resource_patches.push_back(i);
            all_prey_patches.push_back(i);
        }

        // Use std_random for shuffling
        std::shuffle(low_resource_patches.begin(), low_resource_patches.end(), std_random);
        std::shuffle(medium_resource_patches.begin(), medium_resource_patches.end(), std_random);
        std::shuffle(high_resource_patches.begin(), high_resource_patches.end(), std_random);
        std::shuffle(all_prey_patches.begin(), all_prey_patches.end(), std_random);

        int prey_patch_idx_counter = 0;

        auto add_org_if_empty = [&](Organism* org, int patch_idx, int birth_zone_val = -1) {
            if (patch_idx >= 0 && patch_idx < (int)patches.size() && patches[patch_idx].occupants.empty()) {
                if (birth_zone_val != -1) {
                    org->SetBirthZone(birth_zone_val);
                } else {
                    org->SetBirthZone(ClassifyZone(patches[patch_idx].resource_level));
                }
                patches[patch_idx].occupants.push_back(org);
                return true;
            }
            delete org;
            return false;
        };

        for (int i = 0; i < initial_prey1 && prey_patch_idx_counter < (int)all_prey_patches.size(); ++i) {
            add_org_if_empty(new Prey(0.5, 1.0, 0.5), all_prey_patches[prey_patch_idx_counter++]);
        }

        for (int i = 0; i < initial_prey2 && prey_patch_idx_counter < (int)all_prey_patches.size(); ++i) {
             add_org_if_empty(new Prey2(0.5, 0.0, 0.5), all_prey_patches[prey_patch_idx_counter++]);
        }

        int current_pred_idx_low = 0;
        int current_pred_idx_medium = 0;
        int current_pred_idx_high = 0;

        for (int i = 0; i < initial_predators_low_resource; ++i) {
            if (current_pred_idx_low < (int)low_resource_patches.size()) {
                int patch_idx = low_resource_patches[current_pred_idx_low++];
                add_org_if_empty(new Predator(0.5, 0.8, 0.5), patch_idx, 0);
            }
        }

        for (int i = 0; i < initial_predators_medium_resource; ++i) {
            if (current_pred_idx_medium < (int)medium_resource_patches.size()) {
                int patch_idx = medium_resource_patches[current_pred_idx_medium++];
                add_org_if_empty(new Predator(0.5, 0.8, 0.5), patch_idx, 1);
            }
        }

        for (int i = 0; i < initial_predators_high_resource; ++i) {
            if (current_pred_idx_high < (int)high_resource_patches.size()) {
                int patch_idx = high_resource_patches[current_pred_idx_high++];
                add_org_if_empty(new Predator(0.5, 0.8, 0.5), patch_idx, 2);
            }
        }
    }
};

#endif