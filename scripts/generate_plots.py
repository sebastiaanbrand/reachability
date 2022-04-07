import os
import sys
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

maxtime = 600 # time in seconds

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
markers = {'beem': 's', 'ptri' : '^', 'prom' : 'P'}
marker_size = {'beem': 8, 'ptri': 10, 'prom': 15}
marker_colors = {'beem' : 'tab:blue', 'ptri' : 'tab:orange', 'prom' : 'tab:green'}
stratIDs     = {'bfs' : 0,
                'sat' : 2,
                'rec' : 4,
                'bfs-plain' : 5,
                'rec-par' : 14,
                'rec-copy' : 104}
axis_label = {('bfs','bdd') : 'BFS',
              ('sat','bdd') : 'Saturation',
              ('rec','bdd') : 'Algorithm 1',
              ('bfs','ldd') : 'BFS',
              ('sat','ldd') : 'Saturation',
              ('rec','ldd') : 'Algorithm 3',
              ('bfs-plain', 'ldd') : 'Plain BFS w/ custom img'}

metric_labels={'var-avg-bw':'average relative bandwidth', 
               'var-max-bw':'maximum relative bandwidth',
               'var-density':'matrix density',
               'rel-avg-bw':'average relative bandwidth', 
               'rel-max-bw':'maximum relative bandwidth',
               'rel-density':'matrix density'}

n_workers = set() # unique number of workers

verbose = True


def info(str, end='\n'):
    if (verbose):
        print(str, end=end)

def parse_args():
    if (len(sys.argv) <= 1):
        print("please specify a subfolder in bench_data/")
        exit(1)
    subfolder = sys.argv[1]
    add_merge_time=True
    if (len(sys.argv) > 2):
        if (sys.argv[2] == 'no-merge-time'):
            add_merge_time=False
    return subfolder, add_merge_time
       

def try_load_data(key, filepath):
    global datamap
    if (os.path.exists(filepath)):
        info("loading file {}".format(filepath))
        datamap[key] = pd.read_csv(filepath)
    else:
        return 0


def load_data(data_folder, expected=0):
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

    # Static BDDs / LDDs
    try_load_data(('ptri', 'sl-static-9-bdd'), data_folder + 'petrinets_sloan_stats_bdd_static_9.csv')
    try_load_data(('ptri', 'sl-static-9-ldd'), data_folder + 'petrinets_sloan_stats_ldd_static_9.csv')

    # Deadlocks
    try_load_data(('ptri-dl', 'sl-bdd'), data_folder + 'petrinets_sloan_stats_bdd_deadlocks.csv')

    return ( len(datamap.keys()) >= expected )


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
    
    # get all the unique number of workers
    global n_workers
    n_workers = set()
    for df in datamap.values():
        _workers = np.unique(df['workers'])
        n_workers.update(_workers)


def load_its_data(its_type):
    # load csv
    if (its_type == 'RD'):
        its_data = pd.read_csv('bench_data/its_tools/its_tools_petrinets_deadlocks.csv')
    else:
        its_data = pd.read_csv('bench_data/its_tools/its_tools_petrinets.csv')

    # some pre-processing
    its_data.columns = its_data.columns.str.strip()
    its_data = its_data.astype({"benchmark" : str, "type" : str,
                                "reach_time" : float,
                                "final_states" : float})
    its_data['type'] = its_data['type'].str.strip()

    return its_data
    

def assert_states_nodes():
    print("checking the state- and nodecounts")
    n_states = {} # dict : benchmark -> number of final states
    for df_key, df in datamap.items():
        n_nodes  = {} # dict : benchmark -> number of final nodes
        total_wrong_states = 0
        for _, row in df.iterrows():
            # check final_states
            if (row['benchmark'] not in n_states):
                n_states[row['benchmark']] = row['final_states']
            else:
                if (n_states[row['benchmark']] != row['final_states']):
                    print("final_states not equal for {}: {} != {} ({})".format(
                          row['benchmark'],
                          n_states[row['benchmark']],
                          row['final_states'],
                          df_key
                    ))
                    total_wrong_states += 1
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
        print("{}/{} \twrong statecount for {}".format(total_wrong_states,
                                                       len(df.index),
                                                       df_key))

def round_sig(x, sig):
    if(np.isnan(x) or np.isinf(x) or np.isneginf(x)):
        return x
    return np.round(x, sig-int(np.floor(np.log10(np.abs(x))))-1)


def compare_counts_its_reach(data_label, dd_type):
    # this removes all entries from the (data_label, dd_type) df which 
    # disagree with  the state counts from its-reach
    data_its = load_its_data('not RD')
    data_dd = datamap[(data_label, dd_type)]

    data_dd['benchmark'] = data_dd['benchmark'].str.replace('.ldd', '', regex=False)
    data_dd['benchmark'] = data_dd['benchmark'].str.replace('.bdd', '', regex=False)

    # inner join
    data_its = data_its.set_index('benchmark')
    joined = data_dd.join(data_its, on='benchmark', how='outer', lsuffix='_dd', rsuffix='_its')

    final_states_dd = joined['final_states_dd'].to_numpy()
    final_states_its = joined['final_states_its'].to_numpy()
    for i in range(len(joined)):
        final_states_dd[i] = round_sig(final_states_dd[i], sig=6)
        final_states_its[i] = round_sig(final_states_its[i], sig=6)

    disagree = final_states_dd != final_states_its

    disagree_bench = joined.loc[disagree]
    disagree_bench = disagree_bench.loc[np.logical_not(np.isnan(disagree_bench['final_states_its']))]
    disagree_bench = disagree_bench.loc[np.logical_not(np.isnan(disagree_bench['final_states_dd']))]
    print("ITS disagrees with ({}, {}) on:".format(data_label, dd_type))
    print(disagree_bench)

    print("removing these from ({}, {}) data\n".format(data_label, dd_type))
    new_df = data_dd.loc[~data_dd['benchmark'].isin(disagree_bench['benchmark'])]
    datamap[(data_label, dd_type)] = new_df


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


def _plot_time_comp(ax, x_strat, x_dd, y_strat, y_dd, add_merge_time):
    """
    The actual plotting of the runtime comparisons happens here.
    """

    max_val = 0
    min_val = 1e9
    meta = {'xs' : [], 'ys' : [], 'names' : []}
    for ds_name in datasetnames:
        # get the relevant data
        try:
            x_data = datamap[(ds_name, x_dd)]
            y_data = datamap[(ds_name, y_dd)]
        except:
            print("Could not find data for '{}', skipping...".format(ds_name))
            continue

        # select subsets of x and y data
        group_x = x_data.loc[x_data['strategy'] == stratIDs[x_strat]]
        group_y = y_data.loc[y_data['strategy'] == stratIDs[y_strat]]

        # outer join of x and y
        group_y = group_y.set_index('benchmark')
        joined  = group_x.join(group_y, on='benchmark', how='outer',
                               lsuffix='_x', rsuffix='_y')

        # plot reachability time of x vs y
        xs = joined['reach_time_x'].to_numpy()
        ys = joined['reach_time_y'].to_numpy()
        if (add_merge_time):
            xs += joined['merge_time_x'].to_numpy()
            ys += joined['merge_time_y'].to_numpy()
        
        # some styling
        s = marker_size[ds_name]
        m = markers[ds_name]
        l = legend_names[ds_name]
        ec = marker_colors[ds_name]
        fc = np.array([marker_colors[ds_name]]*len(xs))
        fc[np.isnan(xs)] = 'none'
        fc[np.isnan(ys)] = 'none'
        factor = 1 # factor for vizualization
        xs[np.isnan(xs)] = maxtime*factor
        ys[np.isnan(ys)] = maxtime*factor

        # actually plot the data points
        ax.scatter(xs, ys, s=s, marker=m, facecolors=fc, edgecolors=ec, label=l)

        # track for diagonal lines
        max_val = max(max_val, np.max(xs), np.max(ys))
        min_val = min(min_val, np.min(xs), np.min(ys))

        # track names for annotations
        meta['xs'].extend(xs)
        meta['ys'].extend(ys)
        meta['names'].extend(joined['benchmark'])

    ax.set_xscale('log')
    ax.set_yscale('log')

    meta['max_val'] = max_val
    meta['min_val'] = min_val
    return ax, meta


def _plot_diagonal_lines(ax, max_val, min_val):
    """
    Add diagonal lines to ax
    """
    
    # bit of margin for vizualization
    ax.set_xlim([min_val-0.15*min_val, max_val+0.15*max_val])
    ax.set_ylim([min_val-0.15*min_val, max_val+0.15*max_val])

    # diagonal lines
    ax.plot([min_val, max_val], [min_val, max_val], ls="--", c="gray")
    ax.plot([min_val, max_val], [min_val*10, max_val*10], ls=":", c="#767676")
    ax.plot([min_val, max_val], [min_val/10, max_val/10], ls=":", c="#767676")

    return ax

def plot_comparison(x_strat, x_dd, y_strat, y_dd, add_merge_time, xlabel='', ylabel=''):
    info("plotting {} ({}) vs {} ({})".format(x_strat, x_dd, y_strat, y_dd))

    scaling = 5.0 # default = ~6.0
    fig, ax = plt.subplots(figsize=(scaling, scaling*0.75))

    # actually fills in the subplot
    ax, meta = _plot_time_comp(ax, x_strat, x_dd, y_strat, y_dd, add_merge_time)
    ax = _plot_diagonal_lines(ax, meta['max_val'], meta['min_val'])
    
    # set axis labels + legend
    if (xlabel == ''):
        xlabel = '{} time (s)'.format(axis_label[(x_strat,x_dd[-3:],)])
    if (ylabel == ''):
        ylabel = '{} time (s)'.format(axis_label[(y_strat,y_dd[-3:],)])
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.legend(framealpha=1.0)
    plt.tight_layout()

    # plots without data-point lables
    for fig_format in fig_formats:
        subfolder = plots_folder.format(fig_format)
        fig_name = '{}reachtime_{}_{}_vs_{}_{}_incMergeTime{}.{}'.format(subfolder,
                                                          x_strat, x_dd,
                                                          y_strat, y_dd,
                                                          add_merge_time,
                                                          fig_format)
        fig.savefig(fig_name, dpi=300)

    # add data-point labes and plot as pdf
    for i, bench_name in enumerate(meta['names']):
        ax.annotate(bench_name, (meta['xs'][i], meta['ys'][i]), fontsize=1.0)
    fig_name = '{}reachtime_{}_{}_vs_{}_{}.{}'.format(label_folder,
                                                      x_strat, x_dd,
                                                      y_strat, y_dd,
                                                      'pdf')
    fig.savefig(fig_name, dpi=300)
    plt.close(fig)


def plot_comparison_sbs(x1_strat, x1_dd, y1_strat, y1_dd,
                        x2_strat, x2_dd, y2_strat, y2_dd,
                        x_label, y_label, add_merge_time):
    info("plotting")
    info(" left:  {} ({}) vs {} ({})".format(x1_strat, x1_dd, y1_strat, y1_dd))
    info(" right: {} ({}) vs {} ({})".format(x2_strat, x2_dd, y2_strat, y2_dd))

    scaling = 4.9 # default = ~6.0
    w = 1.6 # relative width
    fig, axs = plt.subplots(1, 2, sharey=True, figsize=(w*scaling, scaling*0.75))

    # actually fill in the subplots
    axs[0], meta0 = _plot_time_comp(axs[0], x1_strat, x1_dd, y1_strat, y1_dd, add_merge_time)
    axs[1], meta1 = _plot_time_comp(axs[1], x2_strat, x2_dd, y2_strat, y2_dd, add_merge_time)

    # diagonal lines
    max_val = max(meta0['max_val'], meta1['max_val'])
    min_val = min(meta0['min_val'], meta1['min_val'])
    axs[0] = _plot_diagonal_lines(axs[0], max_val, min_val)
    axs[1] = _plot_diagonal_lines(axs[1], max_val, min_val)

    # axis labels + legend
    axs[0].set_ylabel(y_label)
    axs[0].set_title('{}s'.format(x1_dd[-3:].upper()))
    axs[1].set_title('{}s'.format(x2_dd[-3:].upper()))
    fig.text(0.54, 0.01, x_label, ha='center')
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
                                                        x1_strat, x1_dd,
                                                        y1_strat, y1_dd,
                                                        x2_strat, x2_dd,
                                                        y2_strat, y2_dd,
                                                        add_merge_time,
                                                        fig_format)
        fig.savefig(fig_name, dpi=300)
    plt.close(fig)


def plot_rec_over_sat_vs_rel_metric(data_label, metric, add_merge_time=False):
    info("plotting rec/sat against {} on {}".format(metric, data_label), end='')
    if (metric[:3] == 'rel'):
        info(" (takes some time)")
    
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

        if (add_merge_time):
            sat_times += joined['merge_time_sat'].to_numpy()
            rec_times += joined['merge_time_rec'].to_numpy()

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
        fig_name = '{}rec_over_sat_vs_metric_{}_on_{}_incmergetime{}.{}'.format(subfolder, 
                                                metric, data_label, add_merge_time, fig_format)
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
        fig_name = '{}rec_over_sat_vs_metric_{}_on_{}_incmergetime{}_trend.{}'.format(subfolder, 
                                                metric, data_label, add_merge_time, fig_format)
        fig.savefig(fig_name, dpi=300)

    # add data-point labes and plot as pdf
    for i, bench_name in enumerate(all_names):
        ax.annotate(bench_name, (all_xs[i], all_ys[i]), fontsize=1.0)
    fig_name = '{}rec_over_sat_vs_metric_{}_on_{}.{}'.format(label_folder,
                                                      metric, data_label,'pdf')
    fig.savefig(fig_name, dpi=300)
    plt.close(fig)


def plot_parallel(strat1, strat2, strat3, data_label, min_time, plot_legend=True):
    info("plotting parallel speedups {}, {}, {} on {} (mintime = {}s)".format(
        strat1, strat2, strat3, data_label, min_time))
    
    

    all_xs = []
    all_ys = []
    all_names = [] # track for annotations

    speedups1 = {}
    speedups2 = {}
    speedups3 = {}
    for workers in sorted(n_workers):
        speedups1[workers] = []
        speedups2[workers] = []
        speedups3[workers] = []
    
    scaling = 5.0 # default = ~6.0
    width = 1
    if (plot_legend == False):
        width = 1.25
        scaling = 4.0
    fig, ax = plt.subplots(figsize=(scaling*0.85, scaling*0.75*width)) # *0.70
    point_size = 8.0

    for ds_name in datasetnames:
        if (not (ds_name, data_label) in datamap):
            continue

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
    ax.set_xlabel('number of cores')
    ax.set_ylabel('speedup')
    ax.tick_params(axis='x', which='both',length=0)
    if (plot_legend):
        ax.legend([bplot["boxes"][0], bplot["boxes"][1], bplot["boxes"][2]], 
                ['Parallel saturation', 'ReachBDD', 'ReachBDD-par'],
                loc='upper left', framealpha=1.0)
    plt.tight_layout()

     # plots without data-point lables
    for fig_format in fig_formats:
        subfolder = plots_folder.format(fig_format)
        fig_name = '{}speedups_{}_{}_{}_on{}_mintime_{}_{}_cores.{}'.format(subfolder, 
                    strat1, strat2, strat3, data_label, min_time, max(n_workers), fig_format)
        fig.savefig(fig_name, dpi=300)
    plt.close(fig)


def plot_merge_overhead(data_label):
    info("plotting relative merge overhead for {}".format(data_label))

    # get the relevant data from beem, petri, prom datasets
    datasets = []
    for ds_name in datasetnames:
        datasets.append(datamap[(ds_name, data_label)])
    data = pd.concat(datasets)

    # separate into data for saturation and recursive alg
    sat_data = data.loc[data['strategy'] == stratIDs['sat']]
    rec_data = data.loc[data['strategy'] == stratIDs['rec']]

    # inner join sat and rec data on 'benchmark'
    sat_data = sat_data.set_index('benchmark')
    joined = rec_data.join(sat_data, on='benchmark', how='inner',
                            lsuffix='_rec', rsuffix='_sat')

    # rec_reach_time / sat_reach_time
    relative_reach = joined['reach_time_rec'] / joined['reach_time_sat']
    relative_merge = joined['merge_time_rec'] / joined['reach_time_sat']

    # create buckets (log scale)
    lowest = 10**(np.floor(np.log10(min(joined['reach_time_sat']))))
    highest = 10**(np.ceil(np.log10(max(joined['reach_time_sat']))))
    bucket_labels = []
    bucket_reach  = []
    bucket_merge  = []

    while (lowest <= highest):
        lower_edge = joined['reach_time_sat'] >= lowest/2
        upper_edge = joined['reach_time_sat'] < lowest*5
        selection = lower_edge & upper_edge
        if (sum(selection) > 0):
            bucket_reach.append(relative_reach.loc[selection])
            bucket_merge.append(relative_merge.loc[selection])
            bucket_labels.append('[{},\n{})'.format(lowest/2, lowest*5))
        lowest *= 10

    # compute averages
    bucket_sizes = [len(x) for x in bucket_reach]
    avg_reach = [np.average(x) for x in bucket_reach]
    avg_merge = [np.average(x) for x in bucket_merge]
    info('\tbucket sizes: {}'.format(bucket_sizes))
    #print(avg_reach)
    #print(avg_merge)

    #  plot data, formatting and lables
    reach_label = "Alg. 2 time (average)"
    if (data_label[-3:] == 'ldd'):
        reach_label = "Alg. 3 time (average)"
    merge_label = "relation merging overhead (average)"
    fig, ax = plt.subplots()
    ax.bar(bucket_labels, avg_reach, width=0.35, label=reach_label)
    ax.bar(bucket_labels, avg_merge, width=0.35, bottom=avg_reach, label=merge_label)
    ax.legend(framealpha=1.0)
    ax.set_xlabel('saturation time (s)')
    ax.set_ylabel('average runtime relative to saturation')
    plt.tight_layout()

    # plot for all formats
    for fig_format in fig_formats:
        subfolder = plots_folder.format(fig_format)
        fig_name = '{}merge_overhead_{}.{}'.format(subfolder, data_label, fig_format)
        fig.savefig(fig_name, dpi=300)
    plt.close(fig)


def plot_its_vs_dd(dd_type, dd_strat, its_type):
    data_its = load_its_data(its_type)
    data_dd = datamap[('ptri', dd_type)]

    data_dd['benchmark'] = data_dd['benchmark'].str.replace('.ldd', '', regex=False)
    data_dd['benchmark'] = data_dd['benchmark'].str.replace('.bdd', '', regex=False)
    
    # select relevant subset
    data_dd = data_dd.loc[data_dd['strategy'] == stratIDs[dd_strat]]
    data_its = data_its.loc[data_its['type'] == its_type]
    its_zero_time = data_its.loc[data_its['reach_time'] == 0]
    data_its = data_its.loc[data_its['reach_time'] != 0]

    print(its_zero_time)

    # inner join
    data_its = data_its.set_index('benchmark')
    joined = data_dd.join(data_its, on='benchmark', how='outer',
                            lsuffix='_dd', rsuffix='_its')
    
    if ('final_states_its' in joined):
        neq_states = joined['final_states_dd'] != joined['final_states_its']
        if (sum(neq_states) > 0):
            print("WARNING: ITS-tools and Sylvan disagree on # states for:")
            issue_cases = joined.loc[neq_states]
            print(issue_cases)
    
    ys_key = 'reach_time_dd' # total_time or reach_time_dd ?
    xs = joined['reach_time_its'].to_numpy()
    ys = joined[ys_key].to_numpy() 

    # some styling
    scaling = 4.8 # default = ~6.0
    fig, ax = plt.subplots(1, 1, figsize=(scaling, scaling*0.75))
    c = marker_colors['ptri']
    s = marker_size['ptri']
    m = markers['ptri']
    l = legend_names['ptri']
    ec = marker_colors['ptri']
    fc  = np.array([marker_colors['ptri']]*len(xs))
    fc[np.isnan(xs)] = 'none'
    fc[np.isnan(ys)] = 'none'

    factor = 1 # factor for vizualization
    xs[np.isnan(xs)] = maxtime*factor
    ys[np.isnan(ys)] = maxtime*factor

    # scatter plot
    ax.scatter(xs, ys, s=s, marker=m, facecolors=fc, edgecolors=ec, label=legend_names['ptri'])
    all_names = joined['benchmark']

    # diagonal lines
    max_val = max(np.max(xs), np.max(ys))
    min_val = min(np.min(xs), np.min(ys))
    ax.plot([min_val, max_val], [min_val, max_val], ls='--', c="#5d5d5d")
    ax.plot([min_val, max_val], [min_val*10, max_val*10], ls=":", c="#767676")
    ax.plot([min_val, max_val], [min_val/10, max_val/10], ls=":", c="#767676")

    # lables and formatting
    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_ylabel("{}".format(axis_label[(dd_strat,dd_type[-3:])]))
    ax.set_xlabel("ITS-tools")
    ax.set_xlim([min_val-0.15*min_val, max_val+0.15*max_val])
    ax.set_ylim([min_val-0.15*min_val, max_val+0.15*max_val])
    ax.legend(framealpha=1.0)
    plt.tight_layout()

    for fig_format in fig_formats:
        subfolder = plots_folder.format(fig_format)
        fig_name = '{}its{}_vs_{}_{}_{}.{}'.format(subfolder,
                                            its_type.replace('.','_'), 
                                            dd_type, 
                                            dd_strat,
                                            ys_key,
                                            fig_format)
        fig.savefig(fig_name, dpi=300)
    
    # add data-point labels and plot as pdf
    for i, bench_name in enumerate(all_names):
        ax.annotate(bench_name, (xs[i], ys[i]), fontsize=1.0)
    fig_name = fig_name = '{}its{}_vs_{}_{}_{}.{}'.format(label_folder,
                                            its_type.replace('.','_'), 
                                            dd_type, 
                                            dd_strat,
                                            ys_key,
                                            'pdf')
    fig.savefig(fig_name, dpi=300)
    plt.close(fig)

def plot_its_vs_dd_deadlocks(dd_type, dd_strat, its_type):
    data_its = load_its_data(its_type)
    data_dd = datamap[('ptri-dl', dd_type)]

    data_dd['benchmark'] = data_dd['benchmark'].str.replace('.ldd', '', regex=False)
    data_dd['benchmark'] = data_dd['benchmark'].str.replace('.bdd', '', regex=False)
    
    # select relevant subset
    data_dd = data_dd.loc[data_dd['strategy'] == stratIDs[dd_strat]]
    data_its = data_its.loc[data_its['type'] == its_type]
    data_its = data_its.loc[data_its['reach_time'] != 0]

    # inner join
    data_its = data_its.set_index('benchmark')
    joined = data_dd.join(data_its, on='benchmark', how='outer',
                            lsuffix='_dd', rsuffix='_its')
    
    if ('final_states_its' in joined):
        neq_states = joined['final_states_dd'] != joined['final_states_its']
        if (sum(neq_states) > 0):
            print("WARNING: ITS-tools and Sylvan disagree on # states for:")
            issue_cases = joined.loc[neq_states]
            print(issue_cases)
    
    # split into deadlocks and no deadlocks
    joined_deadlocks = joined.loc[joined['deadlocks_its'] == 1]
    joined_no_deadlocks = joined.loc[joined['deadlocks_its'] == 0]
    x_key = 'reach_time_its'
    y_key = 'total_time'
    xs = joined[x_key].to_numpy()
    ys = joined[y_key].to_numpy()
    xsDL = joined_deadlocks[x_key].to_numpy()
    ysDL = joined_deadlocks[y_key].to_numpy()
    xsNDL = joined_no_deadlocks[x_key].to_numpy()
    ysNDL = joined_no_deadlocks[y_key].to_numpy()

    # some styling
    scaling = 4.8 # default = ~6.0
    fig, ax = plt.subplots(1, 1, figsize=(scaling, scaling*0.75))
    c = marker_colors['ptri']
    s = marker_size['ptri']
    m = markers['ptri']
    l = legend_names['ptri']
    ecDL  = 'tab:blue'
    ecNDL = 'tab:orange'
    fcDL  = np.array([ecDL]*len(xsDL))
    fcNDL = np.array([ecNDL]*len(xsNDL))
    fcDL[np.isnan(xsDL)] = 'none'
    fcDL[np.isnan(ysDL)] = 'none'
    fcNDL[np.isnan(xsNDL)] = 'none'
    fcNDL[np.isnan(ysNDL)] = 'none'

    factor = 1 # factor for vizualization
    xs[np.isnan(xs)] = maxtime*factor
    ys[np.isnan(ys)] = maxtime*factor
    xsDL[np.isnan(xsDL)] = maxtime*factor
    ysDL[np.isnan(ysDL)] = maxtime*factor
    xsNDL[np.isnan(xsNDL)] = maxtime*factor
    ysNDL[np.isnan(ysNDL)] = maxtime*factor

    # scatter plot
    ax.scatter(xsDL, ysDL, s=s, marker=m, facecolors=fcDL, edgecolors=ecDL, label="PTs with deadlocks")
    ax.scatter(xsNDL, ysNDL, s=s, marker=m, facecolors=fcNDL, edgecolors=ecNDL, label="PTs without deadlocks")
    all_names = joined['benchmark']

    # diagonal lines
    max_val = max(np.max(xs), np.max(ys))
    min_val = min(np.min(xs), np.min(ys))
    ax.plot([min_val, max_val], [min_val, max_val], ls='--', c="#5d5d5d")
    ax.plot([min_val, max_val], [min_val*10, max_val*10], ls=":", c="#767676")
    ax.plot([min_val, max_val], [min_val/10, max_val/10], ls=":", c="#767676")

    # lables and formatting
    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_ylabel("{} + compute deadlocks".format(axis_label[(dd_strat,dd_type[-3:])]))
    ax.set_xlabel("ITS-tools")
    ax.set_xlim([min_val-0.15*min_val, max_val+0.15*max_val])
    ax.set_ylim([min_val-0.15*min_val, max_val+0.15*max_val])
    ax.legend(framealpha=1.0)
    plt.tight_layout()

    for fig_format in fig_formats:
        subfolder = plots_folder.format(fig_format)
        fig_name = '{}its{}_vs_{}_{}.{}'.format(subfolder,
                                            its_type.replace('.','_'), 
                                            dd_type, 
                                            dd_strat,
                                            fig_format)
        fig.savefig(fig_name, dpi=300)
    
    # add data-point labels and plot as pdf
    for i, bench_name in enumerate(all_names):
        ax.annotate(bench_name, (xs[i], ys[i]), fontsize=1.0)
    fig_name = fig_name = '{}its{}_vs_{}_{}.{}'.format(label_folder,
                                            its_type.replace('.','_'), 
                                            dd_type, 
                                            dd_strat,
                                            'pdf')
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


def plot_image_test_comparison():
    data_folder = 'bench_data/all/custom_image_test/'
    load_data(data_folder)
    pre_process()
    assert_states_nodes()

    set_subfolder_name('all/Custom image test REACH')
    plot_comparison('sat', 'sl-ldd', 'rec', 'sl-ldd', add_merge_time=False)
    plot_comparison('sat', 'sl-ldd', 'rec', 'sl-ldd', add_merge_time=True)


def plot_paper_plot_sat_vs_rec(subfolder, add_merge_time):
    set_subfolder_name(subfolder + '/Saturation vs REACH (Figure 9)')

    # this removes all entries from the ('ptri','sl-ldd-static') df which 
    # disagree with  the state counts from its-reach
    #compare_counts_its_reach('ptri', 'sl-ldd-static')

    plot_comparison_sbs('sat', 'sl-bdd', 'rec', 'sl-bdd', 
                        'sat', 'sl-ldd', 'rec', 'sl-ldd', 
                        'Saturation time (s)', 'ReachBDD/MDD time (s)', 
                        add_merge_time=add_merge_time)
    plot_comparison('sat', 'sl-ldd', 'rec', 'sl-ldd', add_merge_time)

def plot_pnml_encode_tests(subfolder):
    set_subfolder_name(subfolder + '/pnml-encode test copy nodes')

    # Saturation pnml2lts-sym vs pnml-encode
    plot_comparison('sat', 'sl-ldd', 'sat', 'sl-static-9-ldd', False, 
                    xlabel='saturation on pnml2lts-sym LDDs',
                    ylabel='saturation on pnml-encode LDDs')
    
    # REACH pnml2lts-sym vs pnml-encode
    plot_comparison('rec', 'sl-ldd', 'rec', 'sl-static-9-ldd', False, 
                    xlabel='REACH on pnml2lts-sym LDDs',
                    ylabel='REACH on pnml-encode LDDs')
    plot_comparison('rec', 'sl-ldd', 'rec', 'sl-static-9-ldd', True, 
                    xlabel='REACH on pnml2lts-sym LDDs',
                    ylabel='REACH on pnml-encode LDDs')
    
    # Saturation pnml-encode vs REACH pnml-encode
    plot_comparison('sat', 'sl-static-9-ldd', 'rec', 'sl-static-9-ldd', False, 
                    xlabel='saturation on pnml-encode LDDs',
                    ylabel='REACH on pnml-encode LDDs')
    plot_comparison('sat', 'sl-static-9-ldd', 'rec', 'sl-static-9-ldd', True, 
                    xlabel='saturation on pnml-encode LDDs',
                    ylabel='REACH on pnml-encode LDDs')
    
    # Saturation pnml2lts-sym vs REACH pnml-encode
    plot_comparison('sat', 'sl-ldd', 'rec', 'sl-static-9-ldd', False, 
                    xlabel='saturation on pnml2lts-sym LDDs',
                    ylabel='REACH on pnml-encode LDDs')
    plot_comparison('sat', 'sl-ldd', 'rec', 'sl-static-9-ldd', True, 
                    xlabel='saturation on pnml2lts-sym LDDs',
                    ylabel='REACH on pnml-encode LDDs')
    
    # Saturation pnml2lts-sym vs REACH pnml2lts-sym
    plot_comparison('sat', 'sl-ldd', 'rec', 'sl-ldd', False, 
                    xlabel='saturation on pnml2lts-sym LDDs',
                    ylabel='REACH on pnml2lts-sym LDDs')
    plot_comparison('sat', 'sl-ldd', 'rec', 'sl-ldd', True, 
                    xlabel='saturation on pnml2lts-sym LDDs',
                    ylabel='REACH on pnml2lts-sym LDDs')

def plot_paper_plot_locality(subfolder, add_merge_time):
    set_subfolder_name(subfolder + '/Locality metric comparison (Figure 10)')
    plot_rec_over_sat_vs_rel_metric('sl-ldd', 'rel-avg-bw', add_merge_time)
    plot_rec_over_sat_vs_rel_metric('sl-bdd', 'rel-avg-bw', add_merge_time)


def plot_paper_plot_parallel(subfolder, plot_legend=True):
    set_subfolder_name(subfolder + '/Parallel speedups (Figure 11)')
    if subfolder == 'subset':
        plot_parallel('sat', 'rec', 'rec-par', 'sl-bdd', 0, plot_legend)
    else:
        plot_parallel('sat', 'rec', 'rec-par', 'sl-bdd', 1, plot_legend)


def plot_paper_plot_merge_overhead(subfolder):
    set_subfolder_name(subfolder + '/Merge overhead (New Figure)')
    plot_merge_overhead('sl-bdd')
    plot_merge_overhead('sl-ldd')


def plot_paper_plot_its_tools_vs_dds(subfolder):
    set_subfolder_name(subfolder + '/ITS-Tools vs DDs (New Figure)')
    plot_its_vs_dd('sl-ldd', 'rec', '.gal')
    plot_its_vs_dd('sl-ldd', 'rec', '.img.gal')
    #plot_its_vs_dd('sl-bdd', 'rec', '.gal')
    #plot_its_vs_dd('sl-bdd', 'rec', '.img.gal')
    plot_its_vs_dd_deadlocks('sl-bdd', 'rec', 'RD')


def plot_paper_plots(subfolder, add_merge_time):   
    # Plot Fig 9, Fig 10, New Fig
    data_folder = 'bench_data/'+ subfolder + '/single_worker/2m/'
    if(load_data(data_folder, expected=6)):
        pre_process()
        assert_states_nodes()
    
        # Plot saturation vs REACH on Sloan BDDs/LDDs (Figure 9)
        #plot_paper_plot_sat_vs_rec(subfolder, add_merge_time)

        plot_pnml_encode_tests(subfolder)

        # Plot locality metric correlation (Figure 10) (on same data)
        #plot_paper_plot_locality(subfolder, add_merge_time=False)

        # Plot relative merge time overhead (New Figure)
        #plot_paper_plot_merge_overhead(subfolder)

        # Plot ITStools vs DDs
        #plot_paper_plot_its_tools_vs_dds(subfolder)
    else:
        print('no complete data found in ' + data_folder)

    
    # Plot parallel (Figure 11)
    """
    data_folder = 'bench_data/' + subfolder + '/par_8/'
    if(load_data(data_folder, expected=3)):
        pre_process()
        assert_states_nodes()
        plot_paper_plot_parallel(subfolder, plot_legend=False)
    else:
        print('no complete data found in ' + data_folder)
    
    # Plot parallel (Figure 11)
    data_folder = 'bench_data/' + subfolder + '/par_96/'
    if(load_data(data_folder, expected=1)):
        pre_process()
        assert_states_nodes()
        plot_paper_plot_parallel(subfolder)
    else:
        print('no complete data found in ' + data_folder)
    """


if __name__ == '__main__':
    subfolder, add_merge_time = parse_args()
    plot_paper_plots(subfolder, add_merge_time)
    #plot_image_test_comparison()
    

