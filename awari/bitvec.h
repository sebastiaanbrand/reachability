#ifndef BITVEC_H
#define BITVEC_H
#include <functional>
#include <iostream>
#include <vector>
#include <sylvan_obj.hpp>


using namespace sylvan;
// using namespace std;

// namespace sylvan {

struct adder_result;

class Bitvec
{
public:
    std::vector<Bdd> bitvec;
    Bitvec() = default;
    Bitvec(size_t bitnum, const Bdd& value);

    static adder_result bitvec_add(const Bitvec& left, const Bitvec& right);

    // return the conjunction of all bits by &
    Bdd combineBdd();

    // return the results of XNOR for left and this (the object itself)
    Bdd bitvecXnor(const Bitvec& left);

    // return the number of bits.
    size_t bitnum() const;

    // [] overload
    Bdd& operator[](size_t i);
    const Bdd& operator[](size_t i) const;

    // if then else
    Bitvec bitvecIte(const Bdd& val, const Bitvec& left, const Bitvec& right);

};
// }

// the form of addition of 2 bitvec, this is for return the last carry.
struct adder_result{
    Bitvec sum;
    Bdd carry;
};

#endif