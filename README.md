# Local-MIP
**A standalone local search solver for general mixed integer programming**

**Author: Peng Lin, Mengchuan Zou, and Shaowei Cai\***

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
1. [https://miplib.zib.de/instance_details_sorrell7.html](https://miplib.zib.de/instance_details_sorrell7.html)
2. [https://miplib.zib.de/instance_details_genus-sym-g31-8.html](https://miplib.zib.de/instance_details_genus-sym-g31-8.html)
3. [https://miplib.zib.de/instance_details_supportcase22.html](https://miplib.zib.de/instance_details_supportcase22.html)
4. [https://miplib.zib.de/instance_details_cdc7-4-3-2.html](https://miplib.zib.de/instance_details_cdc7-4-3-2.html)
5. [https://miplib.zib.de/instance_details_genus-sym-g62-2.html](https://miplib.zib.de/instance_details_genus-sym-g62-2.html)
6. [https://miplib.zib.de/instance_details_genus-g61-25.html](https://miplib.zib.de/instance_details_genus-g61-25.html)
7. [https://miplib.zib.de/instance_details_ns1828997.html](https://miplib.zib.de/instance_details_ns1828997.html)
8. [https://miplib.zib.de/instance_details_neos-4232544-orira.html](https://miplib.zib.de/instance_details_neos-4232544-orira.html)
9. [https://miplib.zib.de/instance_details_scpm1.html](https://miplib.zib.de/instance_details_scpm1.html)
10. [https://miplib.zib.de/instance_details_scpn2.html](https://miplib.zib.de/instance_details_scpn2.html)

## News
- [The three common CO problems mps datasets (bpp, jsp, and osp) used in the article have been shared.](https://drive.google.com/drive/folders/1jWVl8gdvmrJD_LbZtFL6aTUh0Vg3P10k?usp=drive_link)
