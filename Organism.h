#ifndef ORGANISM_H
#define ORGANISM_H

// Abstract base class for all organisms (Prey and Predator)
class Organism {
protected:
    double alpha;     // Sensitivity to differences in patch value (higher = more selective)
    double tau;       // Preference for food (1.0) vs safety (0.0)
    double move_rate; // Probability of moving to a different patch each turn

public:
    Organism(double a, double t, double m)
        : alpha(a), tau(t), move_rate(m) {}

    virtual ~Organism() = default;

    double GetAlpha() const { return alpha; }
    double GetTau() const { return tau; }
    double GetMoveRate() const { return move_rate; }

    // Returns true if this organism is a prey, false if predator
    virtual bool IsPrey() const = 0;

    // Update alpha value (sensitivity to patch score differences)
    virtual void SetAlpha(double new_alpha) = 0;

    // Create a new copy of this organism (used for reproduction)
    virtual Organism* Clone() const = 0;

    // Return true if the organism is dead
    virtual bool IsDead() const = 0;
};

#endif




