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
    const int num_columns = 60;
    const int num_rows = 60;
    const int cell_width = 10;
    const int cell_height = 10;

    World world;
    emp::web::Canvas canvas;
    std::ofstream csv_file;
    int generation = 0;

public:
    WebAnimator()
        : world(num_columns * num_rows),
          canvas(num_columns * cell_width, num_rows * cell_height, "canvas") {

        doc << canvas;
        doc << GetToggleButton("Start") << GetStepButton("Step");
        ResetWorld();
    }

    void ResetWorld() {
        world = World(num_columns * num_rows);
        generation = 0;

        struct PatchZone {
            int x_start, y_start;
            double resource;
        };

        std::vector<PatchZone> zones = {
            {0, 0, 0.9},   {15, 0, 0.9},   {30, 0, 0.9},
            {0, 30, 0.5},  {15, 30, 0.5},
            {30, 30, 0.1}, {45, 30, 0.1}, {45, 0, 0.1}
        };

        for (const auto& zone : zones) {
            for (int dy = 0; dy < 30; ++dy) {
                for (int dx = 0; dx < 15; ++dx) {
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

            for (int dy = 0; dy < 30; ++dy) {
                for (int dx = 0; dx < 15; ++dx) {
                    int x = zone.x_start + dx;
                    int y = zone.y_start + dy;
                    int idx = y * num_columns + x;
                    patch_indices.push_back(idx);
                }
            }

            int center_x = zone.x_start + 7;
            int center_y = zone.y_start + 15;
            int center_idx = center_y * num_columns + center_x;

            world.AddOrganism(new Predator(0.5, 0.5, 0.5), center_idx);

            std::vector<int> prey_indices;
            for (int idx : patch_indices) {
                if (idx != center_idx) prey_indices.push_back(idx);
            }

            for (int i = 0; i < 5; ++i) {
                int idx = prey_indices[random.GetUInt(prey_indices.size())];
                world.AddOrganism(new Prey(0.5, 0.5, 0.5), idx);
            }
        }

        Draw();
    }

    std::string ResourceColor(double resource) {
        if (resource < 0.33) return "#ffffcc";
        else if (resource < 0.66) return "#ffeb3b";
        else return "#fbc02d";
    }

    void DoFrame() override {
        if (generation < 25) {
            world.Step();

            double average_alpha_prey = world.GetAverageAlpha(true);
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

            generation++;
            Draw();
        } else {
            Stop();
        }
    }

    int ClassifyZone(double resource_level) {
        if (resource_level < 0.33) return 0;
        else if (resource_level < 0.66) return 1;
        else return 2;
    }

    void Draw() {
        canvas.Clear();
        const auto& patches = world.GetPatches();

        for (size_t i = 0; i < patches.size(); ++i) {
            int x = (i % num_columns) * cell_width;
            int y = (i / num_columns) * cell_height;

            bool has_predator = false;
            bool has_prey = false;

            for (Organism* org : patches[i].occupants) {
                if (org->IsPrey()) has_prey = true;
                else has_predator = true;
            }

            if (has_predator) {
                canvas.Rect(x, y, cell_width, cell_height, "red", "black");
            } else if (has_prey) {
                canvas.Rect(x, y, cell_width, cell_height, "blue", "black");
            } else {
                canvas.Rect(x, y, cell_width, cell_height,
                            ResourceColor(patches[i].resource_level), "black");
            }
        }
    }
};

WebAnimator anim;

int main() {
    anim.Step();
}





