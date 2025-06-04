#define UIT_VENDORIZE_EMP
#define UIT_SUPPRESS_MACRO_INSEEP_WARNINGS

#include "emp/web/web.hpp"
#include "emp/web/Animate.hpp"
#include "World.h"
#include "Prey.h"
#include "Prey2.h"
#include "Predator.h"
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <iostream>

// Connect to the HTML element with id="target"
emp::web::Document doc("target");

// This class manages the animation and drawing for the web version
class WebAnimator : public emp::web::Animate {
    const int num_columns = 30;
    const int num_rows = 30;
    const int cell_width = 20;
    const int cell_height = 20;

    World world;
    emp::web::Canvas canvas;
    int generation = 0;
    double predator_death_rate = 0.0001;

public:
    // Constructor initializes canvas and world
    WebAnimator()
        : world(num_columns * num_rows),
          canvas(num_columns * cell_width, num_rows * cell_height, "canvas") {

        doc << canvas;
        doc << GetToggleButton("Start") << GetStepButton("Step");
        doc << "<p style='color: black;'>"
        << "Red is low resources, Yellow is medium, and Greeen is High.<br>"
        << "Predators are pink, Prey are blue, and Prey2 are cyan.<br>"
        << "Click 'Start' to run the simulation, or 'Step' to advance one generation.<br>"
        << "The simulation will stop automatically after 100 generations."
        
        << "</p>";

        // Tell the world how to clone organisms
        world.SetCloneFunction([](bool is_prey, double a, double t, double m) -> Organism* {
            if (!is_prey) return new Predator(a, t, m);
            if (t > 0.5) return new Prey(a, t, m);
            return new Prey2(a, t, m);
        });

        ResetWorld();
    }

    // This function resets the world and fills it with organisms
    void ResetWorld() {
        world = World(num_columns * num_rows);
        generation = 0;
        world.SetPredatorDeathRate(predator_death_rate);

        world.SetCloneFunction([](bool is_prey, double a, double t, double m) -> Organism* {
            if (!is_prey) return new Predator(a, t, m);
            if (t > 0.5) return new Prey(a, t, m);
            return new Prey2(a, t, m);
        });

        // Define rectangular zones with different resource levels
        struct PatchZone {
            int x_start, y_start;
            double resource;
        };

        std::vector<PatchZone> zones = {
            {0, 0, 0.9},  {10, 0, 0.9},  {20, 0, 0.9},
            {0, 10, 0.5}, {10, 10, 0.5}, {20, 10, 0.5},
            {0, 20, 0.1}, {10, 20, 0.1}, {20, 20, 0.1}
        };

        // Assign resource levels to patches
        for (const auto& zone : zones) {
            for (int dy = 0; dy < 10; ++dy) {
                for (int dx = 0; dx < 10; ++dx) {
                    int x = zone.x_start + dx;
                    int y = zone.y_start + dy;
                    int idx = y * num_columns + x;
                    world.GetPatchesMutable()[idx].resource_level = zone.resource;
                }
            }
        }

        // Add organisms to each zone
        emp::Random random;

        for (const auto& zone : zones) {
            std::vector<int> patch_indices;

            for (int dy = 0; dy < 10; ++dy) {
                for (int dx = 0; dx < 10; ++dx) {
                    int x = zone.x_start + dx;
                    int y = zone.y_start + dy;
                    int idx = y * num_columns + x;
                    patch_indices.push_back(idx);
                }
            }

            // Add predator in the center of each zone
            int center_x = zone.x_start + 5;
            int center_y = zone.y_start + 5;
            int center_idx = center_y * num_columns + center_x;
            world.AddOrganism(new Predator(0.5, 0.0, 0.5), center_idx);

            // Shuffle remaining patch locations
            for (size_t i = 0; i < patch_indices.size(); ++i) {
                size_t j = random.GetUInt(i + 1);
                std::swap(patch_indices[i], patch_indices[j]);
            }

            // Add a mix of Prey and Prey2
            int num_prey = std::min(10, (int)patch_indices.size());
            for (int i = 0; i < num_prey; ++i) {
                if (random.P(0.5))
                    world.AddOrganism(new Prey(0.5, 1.0, 0.5), patch_indices[i]);
                else
                    world.AddOrganism(new Prey2(0.5, 0.0, 0.5), patch_indices[i]);
            }
        }

        Draw();
    }

    // Pick background color for each patch based on its resources
    std::string ResourceColor(double resource) {
        if (resource < 0.33) return "#ff0000";      // Red = low
        else if (resource < 0.66) return "#ff9900"; // Orange = medium
        else return "#00cc00";                      // Green = high
    }

    // Runs each animation frame
    void DoFrame() override {
        if (generation < 100) {
            world.Step();
            generation++;
            Draw();
        } else {
            Stop(); // Stop simulation after 100 generations
        }
    }

    // Draw all the patches and organisms on the canvas
    void Draw() {
        canvas.Clear();
        const auto& patches = world.GetPatches();

        for (size_t i = 0; i < patches.size(); ++i) {
            int x = (i % num_columns) * cell_width;
            int y = (i / num_columns) * cell_height;

            const auto& patch = patches[i];
            std::string color = ResourceColor(patch.resource_level);
            std::string fill = color;

            for (Organism* org : patch.occupants) {
                if (!org->IsPrey()) fill = "pink";             // Predator
                else if (org->GetTau() > 0.5) fill = "blue";   // Prey
                else fill = "cyan";                            // Prey2
            }

            canvas.Rect(x, y, cell_width, cell_height, color, color); // background
            canvas.Rect(x + 2, y + 2, cell_width - 4, cell_height - 4, fill, "black"); // organism
        }
    }
};

// Make the animator
WebAnimator anim;

// Show the initial state
int main() {
    anim.Step();
}

