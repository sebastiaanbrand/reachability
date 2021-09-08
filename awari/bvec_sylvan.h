#ifndef BVEC_SYLVAN_H
#define BVEC_SYLVAN_H

#include <functional>
#include <iterator>
#include <iostream>
#include <vector>
#include <sylvan_obj.hpp>

namespace sylvan {

class Bvec {
    std::vector<Bdd> m_bitvec;

public:

    Bvec() = default;

    Bvec(size_t bitnum, const Bdd& value);

    Bvec(const Bvec& other);

    Bvec(const size_t bitnum, const size_t val);

    Bvec&
    operator=(Bvec other);

    ~Bvec() = default;

    void
    set(size_t i, const Bdd& con);

    void
    push_back(const Bdd& con);

    Bdd&
    operator[](size_t i);

    const Bdd&
    operator[](size_t i) const;

    size_t
    bitnum() const;

    bool
    empty() const;

    static Bvec
    bvec_build(size_t bitnum, bool isTrue);

    static Bvec
    bvec_true(size_t bitnum);

    static Bvec
    bvec_false(size_t bitnum);

    static Bvec
    bvec_con(size_t bitnum, int val);

    static Bvec
    bvec_ncon(size_t bitnum, int val);

    static Bvec
    bvec_var(size_t bitnum, int offset, int step);

    static Bvec
    bvec_varvec(size_t bitnum, int *var);

    Bvec
    bvec_coerce(size_t bitnum) const;

    static Bvec
    arithmetic_neg(const Bvec& src);


    bool
    bvec_isConst() const;

    int
    bvec_val() const;

    int
    bvec_nval() const;

    static Bvec
    bvec_map1(const Bvec& src, std::function<Bdd(const Bdd&)> fun);

    static Bvec
    bvec_map2(const Bvec& first, const Bvec& second, std::function<Bdd(const Bdd& , const Bdd&)> fun);

    static Bvec
    bvec_map3(const Bvec& first, const Bvec& second, const Bvec& third,
      std::function<Bdd(const Bdd& , const Bdd& , const Bdd&)> fun);

    static Bvec
    bvec_add(Bdd *comp, const Bvec& left, const Bvec& right);

    static Bvec
    bvec_add(const Bvec& left, const Bvec& right);

    static Bvec
    bvec_sub(const Bvec& left, const Bvec& right);

    Bvec
    bvec_mulfixed(int con) const;

    static Bvec
    bvec_mul(const Bvec& left, const Bvec& right);

    int
    bvec_divfixed(int con, Bvec& result, Bvec& rem);

    static int
    bvec_div(const Bvec& left, const Bvec& right, Bvec& result, Bvec& rem);

    static int
    bvec_sdiv(const Bvec& left, const Bvec& right, Bvec& result, Bvec& rem);

    static Bvec
    bvec_ite(const Bdd& a, const Bvec& left, const Bvec& right);

    Bvec
    bvec_shlfixed(size_t pos, const Bdd& con) const;

    static Bvec
    bvec_shl(const Bvec& left, const Bvec& right, const Bdd& con);

    Bvec
    bvec_shrfixed(size_t pos, const Bdd& con) const;

    static Bvec
    bvec_shr(const Bvec& left, const Bvec& right, const Bdd& con);

    static Bdd
    bvec_lth(const Bvec& left, const Bvec& right);

    static Bdd
    bvec_lte(const Bvec& left, const Bvec& right);

    static Bdd
    bvec_gth(const Bvec& left, const Bvec& right);

    static Bdd
    bvec_gte(const Bvec& left, const Bvec& right);

    static Bdd
    bvec_equ(const Bvec& left, const Bvec& right);

    static Bdd
    bvec_slth(const Bvec& left, const Bvec& right);

    static Bdd
    bvec_slte(const Bvec& left, const Bvec& right);

    static Bdd
    bvec_sgth(const Bvec& left, const Bvec& right);

    static Bdd
    bvec_sgte(const Bvec& left, const Bvec& right);

    Bvec
    operator&(const Bvec& other) const { return bvec_map2(*this, other, bdd_and); }

    Bvec
    operator^(const Bvec& other) const { return bvec_map2(*this, other, bdd_xor); }

    Bvec
    operator|(const Bvec& other) const { return bvec_map2(*this, other, bdd_or); }

    Bvec
    operator!(void) const { return bvec_map1(*this, bdd_not); }

    Bvec
    operator~(void) const { return bvec_map1(*this, bdd_not); }

    Bvec
    operator<<(int con) const { return bvec_shlfixed(con, Bdd::bddZero()); }

    Bvec
    operator<<(const Bvec& other) const { return bvec_shl(*this, other,Bdd::bddZero()); }

    Bvec
    operator>>(int con) const { return bvec_shrfixed(con, Bdd::bddZero()); }

    Bvec
    operator>>(const Bvec& other) const { return bvec_shr(*this, other, Bdd::bddZero()); }

    Bvec
    operator+(const Bvec& other) const { return bvec_add(*this, other); }

    Bvec
    operator+=(const Bvec& other) { *this = bvec_add(*this, other); return *this; }

    Bvec
    operator-(const Bvec& other) { return bvec_sub(*this, other); }

    Bvec
    operator-=(const Bvec& other) { *this = bvec_sub(*this, other); return *this; }

    Bvec
    operator*(int con) const { return bvec_mulfixed(con); }

    Bvec
    operator*=(int con) { this->bvec_mulfixed(con); return *this; }

    Bvec
    operator*(const Bvec& other) const { return bvec_mul(*this, other); }

    Bvec
    operator*=(const Bvec& other) { *this = bvec_mul(*this, other); return *this; }

    Bdd
    operator<(const Bvec& other) const { return bvec_lth(*this, other); }

    Bdd
    operator<=(const Bvec& other) const { return bvec_lte(*this, other); }

    Bdd
    operator>(const Bvec& other) const { return bvec_gth(*this, other); }

    Bdd
    operator>=(const Bvec& other) const { return bvec_gte(*this, other); }

    Bdd
    operator==(const Bvec& other) const { return bvec_equ(*this, other); }

    Bdd
    operator!=(const Bvec& other) const { return !(*this == other); }

    Bdd
	operator==(const size_t val) const { return *this == Bvec(bitnum(), val); };

private:

    static void
    bvec_div_rec(Bvec divisor, Bvec& remainder, Bvec& result, size_t step);

    static Bdd
    bdd_and(const Bdd& first, const Bdd& second);

    static Bdd
    bdd_xor(const Bdd& first, const Bdd& second);

    static Bdd
    bdd_or(const Bdd& first, const Bdd& second);

    static Bdd
    bdd_not(const Bdd& src);

    static Bdd
    get_signs(const Bdd& left, const Bdd& right);

    void
    swap(Bvec& other);

    static Bvec
    reserve(size_t bitnum);

    static void
    reserve(Bvec& bitvector, size_t bitnum);

};
}

#endif //BVEC_SYLVAN_H
