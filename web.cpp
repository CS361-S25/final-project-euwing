#define UIT_VENDORIZE_EMP
#define UIT_SUPPRESS_MACRO_INSEEP_WARNINGS

#include "emp/web/web.hpp"
#include "emp/web/Animate.hpp"
#include "World.h"
#include "Prey.h"
#include "Prey2.h"
#include "Predator.h"
#include <sstream>
#include <random> 
#include <numeric> 

emp::web::Document doc("target");

class WebAnimator : public emp::web::Animate {
    // These constants can also be made configurable in the GUI if desired
    const int num_columns = 30;
    const int num_rows = 30;
    const int cell_width = 20;
    const int cell_height = 20;
    const int generation_limit = 1000; // Increased generation limit for better observation

    World world;
    emp::web::Canvas canvas;
    int generation = 0;

    // --- Configuration Inputs ---
    emp::web::Element predator_death_rate_input;
    emp::web::Element initial_prey1_input;
    emp::web::Element initial_prey2_input;
    emp::web::Element initial_predator_low_input;    // New: Low resource predator input
    emp::web::Element initial_predator_medium_input; // New: Medium resource predator input
    emp::web::Element initial_predator_high_input;   // New: High resource predator input
    emp::web::Element mutation_rate_input;
    emp::web::Element mutation_sd_input;

    // --- Control Buttons ---
    emp::web::Button step_btn;
    emp::web::Button start_stop_btn;
    emp::web::Button reset_btn;

    // --- Display Elements ---
    emp::web::Div stats_div;
    emp::web::Div config_div;
    emp::web::Div suggestions_div;

    // Initial values (can be updated from GUI)
    double current_predator_death_rate = 0.00001;
    int current_initial_prey1 = 10;
    int current_initial_prey2 = 10;
    // Old: int current_initial_predator = 5;
    int current_initial_predators_low = 0; // Red
    int current_initial_predators_medium = 3; // Yellow
    int current_initial_predators_high = 6; // Green
    double current_mutation_rate = 0.05;
    double current_mutation_sd = 0.025;

public:
    WebAnimator()
        : world(num_columns * num_rows),
          canvas(num_columns * cell_width, num_rows * cell_height, "canvas"),
          // Initialize buttons
          step_btn([this]() { Step(); }, "Step"),
          start_stop_btn([this]() { ToggleActive(); }, "Start/Stop"),
          reset_btn([this]() { ResetSimulation(); }, "Reset Simulation"),
          // Initialize input elements
          predator_death_rate_input("input"),
          initial_prey1_input("input"),
          initial_prey2_input("input"),
          initial_predator_low_input("input"),    
          initial_predator_medium_input("input"), 
          initial_predator_high_input("input"),   
          mutation_rate_input("input"),
          mutation_sd_input("input")
    {
        SetupInputs();
        SetupLayout();
        SetupWorldCloning();
        ResetSimulation(); // Initial reset to set up the world
    }

    void SetupInputs() {
        // Predator Death Rate
        predator_death_rate_input.SetAttr("type", "text");
        predator_death_rate_input.SetAttr("value", std::to_string(current_predator_death_rate));
        predator_death_rate_input.On("change", [this]() {
            try {
                current_predator_death_rate = std::stod(predator_death_rate_input.GetAttr("value"));
                world.SetPredatorDeathRate(current_predator_death_rate);
            } catch (...) {}
        });

        // Initial Prey1 Count
        initial_prey1_input.SetAttr("type", "number");
        initial_prey1_input.SetAttr("value", std::to_string(current_initial_prey1));
        initial_prey1_input.On("change", [this]() {
            try {
                current_initial_prey1 = std::stoi(initial_prey1_input.GetAttr("value"));
            } catch (...) {}
        });

        // Initial Prey2 Count
        initial_prey2_input.SetAttr("type", "number");
        initial_prey2_input.SetAttr("value", std::to_string(current_initial_prey2));
        initial_prey2_input.On("change", [this]() {
            try {
                current_initial_prey2 = std::stoi(initial_prey2_input.GetAttr("value"));
            } catch (...) {}
        });

        // New: Initial Predator Count (Low Resource - Red)
        initial_predator_low_input.SetAttr("type", "number");
        initial_predator_low_input.SetAttr("value", std::to_string(current_initial_predators_low));
        initial_predator_low_input.On("change", [this]() {
            try {
                current_initial_predators_low = std::stoi(initial_predator_low_input.GetAttr("value"));
            } catch (...) {}
        });

        // New: Initial Predator Count (Medium Resource - Yellow)
        initial_predator_medium_input.SetAttr("type", "number");
        initial_predator_medium_input.SetAttr("value", std::to_string(current_initial_predators_medium));
        initial_predator_medium_input.On("change", [this]() {
            try {
                current_initial_predators_medium = std::stoi(initial_predator_medium_input.GetAttr("value"));
            } catch (...) {}
        });

        // New: Initial Predator Count (High Resource - Green)
        initial_predator_high_input.SetAttr("type", "number");
        initial_predator_high_input.SetAttr("value", std::to_string(current_initial_predators_high));
        initial_predator_high_input.On("change", [this]() {
            try {
                current_initial_predators_high = std::stoi(initial_predator_high_input.GetAttr("value"));
            } catch (...) {}
        });


        // Mutation Rate
        mutation_rate_input.SetAttr("type", "text");
        mutation_rate_input.SetAttr("value", std::to_string(current_mutation_rate));
        mutation_rate_input.On("change", [this]() {
            try {
                current_mutation_rate = std::stod(mutation_rate_input.GetAttr("value"));
                world.SetMutationRate(current_mutation_rate);
            } catch (...) {}
        });

        // Mutation Standard Deviation
        mutation_sd_input.SetAttr("type", "text");
        mutation_sd_input.SetAttr("value", std::to_string(current_mutation_sd));
        mutation_sd_input.On("change", [this]() {
            try {
                current_mutation_sd = std::stod(mutation_sd_input.GetAttr("value"));
                world.SetMutationSD(current_mutation_sd);
            } catch (...) {}
        });
    }

    void SetupLayout() {
        doc << "<h3>Artificial Ecosystem Simulation</h3>";
        doc << canvas;
        doc << "<br>" << start_stop_btn << " " << step_btn << " " << reset_btn << "<br>";

        // Configuration Panel
        config_div << "<h4>Configuration:</h4>";
        config_div << "Predator Death Rate: " << predator_death_rate_input << "<br>";
        config_div << "Initial Prey1: " << initial_prey1_input << "<br>";
        config_div << "Initial Prey2: " << initial_prey2_input << "<br>";
        config_div << "Initial Predators (Red Zone): " << initial_predator_low_input << "<br>";    
        config_div << "Initial Predators (Yellow Zone): " << initial_predator_medium_input << "<br>"; 
        config_div << "Initial Predators (Green Zone): " << initial_predator_high_input << "<br>";   
        config_div << "Mutation Rate: " << mutation_rate_input << "<br>";
        config_div << "Mutation SD: " << mutation_sd_input << "<br>";
        doc << config_div;

        doc << "<br><b>Legend:</b><br>"
            << "Red = low resource, Orange = medium, Green = high<br>"
            << "Prey1 (Mobile) = blue, Prey2 (Immobile) = cyan, Predators = pink<br><br>";
        doc << stats_div;

        // Suggestions Panel
        suggestions_div << "<h4>Suggestions:</h4>"
                        << "<ul>"
                        << "<li>Try setting Predator Death Rate to 0.001 to observe faster predator population decline and its impact on prey evolution.</li>"
                        << "<li>Increase initial Prey1 and decrease Prey2 to see if Prey1 can outcompete Prey2 without initial numerical advantage.</li>"
                        << "<li>Experiment with different mutation rates: a very low rate might slow down adaptation, a very high rate might lead to chaotic evolution.</li>"
                        << "<li>Observe how the average Tau of Prey1 changes over generations in different resource zones. Does it converge?</li>"
                        << "<li>How does the presence of immobile Prey2 affect the selection pressure on mobile Prey1's escape velocity?</li>"
                        << "</ul>";
        doc << suggestions_div;
    }

    void SetupWorldCloning() {
        // This ensures new organisms are created as the correct derived types after mutation
        world.SetCloneFunction([](bool is_prey, double a, double t, double m) -> Organism* {
            if (!is_prey) return new Predator(a, t, m);
            if (t > 0.5) return new Prey(a, t, m); // Prey1 (mobile)
            return new Prey2(a, t, m); // Prey2 (immobile)
        });
    }

    void ResetSimulation() {
        // Ensure the world's parameters are updated from GUI inputs before reset
        world.SetPredatorDeathRate(current_predator_death_rate);
        world.SetMutationRate(current_mutation_rate);
        world.SetMutationSD(current_mutation_sd);

        world = World(num_columns * num_rows); // Re-initialize world to clear all organisms and patches
        world.SetPredatorDeathRate(current_predator_death_rate); // Re-apply after world creation
        world.SetMutationRate(current_mutation_rate);
        world.SetMutationSD(current_mutation_sd);
        SetupWorldCloning(); // Re-apply clone function to new world instance

        generation = 0;

        // Setup zones and resources
        std::vector<std::tuple<int, int, double>> zones = {
            {0, 0, 0.9}, {10, 0, 0.9}, {20, 0, 0.9}, // High resource zones
            {0, 10, 0.5}, {10, 10, 0.5}, {20, 10, 0.5}, // Medium resource zones
            {0, 20, 0.1}, {10, 20, 0.1}, {20, 20, 0.1}  // Low resource zones
        };

        // Initialize patches with resources
        for (const auto& [x_start, y_start, r] : zones) {
            for (int dy = 0; dy < 10; ++dy) {
                for (int dx = 0; dx < 10; ++dx) {
                    int x = x_start + dx;
                    int y = y_start + dy;
                    int idx = y * num_columns + x;
                    world.GetPatchesMutable()[idx].resource_level = r;
                }
            }
        }

        // Add initial organisms using the new ResetOrganisms function
        world.ResetOrganisms(
            current_initial_prey1, current_initial_prey2,
            current_initial_predators_low,
            current_initial_predators_medium,
            current_initial_predators_high
        );

        Draw();
        UpdateStats();
        // Stop animation if it was running, as we just reset
        if (emp::web::Animate::GetActive()) emp::web::Animate::ToggleActive();
    }

    std::string ResourceColor(double r) {
        if (r < 0.33) return "#ff0000"; // Red for low
        if (r < 0.66) return "#ff9900"; // Orange for medium
        return "#00cc00";               // Green for high
    }

    void DoFrame() override {
        world.Step();
        generation++;
        Draw();
        UpdateStats();

        // Stop simulation if generation limit is reached
        if (generation >= generation_limit) {
            ToggleActive(); // Stops the animation
        }
    }

    void Draw() {
        canvas.Clear();
        const auto& patches = world.GetPatches();

        for (size_t i = 0; i < patches.size(); ++i) {
            int x = (i % num_columns) * cell_width;
            int y = (i / num_columns) * cell_height;
            const auto& patch = patches[i];

            std::string bg = ResourceColor(patch.resource_level);
            std::string occupant_fill_color = bg; // Default to background color

            // Determine occupant color based on the *first* organism found
            // For more complex rendering (e.g., multiple organisms), you'd need overlays
            for (Organism* org : patch.occupants) {
                if (!org->IsPrey()) occupant_fill_color = "pink"; // Predator
                else if (org->GetTau() > 0.5) occupant_fill_color = "blue"; // Prey1 (mobile)
                else occupant_fill_color = "cyan"; // Prey2 (immobile)
                break; // Only draw the first occupant's color
            }

            canvas.Rect(x, y, cell_width, cell_height, bg, bg); // Draw patch background
            // Draw a smaller inner rectangle for the occupant, if any
            if (!patch.occupants.empty()) {
                canvas.Rect(x + 2, y + 2, cell_width - 4, cell_height - 4, occupant_fill_color, "black");
            }
        }
    }

    void UpdateStats() {
        std::ostringstream out;
        out.precision(4); // Set precision for double values

        out << "<b>Generation:</b> " << generation << "<br>";
        out << "<b>Total Organisms:</b> " << world.GetTotalOrganismCount() << "<br>";
        out << "<br>";

        // Display counts for each type across all zones
        out << "<b>Prey1 (Mobile):</b> " << world.GetPrey1Count()
            << " | Avg Alpha: " << std::fixed << world.GetAveragePreyAlpha(true)
            << " | Avg Tau: " << std::fixed << world.GetAveragePreyTau(true) << "<br>";
        out << "<b>Prey2 (Immobile):</b> " << world.GetPrey2Count()
            << " | Avg Alpha: " << std::fixed << world.GetAveragePreyAlpha(false)
            << " | Avg Tau: " << std::fixed << world.GetAveragePreyTau(false) << "<br>";
        out << "<b>Predators:</b> " << world.GetPredatorCount() << "<br>";

        stats_div.Clear();
        stats_div << out.str();
    }
};

WebAnimator anim; // Create the animator object

int main() {
    // The main function can be minimal, as emp::web::Animate handles the loop
}