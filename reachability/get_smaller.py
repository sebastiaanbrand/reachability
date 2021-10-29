import os
import numpy as np
import pandas as pd
from pathlib import Path

datamap = {}

outputfiles = { ('beem','sl-bdd') : 'models/beem/small_bdd.txt',
                ('ptri','sl-bdd') : 'models/petrinets/small_bdd.txt',
                ('prom','sl-bdd') : 'models/promela/small_bdd.txt',
                ('beem','sl-ldd') : 'models/beem/small_ldd.txt',
                ('ptri','sl-ldd') : 'models/petrinets/small_ldd.txt',
                ('prom','sl-ldd') : 'models/promela/small_ldd.txt',}

verbose = True
def info(str, end='\n'):
    if (verbose):
        print(str, end=end)

def try_load_data(key, filepath):
    global datamap
    if (os.path.exists(filepath)):
        info("loading file {}".format(filepath))
        datamap[key] = pd.read_csv(filepath)
    else:
        return 0

def load_data(data_folder):
    global datamap
    try_load_data(('beem','sl-bdd'), data_folder + 'beem_sloan_stats_bdd.csv')
    try_load_data(('ptri','sl-bdd'), data_folder + 'petrinets_sloan_stats_bdd.csv')
    try_load_data(('prom','sl-bdd'), data_folder + 'promela_sloan_stats_bdd.csv')
    try_load_data(('beem','sl-ldd'), data_folder + 'beem_sloan_stats_ldd.csv')
    try_load_data(('ptri','sl-ldd'), data_folder + 'petrinets_sloan_stats_ldd.csv')
    try_load_data(('prom','sl-ldd'), data_folder + 'promela_sloan_stats_ldd.csv')

def pre_process():
    info("pre-processing data")

    # strip whitespace from column names
    for df in datamap.values():
        df.columns = df.columns.str.strip()

    # make the relevant columns are actually treated as numbers
    for key in datamap.keys():
        datamap[key] = datamap[key].astype(
                        {"benchmark" : str, "strategy" : int, 
                         "merg_rels" : int, "workers" : int,
                         "reach_time" : float, "merge_time" : float,
                         "final_states" : float, "final_nodecount" : int})

def write_smaller_models(maxtime=5):
    
    for ds in ['beem', 'ptri', 'prom']:
        for dl in ['sl-bdd', 'sl-ldd']:
            data = datamap[(ds, dl)]
            data_bfs = data.loc[data['strategy'] == 0]
            data_sat = data.loc[data['strategy'] == 2]
            data_rec = data.loc[data['strategy'] == 4]

            fast_bfs = data_bfs.loc[data_bfs['reach_time'] + data_bfs['merge_time'] < maxtime]
            fast_sat = data_sat.loc[data_sat['reach_time'] + data_sat['merge_time'] < maxtime]
            fast_rec = data_rec.loc[data_rec['reach_time'] + data_rec['merge_time'] < maxtime]
            
            fast_bfs = set(fast_bfs['benchmark'])
            fast_sat = set(fast_sat['benchmark'])
            fast_rec = set(fast_rec['benchmark'])
            fast_all = fast_bfs.intersection(fast_sat.intersection(fast_rec))
            
            info("Writing small instances to {}".format(outputfiles[(ds, dl)]))
            with open(outputfiles[(ds, dl)], 'w') as f:
                for model in fast_all:
                    f.write("{}\n".format(model))            


if __name__ == '__main__':
    load_data('bench_data/old/5_sloan_data/')
    pre_process()
    write_smaller_models()

