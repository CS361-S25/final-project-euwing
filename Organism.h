#ifndef ORGANISM_H
#define ORGANISM_H

// Base class for all organisms (prey and predators).
class Organism {
protected:
    double alpha; // Organism's alpha trait.
    double tau;   // Organism's tau trait.
    double move_rate; // Probability of moving to a different patch.
    int birth_zone = -1; // Resource zone where the organism was born.

public:
    // Constructor initializes organism traits.
    Organism(double a, double t, double m)
        : alpha(a), tau(t), move_rate(m) {}

    // Virtual destructor.
    virtual ~Organism() = default;

    // Returns organism's alpha trait.
    double GetAlpha() const { return alpha; }
    // Returns organism's tau trait.
    double GetTau() const { return tau; }
    // Returns organism's move rate.
    virtual double GetMoveRate() const { return move_rate; }

    // Returns true if prey, false if predator.
    virtual bool IsPrey() const = 0;
    // Sets organism's alpha trait.
    virtual void SetAlpha(double new_alpha) = 0;
    // Creates a copy of the organism.
    virtual Organism* Clone() const = 0;
    // Checks if organism is dead.
    virtual bool IsDead() const = 0;

    // Sets the birth zone.
    void SetBirthZone(int z) { birth_zone = z; }
    // Returns the birth zone.
    int GetBirthZone() const { return birth_zone; }
};

#endif






