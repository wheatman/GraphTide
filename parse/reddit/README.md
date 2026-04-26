# Reddit Graph

This graph is a bipartite graph with users on one side and subreddits on the other.  Each edge is a comment from a user on a particular subreddit.  

The data was originally downloaded from `https://files.pushshift.io/reddit/comments/` but the site is now down.
That means this code is probably not very useful anymore.

`data_get.py` was to help download and uncompress the datafiles.
`parse_data.py` was to help parse these files and turn them into the bipartite graph stored in binary form.
