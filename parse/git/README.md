# Repos
This is designed to make a bipartite graph from any git repo.
The graph has users on one side and files on the other.  Each commit is a set of edges at the time of the commit if the user modified the file.

Some examples or large open source repos would be 
- https://github.com/torvalds/linux
- https://github.com/llvm/llvm-project


## directions
`git clone` any repo then call `parse_git_repo.py` witht the path to the git repo and the path to the ourput file location.