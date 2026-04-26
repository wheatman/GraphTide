import csv

auth = {}
memo = {}

book = {}
momo = {}

refs = {}

def calc(data):
    if not data["year"]:
        return
    n = len(data["auth"])
    r = data["year"]
    c = data["index"]
    for i in range(n):
        p = data["auth"][i]
        momo[r].append((c, p))
        for j in range(i + 1, n):
            q = data["auth"][j]
            memo[r].append((p, q))
    for p in data["refs"]:
        refs[r].append((c, p))
    
def read(path):
    for i in range(1900, 2030):
        memo[str(i)] = []
        momo[str(i)] = []
        refs[str(i)] = []
    c = 0
    s = 0
    data = {}
    data["year"] = ""
    with open(path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if line.startswith("#index"):
                calc(data)
                data['index'] = line[6:].strip()
                c = data['index']
                data["auth"] = []
                data["refs"] = []
                data["year"] = ""
                continue
            
            item = line[2:].strip()
            if line.startswith("#*"):
                book[c] = item
            elif line.startswith("#t"):
                data['year'] = item
            elif line.startswith("#%"):
                data['refs'].append(item)
            elif line.startswith("#@"):
                auths = [author.strip() for author in line[len("#@"):].strip().split(';')]
                for x in auths:
                    if x not in auth:
                        s+=1
                        auth[x] = s
                    data["auth"].append(auth[x])
    f.close()


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


def save_refs():
    path = "reference.edges"
    f = open(path, "w", newline="", encoding="utf8")
    w = csv.writer(f, delimiter=" ")
    w.writerow(["source", "dest", "timestamp"])
    for k, item in refs.items():
        for v in item:
            w.writerow([v[0], v[1], k])
    f.close()


if __name__ == "__main__":
    read('AMiner-Paper.txt')

    save_coauthor()
    save_bigraph()
    save_refs()
