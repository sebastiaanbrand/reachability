import os
import numpy as np
import pandas as pd
from pathlib import Path
import matplotlib.pyplot as plt
import scipy
from scipy import stats


fig_formats = ['png', 'pdf', 'eps']
plots_folder_temp = 'plots/{}/{}/' # output in plots/subfolder/fig_format/
label_folder_temp = 'plots/{}/labeled/' # for plots with labels for all data-points
plots_folder = ''
label_folder = ''

datamap = {} # map from ('dataset','type') -> dataframe
matrix_folders = {  ('beem','vn-bdd') : 'models/beem/matrices/bdds/vanilla/', 
                    ('beem','sl-bdd') : 'models/beem/matrices/bdds/sloan/',
                    ('ptri','vn-bdd') : 'models/petrinets/matrices/bdds/vanilla/',
                    ('ptri','sl-bdd') : 'models/petrinets/matrices/bdds/sloan/',
                    ('prom','vn-bdd') : 'models/promela/matrices/bdds/vanilla/',
                    ('prom','sl-bdd') : 'models/promela/matrices/bdds/sloan/',
                    ('beem','vn-ldd') : 'models/beem/matrices/ldds/vanilla/', 
                    ('beem','sl-ldd') : 'models/beem/matrices/ldds/sloan/',
                    ('ptri','vn-ldd') : 'models/petrinets/matrices/ldds/vanilla/',
                    ('ptri','sl-ldd') : 'models/petrinets/matrices/ldds/sloan/',
                    ('prom','vn-ldd') : 'models/promela/matrices/ldds/vanilla/',
                    ('prom','sl-ldd') : 'models/promela/matrices/ldds/sloan/',}

datasetnames = ['beem', 'ptri', 'prom']  # ['beem, 'ptri', 'prom']
legend_names = {'beem' : 'dve', 'ptri' : 'petrinets', 'prom' : 'promela'}
markers = {'beem': 's', 'ptri' : '^', 'prom' : 'x'}
marker_size = {'beem': 8, 'ptri': 10, 'prom': 15}
stratIDs     = {'bfs' : 0,
                'sat' : 2,
                'rec' : 4,
                'rec-par' : 14}
axis_label = {('bfs','bdd') : 'BFS',
              ('sat','bdd') : 'Saturation',
              ('rec','bdd') : 'Algorithm 1',
              ('bfs','ldd') : 'BFS',
              ('sat','ldd') : 'Saturation',
              ('rec','ldd') : 'Algorithm 3'}

metric_labels={'var-avg-bw':'average relative bandwidth', 
               'var-max-bw':'maximum relative bandwidth',
               'var-density':'matrix density',
               'rel-avg-bw':'average relative bandwidth', 
               'rel-max-bw':'maximum relative bandwidth',
               'rel-density':'matrix density'}

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
    # make sure datamap is empty
    global datamap
    datamap = {} 

    # BDD data
    try_load_data(('beem','vn-bdd'), data_folder + 'beem_vanilla_stats_bdd.csv')
    try_load_data(('beem','sl-bdd'), data_folder + 'beem_sloan_stats_bdd.csv')
    try_load_data(('ptri','vn-bdd'), data_folder + 'petrinets_vanilla_stats_bdd.csv')
    try_load_data(('ptri','sl-bdd'), data_folder + 'petrinets_sloan_stats_bdd.csv')
    try_load_data(('prom','vn-bdd'), data_folder + 'promela_vanilla_stats_bdd.csv')
    try_load_data(('prom','sl-bdd'), data_folder + 'promela_sloan_stats_bdd.csv')

    # LDD data
    try_load_data(('beem','vn-ldd'), data_folder + 'beem_vanilla_stats_ldd.csv')
    try_load_data(('beem','sl-ldd'), data_folder + 'beem_sloan_stats_ldd.csv')
    try_load_data(('ptri','vn-ldd'), data_folder + 'petrinets_vanilla_stats_ldd.csv')
    try_load_data(('ptri','sl-ldd'), data_folder + 'petrinets_sloan_stats_ldd.csv')
    try_load_data(('prom','vn-ldd'), data_folder + 'promela_vanilla_stats_ldd.csv')
    try_load_data(('prom','sl-ldd'), data_folder + 'promela_sloan_stats_ldd.csv')


def pre_process():
    print("pre-processing data")

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


def assert_states_nodes():
    print("checking the state- and nodecounts")
    n_states = {} # dict : benchmark -> number of final states
    for df in datamap.values():
        n_nodes  = {} # dict : benchmark -> number of final nodes
        for _, row in df.iterrows():
            # check final_states
            if (row['benchmark'] not in n_states):
                n_states[row['benchmark']] = row['final_states']
            else:
                if (n_states[row['benchmark']] != row['final_states']):
                    print("final_states not equal for {} ({} != {})".format(
                          row['benchmark'],
                          n_states[row['benchmark']],
                          row['final_states']
                    ))
            # check final nodes
            if (row['benchmark'] not in n_nodes):
                n_nodes[row['benchmark']] = row['final_nodecount']
            else:
                if (n_nodes[row['benchmark']] != row['final_nodecount']):
                    print("final_nodecount not equal for {} ({} != {})".format(
                          row['benchmark'],
                          n_nodes[row['benchmark']],
                          row['final_nodecount']
                    ))


def load_matrix(filepath):
    #info("reading {}".format(filepath))
    file = open(filepath, 'r')
    matrix = []
    max_length = 0
    for line in file:
        row = []
        for char in line:
            if (char == '+' or char == 'r' or char == 'w'):
                row.append(1)
            elif (char == '-'):
                row.append(0)
        matrix.append(row)
        max_length = max(max_length, len(row))
    # for LDDs this can produce ragged lists, so extend with 0's
    for i, row in enumerate(matrix):
        row.extend([0] * (max_length - len(row)) )
    return np.array(matrix)


def create_rel_vs_rel(var_matrix):
    n = len(var_matrix)
    rel_matrix = np.zeros((n,n), dtype=int)
    for i in range(n):
        for j in range(n):
            shared = np.logical_and(var_matrix[i], var_matrix[j])
            rel_matrix[i,j] = (np.count_nonzero(shared) != 0)
    return rel_matrix


def get_bandwidth(matrix):
    max_bw = 0
    sum_bw = 0
    for row in matrix:
        non_zeros = np.nonzero(row)[0]
        if (len(non_zeros > 1)):
            bw = non_zeros[-1] - non_zeros[0] + 1
        else:
            bw = 1
        max_bw = max(max_bw, bw)
        sum_bw += bw
    return (max_bw, sum_bw / len(matrix))


def process_matrix(filepath, return_metric):
    # Matrix of vars vs rels
    var_matrix = load_matrix(filepath)
    var_density = np.sum(var_matrix) / np.size(var_matrix)
    var_max_bw, var_avg_bw = get_bandwidth(var_matrix)
    if (return_metric == 'var-avg-bw'):
        return var_avg_bw / len(var_matrix[0]) # normalize by matrix width
    elif (return_metric == 'var-max-bw'):
        return var_max_bw / len(var_matrix[0]) # normalize by matrix width
    elif (return_metric == 'var-density'):
        return var_density
    
    # Matrix of rels vs rels
    rel_matrix = create_rel_vs_rel(var_matrix)
    rel_density = np.sum(rel_matrix) / np.size(rel_matrix)
    rel_max_bw, rel_avg_bw = get_bandwidth(rel_matrix)
    if (return_metric == 'rel-avg-bw'):
        return rel_avg_bw / len(rel_matrix[0]) # normalize by matrix width
    elif (return_metric == 'rel-max-bw'):
        return rel_max_bw / len(rel_matrix[0]) # normalize by matrix width
    elif (return_metric == 'rel-density'):
        return rel_density


def get_matrix_metrics(bench_names, data_label, metric):
    # for all the benchmarks in bench_names, look up matrix info in folder
    # which corresponds to data_label = ('dataset','ordering-ddtype')
    metrics = np.zeros((len(bench_names)))
    for i, bench_name in enumerate(bench_names):
        filepath = matrix_folders[data_label] + bench_name + ".matrix"
        metrics[i] = process_matrix(filepath, metric)
    return metrics


def plot_comparison(x_strat, x_data_label, y_strat, y_data_label):
    info("plotting {} ({}) vs {} ({})".format(x_strat, x_data_label, 
                                              y_strat, y_data_label))

    scaling = 5.0 # default = ~6.0
    fig, ax = plt.subplots(figsize=(scaling, scaling*0.75))
    point_size = 8.0

    max_val = 0
    min_val = 1e9
    all_xs = []
    all_ys = []
    all_names = [] # track for annotations
    for ds_name in datasetnames:
        # get the relevant data
        x_data = datamap[(ds_name, x_data_label)]
        y_data = datamap[(ds_name, y_data_label)]

        # select subsets of x and y data
        group_x = x_data.loc[x_data['strategy'] == stratIDs[x_strat]]
        group_y = y_data.loc[y_data['strategy'] == stratIDs[y_strat]]

        # inner join x and y
        group_y = group_y.set_index('benchmark')
        joined = group_x.join(group_y, on='benchmark', how='inner', 
                                    lsuffix='_x', rsuffix='_y')

        # plot reachability time of x vs y
        xs = joined['reach_time_x'].to_numpy()
        ys = joined['reach_time_y'].to_numpy()
        ax.scatter(xs, ys, s=point_size, label=legend_names[ds_name])

        # track for annotations
        all_xs.extend(xs)
        all_ys.extend(ys)
        all_names.extend(joined['benchmark'])

        # max and min for diagonal line
        max_val = max(max_val, np.max(xs), np.max(ys))
        min_val = min(min_val, np.min(xs), np.min(ys))

    # diagonal line
    ax.plot([min_val, max_val], [min_val, max_val], ls="--", c="gray")

    # labels and formatting
    ax.set_xlabel('{} time (s)'.format(axis_label[(x_strat,x_data_label[-3:],)]))
    ax.set_ylabel('{} time (s)'.format(axis_label[(y_strat,y_data_label[-3:],)]))
    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlim([min_val-0.15*min_val, max_val+0.15*max_val])
    ax.set_ylim([min_val-0.15*min_val, max_val+0.15*max_val])
    ax.legend(framealpha=1.0)
    plt.tight_layout()

    # plots without data-point lables
    for fig_format in fig_formats:
        subfolder = plots_folder.format(fig_format)
        fig_name = '{}reachtime_{}_{}_vs_{}_{}.{}'.format(subfolder,
                                                          x_strat, x_data_label,
                                                          y_strat, y_data_label,
                                                          fig_format)
        fig.savefig(fig_name, dpi=300)

    # add data-point labes and plot as pdf
    for i, bench_name in enumerate(all_names):
        ax.annotate(bench_name, (all_xs[i], all_ys[i]), fontsize=1.0)
    fig_name = '{}reachtime_{}_{}_vs_{}_{}.{}'.format(label_folder,
                                                      x_strat, x_data_label,
                                                      y_strat, y_data_label,
                                                      'pdf')
    fig.savefig(fig_name, dpi=300)
    plt.close(fig)


def plot_comparison_shared_y(x1_strat, x1_data_label, 
                             x2_strat, x2_data_label, 
                             y_strat,  y_data_label):
    info("plotting x1 = {} ({}), x2 = {} ({}) vs y = {} ({})".format(
          x1_strat, x1_data_label,
          x2_strat, x2_data_label,
          y_strat,  y_data_label))
    
    scaling = 4.8 # default = ~6.0
    w = 1.55 # relative width
    fig, axs = plt.subplots(1, 2, sharey=True, figsize=(w*scaling, scaling*0.75))
    point_size = 8.0

    max_val = 0
    min_val = 1e9
    for ds_name in datasetnames:
        # get the relevant data
        x1_data = datamap[(ds_name, x1_data_label)]
        x2_data = datamap[(ds_name, x2_data_label)]
        y_data  = datamap[(ds_name, y_data_label)]

        # select subsets of x and y data
        group_x1 = x1_data.loc[x1_data['strategy'] == stratIDs[x1_strat]]
        group_x2 = x2_data.loc[x2_data['strategy'] == stratIDs[x2_strat]]
        group_y  = y_data.loc[y_data['strategy'] == stratIDs[y_strat]]

        # inner join x and y
        group_y = group_y.set_index('benchmark')
        joined1 = group_x1.join(group_y, on='benchmark', how='inner', 
                                    lsuffix='_x', rsuffix='_y')
        joined2 = group_x2.join(group_y, on='benchmark', how='inner', 
                                    lsuffix='_x', rsuffix='_y')
        
        # plot reachability time of x1 vs y and x2 vs y
        x1s = joined1['reach_time_x'].to_numpy()
        y1s = joined1['reach_time_y'].to_numpy()
        axs[0].scatter(x1s, y1s, s=point_size, label=legend_names[ds_name])
        x2s = joined2['reach_time_x'].to_numpy()
        y2s = joined2['reach_time_y'].to_numpy()
        axs[1].scatter(x2s, y2s, s=point_size, label=legend_names[ds_name])

        
        # max and min for diagonal lines
        max_val = max(max_val, np.max(x1s), np.max(y1s), np.max(x2s), np.max(y2s))
        min_val = min(min_val, np.min(x1s), np.min(y1s), np.min(x2s), np.min(y2s))

    # diagonal line
    axs[0].plot([min_val, max_val], [min_val, max_val], ls="--", c="gray")
    axs[1].plot([min_val, max_val], [min_val, max_val], ls="--", c="gray")

    # labels and formatting
    axs[0].set_xlabel('{} time (s)'.format(axis_label[(x1_strat,x1_data_label[-3:])]))
    axs[1].set_xlabel('{} time (s)'.format(axis_label[(x2_strat,x2_data_label[-3:])]))
    axs[0].set_ylabel('{} time (s)'.format(axis_label[(y_strat, y_data_label[-3:])]))
    axs[0].set_xscale('log')
    axs[1].set_xscale('log')
    axs[0].set_yscale('log')
    axs[0].set_xlim([min_val-0.15*min_val, max_val+0.15*max_val])
    axs[1].set_xlim([min_val-0.15*min_val, max_val+0.15*max_val])
    axs[0].set_ylim([min_val-0.15*min_val, max_val+0.15*max_val])
    axs[1].axes.yaxis.set_visible(False)
    axs[0].legend(framealpha=1.0)
    plt.tight_layout()

    # plot for all formats
    for fig_format in fig_formats:
        subfolder = plots_folder.format(fig_format)
        fig_name = '{}reachtime_{}_{}_and_{}_{}_vs_{}_{}.{}'.format(subfolder,
                                                        x1_strat, x1_data_label,
                                                        x2_strat, x2_data_label,
                                                        y_strat, y_data_label,
                                                        fig_format)
        fig.savefig(fig_name, dpi=300)
    plt.close(fig)


def plot_comparison_shared_x_y(x11_strat, x11_data_label, 
                               x12_strat, x12_data_label,
                               y1_strat,  y1_data_label,
                               x21_strat, x21_data_label,
                               x22_strat, x22_data_label,
                               y2_strat,  y2_data_label):
    info("plotting x11 = {} ({}), x12 = {} ({}) vs y1 = {} ({}) \n"
         "     and x21 = {} ({}), x22 = {} ({}) vs y2 = {} ({})".format(
          x11_strat, x11_data_label,
          x12_strat, x12_data_label,
          y1_strat,  y1_data_label,
          x21_strat, x21_data_label,
          x22_strat, x22_data_label,
          y2_strat,  y2_data_label))

    scaling = 7.5 # default = ~6.0
    w = 0.95 # relative width
    fig, axs = plt.subplots(2, 2, sharey='all', sharex='all', figsize=(w*scaling, scaling*0.75))

    max_val = 0
    min_val = 1e9
    for ds_name in datasetnames:
        # get the relevant data
        x11_data = datamap[(ds_name, x11_data_label)]
        x12_data = datamap[(ds_name, x12_data_label)]
        y1_data  = datamap[(ds_name, y1_data_label)]
        x21_data = datamap[(ds_name, x21_data_label)]
        x22_data = datamap[(ds_name, x22_data_label)]
        y2_data  = datamap[(ds_name, y2_data_label)]

        # select subsets of x and y data
        group_x11 = x11_data.loc[x11_data['strategy'] == stratIDs[x11_strat]]
        group_x12 = x12_data.loc[x12_data['strategy'] == stratIDs[x12_strat]]
        group_y1  = y1_data.loc[y1_data['strategy'] == stratIDs[y1_strat]]
        group_x21 = x21_data.loc[x21_data['strategy'] == stratIDs[x21_strat]]
        group_x22 = x22_data.loc[x22_data['strategy'] == stratIDs[x22_strat]]
        group_y2  = y2_data.loc[y2_data['strategy'] == stratIDs[y2_strat]]

        # inner join x and y
        group_y1 = group_y1.set_index('benchmark')
        joined11 = group_x11.join(group_y1, on='benchmark', how='inner', 
                                    lsuffix='_x', rsuffix='_y')
        joined12 = group_x12.join(group_y1, on='benchmark', how='inner', 
                                    lsuffix='_x', rsuffix='_y')
        group_y2 = group_y2.set_index('benchmark')
        joined21 = group_x21.join(group_y2, on='benchmark', how='inner', 
                                    lsuffix='_x', rsuffix='_y')
        joined22 = group_x22.join(group_y2, on='benchmark', how='inner', 
                                    lsuffix='_x', rsuffix='_y')

        # some styling
        s = marker_size[ds_name]
        m = markers[ds_name]
        l = legend_names[ds_name]
        
        # plot reachability times
        x11s = joined11['reach_time_x'].to_numpy()
        y11s = joined11['reach_time_y'].to_numpy()
        axs[0,0].scatter(x11s, y11s, s=s, marker=m, label=l)
        x12s = joined12['reach_time_x'].to_numpy()
        y12s = joined12['reach_time_y'].to_numpy()
        axs[0,1].scatter(x12s, y12s, s=s, marker=m, label=l)
        x21s = joined21['reach_time_x'].to_numpy()
        y21s = joined21['reach_time_y'].to_numpy()
        axs[1,0].scatter(x21s, y21s, s=s, marker=m, label=l)
        x22s = joined22['reach_time_x'].to_numpy()
        y22s = joined22['reach_time_y'].to_numpy()
        axs[1,1].scatter(x22s, y22s, s=s, marker=m, label=l)

        # max and min for diagonal lines
        max_val = max(max_val, np.max(x11s), np.max(y11s), np.max(x12s), np.max(y12s))
        min_val = min(min_val, np.min(x11s), np.min(y11s), np.min(x12s), np.min(y12s))
        max_val = max(max_val, np.max(x21s), np.max(y21s), np.max(x22s), np.max(y22s))
        min_val = min(min_val, np.min(x21s), np.min(y21s), np.min(x22s), np.min(y22s))

    # diagonal lines
    axs[0,0].plot([min_val, max_val], [min_val, max_val], ls="--", c="#5d5d5d")
    axs[0,1].plot([min_val, max_val], [min_val, max_val], ls="--", c="#5d5d5d")
    axs[1,0].plot([min_val, max_val], [min_val, max_val], ls="--", c="#5d5d5d")
    axs[1,1].plot([min_val, max_val], [min_val, max_val], ls="--", c="#5d5d5d")

    # * 10 line
    axs[0,0].plot([min_val, max_val], [min_val*10, max_val*10], ls=":", c="#767676")
    axs[0,1].plot([min_val, max_val], [min_val*10, max_val*10], ls=":", c="#767676")
    axs[1,0].plot([min_val, max_val], [min_val*10, max_val*10], ls=":", c="#767676")
    axs[1,1].plot([min_val, max_val], [min_val*10, max_val*10], ls=":", c="#767676")

    # / 10 line
    axs[0,0].plot([min_val, max_val], [min_val/10, max_val/10], ls=":", c="#767676")
    axs[0,1].plot([min_val, max_val], [min_val/10, max_val/10], ls=":", c="#767676")
    axs[1,0].plot([min_val, max_val], [min_val/10, max_val/10], ls=":", c="#767676")
    axs[1,1].plot([min_val, max_val], [min_val/10, max_val/10], ls=":", c="#767676")

    # labels and formatting
    axs[1,0].set_xlabel('{} time (s)'.format(axis_label[(x11_strat,x11_data_label[-3:])]))
    axs[1,1].set_xlabel('{} time (s)'.format(axis_label[(x12_strat,x12_data_label[-3:])]))
    axs[0,0].set_ylabel('{} time (s)'.format(axis_label[(y1_strat, y1_data_label[-3:])]))
    axs[1,0].set_ylabel('{} time (s)'.format(axis_label[(y2_strat, y2_data_label[-3:])]))
    axs[0,0].set_xscale('log')
    axs[0,1].set_xscale('log')
    axs[0,0].set_yscale('log')
    axs[1,0].set_xscale('log')
    axs[1,1].set_xscale('log')
    axs[1,0].set_yscale('log')
    axs[0,0].set_xlim([min_val-0.15*min_val, max_val+0.15*max_val])
    axs[0,1].set_xlim([min_val-0.15*min_val, max_val+0.15*max_val])
    axs[0,0].set_ylim([min_val-0.15*min_val, max_val+0.15*max_val])
    axs[1,0].set_xlim([min_val-0.15*min_val, max_val+0.15*max_val])
    axs[1,1].set_xlim([min_val-0.15*min_val, max_val+0.15*max_val])
    axs[1,0].set_ylim([min_val-0.15*min_val, max_val+0.15*max_val])
    axs[0,1].axes.yaxis.set_visible(False)
    axs[0,0].legend(framealpha=1.0)
    plt.tight_layout()

    # plot for all formats
    for fig_format in fig_formats:
        subfolder = plots_folder.format(fig_format)
        fig_name = '{}reachtime_{}_{}_{}_{}_{}_{}_{}_{}_{}_{}_{}_{}.{}'.format(subfolder,
                    x11_strat, x11_data_label, x12_strat, x12_data_label, y1_strat, y1_data_label,
                    x21_strat, x21_data_label, x22_strat, x22_data_label, y2_strat, y2_data_label,
                    fig_format)
        fig.savefig(fig_name, dpi=300)
    plt.close(fig)


def plot_comparison_sbs(x1_strat, x1_data_label, 
                        y1_strat, y1_data_label,
                        x2_strat, x2_data_label, 
                        y2_strat, y2_data_label,
                        x_label,  y_label,
                        add_merge_time=False):
    info("plotting")
    info(" left:  {} ({}) vs {} ({})".format(x1_strat, x1_data_label, 
                                            y1_strat, y1_data_label))
    info(" right: {} ({}) vs {} ({})".format(x2_strat, x2_data_label, 
                                            y2_strat, y2_data_label))

    scaling = 4.8 # default = ~6.0
    w = 1.5 # relative width
    fig, axs = plt.subplots(1, 2, sharey=True, figsize=(w*scaling, scaling*0.75))
    point_size = 8.0

    max_val = 0
    min_val = 1e9
    for ds_name in datasetnames:
        # get the relevant data
        x1_data = datamap[(ds_name, x1_data_label)]
        y1_data = datamap[(ds_name, y1_data_label)]
        x2_data = datamap[(ds_name, x2_data_label)]
        y2_data = datamap[(ds_name, y2_data_label)]

        # select subsets of x and y data
        group_x1 = x1_data.loc[x1_data['strategy'] == stratIDs[x1_strat]]
        group_y1 = y1_data.loc[y1_data['strategy'] == stratIDs[y1_strat]]
        group_x2 = x2_data.loc[x2_data['strategy'] == stratIDs[x2_strat]]
        group_y2 = y2_data.loc[y2_data['strategy'] == stratIDs[y2_strat]]

        # inner join x and y
        group_y1 = group_y1.set_index('benchmark')
        group_y2 = group_y2.set_index('benchmark')
        joined1 = group_x1.join(group_y1, on='benchmark', how='inner', 
                                    lsuffix='_x', rsuffix='_y')
        joined2 = group_x2.join(group_y2, on='benchmark', how='inner', 
                                    lsuffix='_x', rsuffix='_y')
        
        # some styling
        s = marker_size[ds_name]
        m = markers[ds_name]
        l = legend_names[ds_name]
        
        # plot reachability time of x1 vs y and x2 vs y
        x1s = joined1['reach_time_x'].to_numpy()
        y1s = joined1['reach_time_y'].to_numpy()
        axs[0].scatter(x1s, y1s, s=s, marker=m, label=l)
        x2s = joined2['reach_time_x'].to_numpy()
        y2s = joined2['reach_time_y'].to_numpy()
        axs[1].scatter(x2s, y2s, s=s, marker=m, label=l)

        if (add_merge_time):
            x1s += joined1['merge_time_x'].to_numpy()
            y1s += joined1['merge_time_y'].to_numpy()
            x2s += joined2['merge_time_x'].to_numpy()
            y2s += joined2['merge_time_y'].to_numpy()
        
        # max and min for diagonal lines
        max_val = max(max_val, np.max(x1s), np.max(y1s), np.max(x2s), np.max(y2s))
        min_val = min(min_val, np.min(x1s), np.min(y1s), np.min(x2s), np.min(y2s))

    # diagonal line
    axs[0].plot([min_val, max_val], [min_val, max_val], ls="--", c="#5d5d5d")
    axs[1].plot([min_val, max_val], [min_val, max_val], ls="--", c="#5d5d5d")

    # * 10 line
    axs[0].plot([min_val, max_val], [min_val*10, max_val*10], ls=":", c="#767676")
    axs[1].plot([min_val, max_val], [min_val*10, max_val*10], ls=":", c="#767676")

    # / 10 line
    axs[0].plot([min_val, max_val], [min_val/10, max_val/10], ls=":", c="#767676")
    axs[1].plot([min_val, max_val], [min_val/10, max_val/10], ls=":", c="#767676")

    # labels and formatting
    #axs[0].set_xlabel('{}'.format(x1_data_label[-3:].upper()))
    #axs[1].set_xlabel('{}'.format(x2_data_label[-3:].upper()))
    axs[0].set_title('BDDs')
    axs[1].set_title('LDDs')
    fig.text(0.54, 0.01, x_label, ha='center')
    #fig.text(0.54, 0.1, x_label, ha='center')
    axs[0].set_ylabel(y_label)
    axs[0].set_xscale('log')
    axs[1].set_xscale('log')
    axs[0].set_yscale('log')
    axs[0].set_xlim([min_val-0.15*min_val, max_val+0.15*max_val])
    axs[1].set_xlim([min_val-0.15*min_val, max_val+0.15*max_val])
    axs[0].set_ylim([min_val-0.15*min_val, max_val+0.15*max_val])
    axs[1].axes.yaxis.set_visible(False)
    axs[0].legend(framealpha=1.0)
    plt.tight_layout()
    #fig.subplots_adjust(bottom=0.22)
    fig.subplots_adjust(bottom=0.15)

    # plot for all formats
    for fig_format in fig_formats:
        subfolder = plots_folder.format(fig_format)
        fig_name = '{}reachtime_{}_{}_vs_{}_{}_and_{}_{}vs_{}_{}_incmergetime{}.{}'.format(
                                                        subfolder,
                                                        x1_strat, x1_data_label,
                                                        y1_strat, y1_data_label,
                                                        x2_strat, x2_data_label,
                                                        y2_strat, y2_data_label,
                                                        add_merge_time,
                                                        fig_format)
        fig.savefig(fig_name, dpi=300)
    plt.close(fig)


# TODO: add merge time
def plot_rec_over_sat_vs_rel_metric(data_label, metric):
    info("plotting rec/sat against {} on {}".format(metric, data_label), end='')
    if (metric[:3] == 'rel'):
        info(" (might take some time)")
    
    scaling = 5.0 # default = ~6.0
    fig, ax = plt.subplots(figsize=(scaling, scaling*0.75))
    point_size = 8.0

    max_val = 0
    min_val = 1e9
    all_xs = []
    all_ys = []
    all_names = [] # track for annotations
    for ds_name in datasetnames:
        # get the relevant data
        data = datamap[(ds_name), data_label]

        # select sat and rec of y data
        data_sat = data.loc[data['strategy'] == stratIDs['sat']]
        data_rec = data.loc[data['strategy'] == stratIDs['rec']]

        # inner join sat and rec results
        data_rec = data_rec.set_index('benchmark')
        joined   = data_sat.join(data_rec, on='benchmark', how='inner', 
                                 lsuffix='_sat', rsuffix='_rec')

        # get relative time of rec (this will be the y value in the plot)
        sat_times = joined['reach_time_sat'].to_numpy()
        rec_times = joined['reach_time_rec'].to_numpy()
        relative_rec_times = rec_times / sat_times

        # get relation matrix metric of each of the datapoints (x value)
        matrix_metrics = get_matrix_metrics(joined['benchmark'], 
                                            (ds_name, data_label), metric)

        # some styling
        s = marker_size[ds_name]
        m = markers[ds_name]
        l = legend_names[ds_name]

        # plot x vs y
        ax.scatter(matrix_metrics, relative_rec_times, marker=m, s=s, label=l)

        # track for annotations (and trend line)
        all_xs.extend(matrix_metrics)
        all_ys.extend(relative_rec_times)
        all_names.extend(joined['benchmark'])

    # labels and formatting
    ax.hlines(1, 0, 1, colors=['grey'], linestyles='--')
    ax.set_xlabel(metric_labels[metric])
    if (data_label[-3:] == 'bdd'):
        ax.set_ylabel('time ReachBDD / saturation')
    elif (data_label[-3:] == 'ldd'):
        ax.set_ylabel('time ReachMDD / saturation')
    ax.set_yscale('log')
    ax.legend(framealpha=1.0)
    plt.tight_layout()

    # plots without data-point lables
    for fig_format in fig_formats:
        subfolder = plots_folder.format(fig_format)
        fig_name = '{}rec_over_sat_vs_metric_{}_on_{}.{}'.format(subfolder, 
                                                metric, data_label, fig_format)
        fig.savefig(fig_name, dpi=300)
    
    # add trendline
    a, b, r_val, p_val, std_err = scipy.stats.linregress(all_xs, all_ys)
    p = np.poly1d([a, b])
    x = np.linspace(min(all_xs), max(all_xs), num=100)
    plt.plot(x, p(x), color='red', linestyle='--', label='trend ($r={}$)'.format(round(r_val,2)))
    ax.legend(framealpha=1.0)

    # plots with trendline
    for fig_format in fig_formats:
        subfolder = plots_folder.format(fig_format)
        fig_name = '{}rec_over_sat_vs_metric_{}_on_{}_trend.{}'.format(subfolder, 
                                                metric, data_label, fig_format)
        fig.savefig(fig_name, dpi=300)

    # add data-point labes and plot as pdf
    for i, bench_name in enumerate(all_names):
        ax.annotate(bench_name, (all_xs[i], all_ys[i]), fontsize=1.0)
    fig_name = '{}rec_over_sat_vs_metric_{}_on_{}.{}'.format(label_folder,
                                                      metric, data_label,'pdf')
    fig.savefig(fig_name, dpi=300)
    plt.close(fig)


def plot_parallel(strat1, strat2, strat3, data_label, min_time):
    info("plotting parallel speedups {}, {}, {} on {} (mintime = {}s)".format(
        strat1, strat2, strat3, data_label, min_time))
    
    scaling = 5.0 # default = ~6.0
    fig, ax = plt.subplots(figsize=(scaling*0.85, scaling*0.75)) # *0.70
    point_size = 8.0

    all_xs = []
    all_ys = []
    all_names = [] # track for annotations

    speedups1 = {2 : [], 4 : [], 8 : [], 16 : [], 32 : [], 64 : [], 96 : []}
    speedups2 = {2 : [], 4 : [], 8 : [], 16 : [], 32 : [], 64 : [], 96 : []}
    speedups3 = {2 : [], 4 : [], 8 : [], 16 : [], 32 : [], 64 : [], 96 : []}

    for ds_name in datasetnames:
        # get the relevant data (only where reach_time is at least 'min_time')
        data = datamap[(ds_name, data_label)]
        data = data.loc[data['reach_time'] >= min_time]

        # select subsets of data for each strat
        group1 = data.loc[data['strategy'] == stratIDs[strat1]]
        group2 = data.loc[data['strategy'] == stratIDs[strat2]]
        group3 = data.loc[data['strategy'] == stratIDs[strat3]]

        # get numer number of workers
        num_workers = np.unique(data['workers'])
        assert(num_workers[0] == 1)

        # for group1, 2 and 3, get data for 1 workers as ref
        group1_w1 = group1.loc[data['workers'] == 1].set_index('benchmark')
        group2_w1 = group2.loc[data['workers'] == 1].set_index('benchmark')
        group3_w1 = group3.loc[data['workers'] == 1].set_index('benchmark')

        for w in num_workers[1:]:
            group1_ww = group1.loc[data['workers'] == w]
            group2_ww = group2.loc[data['workers'] == w]
            group3_ww = group3.loc[data['workers'] == w]

            # inner join on 'benchmark' of groupj_w1 with groupj_ww
            joined1 = group1_ww.join(group1_w1, on='benchmark', how='inner', lsuffix='_ww', rsuffix='_w1')
            joined2 = group2_ww.join(group2_w1, on='benchmark', how='inner', lsuffix='_ww', rsuffix='_w1')
            joined3 = group3_ww.join(group3_w1, on='benchmark', how='inner', lsuffix='_ww', rsuffix='_w1')

            # compute speedups relative to 1 worker
            speedups1[w].extend(joined1['reach_time_w1'].to_numpy() / joined1['reach_time_ww'].to_numpy())
            speedups2[w].extend(joined2['reach_time_w1'].to_numpy() / joined2['reach_time_ww'].to_numpy())
            speedups3[w].extend(joined3['reach_time_w1'].to_numpy() / joined3['reach_time_ww'].to_numpy())

    # put speedups in boxplot (little bit of a hack with the position)
    all_data = []
    labels = []
    positions = []
    ticks = []
    colors = []
    i = 1
    for key in speedups1.keys():
        if (len(speedups1[key]) > 0):
            all_data.append(speedups1[key])
            all_data.append(speedups2[key])
            all_data.append(speedups3[key])
            labels.extend(['', key, ''])
            colors.extend(['pink','lightblue','lightgreen'])
            positions.extend([i, i+1, i+2])
            ticks.append(i+1)
            i += 5 # keep a blank spot (or two)
    bplot = ax.boxplot(all_data, patch_artist=True, labels=labels, 
                       positions=positions, #showfliers=False, 
                       medianprops={'color' : 'black'})

    for patch, color in zip(bplot['boxes'], colors):
        patch.set_facecolor(color)

    # labels and formatting
    ax.hlines(1, min(positions), max(positions), colors=['grey'], linestyles='--')
    ax.set_xlabel('number of cores\n (96 core machine)')
    ax.set_ylabel('speedup')
    ax.tick_params(axis='x', which='both',length=0)
    ax.legend([bplot["boxes"][0], bplot["boxes"][1], bplot["boxes"][2]], 
              ['Parallel saturation', 'ReachableBDD', 'ReachableBDD-par'],
              loc='upper left', framealpha=1.0)
    plt.tight_layout()

     # plots without data-point lables
    for fig_format in fig_formats:
        subfolder = plots_folder.format(fig_format)
        fig_name = '{}speedups_{}_{}_{}_on{}_mintime_{}_96_cores.{}'.format(subfolder, 
                    strat1, strat2, strat3, data_label, min_time, fig_format)
        fig.savefig(fig_name, dpi=300)
    plt.close(fig)


def plot_96_core_plot():
    pass


def set_subfolder_name(subfolder_name):
    global plots_folder, plots_folder_temp, label_folder, label_folder_temp
    plots_folder = plots_folder_temp.format(subfolder_name, '{}')
    label_folder = label_folder_temp.format(subfolder_name)

    # make plots folders if necessary
    for fig_format in fig_formats:
        Path(plots_folder.format(fig_format)).mkdir(parents=True, exist_ok=True)
    Path(label_folder).mkdir(parents=True, exist_ok=True)


def plot_paper_plot_sat_vs_rec():
    set_subfolder_name('Saturation vs REACH (Figure 9)')
    plot_comparison_sbs('sat', 'sl-bdd', 'rec', 'sl-bdd', 
                        'sat', 'sl-ldd', 'rec', 'sl-ldd', 
                        'Saturation time (s)', 'ReachableBDD/MDD time (s)', 
                        add_merge_time=True)


def plot_paper_plot_locality():
    set_subfolder_name('Locality metric comparison (Figure 10)')
    plot_rec_over_sat_vs_rel_metric('sl-ldd', 'rel-avg-bw')


def plot_paper_plot_parallel():
    set_subfolder_name('Parallel speedups (Figure 11)')
    plot_parallel('sat', 'rec', 'rec-par', 'sl-bdd', 1)


def plot_paper_plots():
    # Plot saturation vs REACH on Sloan BDDs/LDDs (Figure 9)
    load_data('bench_data/old/5_sloan_data/')
    pre_process()
    assert_states_nodes()
    plot_paper_plot_sat_vs_rec()

    # Plot locality metric correlation (Figure 10) (on same data)
    plot_paper_plot_locality()

    # Plot parallel
    load_data('bench_data/old/6_parallel_data/')
    pre_process()
    assert_states_nodes()
    plot_paper_plot_parallel()


if __name__ == '__main__':
    plot_paper_plots()

