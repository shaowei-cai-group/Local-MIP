# Local-MIP

Local-MIP is a C++20 local-search solver for mixed integer programming. The solver provides a lightweight CLI, a small C++ API surface, and a set of callback hooks so you can customize starts, restarts, weights, and neighborhood behaviors.

## Version History

**Note:** Local-MIP 1.0 has been archived and is available in `archive/Local-MIP-1.0/`. The experimental results reported in the referenced papers (CP 2024 and Artificial Intelligence 2025) were obtained using Local-MIP 1.0.

## Repository Layout
- `src/` – solver entry (`src/local_mip/Local_MIP.cpp`), CLI wrapper (`src/utils/main.cpp`), search logic in `src/local_search/`, parsing in `src/reader/`, and model utilities in `src/model_data/`.
- `tests/` – unit, integration, and instance-driven tests (CMake/CTest targets).
- `example/` – standalone API examples; buildable independently after preparing headers/static lib.
- `test-set/` – sample `.mps`/`.lp` instances and reference `test.sol`.
- `build/` – generated build artifacts (created by `./build.sh`).
- `python-bindings/` – optional pybind11 bindings (built separately; see Python section).
- `default.set` – default parameter configuration file (see Parameter Configuration section).

## Requirements
- CMake ≥ 3.15
- A C++20 compiler (GCC/Clang) and pthreads
- bash, make, and standard POSIX utilities

## Build the Solver
From the repository root:
```bash
# Release build (recommended)
./build.sh release

# Debug build with assertions/logging
./build.sh debug

# Clean build artifacts
./build.sh clean
```
The solver binary and static library are produced under `build/` (e.g., `build/Local-MIP`, `build/libLocalMIP.a`).

## Run the Solver
Run from `build/` so relative paths resolve:
```bash
cd build
./Local-MIP -i ../test-set/2club200v15p5scn.mps -t 300 -b 1 -l 1
```

### Command Line Parameters
Key flags (see `src/utils/paras.h` for full list):
- `-i` / `--model_file` – input MPS/LP file (required)
- `-t` / `--time_limit` – time limit in seconds
- `-b` / `--bound_strengthen` – bound strengthen level (0-off, 1-ip, 2-mip)
- `-l` / `--log_obj` – enable objective logging (0/1)
- `-s` / `--sol_path` – solution output file path (.sol format)
- `-c` / `--param_set_file` – parameter configuration file (.set format)

### Parameter Configuration File
You can use a configuration file to set parameters instead of (or in addition to) command line arguments. The repository includes `default.set` as a template with all available parameters and their default values.

Example usage:
```bash
cd build
./Local-MIP --param_set_file ../default.set --model_file ../test-set/2club200v15p5scn.mps
```

Configuration file format:
- One parameter per line: `parameter_name = value` or `parameter_name value`
- Lines starting with `#` or `;` are comments
- Command line arguments override values from the configuration file
- See `default.set` for a complete list of parameters with descriptions and valid ranges

## Build & Run Examples
Examples are decoupled from the main tree. Prepare once, then build:
```bash
cd example
./prepare.sh   # copies libLocalMIP.a, headers, and sample test-set files
./build.sh     # builds all demos
```
Notable demo directories:
- `simple-api/` – minimal solver usage
- `start-callback/`, `restart-callback/`, `weight-callback/` – callback hooks
- `scoring-lift/`, `scoring-neighbor/` – custom scoring in feasible/infeasible phases
- `neighbor-config/`, `neighbor-userdata/` – neighbor configuration and custom operators

Each demo binary is emitted inside its directory (e.g., `example/simple-api/simple_api_demo`).

## Tests
CTest targets are defined in `tests/CMakeLists.txt`.
```bash
# Configure (same as normal build)
./build.sh debug   # or release
cd build

# Run unit tests subset
ctest --output-on-failure -R "^(api|callbacks|constraint_recognition|scoring|model_manager|reader|move_operations|neighbor_config)$"

# Run integration tests
ctest --output-on-failure -R "^integration$"

# Run all tests (including per-instance sweeps)
ctest --output-on-failure
```

## Python
Located in `python-bindings/` (separate from the core). Quick start:
```bash
python-bindings/build.sh   # builds core if needed and compiles the pybind11 module
export PYTHONPATH=$PWD/python-bindings/build:$PYTHONPATH
python3 python-bindings/sample.py
```
The module (`localmip_py*.so`) links against the core static library. The sample loads `test-set/2club200v15p5scn.mps`, runs the solver, and writes `py_example.sol`. Callback contexts are passed as opaque capsules; extend `python-bindings/local_mip_py.cpp` if you need field-level access.

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
