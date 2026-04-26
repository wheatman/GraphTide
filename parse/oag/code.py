import json
import sys

vens = "ven.edges"
orgs = "org.edges"
refs = "ref.edges"
coas = "coa.edges"
pubs = "pub.edges"
info = "infos.txt"


def calc(p, q, r):
    return f"{p} {q} {r}\n"


def stas(file, cp, ca, co, cf, st, et):
    with open(info, "a") as f:
        f.write(f"======infos for {file} ======== \n")
        # f.write(f"num of publications: {cp} \n")
        # f.write(f"num of authors: {ca} (with duplicate) \n")
        f.write(f"num of coauthor edges: {co} \n")
        f.write(f"num of ref edges: {cf} \n")
        f.write(f"start year: {st} \n")
        f.write(f"end year: {et} \n\n")


def work(file):
    cp, ca, co, cf = 0, 0, 0, 0
    st, et = 9999, 0
    print("parsing file " + file + " wating: ")
    with open(file, "r") as f, open(vens, "a") as ven, open(orgs, "a") as org, open(
        refs, "a"
    ) as ref, open(coas, "a") as coa, open(pubs, "a") as pub:
        for line in f:
            if cp%100000==0:
                print("=", end="")         
            data = json.loads(line)
            p = data["id"]
            t = data["year"]
            if not p or not t:
                continue
            cp += 1
            st = min(st, t)
            et = max(et, t)
            v = data["venue_id"]
            if v:
                ven.write(calc(p, v, t))
            a = data["authors"]
            n = len(a)
            for i in range(n):
                ai = a[i]["id"]
                if not ai:
                    continue
                ca += 1
                ao = a[i]["org_id"]
                if ao:
                    org.write(calc(ai, ao, t))
                pub.write(calc(p, ai, t))
                for j in range(i + 1, n):
                    aj = a[j]["id"]
                    if not aj:
                        continue
                    co += 1
                    coa.write(calc(ai, aj, t))
            r = data["references"]
            for q in r:
                if not q:
                    continue
                cf += 1
                ref.write(calc(p, q, t))
    stas(file, cp, ca, co, cf, st, et)

work(f"v5_oag_publication_{sys.argv[1]}.json")
