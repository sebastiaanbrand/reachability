#include "bitvec.h"


Bdd& Bitvec::operator[](size_t i) {
        return bitvec[i];
    }

const Bdd& Bitvec::operator[](size_t i) const {
        return bitvec[i];
    }

Bitvec::Bitvec(size_t bitnum, const Bdd& value) : bitvec(bitnum, value) {}
size_t Bitvec::bitnum() const {
    return bitvec.size();
}

Bdd Bitvec::combineBdd(){
    Bdd res = Bdd::bddOne();
    for(auto i : bitvec){
        res &= i;
    }
    return res;
}

Bdd Bitvec::bitvecXnor(const Bitvec& left){
    Bdd res = Bdd::bddOne();
    Bitvec right;
    right.bitvec = this->bitvec;

    if(left.bitnum() == 0 || right.bitnum() == 0){
        return res;
    }
    if(left.bitnum() != right.bitnum()){
        throw std::logic_error("bitnum of input bitvectors is not equal");
    }

    for(size_t i = 0; i < left.bitnum(); i++){
        res &= left[i].Xnor(bitvec[i]);
    }

    return res;
}

// No error handling now. Assuming left and right have the same size.
adder_result Bitvec::bitvec_add(const Bitvec& left, const Bitvec& right){
    LACE_ME;
    Bitvec re;
    Bdd carry = Bdd::bddZero();
    int count = 0;
    adder_result res;

    re.bitvec.reserve(left.bitnum());

    if(left.bitnum() == 0 || right.bitnum() == 0){
        return res;
    }

    if(left.bitnum() != right.bitnum()){
        throw std::logic_error("bitnum of input bitvectors is not equal");
    }

    while (count < int(left.bitnum())){
    // one bit adder.
        re.bitvec.push_back(left[count] ^ right[count] ^ carry);
        carry = ((left[count] | right[count]) & carry) | (left[count] & right[count]);
        count++;
    }

    res.sum = re;
    res.carry = carry;

    return res;
}

Bitvec Bitvec::bitvecIte(const Bdd& val, const Bitvec& left, const Bitvec& right){
    LACE_ME;
    Bitvec res;
    if (left.bitnum() == 0 || right.bitnum() == 0) {
        return res;
    }

    res.bitvec.reserve(left.bitnum());
    for (size_t i = 0; i < left.bitnum(); ++i) {
        res.bitvec.push_back(val.Ite(left[i], right[i]));
    }
    return res;
}