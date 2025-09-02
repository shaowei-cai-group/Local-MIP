# Local-MIP
**A standalone local search solver for general mixed integer programming**

---

## Run
```bash
cd bin
chmod a+x Local-MIP
./Local-MIP --instance=<instance.mps> --cutoff=<cutoff>
```
or
```bash
mkdir build
cd code
chmod a+x run
./run --instance=<instance.mps> --cutoff=<cutoff>
```

- `<instance.mps>`: path to the MIP instance in `.mps` format  
- `<cutoff>`: runtime limit in seconds  

---

## Reference
If you use **Local-MIP** in an academic context, please cite the following articles:

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

---

## News
- [The benchmark datasets (BPP, JSP, and OSP) used in the article have been shared here.](https://drive.google.com/drive/folders/1jWVl8gdvmrJD_LbZtFL6aTUh0Vg3P10k?usp=drive_link)
