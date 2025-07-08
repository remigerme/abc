# Experiences from "Parallel Combinational Equivalence Checking"

## Commands

For circuits (except `sixteen`, `twenty`, `twentythree`):

```shell
export CIRCUIT_NAME=path_to_circuit_without_dot_aig_ext
```

**Creating circuits**:

```shell
./abc -c "r $CIRCUIT_NAME.aig; logic; double; double; double; double; double; double; double; double; double; double; strash; write_aiger "$CIRCUIT_NAME"_10x.aig;"
```

**Optimizing circuits**:

```shell
./abc -c "&r "$CIRCUIT_NAME"_10x.aig; &dc2; &w "$CIRCUIT_NAME"_10x_dc2.aig;"
```

**Running equivalence checking**:

```shell
./abc -c "&r "$CIRCUIT_NAME"_10x.aig; &cec "$CIRCUIT_NAME"_10x_dc2.aig;"
```

## Reference

V. N. Possani, A. Mishchenko, R. P. Ribas and A. I. Reis, "Parallel Combinational Equivalence Checking," in IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems, vol. 39, no. 10, pp. 3081-3092, Oct. 2020, doi: 10.1109/TCAD.2019.2946254
