#include "bvec_sylvan.h"

namespace sylvan {

    Bvec::Bvec(size_t bitnum, const Bdd& value) : m_bitvec(bitnum, value) {}

    Bvec::Bvec(const Bvec& other) : m_bitvec(other.m_bitvec) {}

    Bvec::Bvec(const size_t bitnum, const size_t v) {
		m_bitvec.reserve(bitnum);
		size_t val = v;
        for (size_t i = 0U; i < bitnum; ++i) {
            if (val & 1U) {
                m_bitvec.push_back(Bdd::bddOne());
            } else {
                m_bitvec.push_back(Bdd::bddZero());
            }
            val >>= 1U;
        }
	}


    Bvec&
    Bvec::operator=(Bvec other) {
        swap(other);
        return *this;
    }

    void
    Bvec::set(size_t i, const Bdd& con) {
        m_bitvec.at(i) = con;
    }

    void
    Bvec::push_back(const Bdd& con) {
        m_bitvec.push_back(con);
    }

    Bdd&
    Bvec::operator[](size_t i) {
        return m_bitvec.at(i);
    }

    const Bdd&
    Bvec::operator[](size_t i) const {
        return m_bitvec.at(i);
    }

    size_t
    Bvec::bitnum() const {
        return m_bitvec.size();
    }

    bool
    Bvec::empty() const {
        return m_bitvec.empty();
    }

    Bvec
    Bvec::bvec_build(size_t bitnum, bool isTrue) {
        return Bvec(bitnum, isTrue ? Bdd::bddOne() : Bdd::bddZero());
    }

    Bvec
    Bvec::bvec_true(size_t bitnum) {
        return bvec_build(bitnum, true);
    }

    Bvec
    Bvec::bvec_false(size_t bitnum) {
        return bvec_build(bitnum, false);
    }

    Bvec
    Bvec::bvec_con(size_t bitnum, int val) {
        return Bvec(bitnum, val);
    }

    Bvec
    Bvec::bvec_ncon(size_t bitnum, int val) {
        if (val > 0) {
            throw std::logic_error("use bvec_con for positive values");
        }
        val *= -1;
        return arithmetic_neg(bvec_con(bitnum, val));
    }

    Bvec
    Bvec::bvec_var(size_t bitnum, int offset, int step) {
        LACE_ME;
        Bvec res = reserve(bitnum);
        for (uint32_t i = 0U; i < bitnum; ++i) {
            res.m_bitvec.push_back(Bdd::bddVar(offset + i * step));
        }
        return res;
    }

    Bvec
    Bvec::bvec_varvec(size_t bitnum, int *var) {
        LACE_ME;
        Bvec res = reserve(bitnum);
        for (size_t i = 0U; i < bitnum; ++i) {
            res.m_bitvec.push_back(Bdd::bddVar(var[i]));
        }
        return res;
    }

    Bvec
    Bvec::bvec_coerce(size_t bits) const {
        LACE_ME;
        Bvec res = bvec_false(bits);
        size_t minnum = std::min(bits, bitnum());
        for (size_t i = 0U; i < minnum; ++i) {
            res[i] = m_bitvec[i];
        }
        return res;
    }

    Bvec
    Bvec::arithmetic_neg(const Bvec& src) {
        return ~src + bvec_con(src.bitnum(), 1);
    }

    bool
    Bvec::bvec_isConst() const {
        LACE_ME;
        for (size_t i = 0U; i < bitnum(); ++i) {
            if (!m_bitvec[i].isConstant()) {
                return false;
            }
        }
        return true;
    }

    int
    Bvec::bvec_val() const {
        LACE_ME;
        int val = 0;
        for (size_t i = bitnum(); i >= 1U; --i) {
            if (m_bitvec[i - 1U].isOne())
                val = (val << 1) | 1;
            else if (m_bitvec[i - 1U].isZero())
                val = val << 1;
            else
                return 0;
        }
        return val;
    }

    int
    Bvec::bvec_nval() const {
        int val = (~(*this) + bvec_con(bitnum(), 1U)).bvec_val();
        return val * -1;
    }

    Bvec
    Bvec::bvec_map1(const Bvec& src, std::function<Bdd(const Bdd&)> fun) {
        LACE_ME;
        Bvec res = reserve(src.bitnum());
        for (size_t i = 0U; i < src.bitnum(); ++i) {
            res.m_bitvec.push_back(fun(src[i]));
        }
        return res;
    }

    Bvec
    Bvec::bvec_map2(const Bvec& fst, const Bvec& snd, std::function<Bdd(const Bdd&, const Bdd&)> fun) {
        LACE_ME;
        Bvec res;
        if (fst.bitnum() != snd.bitnum()) {
            throw std::logic_error("bitnum of input bitvectors is not equal");
        }

        reserve(res, fst.bitnum());
        for (size_t i = 0U; i < fst.bitnum(); ++i) {
            res.m_bitvec.push_back(fun(fst[i], snd[i]));
        }
        return res;
    }

    Bvec
    Bvec::bvec_map3(const Bvec& fst, const Bvec& snd, const Bvec& trd, std::function<Bdd(const Bdd&, const Bdd&, const Bdd&)> fun) {
        LACE_ME;
        Bvec res;
        if (fst.bitnum() != snd.bitnum() || snd.bitnum() != trd.bitnum()) {
            throw std::logic_error("bitnum of input bitvectors is not equal");
        }

        reserve(res, fst.bitnum());
        for (size_t i = 0U; i < fst.bitnum(); ++i) {
            res.m_bitvec.push_back(fun(fst[i], snd[i], trd[i]));
        }
        return res;
    }

    Bvec
    Bvec::bvec_add(Bdd *carry, const Bvec& left, const Bvec& right) {
        LACE_ME;
        Bvec res;
        Bdd comp = Bdd::bddZero();

        if (left.bitnum() == 0 || right.bitnum() == 0) {
            return res;
        }

        if (left.bitnum() != right.bitnum()) {
            throw std::logic_error("bitnum of input bitvectors is not equal");
        }

        reserve(res, left.bitnum());

        for (size_t i = 0U; i < left.bitnum(); ++i) {

            /* bitvec[i] = l[i] ^ r[i] ^ c; */
            res.m_bitvec.push_back((left[i] ^ right[i]) ^ comp);

            /* c = (l[i] & r[i]) | (c & (l[i] | r[i])); */
            comp = (left[i] & right[i]) | (comp & (left[i] | right[i]));
        }
        *carry |= comp;
        return res;
    }

    Bvec
    Bvec::bvec_add(const Bvec& left, const Bvec& right) {
        Bdd comp;
        return bvec_add(&comp, left, right);
    }

    Bvec
    Bvec::bvec_sub(const Bvec& left, const Bvec& right) {
        LACE_ME;
        Bvec res;
        Bdd comp = Bdd::bddZero();

        if (left.bitnum() == 0 || right.bitnum() == 0) {
            return res;
        }

        if (left.bitnum() != right.bitnum()) {
            throw std::logic_error("bitnum of input bitvectors is not equal");
        }

        reserve(res, left.bitnum());

        for (size_t i = 0U; i < left.bitnum(); ++i) {

            /* bitvec[n] = l[n] ^ r[n] ^ comp; */
            res.m_bitvec.push_back((left[i] ^ right[i]) ^ comp);

            /* comp = (l[n] & r[n] & comp) | (!l[n] & (r[n] | comp)); */
            comp = (left[i] & right[i] & comp) | (~left[i] & (right[i] | comp));
        }
        return res;
    }

    Bvec
    Bvec::bvec_mulfixed(int con) const {
        LACE_ME;
        Bvec res;

        if (bitnum() == 0) {
            return res;
        }

        if (con == 0) {
            return bvec_false(bitnum());  /* return false array (base case) */
        }

        Bvec next = bvec_false(bitnum());
        for (size_t i = 1; i < bitnum(); ++i) {
            next[i] = m_bitvec[i - 1];
        }

        Bvec rest = next.bvec_mulfixed(con >> 1);

        if (con & 0x1) {
            res = bvec_add(*this, rest);
        } else {
            res = rest;
        }
        return res;
    }

    Bvec
    Bvec::bvec_mul(const Bvec& left, const Bvec& right) {
        LACE_ME;
        size_t bitnum = left.bitnum() + right.bitnum();
        Bvec res = bvec_false(bitnum);

        if (left.bitnum() == 0 || right.bitnum() == 0) {
            return res;
        }

        Bvec leftshifttmp = Bvec(left);
        Bvec leftshift = leftshifttmp.bvec_coerce(bitnum);

        for (size_t i = 0U; i < right.bitnum(); ++i) {
            Bvec added = bvec_add(res, leftshift);

            for (size_t m = 0U; m < bitnum; ++m) {
                res[m] = right[i].Ite(added[m], res[m]);
            }

            /* Shift 'leftshift' one bit left */
            for (size_t m = bitnum - 1; m >= 1; --m) {
                leftshift[m] = leftshift[m - 1];
            }

            leftshift[0] = Bdd::bddZero();
        }

        return res;
    }

    void
    Bvec::bvec_div_rec(Bvec divisor, Bvec& remainder, Bvec& result, size_t step) {
        LACE_ME;
        Bdd isSmaller = bvec_lte(divisor, remainder);
        Bvec newResult = result.bvec_shlfixed(1, isSmaller);
        Bvec zero = bvec_false(divisor.bitnum());
        Bvec sub = reserve(divisor.bitnum());

        for (size_t i = 0U; i < divisor.bitnum(); ++i) {
            sub.m_bitvec.push_back(isSmaller.Ite(divisor[i], zero[i]));
        }

        Bvec tmp = remainder - sub;
        Bvec newRemainder = tmp.bvec_shlfixed(1, result[divisor.bitnum() - 1]);

        if (step > 1) {
            bvec_div_rec(divisor, newRemainder, newResult, step - 1);
        }

        result = newResult;
        remainder = newRemainder;
    }

    int
    Bvec::bvec_divfixed(int con, Bvec& result, Bvec& rem) {
        LACE_ME;
        if (con < 0) {
            throw std::logic_error("divided by zero");
        }
        Bvec divisor = bvec_con(bitnum(), con);
        Bvec tmp = bvec_false(bitnum());
        Bvec tmpremainder = tmp.bvec_shlfixed(1, m_bitvec[bitnum() - 1]);
        Bvec res = bvec_shlfixed(1, Bdd::bddZero());

        bvec_div_rec(divisor, tmpremainder, result, divisor.bitnum());
        Bvec remainder = tmpremainder.bvec_shrfixed(1, Bdd::bddZero());

        result = res;
        rem = remainder;
        return 0;
    }

    int
    Bvec::bvec_div(const Bvec& left, const Bvec& right, Bvec& result, Bvec& remainder) {
        LACE_ME;
        size_t bitnum = left.bitnum() + right.bitnum();
        if (left.bitnum() == 0 || right.bitnum() == 0) {
            return 1;
        }

        Bvec rem = left.bvec_coerce(bitnum);
        Bvec divtmp = right.bvec_coerce(bitnum);
        Bvec div = divtmp.bvec_shlfixed(left.bitnum(), Bdd::bddZero());

        Bvec res = bvec_false(right.bitnum());

        for (size_t i = 0U; i < right.bitnum() + 1; ++i)    {
            Bdd divLteRem = bvec_lte(div, rem);
            Bvec remSubDiv = bvec_sub(rem, div);

            for (size_t j = 0U; j < bitnum; ++j) {
                rem[j] = divLteRem.Ite(remSubDiv[j], rem[j]);
            }

            if (i > 0) {
                res[right.bitnum() - i] = divLteRem;
            }

            /* Shift 'div' one bit right */
            for (size_t j = 0U; j < bitnum - 1; ++j) {
                div[j] = div[j + 1];
            }
            div[bitnum - 1] = Bdd::bddZero();
        }

        result = res;
        remainder = rem.bvec_coerce(right.bitnum());
        return 0;
    }

    int
    Bvec::bvec_sdiv(const Bvec& left, const Bvec& right, Bvec& res, Bvec& rem) {
        if (left.bitnum() == 0 || right.bitnum() == 0) {
            return 1;
        }
        if (left.bitnum() != right.bitnum()) {
            throw std::logic_error("bitnum of input bitvectors is not equal");
        }
        size_t size = left.bitnum() - 1;
        const Bdd& lhead = left[size];
        const Bdd& rhead = right[size];

        int result = 0;
        if (lhead.isZero() && rhead.isZero()) {
            result = bvec_div(left, right, res, rem);
        } else if (lhead.isOne() && rhead.isZero()) {
            result = bvec_div(arithmetic_neg(left), right, res, rem);
            res = arithmetic_neg(res);
        } else if (lhead.isZero() && rhead.isOne()) {
            result = bvec_div(left, arithmetic_neg(right), res, rem);
            res = arithmetic_neg(res);
        } else {
            result = bvec_div(arithmetic_neg(left), arithmetic_neg(right), res, rem);
        }
        return result;
    }

    Bvec
    Bvec::bvec_ite(const Bdd& val, const Bvec& left, const Bvec& right) {
        LACE_ME;
        Bvec res;
        if (left.bitnum() == 0 || right.bitnum() == 0) {
            return res;
        }

        reserve(res, left.bitnum());
        for (size_t i = 0U; i < left.bitnum(); ++i) {
            res.m_bitvec.push_back(val.Ite(left[i], right[i]));
        }
        return res;
    }

    Bvec
    Bvec::bvec_shlfixed(size_t pos, const Bdd& con) const {
        LACE_ME;
        size_t min = bitnum() < pos ? bitnum() : pos;

        Bvec res(bitnum(), con);
        if (bitnum() == 0) {
            return res;
        }

        for (size_t i = min; i < bitnum(); i++) {
            res[i] = m_bitvec[i - pos];
        }

        return res;
    }

    Bvec
    Bvec::bvec_shl(const Bvec& left, const Bvec& right, const Bdd& con) {
        LACE_ME;
        if (left.bitnum() == 0 || right.bitnum() == 0) {
            return bvec_false(0);
        }
        Bvec res = bvec_false(left.bitnum());


        for ( size_t i = 0U; i <= left.bitnum(); ++i) {
            Bvec val = bvec_con(right.bitnum(), i);
            Bdd rEquN = bvec_equ(right, val);

            for ( size_t j = 0U; j < left.bitnum(); ++j) {
                Bdd tmp1(rEquN);
                /* Set the m'th new location to be the (m+n)'th old location */
                if (static_cast<int>(j - i) >= 0) {
                    tmp1 &= left[j - i];
                } else {
                    tmp1 &= con;
                }
                res[j] |= tmp1;
            }
        }

        /* At last make sure 'c' is shiftet in for r-values > l-bitnum */
        Bvec val = bvec_con( right.bitnum(), left.bitnum());
        Bdd rEquN = bvec_gth(right, val);

        for (size_t i = 0U; i < left.bitnum(); i++) {
            res[i] |= (rEquN & con);
        }
        return res;
    }

    Bvec
    Bvec::bvec_shrfixed(size_t pos, const Bdd& con) const {
        LACE_ME;

        Bvec res(bitnum(), con);
        if (bitnum() == 0) {
            return res;
        }


        size_t maxnum = std::max(static_cast<int>(bitnum() - pos), 0);

        for (size_t i = 0U; i < maxnum; ++i) {
            res[i] = m_bitvec[i + pos];
        }
        return res;
    }

    Bvec
    Bvec::bvec_shr(const Bvec& left, const Bvec& right, const Bdd& con) {
        LACE_ME;
        Bvec res;
        Bdd tmp1, tmp2, rEquN;

        if (left.bitnum() == 0 || right.bitnum() == 0) {
            return res;
        }

        res = bvec_false(left.bitnum());

        for ( size_t i = 0U; i <= left.bitnum(); ++i) {
            Bvec val = bvec_con(right.bitnum(), i);
            rEquN = right == val;

            for (size_t j = 0U; j < left.bitnum(); ++j) {
                /* Set the m'th new location to be the (m+n)'th old location */
                if (j + i <= 2) {
                    tmp1 = rEquN & left[j + i];
                } else {
                    tmp1 = rEquN & con;
                }
                res[j] |= tmp1;
            }
        }

        /* At last make sure 'con' is shiftet in for r-values > l-bitnum */
        Bvec val = bvec_con(right.bitnum(), left.bitnum());
        rEquN = bvec_gth(right, val);
        tmp1 = rEquN & con;

        for ( size_t i = 0U; i < left.bitnum(); ++i) {
            res[i] |= tmp1;
        }
        return res;
    }

    Bdd
    Bvec::bvec_lth(const Bvec& left, const Bvec& right) {
        LACE_ME;
        Bdd p = Bdd::bddZero();
        if (left.bitnum() == 0 || right.bitnum() == 0) {
            return p;
        }

        if (left.bitnum() != right.bitnum()) {
            throw std::logic_error("bitnum of input bitvectors is not equal");
        }

        for (size_t i = 0U; i < left.bitnum(); ++i) {
            /* p = (!l[n] & r[n]) |
             *     bdd_apply(l[n], r[n], bddop_biimp) & p; */
            p = left[i].Ite(Bdd::bddZero(), right[i]) |
                (left[i].Xnor(right[i]) & p);
        }

        return p;
    }

    Bdd
    Bvec::bvec_lte(const Bvec& left, const Bvec& right) {
        LACE_ME;
        Bdd p = Bdd::bddOne();

        if (left.bitnum() == 0 || right.bitnum() == 0) {
            return p;
        }

        if (left.bitnum() != right.bitnum()) {
            throw std::logic_error("bitnum of input bitvectors is not equal");
        }

        for (size_t i = 0U; i < left.bitnum(); ++i) {
            /* p = (!l[n] & r[n]) |
             *     bdd_apply(l[n], r[n], bddop_biimp) & p; */

            p = left[i].Ite(Bdd::bddZero(), right[i]) |
                (left[i].Xnor(right[i]) & p);

        }

        return p;
    }

    Bdd
    Bvec::bvec_gth(const Bvec& left, const Bvec& right) {
        return !bvec_lte(left, right);
    }

    Bdd
    Bvec::bvec_gte(const Bvec& left, const Bvec& right) {
        return !bvec_lth(left, right);
    }

    Bdd
    Bvec::bvec_slth(const Bvec& left, const Bvec& right) {
        if (left.bitnum() == 0 || right.bitnum() == 0) {
            return Bdd::bddZero();
        }

        size_t size = left.bitnum() - 1;

        return get_signs(left[size], right[size]) &
            bvec_lth(left.bvec_coerce(size), right.bvec_coerce(size));
    }

    Bdd
    Bvec::bvec_slte(const Bvec& left, const Bvec& right) {
        if (left.bitnum() == 0 || right.bitnum() == 0) {
            return Bdd::bddOne();
        }

        size_t size = left.bitnum() - 1;

        return get_signs(left[size], right[size]) &
            bvec_lte(left.bvec_coerce(size), right.bvec_coerce(size));
    }

    Bdd
    Bvec::get_signs(const Bdd& left, const Bdd& right) {
        Bdd differentSigns = left.Xnor(Bdd::bddOne()) & right.Xnor(Bdd::bddZero());
        return differentSigns | left.Xnor(right);
    }

    Bdd
    Bvec::bvec_sgth(const Bvec& left, const Bvec& right) {
        return !bvec_slte(left, right);
    }

    Bdd
    Bvec::bvec_sgte(const Bvec& left, const Bvec& right) {
        return !bvec_slth(left, right);
    }

    Bdd
    Bvec::bvec_equ(const Bvec& left, const Bvec& right) {
        LACE_ME;
        Bdd p = Bdd::bddOne();

        if (left.bitnum() == 0 || right.bitnum() == 0) {
            return p;
        }

        if (left.bitnum() != right.bitnum()) {
            throw std::logic_error("bitnum of input bitvectors is not equal");
        }

        for (size_t i = 0U; i < left.bitnum(); ++i) {
            p &= left[i].Xnor(right[i]);
        }

        return p;
    }

    Bdd
    Bvec::bdd_and(const Bdd& first, const Bdd& second) {
        return first & second;
    }

    Bdd
    Bvec::bdd_xor(const Bdd& first, const Bdd& second) {
        return first ^ second;
    }

    Bdd
    Bvec::bdd_or(const Bdd& first, const Bdd& second) {
        return first | second;
    }

    Bdd
    Bvec::bdd_not(const Bdd& src) {
        return !src;
    }

    void
    Bvec::swap(Bvec& other) {
        using std::swap;
        swap(m_bitvec, other.m_bitvec);
    }

    Bvec
    Bvec::reserve(size_t bitnum) {
        Bvec res;
        res.m_bitvec.reserve(bitnum);
        return res;
    }

    void
    Bvec::reserve(Bvec& bitvector, size_t bitnum) {
        bitvector.m_bitvec.reserve(bitnum);
    }
} // sylvan

