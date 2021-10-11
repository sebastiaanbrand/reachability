import os
import numpy as np
import pandas as pd
from pathlib import Path
import matplotlib.pyplot as plt


selections = {
    'philosophers' : ['Philosophers-5.bdd', 'Philosophers-10.bdd', 'Philosophers-20.bdd'],
    'eratosthenes' : ['eratosthenes-010.bdd', 'eratosthenes-020.bdd','eratosthenes-050.bdd', 'eratosthenes-100.bdd'],
    'RAS-C' : ['RAS-C-3.bdd', 'RAS-C-5.bdd', 'RAS-C-10.bdd', 'RAS-C-15.bdd', 'RAS-C-20.bdd']
 }

fig_formats = ['png', 'pdf', 'eps']
data_folder  = 'bench_data/old/5 - oct 8/'
plots_folder_temp = 'plots/{}/{}/' # output in plots/subfolder/fig_format/
label_folder_temp = 'plots/{}/labeled/' # for plots with labels for all data-points
plots_folder = ''
label_folder = ''

datamap = {} # map from ('dataset','type') -> dataframe
matrix_folders = {  ('beem','vn-bdd') : 'models/beem/matrices/bdds/vanilla/', 
                    ('beem','sl-bdd') : 'models/beem/matrices/bdds/sloan/',
                    ('ptri','vn-bdd') : 'models/petrinets/matrices/bdds/vanilla/',
                    ('ptri','sl-bdd') : 'models/petrinets/matrices/bdds/sloan/',
                    ('beem','vn-ldd') : 'models/beem/matrices/ldds/vanilla/', 
                    ('beem','sl-ldd') : 'models/beem/matrices/ldds/sloan/',
                    ('ptri','vn-ldd') : 'models/petrinets/matrices/ldds/vanilla/',
                    ('ptri','sl-ldd') : 'models/petrinets/matrices/ldds/sloan/'}

datasetnames = ['beem', 'ptri']  # ['beem, 'ptri', 'prom']
legend_names = {'beem' : 'dve', 'ptri' : 'petrinets', 'prom' : 'promela'}
stratIDs     = {'bfs' : 0,
                'sat' : 2,
                'rec' : 4}
axis_label = {'bfs' : 'BFS',
              'sat' : 'saturation',
              'rec' : 'new algorithm'}

verbose = True


def info(str):
    if (verbose):
        print(str)


def try_load_data(key, filepath):
    global datamap
    if (os.path.exists(filepath)):
        info("loading file {}".format(filepath))
        datamap[key] = pd.read_csv(filepath)
    else:
        return 0


def load_data():
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
        return var_avg_bw
    elif (return_metric == 'var-max-bw'):
        return var_max_bw
    elif (return_metric == 'var-density'):
        return var_density
    
    # Matrix of rels vs rels
    rel_matrix = create_rel_vs_rel(var_matrix)
    rel_density = np.sum(rel_matrix) / np.size(rel_matrix)
    rel_max_bw, rel_avg_bw = get_bandwidth(rel_matrix)
    if (return_metric == 'rel-avg-bw'):
        return rel_avg_bw
    elif (return_metric == 'rel-max-bw'):
        return rel_max_bw
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
    ax.set_xlabel('{} time (s)'.format(axis_label[x_strat]))
    ax.set_ylabel('{} time (s)'.format(axis_label[y_strat]))
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


def plot_selection(x_strat, x_data_label, y_strat, y_data_label):
    info("plotting selection on {} ({}) vs {} ({})".format(x_strat, x_data_label, 
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

        for sel_name, selection in selections.items():
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

            joined = joined[joined['benchmark'].isin(selection)]
            if (len(joined) == 0):
                continue

            # plot reachability time of x vs y
            xs = joined['reach_time_x'].to_numpy()
            ys = joined['reach_time_y'].to_numpy()
            ax.scatter(xs, ys, s=point_size, label=sel_name)

            # track for annotations
            all_xs.extend(xs)
            all_ys.extend(ys)
            for bench_name in joined['benchmark']:
                short_label = bench_name[len(sel_name)+1:-4]
                all_names.append(int(short_label))

            # max and min for diagonal line
            max_val = max(max_val, np.max(xs), np.max(ys))
            min_val = min(min_val, np.min(xs), np.min(ys))

    # diagonal line
    ax.plot([min_val, max_val], [min_val, max_val], ls="--", c="gray")

    # labels and formatting
    ax.set_xlabel('{} time (s)'.format(axis_label[x_strat]))
    ax.set_ylabel('{} time (s)'.format(axis_label[y_strat]))
    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlim([min_val-0.15*min_val, max_val+0.15*max_val])
    ax.set_ylim([min_val-0.15*min_val, max_val+0.15*max_val])
    ax.legend(framealpha=1.0)
    plt.tight_layout()


    # add data-point labes and plot as pdf
    for i, bench_name in enumerate(all_names):
        ax.annotate(bench_name, (all_xs[i], all_ys[i]), fontsize=8.0, ha='left', rotation=0)
    fig_name = '{}reachtime_{}_{}_vs_{}_{}_selection.{}'.format(label_folder,
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
    axs[0].set_xlabel('{} time (s)'.format(axis_label[x1_strat]))
    axs[1].set_xlabel('{} time (s)'.format(axis_label[x2_strat]))
    axs[0].set_ylabel('{} time (s)'.format(axis_label[y_strat]))
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


def plot_comparison_sbs(x1_strat, x1_data_label, 
                        y1_strat, y1_data_label,
                        x2_strat, x2_data_label, 
                        y2_strat, y2_data_label,
                        x_label,  y_label):
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
    axs[0].set_xlabel('{}'.format(axis_label[x1_strat]))
    axs[1].set_xlabel('{}'.format(axis_label[x2_strat]))
    fig.text(0.54, 0.01, x_label, ha='center')
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
    fig.subplots_adjust(bottom=0.22)

    # plot for all formats
    for fig_format in fig_formats:
        subfolder = plots_folder.format(fig_format)
        fig_name = '{}reachtime_{}_{}_vs_{}_{}_and_{}_{}vs_{}_{}.{}'.format(
                                                        subfolder,
                                                        x1_strat, x1_data_label,
                                                        y1_strat, y1_data_label,
                                                        x2_strat, x2_data_label,
                                                        y2_strat, y2_data_label,
                                                        fig_format)
        fig.savefig(fig_name, dpi=300)
    plt.close(fig)


def plot_rec_over_sat_vs_rel_metric(data_label, metric):
    info("plotting rec/sat against {} on {}".format(metric, data_label))
    
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

        # plot x vs y
        ax.scatter(matrix_metrics, relative_rec_times, 
                   s=point_size, label=legend_names[ds_name])

        # track for annotations
        all_xs.extend(matrix_metrics)
        all_ys.extend(relative_rec_times)
        all_names.extend(joined['benchmark'])

    # labels and formatting
    ax.hlines(1, 0, 1, colors=['grey'], linestyles='--')
    ax.set_xlabel('{}'.format(metric))
    ax.set_ylabel('new algorithm time / saturation time')
    ax.set_yscale('log')
    ax.legend(framealpha=1.0)
    plt.tight_layout()

    # plots without data-point lables
    for fig_format in fig_formats:
        subfolder = plots_folder.format(fig_format)
        fig_name = '{}rec_over_sat_vs_metric_{}_on_{}.{}'.format(subfolder, 
                                                metric, data_label, fig_format)
        fig.savefig(fig_name, dpi=300)

    # add data-point labes and plot as pdf
    for i, bench_name in enumerate(all_names):
        ax.annotate(bench_name, (all_xs[i], all_ys[i]), fontsize=1.0)
    fig_name = '{}rec_over_sat_vs_metric_{}_on_{}.{}'.format(label_folder,
                                                      metric, data_label,'pdf')
    fig.savefig(fig_name, dpi=300)
    plt.close(fig)



def set_subfolder_name(subfolder_name):
    global plots_folder, plots_folder_temp, label_folder, label_folder_temp
    plots_folder = plots_folder_temp.format(subfolder_name, '{}')
    label_folder = label_folder_temp.format(subfolder_name)

    # make plots folders if necessary
    for fig_format in fig_formats:
        Path(plots_folder.format(fig_format)).mkdir(parents=True, exist_ok=True)
    Path(label_folder).mkdir(parents=True, exist_ok=True)


def plot_things():
    # Relative speedup compared to some metric of the relation matrix
    set_subfolder_name('Relation metric comparison')
    plot_rec_over_sat_vs_rel_metric('sl-bdd', 'var-density')
    plot_rec_over_sat_vs_rel_metric('sl-ldd', 'var-density')

    """
    # BDDs vanilla
    set_subfolder_name('BDDs vanilla')
    plot_comparison_shared_y('bfs', 'vn-bdd', 'sat', 'vn-bdd', 'rec', 'vn-bdd')
    plot_comparison('bfs', 'vn-bdd', 'rec', 'vn-bdd')
    plot_comparison('sat', 'vn-bdd', 'rec', 'vn-bdd')
    plot_comparison('bfs', 'vn-bdd', 'sat', 'vn-bdd')

    # BDDs Sloan
    set_subfolder_name('BDDs Sloan')
    plot_comparison_shared_y('bfs', 'sl-bdd', 'sat', 'sl-bdd', 'rec', 'sl-bdd')
    plot_comparison('bfs', 'sl-bdd', 'rec', 'sl-bdd')
    plot_comparison('sat', 'sl-bdd', 'rec', 'sl-bdd')
    plot_comparison('bfs', 'sl-bdd', 'sat', 'sl-bdd')

    # LDDs vanilla
    set_subfolder_name('LDDs vanilla')
    plot_comparison_shared_y('bfs', 'vn-ldd', 'sat', 'vn-ldd', 'rec', 'vn-ldd')
    plot_comparison('bfs', 'vn-ldd', 'rec', 'vn-ldd')
    plot_comparison('sat', 'vn-ldd', 'rec', 'vn-ldd')
    plot_comparison('bfs', 'vn-ldd', 'sat', 'vn-ldd')

    # LDDs Sloan
    set_subfolder_name('LDDs Sloan')
    plot_comparison_shared_y('bfs', 'sl-ldd', 'sat', 'sl-ldd', 'rec', 'sl-ldd')
    plot_comparison('bfs', 'sl-ldd', 'rec', 'sl-ldd')
    plot_comparison('sat', 'sl-ldd', 'rec', 'sl-ldd')
    plot_comparison('bfs', 'sl-ldd', 'sat', 'sl-ldd')

    # Sloan vs vanilla
    set_subfolder_name('Sloan vs vanilla')
    plot_comparison('bfs', 'vn-bdd', 'bfs', 'sl-bdd')
    plot_comparison('sat', 'vn-bdd', 'sat', 'sl-bdd')
    plot_comparison('rec', 'vn-bdd', 'rec', 'sl-bdd')
    plot_comparison('bfs', 'vn-ldd', 'bfs', 'sl-ldd')
    plot_comparison('sat', 'vn-ldd', 'sat', 'sl-ldd')
    plot_comparison('rec', 'vn-ldd', 'rec', 'sl-ldd')
    """


if __name__ == '__main__':
    load_data()
    pre_process()
    assert_states_nodes()
    plot_things()

