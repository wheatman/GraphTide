# BTC dataset from zenodo

## source
https://zenodo.org/records/12581515



## directions
Download and unpack the data from the link above.  Run `dump_to_el.py`  This has the relative paths hardcoded in.  If this fails check that the hard coded paths have not changed.  

This outputs two files, the first is `btc_stream.edges`, the second is `btc_stream_weighted.edges` and it includes the weights of the edges which are the values of the transactions in satoshi.