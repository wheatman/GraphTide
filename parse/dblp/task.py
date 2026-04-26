import csv
from lxml import etree

typs = {
    "article",
    "inproceedings",
    "proceedings",
    "book",
    "incollection",
    "phdthesis",
    "mastersthesis",
    "www",
}
tags = ["author", "year","title"]

auth = {}
memo = {}

book = {}
momo = {}


def calc(item, features):
    data = {}
    for feature in features:
        data[feature] = []
    for sub in item:
        if sub.tag not in features:
            continue
        text = sub.text
        if text is not None and len(text) > 0:
            data[sub.tag] = data.get(sub.tag) + [text]
    return data


def read(path):
    c = 0
    t = 0
    data = etree.iterparse(source=path, dtd_validation=True, load_dtd=True)
    for i in range(1900, 2030):
        memo[str(i)] = []
        momo[str(i)] = []


    for _, item in data:
        if item.tag not in typs:
            continue
        if item.tag == "article":
            vals = calc(item, tags)
            if not (vals["year"] or vals["author"]):
                continue
            for x in vals["author"]:
                if x in auth:
                    continue
                c += 1
                auth[x] = c
            t += 1
            if vals["title"]:
                book[t] = vals["title"][0]
            else:
                book[t] = "null"

            n = len(vals["author"])
            r = vals["year"][0]
            for i in range(n):
                p = auth[vals["author"][i]]
                momo[r].append((t,p))
                for j in range(i + 1, n):
                    q = auth[vals["author"][j]]
                    memo[r].append((p, q))

        item.clear()
        while item.getprevious() is not None:
            del item.getparent()[0]


def save_coauthor():
    path1 = "authors"
    path2 = "coauthor.edges"
    f = open(path1, "w", newline="", encoding="utf8")
    w = csv.writer(f, delimiter=",")
    w.writerow(["id", "name"])
    for k, v in auth.items():
        w.writerow([v, k])
    f.close()

    f = open(path2, "w", newline="", encoding="utf8")
    w = csv.writer(f, delimiter=" ")
    w.writerow(["source", "dest", "timestamp"])
    for k, item in memo.items():
        for v in item:
            w.writerow([v[0], v[1], k])
    f.close()

def save_bigraph():
    path1 = "article"
    path2 = "article_bigraph.edges"
    f = open(path1, "w", newline="", encoding="utf8")
    w = csv.writer(f, delimiter=",")
    w.writerow(["id", "title"])
    for k, v in book.items():
        w.writerow([k, v])
    f.close()

    f = open(path2, "w", newline="", encoding="utf8")
    w = csv.writer(f, delimiter=" ")
    w.writerow(["source", "dest", "timestamp"]) # source for article, dest for author
    for k, item in momo.items():
        for v in item:
            w.writerow([v[0], v[1], k])
    f.close()

if __name__ == "__main__":
    read("dblp.xml")
    save_coauthor()
    save_bigraph()
