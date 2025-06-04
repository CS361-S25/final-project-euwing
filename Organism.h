#ifndef ORGANISM_H
#define ORGANISM_H

// This is the base class for all organisms (like prey and predator).
// It stores shared traits and defines the basic interface for behavior.
class Organism {
protected:
    double alpha;     // How strongly this organism reacts to patch differences
    double tau;       // What the organism prefers: 1 = food, 0 = safety
    double move_rate; // Probability the organism moves each turn (0 to 1)

public:
    // Constructor: sets up the initial values for alpha, tau, and move rate
    Organism(double a, double t, double m)
        : alpha(a), tau(t), move_rate(m) {}

    // A virtual destructor so derived classes can clean up properly
    virtual ~Organism() = default;

    // Functions to get values (traits)
    double GetAlpha() const { return alpha; }
    double GetTau() const { return tau; }

    // Organisms can override this if their move_rate behaves differently
    virtual double GetMoveRate() const { return move_rate; }

    // These functions must be implemented by any subclass (like Prey or Predator)
    virtual bool IsPrey() const = 0;            // Is this a prey? (true or false)
    virtual void SetAlpha(double new_alpha) = 0; // Change alpha value
    virtual Organism* Clone() const = 0;        // Make a new copy of this organism
    virtual bool IsDead() const = 0;            // Should this organism be removed?
};

#endif





