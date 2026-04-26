# Wikipedia

This dataset is a hyperlink graph of wikipedia.  Each edge is a link from one page on wikipedia to another.  Edges can be added or removed.

## Source
The data can be downloaded from https://dumps.wikimedia.org/

You need the files of pages-meta-history.  These will include the set of all edits done to every page to give the graph its time information.

The dumps are only updated every month or so.  And not every dump includes the full history.  So if the most recent dump is still in progress, or does not include the history you can follow the links to find the most recent dump which is complete which contains the history files.


## Directions
There is a lot of processing to do here so be prepared for this to take while.  The files are stored in xml format with the entire text of each edit to every page.  

The code is currently set up to run on the slurm cluster, but with some extra commands, so things might not work out of the box.  Also, there are some full paths that will need to be updated.  For know view this as more of a guide, then a working pipeline.

`files.txt` is a text document which contains the full location of all the data.  You will want to update this to get the most recent dump. 

The starting point is `download_and_parse.sh`  This file loops over files in files.txt and uses wget to download the file.

This launches `unzip_and_parse.sh` which first unzips the files using `bunzip2` an then calls `xml_parse.py`

`xml_parse.py` parses each xml file, determines the set of new and removed links and writes these out to `links.el.temp`

Once this is done `unzip_and_parse.sh` renames the files to mark that it successfully completed, this way if run again it knows it finished successfully.

These links files will include <+/-> timestamp, source title, dest title.

These link files can then all be combined using `combine_links.py` which will output the real graph file in binary, along with a second file node_mappings.csv which include the page to node id mapping. 

