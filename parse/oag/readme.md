# Open Academic Graph Dataset

This is the dataset from Aminer Open Academic Graph(OAG).

Descriptions can be found here https://open.aminer.cn/open/article?id=5965cf249ed5db41ed4f52bf

This script will use the OAG v3.2 dataset, which can be found here https://open.aminer.cn/open/article?id=67aaf63af4cbd12984b6a5f0

To Download and parse the data



```
bash run.sh
```

### dataset size

Each v5_oag_publication_i.json will be around 15 GB.

After we parse each subdataset, we will append around 12 GB data into coa.edges (coauthor graph) and around 3.5 GB into ref.edges (reference graph). We have 16 subset, in the end we will get over 256GB data in total.

<!-- According to the dataset description, the dataset have 152,525,231 papers and 34,891,863 researchers.

And in the first subdataset, we have 218539292 coauthor edges and 70311887 ref edges. -->

Size of the datasets.


| graph | node num | edge num | file size |
|---|---|---|---|
| coauthor graph | 34,891,863 | 3032478219| 155G |
| ref graph | 152,525,231 | 1138706757 | 58G |
| pub bipartite graph | ≈186,000,000 | almost same as node | 19G |
| org bipartite graph | ≈34,000,000 | almost same as node | 9G |
| ven bipartite graph |  |  | 3G |



### gen files

 _(All the id in this dataset is base 16 hex num)_

 _(The timestamp for the dynamic graph is yearly)_

By run the script, we can get 6 files,

which is 

| filename | format | description |
|---|---|---|
| coa.edges| author1_id author2_id year | dynamic bigraph for  coauthor relations |
| ref.edges |current_id source_id year | dynamic graph for reference relations |
| pub.edges| publication_id author_id year  | bipartite graph for publication-author relations |
| ven.edges| publication_id venue_id year  |bipartite graph for publication-venue mapping |
| org.edges | author_id org_id year  | bipartite graph for author-org mapping |
| infos.txt | |edges count info for each subset of the data |
