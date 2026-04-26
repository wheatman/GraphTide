import bz2
from datetime import datetime

filenames = [
    "RC_2005.edges.bz2",
    "RC_2006.edges.bz2",
    "RC_2007.edges.bz2",
    "RC_2008.edges.bz2",
    "RC_2009.edges.bz2",
    "RC_2010.edges.bz2",
    "RC_2011.edges.bz2",
    "RC_2012.edges.bz2",
    "RC_2013.edges.bz2",
    "RC_2014.edges.bz2",
    "RC_2015.edges.bz2",
    "RC_2016.edges.bz2",
    "RC_2017.edges.bz2",
    "RC_2018.edges.bz2",
    "RC_2019.edges.bz2",
    "RC_2020.edges.bz2",
    "RC_2021_1-6.edges.bz2",
]


def print_stats(d, buckets=10):
    degrees = [(v[1], k) for k, v in d.items()]
    degrees.sort()
    print("top 5 ", degrees[-5:][::-1])
    if len(degrees) < buckets:
        print(degrees)
    else:
        items_to_check = [int(i*len(degrees))
                          for i in [.01, .1, .25, .5, .75, .9, .99]]
        for i in items_to_check:
            print(degrees[i][0], end=", ")
        print("")
    return


edge_count = 0
user_count = 0
sub_count = 0
node_count = 0

users = {}

subs = {}

with open("edges.bin", "wb") as out_file:
    for fname in filenames:
        with bz2.open(fname) as f:
            for line in f:
                line = line.decode("utf-8")
                info = line.split(",")
                user = info[0]
                if user == "[deleted]" or user == "AutoModerator":
                    continue
                sub = info[1]
                timestamp = int(info[2])
                edge_count += 1
                if user not in users:
                    user_count += 1
                    users[user] = [node_count, 1]
                    node_count += 1
                else:
                    users[user][1] += 1
                user_id = users[user][0]
                if sub not in subs:
                    sub_count += 1
                    subs[sub] = [node_count, 1]
                    node_count += 1
                else:
                    subs[sub][1] += 1
                sub_id = subs[sub][0]
                out_file.write(user_id.to_bytes(4, "little"))
                out_file.write(sub_id.to_bytes(4, "little"))
                out_file.write(timestamp.to_bytes(4, "little"))
                if edge_count % 100000000 == 0:
                    print(datetime.utcfromtimestamp(
                        timestamp).strftime('%Y-%m-%d %H:%M:%S'))
                    print("edge count = ", edge_count)
                    print("node count = ", node_count)
                    print("user count = ", user_count)
                    print_stats(users)
                    print("sub count = ", sub_count)
                    print_stats(subs)
                    print("")
        print("done file ", fname)
        print(datetime.utcfromtimestamp(
            timestamp).strftime('%Y-%m-%d %H:%M:%S'))
        print("edge count = ", edge_count)
        print("node count = ", node_count)
        print("user count = ", user_count)
        print_stats(users)
        print("sub count = ", sub_count)
        print_stats(subs)
        print("")
with open("node_mapping.csv", "w") as f:
    f.write("node name, node id\n")
    for sub, id in subs.items():
        f.write(sub+","+str(id[0])+"\n")
    for user, id in users.items():
        f.write(user+","+str(id[0])+"\n")
