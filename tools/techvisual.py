#!/usr/bin/python3

# ************************************************
# *  SMACX Thinker Mod / Tech tree drawing tool  *
# *  https://github.com/induktio/thinker         *
# ************************************************

import random
import argparse
import textwrap
import itertools
import networkx as nx
import matplotlib.pyplot as plt

class Tech():
    def __init__(self, key, name, preq_1, preq_2, color, type, level):
        self.key = key
        self.name = name
        self.preq_1 = preq_1
        self.preq_2 = preq_2
        self.color = color
        self.type = type
        self.level = level

def tech_level(techs, key, level):
    if key == 'None':
        return level
    v1 = tech_level(techs, techs[key].preq_1, level + 1);
    v2 = tech_level(techs, techs[key].preq_2, level + 1);
    return max(v1, v2);

def get_dist(a, b):
    assert(a[1] > b[1])
    dist = abs(a[0]-b[0])
    return dist + dist*dist*0.1

def get_pos(items, key):
    return (items.index(key) - 0.5*len(items), techs[key].level)


parser = argparse.ArgumentParser()
parser.add_argument('inputfile', type=str, default='alphax.txt')
parser.add_argument('-f', '--image-file', type=str, default=None)
parser.add_argument('-s', '--image-size', type=int, default=10,
    help='Set image dimensions, for example 10 equals 1000 px.')
parser.add_argument('-t', '--tech-sort', type=str, default='',
    help='Specify sort order for the first row of techs, comma-separated list of keys.')
args = parser.parse_args()
wrapper = textwrap.TextWrapper(width=16)
random.seed(1234)

with open(args.inputfile, 'r') as f:
    lines = [x.strip() for x in f.readlines()]

pos = {}
techs = {}
labels = {}
techkeys = []
colors = []
nodes = []
edges = []
techlevels = [[] for i in range(90)]


for line in lines[lines.index('#TECHNOLOGY')+1:]:
    if not line or line[0] == '#':
        break
    name, key, Vc, Vd, Vb, Ve, key_preq_1, key_preq_2, flags =\
        [x.strip() for x in line.split(',')[:9]]
    Vmax = max(Vc, Vd, Vb, Ve)
    if key in ('delete', 'deleted', 'User') or key_preq_1 == 'Disable' or key_preq_2 == 'Disable':
        continue
    if Vc == Vmax:
        color = '#f66'
        T = 'C'
    elif Vd == Vmax:
        color = '#ccc'
        T = 'D'
    elif Vb == Vmax:
        color = '#fd0'
        T = 'B'
    else:
        color = '#6f6'
        T = 'E'
    techkeys.append(key)
    colors.append(color)
    techs[key] = Tech(key, name, key_preq_1, key_preq_2, color, T, None)

for key in techkeys:
    tech = techs[key]
    level = tech_level(techs, key, 0)
    techs[key].level = level
    techlevels[level].append(key)
    labels[key] = '\n'.join(wrapper.wrap('%s%d %s' % (techs[key].type, level, techs[key].name)))

    if tech.preq_1 != 'None':
        edges.append((tech.preq_1, key))
    if tech.preq_2 != 'None':
        edges.append((tech.preq_2, key))

if args.tech_sort:
    items = args.tech_sort.split(',')
    for item in techlevels[1]:
        if item not in items:
            items.append(item)
    techlevels[1] = items

for i in range(2, len(techlevels)):
    best_val = 1e9
    if not techlevels[i]:
        continue
    if len(techlevels[i]) > 7:
        A = [random.sample(techlevels[i], k=len(techlevels[i])) for x in range(2000)]
    else:
        A = [k for k in itertools.permutations(techlevels[i])]

    for j in range(len(A)):
        C = A[j]
        val = 0
        for k in range(len(C)):
            p1 = get_pos(C, C[k])
            p2 = get_pos(techlevels[techs[techs[C[k]].preq_1].level], techs[C[k]].preq_1)
            val += get_dist(p1, p2)
            if techs[C[k]].preq_2 != 'None':
                p3 = get_pos(techlevels[techs[techs[C[k]].preq_2].level], techs[C[k]].preq_2)
                val += get_dist(p1, p3)
        if val < best_val:
            best_val = val
            techlevels[i] = C

for key in techs.keys():
    pos[key] = get_pos(techlevels[techs[key].level], key)


plt.figure(figsize=(args.image_size, args.image_size))
plt.axis('off')
G=nx.DiGraph()
G.add_edges_from(edges)
nx.draw_networkx_nodes(G, pos, nodelist=techs.keys(), node_color=colors, node_size=400)
nx.draw_networkx_edges(G, pos, edgelist=edges, edge_color='#aaa', arrows=True, width=1.5)
nx.draw_networkx_labels(G, pos, labels=labels, font_size=8)
plt.tight_layout()

if args.image_file:
    plt.savefig(args.image_file, dpi=100)
else:
    plt.show()



