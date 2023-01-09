# Reachability from CNF input

Reachability can be computed from CNF inputs. This expects three separate [DIMACS](https://jix.github.io/varisat/manual/0.2.0/formats/dimacs.html) `.cnf` files as input:
```shell
./reach_algs/build/fromCNF s.cnf r.cnf t.cnf
```
Where `s.cnf` encodes the initial state(s), `r.cnf` encodes the transition relation, and `t.cnf` encodes the target state(s).
For a state-space of _n_ variables, it is assumed that
* `s.cnf` and `t.cnf` are CNF formulas over odd variables: `{1, 3, ..., 2n-1}`
* `r.cnf` is a CNF formula over odd and even variables `{1, 3, ..., 2n+1} U {2, 4, ..., 2n}` where the odd variables correspond to the source states, and the even variables are "primed copies" of the source variables and correspond to the target states of the transitions.
