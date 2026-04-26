import sys
from datetime import datetime



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
page_count = 0

pages = {}

remove_pages_prefix = ["User:", "Wikipedia:", "WP:", "Project:", "Image:", "image:", "File:", "file:", "MediaWiki:", "Template:", "Help:", "Category:", "Portal:", "Draft:", "TimedText:", "Module:", "Talk:", "User Talk:",  "User talk:", "Template talk:", "Wikipedia talk:", "Portal talk:" ]

with open("wiki_links2.bin", "wb") as out_file:
    for i, fname in enumerate(sys.argv[1:]):
        print("working on file", i, "out of", len(sys.argv))
        with open(fname, encoding='utf-8') as f:
            for line in f:
                info = line.strip().split("|||")
                if info[0] == "+":
                    added_edge = 1
                else:
                    added_edge = 0
                timestamp = int(info[1])
                source = info[2]
                dest = info[3]
                if source == "":
                    continue
                if dest == "":
                    continue
                skip = False
                for prefix in remove_pages_prefix:
                    if source.startswith(prefix):
                        skip = True
                    if dest.startswith(prefix):
                        skip = True
                if skip:
                    continue

                edge_count += 1
                for page in [source, dest]:
                    if page not in pages:
                        page_count += 1
                        pages[page] = [page_count, 1]
                        page_count += 1
                    else:
                        pages[page][1] += 1
                    page_id = pages[page][0]
                out_file.write(added_edge.to_bytes(4, "little"))
                out_file.write(pages[source][0].to_bytes(4, "little"))
                out_file.write(pages[dest][0].to_bytes(4, "little"))
                out_file.write(timestamp.to_bytes(4, "little"))
                if edge_count % 100000000 == 0:
                    print("edge count = ", edge_count)
                    print("page count = ", page_count)
                    print("")
        print("done file ", fname)
        print("edge count = ", edge_count)
        print("page count = ", page_count)
        print_stats(pages)
        print("")

with open("node_mapping.csv", "w") as f:
    f.write("node name, node id, degree\n")
    for page, id in sorted(pages.items(), key = lambda item: item[1][1], reverse=True):
        f.write(page+","+str(id[0])+","+str(id[1])+"\n")
