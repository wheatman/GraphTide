# GraphTide
This repo is to aid in the collection and analysis of dynamic graph datasets.

It is broken into a few different subdirectories.

## analyze
This directory helps to analyze a graph once it has been parsed. It is designed to run on a dynamic graph and determine statistics of the graph.

For build and run instructions, see `analyze/README.md`.



## parse
This directory contains code to help download and parse different dynamic graphs.

It is broken up by data source.

## Graph Types
This repo contains dynamic graphs of two different types.  

### Signal Graphs 
The first, and more common, is what we will call a signal graph.  These graphs each edge is a signal of a connection.  For example, consider a graph of emails.  Each email tells a connection between the sender and receiver, but the email is only an instant in time.  In these graphs their are no edge removals, and how long an edge should last is left up to the user and so removals are not in the underlying datat.

### Update Graphs
These are graphs of actual things. Where the graph is an exact graph of something and edges can be both added and removed explicitly.  For example in a road graph, roads will be opened and closed at exact times.

## File format

A few different file formats are used.

### Signal Graphs 

The basic files are stored as *.edges This will be a white space separated file with each row simply contained source, dest, and a timestamp.  If the file is *weighted.edges then each row also has a 4th field which is the weight.

Some large files are stored with a .bin, in which the three fields are replaced with the binary form with each of the source, dest and timestamp being replaced by 5 bytes.  These can be much smaller and much faster to parse in parallel.


### Update Graphs
Update graphs need an additional field which specifies if it's an addition or a deletion.  This field is at the beginning of the elements about each edge. 