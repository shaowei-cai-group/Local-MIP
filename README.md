# Local-MIP

[Local-MIP](https://local-mip.github.io/) is a C++20 local-search solver for mixed integer programming. It ships with a lightweight CLI plus a focused C++ API and callback hooks to customize starts, restarts, weights, scoring funtions, and neighborhood behavior.

Find out more about Local-MIP at [https://local-mip.github.io/](https://local-mip.github.io/).

Although Local-MIP is freely available under the MIT license, we would be pleased to learn about users’ experiences and offer advice via email at [peng.lin.csor@gmail.com](peng.lin.csor@gmail.com).

## Version History

**Note:** Local-MIP 1.0 has been archived and is available in `archive/Local-MIP-1.0/`. The experimental results reported in the referenced papers (CP 2024 and Artificial Intelligence 2025) were obtained using Local-MIP 1.0.

## Repository Layout
- `src/` – solver entry (`src/local_mip/Local_MIP.cpp`), CLI wrapper (`src/utils/main.cpp`), search logic (`src/local_search/`), parsing (`src/reader/`), and model utilities (`src/model_data/`).
- `tests/` – unit, integration, and instance-driven tests (CMake/CTest).
- `example/` – standalone API examples; buildable independently after preparing headers/static lib.
- `test-set/` – sample `.mps/.lp` instances.
- `build/` – generated build artifacts (created by `./build.sh`).
- `python-bindings/` – optional pybind11 bindings (built separately).
- `default.set` – parameter configuration template.

## Requirements
- CMake ≥ 3.15
- A C++20 compiler (GCC/Clang)
- bash, make, and standard POSIX utilities

---

## Path 1: Use the CLI Solver

### Build
From the repository root:
```bash
# Release build (recommended)
./build.sh release

# Debug build with assertions/logging
./build.sh debug

# Build everything (core + examples + python bindings)
./build.sh all

# Clean build artifacts
./build.sh clean
```
The solver binary and static library are written to `build/` (e.g., `build/Local-MIP`, `build/libLocalMIP.a`). `./build.sh all` additionally prepares/builds the `example/` demos and the pybind11 module under `python-bindings/build/`.

### Run
Run from `build/` so relative paths resolve:
```bash
cd build
./Local-MIP -i ../test-set/2club200v15p5scn.mps -t 300 -b 1 -l 1
```

### Command Line Parameters
Run `./Local-MIP --help` (from `build/`) to see available CLI flags. The `default.set` template lists every parameter with its default value and brief notes; it can be used as-is or customized.

### MPS Support
The reader now handles `RANGES` sections (SOS data is still rejected with an error).

### Parameter Configuration File
You can use a configuration file to set parameters instead of (or in addition to) command line arguments. The repository includes `default.set` as a template with all available parameters and their default values.

Example:
```bash
cd build
./Local-MIP --param_set_file ../default.set --model_file ../test-set/2club200v15p5scn.mps
```

Format rules:
- One parameter per line: `parameter_name = value` 
- Lines starting with `#` or `;` are comments
- Command line arguments override values from the configuration file
- See `default.set` for descriptions and valid ranges

### Tests
CTest targets are defined in `tests/CMakeLists.txt`.
```bash
./build.sh debug   # or release
cd build

# Run unit test subset
ctest --output-on-failure -R "^(api|callbacks|constraint_recognition|scoring|model_manager|reader|move_operations|neighbor_config)$"

# Run integration tests
ctest --output-on-failure -R "^integration$"

# Run all tests (including per-instance sweeps)
ctest --output-on-failure
```

---

## Path 2: Use as a Library

### C++ static library
- The core build produces `build/libLocalMIP.a`; headers live under `src/`.
- Link against the static library directly or follow the example projects under `example/`.

### Callbacks (customize the solver)
Local-MIP exposes multiple callback hooks (start, restart, weight, neighbor generation, neighbor scoring, lift scoring). Predefined demos live under `example/` (e.g., `start-callback/`, `restart-callback/`, `weight-callback/`, `neighbor-config/`, `neighbor-userdata/`, `scoring-neighbor/`, `scoring-lift/`). Refer to the example READMEs and callback type declarations in `src/local_search/` for signatures and available context fields.

### Example projects (C++ API)
Examples are decoupled from the main tree. Prepare once, then build (or run `./build.sh all` to do this automatically):
```bash
cd example
./prepare.sh   # copies libLocalMIP.a, headers, and sample test-set files into example/
./build.sh     # builds all demos
```
Notable demo directories:
- `simple-api/` – minimal solver usage
- `start-callback/`, `restart-callback/`, `weight-callback/` – callback hooks
- `scoring-lift/`, `scoring-neighbor/` – custom scoring in feasible/infeasible phases
- `neighbor-config/`, `neighbor-userdata/` – neighbor configuration and custom operators

Each demo binary is emitted inside its directory; run it from that directory so relative paths resolve to `../test-set`, e.g.:
```bash
cd example/simple-api
./simple_api_demo
```

### Python bindings (pybind11)
Located in `python-bindings/` (separate from the core). Quick start:
```bash
python-bindings/build.sh   # builds core if needed and compiles the pybind11 module
export PYTHONPATH=$PWD/python-bindings/build:$PYTHONPATH
python3 python-bindings/sample.py
```
The module (`localmip_py*.so`) links against the core static library. The sample loads `test-set/2club200v15p5scn.mps`, runs the solver, and writes `py_example.sol`. Callback contexts are passed as opaque capsules; extend `python-bindings/local_mip_py.cpp` if you need field-level access.

---

## Development Notes
- Code style: C++20, two-space indentation, local headers before system headers, prefer `printf`-family.
- Keep CLI parameters in sync with `src/utils/paras.h` and help text.

## Reference
If you use **Local-MIP** in an academic context, please cite the following articles:

**Important:** The experimental results reported in the papers below were obtained using Local-MIP 1.0, which has been archived in `archive/Local-MIP-1.0/`.

1. **Journal Version (Artificial Intelligence, 2025)**  
   Peng Lin, Shaowei Cai, Mengchuan Zou, Jinkun Lin,  
   *Local-MIP: Efficient local search for mixed integer programming*,  
   Artificial Intelligence, Volume 348, 2025, 104405.  
   [doi.org/10.1016/j.artint.2025.104405](https://doi.org/10.1016/j.artint.2025.104405)

2. **Conference Version (CP 2024, Best Paper Award)**  
   Peng Lin, Mengchuan Zou, and Shaowei Cai.  
   *An Efficient Local Search Solver for Mixed Integer Programming.*
   In *Proceedings of the 30th International Conference on Principles and Practice of Constraint Programming (CP 2024)*.  
   [doi.org/10.4230/LIPIcs.CP.2024.19](https://doi.org/10.4230/LIPIcs.CP.2024.19).
---

## BibTeX
```bibtex
@article{LIN2025104405,
title = {Local-MIP: Efficient local search for mixed integer programming},
journal = {Artificial Intelligence},
volume = {348},
pages = {104405},
year = {2025},
issn = {0004-3702},
doi = {https://doi.org/10.1016/j.artint.2025.104405},
url = {https://www.sciencedirect.com/science/article/pii/S0004370225001249},
author = {Peng Lin and Shaowei Cai and Mengchuan Zou and Jinkun Lin},
}

@InProceedings{lin_et_al:LIPIcs.CP.2024.19,
  author =	{Lin, Peng and Zou, Mengchuan and Cai, Shaowei},
  title =	{{An Efficient Local Search Solver for Mixed Integer Programming}},
  booktitle =	{30th International Conference on Principles and Practice of Constraint Programming (CP 2024)},
  pages =	{19:1--19:19},
  series =	{Leibniz International Proceedings in Informatics (LIPIcs)},
  ISBN =	{978-3-95977-336-2},
  ISSN =	{1868-8969},
  year =	{2024},
  volume =	{307},
  URL =		{https://drops.dagstuhl.de/entities/document/10.4230/LIPIcs.CP.2024.19},
  doi =		{10.4230/LIPIcs.CP.2024.19},
}
```

---

## New Records for Open Instances
Local-MIP has set new records for several benchmark instances in the MIPLIB dataset, including:

1. [sorrell7](https://miplib.zib.de/instance_details_sorrell7.html)  
2. [genus-sym-g31-8](https://miplib.zib.de/instance_details_genus-sym-g31-8.html)  
3. [supportcase22](https://miplib.zib.de/instance_details_supportcase22.html)  
4. [cdc7-4-3-2](https://miplib.zib.de/instance_details_cdc7-4-3-2.html)  
5. [genus-sym-g62-2](https://miplib.zib.de/instance_details_genus-sym-g62-2.html)  
6. [genus-g61-25](https://miplib.zib.de/instance_details_genus-g61-25.html)  
7. [ns1828997](https://miplib.zib.de/instance_details_ns1828997.html)  
8. [neos-4232544-orira](https://miplib.zib.de/instance_details_neos-4232544-orira.html)  
9. [scpm1](https://miplib.zib.de/instance_details_scpm1.html)  
10. [scpn2](https://miplib.zib.de/instance_details_scpn2.html)  
