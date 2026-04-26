import xml.etree.ElementTree as ET
import re
import sys
from datetime import datetime


def get_links(text):
    links = set()
    for link in re.findall("\[\[.*?\]\]", text):
        link = link[2:-2].split("|")[0].split("#")[0]
        links.add(link)
    return links

def get_changes(links):
    differences = [(links[0][0], links[0][1], set())]
    for i in range(1, len(links)):
        new_links = links[i][1] - links[i-1][1]
        removed_links = links[i-1][1] - links[i][1]
        differences.append((links[i][0], new_links, removed_links))
    return differences

with open("links.el.temp", "a") as f:
    links = []
    for event, elem in ET.iterparse(sys.argv[1][:-4]):
        child = elem
        if elem.tag.endswith("revision"):
            c = elem
            for c1 in c:
                if c1.tag.endswith("timestamp"):
                    revision_time = datetime.strptime(c1.text, "%Y-%m-%dT%H:%M:%SZ")
                if c1.tag.endswith("text"):
                    links_for_revision = set()
                    if c1.text:
                        links_for_revision = get_links(c1.text)
            links.append((revision_time, links_for_revision))
            elem.clear()
        if elem.tag.endswith("page"):
            child = elem
            for c in child:
                # print(ET.tostring(c))
                if c.tag.endswith("title"):
                    Title = c.text
            changes = get_changes(links)
            links = []
            for change in changes:
                timestamp = int(change[0].timestamp())
                for new in change[1]:
                    f.write("+|||"+str(timestamp)+"|||"+Title+ "|||"+ new + "\n")
                for removed in change[2]:
                    f.write("-|||"+str(timestamp)+"|||"+ Title+ "|||"+ removed + "\n")
            elem.clear()
print("done")