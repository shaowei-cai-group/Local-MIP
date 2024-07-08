# Local-MIP
**A standalone local search solver for general mixed integer programming**

**Author: Peng Lin, Mengchuan Zou, Shaowei Cai**

## Run
```
cd bin
chmod a+x Local-MIP
./Local-MIP --instance=<instance.mps> --cutoff=<cutoff>
```
or
```
mkdir build
cd code
chmod a+x run
./run --instance=<instance.mps> --cutoff=<cutoff>
```
1. `<instance.mps>`: path for the mip instance in mps format
2. `<cutoff>`: seconds to run


## Reference
If you use Local-MIP in an academic context, please acknowledge this and cite the following article.

Peng Lin, Mengchuan Zou, and Shaowei Cai. An Efficient Local Search Solver for Mixed Integer Programming. In Proceedings of CP 2024.

## New Records for Open Instances
1. https://miplib.zib.de/instance_details_genus-sym-g31-8.html
2. https://miplib.zib.de/instance_details_genus-sym-g62-2.html
3. https://miplib.zib.de/instance_details_genus-g61-25.html
4. https://miplib.zib.de/instance_details_neos-4232544-orira.html