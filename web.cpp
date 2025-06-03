#define UIT_VENDORIZE_EMP
#define UIT_SUPPRESS_MACRO_INSEEP_WARNINGS

#include "emp/web/web.hpp"
#include "emp/web/Animate.hpp"
#include "World.h"
#include "Prey.h"
#include "Predator.h"
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <iostream>

emp::web::Document doc("target");

class WebAnimator : public emp::web::Animate {
    const int num_columns = 30;
    const int num_rows = 30;
    const int cell_width = 20;
    const int cell_height = 20;

    World world;
    emp::web::Canvas canvas;
    int generation = 0;
    double predator_death_rate = 0.03;

public:
    WebAnimator()
        : world(num_columns * num_rows),
          canvas(num_columns * cell_width, num_rows * cell_height, "canvas") {

        doc << canvas;
        doc << GetToggleButton("Start") << GetStepButton("Step");

        world.SetCloneFunction([](bool is_prey, double a, double t, double m) {
            return is_prey ? static_cast<Organism*>(new Prey(a, t, m))
                           : static_cast<Organism*>(new Predator(a, t, m));
        });

        ResetWorld();
    }

    void ResetWorld() {
        world = World(num_columns * num_rows);
        generation = 0;
        world.SetPredatorDeathRate(predator_death_rate);

        world.SetCloneFunction([](bool is_prey, double a, double t, double m) {
            return is_prey ? static_cast<Organism*>(new Prey(a, t, m))
                           : static_cast<Organism*>(new Predator(a, t, m));
        });

        struct PatchZone {
            int x_start, y_start;
            double resource;
        };

        // 9 zones, 10×10 each = 100 cells × 9 = 900 (full grid)
        std::vector<PatchZone> zones = {
            {0, 0, 0.9},  {10, 0, 0.9},  {20, 0, 0.9},   // Green
            {0, 10, 0.5}, {10, 10, 0.5}, {20, 10, 0.5},  // Orange
            {0, 20, 0.1}, {10, 20, 0.1}, {20, 20, 0.1}   // Red
        };

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

            int center_x = zone.x_start + 5;
            int center_y = zone.y_start + 5;
            int center_idx = center_y * num_columns + center_x;
            world.AddOrganism(new Predator(0.5, 0.0, 0.5), center_idx);

            std::vector<int> prey_indices;
            for (int idx : patch_indices) {
                if (idx != center_idx) prey_indices.push_back(idx);
            }

            for (size_t i = 0; i < prey_indices.size(); ++i) {
                size_t j = random.GetUInt(i + 1);
                std::swap(prey_indices[i], prey_indices[j]);
            }

            int num_prey = std::min(10, (int)prey_indices.size());
            for (int i = 0; i < num_prey; ++i) {
                world.AddOrganism(new Prey(0.5, 1.0, 0.5), prey_indices[i]);
            }
        }

        Draw();
    }

    std::string ResourceColor(double resource) {
        if (resource < 0.33) return "#ff0000";      // Low = red
        else if (resource < 0.66) return "#ff9900"; // Medium = orange
        else return "#00cc00";                      // High = green
    }

    void DoFrame() override {
        if (generation < 25) {
            world.Step();
            generation++;
            Draw();
        } else {
            Stop();
        }
    }

    void Draw() {
        canvas.Clear();
        const auto& patches = world.GetPatches();

        for (size_t i = 0; i < patches.size(); ++i) {
            int x = (i % num_columns) * cell_width;
            int y = (i / num_columns) * cell_height;

            const auto& patch = patches[i];
            std::string color = ResourceColor(patch.resource_level);

            bool has_predator = false;
            bool has_prey = false;

            for (Organism* org : patch.occupants) {
                if (org->IsPrey()) has_prey = true;
                else has_predator = true;
            }

            std::string outline = color;
            std::string fill;

            if (has_predator) fill = "pink";  
            else if (has_prey) fill = "blue";
            else fill = color;

            canvas.Rect(x, y, cell_width, cell_height, outline, outline);
            canvas.Rect(x + 2, y + 2, cell_width - 4, cell_height - 4, fill, "black");
        }
    }
};

WebAnimator anim;

int main() {
    anim.Step(); // show initial state
}







