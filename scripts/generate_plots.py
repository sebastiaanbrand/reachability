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

set_incorrect_as_timeout = True

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
                'rec-copy' : 104,
                'rec-copy-rr' : 206,
                'bfs-plain-copy' : 105,
                'bfs-plain-copy-rr' : 205}
axis_label = {('bfs','bdd') : 'BFS',
              ('sat','bdd') : 'Saturation',
              ('rec','bdd') : 'Algorithm 1',
              ('rec-par','bdd') : 'ReachBDD-par',
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
        print("please specify which plot to generate (give arg [saturation|locality|parallel|its|static])")
        exit()
    which_plot = sys.argv[1]

    data_folder = None
    if (len(sys.argv) <= 2):
        print("argument 2 (path to data folder) missing")
        exit()
    data_folder = sys.argv[2]
    
    plot_cores = 0
    if (which_plot == 'parallel-scatter'):
        if (len(sys.argv) <= 3):
            print("argument 3 (number of cores to plot, e.g. 8) missing")
            exit()
        plot_cores = [int(sys.argv[3])]
        if (len(sys.argv) >= 5):
            plot_cores = [int(sys.argv[3]), int(sys.argv[4])]
    
    return which_plot, data_folder, plot_cores


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
    try_load_data(('ptri', 'sl-static-bdd'), data_folder + 'petrinets_sloan_stats_bdd_static.csv')
    try_load_data(('ptri', 'sl-static-ldd'), data_folder + 'petrinets_sloan_stats_ldd_static.csv')

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
        its_data = pd.read_csv('bench_data/for_paper/reach-vs-its/its/its_tools_petrinets_deadlocks.csv')
    else:
        its_data = pd.read_csv('bench_data/for_paper/reach-vs-its/its/its_tools_petrinets.csv')

    # some pre-processing
    its_data.columns = its_data.columns.str.strip()
    its_data = its_data.astype({"benchmark" : str, "type" : str,
                                "reach_time" : float,
                                "final_states" : float})
    its_data['type'] = its_data['type'].str.strip()

    return its_data

def load_pnml_encode_data(data_folder):
    # load csv
    data = pd.read_csv(f'{data_folder}/reach/pnml_encode_time_ldd_hmorph.csv')

    # some pre-processing
    data.columns = data.columns.str.strip()
    data = data.replace(' DNF', np.inf)
    data = data.replace(' -', np.inf)
    data = data.astype({"benchmark" : str, 
                        "encode_time" : float,
                        "ddmc_time" : float})
    return data

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
                    if (set_incorrect_as_timeout):
                        # removed entries are plotted as timeouts
                        condition = df['strategy'] == stratIDs['rec']
                        condition |= (df['strategy'] == stratIDs['rec-copy'])
                        condition &= (df['benchmark'] == row['benchmark'])
                        df.drop(df[condition].index, inplace=True)

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


def _plot_time_comp(compare, ax, x_dd, y_dd, x_select, y_select, z_select=None, 
                    min_time=0, join_type='outer', add_merge_time=True):
    """
    The actual plotting of the runtime comparisons happens here.
    compare = [strat | cores]
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
            print(f"Could not find data for '{ds_name}', skipping...")
            continue

        # select subsets of x and y data
        group_x = x_data.loc[x_data['reach_time'] >= min_time]
        group_y = y_data.loc[y_data['reach_time'] >= min_time]
        if (compare == 'strat'):
            group_x = group_x.loc[group_x['strategy'] == stratIDs[x_select]]
            group_y = group_y.loc[group_y['strategy'] == stratIDs[y_select]]
        elif (compare == 'cores'):
            group_x = group_x.loc[group_x['strategy'] == stratIDs[z_select]]
            group_y = group_y.loc[group_y['strategy'] == stratIDs[z_select]]
            group_x = group_x.loc[group_x['workers'] == x_select]
            group_y = group_y.loc[group_y['workers'] == y_select]
        else:
            print(f"ERROR: Argument 'compare' must be [strat|cores]")
            exit(1)

        # outer join of x and y
        group_y = group_y.set_index('benchmark')
        joined  = group_x.join(group_y, on='benchmark', how=join_type,
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


def _plot_diagonal_lines(ax, max_val, min_val, at=[0.1, 10]):
    """
    Add diagonal lines to ax
    """
    
    # bit of margin for vizualization
    ax.set_xlim([min_val-0.15*min_val, max_val+0.15*max_val])
    ax.set_ylim([min_val-0.15*min_val, max_val+0.15*max_val])

    # diagonal lines
    ax.plot([min_val, max_val], [min_val, max_val], ls="--", c="gray")
    for k in at:
        ax.plot([min_val, max_val], [min_val*k, max_val*k], ls=":", c="#767676")

    return ax

def plot_comparison(x_strat, x_dd, y_strat, y_dd, add_merge_time, xlabel='', ylabel=''):
    info("plotting {} ({}) vs {} ({})".format(x_strat, x_dd, y_strat, y_dd))

    scaling = 5.0 # default = ~6.0
    fig, ax = plt.subplots(figsize=(scaling, scaling*0.75))

    # actually fills in the subplot
    ax, meta = _plot_time_comp('strat', ax, x_dd, y_dd, x_strat, y_strat, add_merge_time=add_merge_time)
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
    fig_name = '{}reachtime_{}_{}_vs_{}_{}_incMergeTime{}.{}'.format(label_folder,
                                                      x_strat, x_dd,
                                                      y_strat, y_dd,
                                                      add_merge_time,
                                                      'pdf')
    fig.savefig(fig_name, dpi=300)
    plt.close(fig)


def plot_comparison_sbs(x1_strat, x1_dd, y1_strat, y1_dd,
                        x2_strat, x2_dd, y2_strat, y2_dd,
                        x_label, y_label, add_merge_time):
    info("plotting")
    info(" left:  {} ({}) vs {} ({})".format(x1_strat, x1_dd, y1_strat, y1_dd))
    info(" right: {} ({}) vs {} ({})".format(x2_strat, x2_dd, y2_strat, y2_dd))

    scaling = 4.8 # default = ~6.0
    w = 1.6 # relative width
    fig, axs = plt.subplots(1, 2, sharey=True, figsize=(w*scaling, scaling*0.75))

    # actually fill in the subplots
    axs[0], meta0 = _plot_time_comp('strat', axs[0], x1_dd, y1_dd, x1_strat, y1_strat, add_merge_time=add_merge_time)
    axs[1], meta1 = _plot_time_comp('strat', axs[1], x2_dd, y2_dd, x2_strat, y2_strat, add_merge_time=add_merge_time)

    # diagonal lines
    max_val = max(meta0['max_val'], meta1['max_val'])
    min_val = min(meta0['min_val'], meta1['min_val'])
    axs[0] = _plot_diagonal_lines(axs[0], max_val, min_val)
    axs[1] = _plot_diagonal_lines(axs[1], max_val, min_val)

    # axis labels + legend
    axs[0].set_ylabel(y_label, fontsize=12)
    axs[0].set_title('{}s'.format(x1_dd[-3:].upper()))
    axs[1].set_title('{}s'.format(x2_dd[-3:].upper()))
    axs[1].axes.yaxis.set_visible(False)
    axs[0].legend(framealpha=1.0, fontsize=11)
    fig.text(0.54, 0.02, x_label, ha='center')
    plt.tight_layout(pad=0.2)
    fig.subplots_adjust(bottom=0.15)

    # plot for all formats
    info(f"Writing plots to {plots_folder}")
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


def plot_rec_over_sat_vs_rel_metric(data_label, metric, strat_rec, add_merge_time=False):
    info("plotting rec/sat against {} on {}".format(metric, data_label), end='')
    if (metric[:3] == 'rel'):
        info(" (takes some time)")
    
    scaling = 3.5 # default = ~6.0
    fig, ax = plt.subplots(figsize=(scaling, scaling*0.70))
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
        data_rec = data.loc[data['strategy'] == stratIDs[strat_rec]]

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
        if (False): # hack for quickly redoing the plot
            if (os.path.exists('matrix_metric_{}.npy'.format(ds_name))):
                print("loading from matrix_metric_{}.npy".format(ds_name))
                matrix_metrics = np.load('matrix_metric_{}.npy'.format(ds_name))
            else:
                matrix_metrics = get_matrix_metrics(joined['benchmark'], 
                                                (ds_name, data_label), metric)
                print("saving to matrix_metric_{}.npy".format(ds_name))
                np.save('matrix_metric_{}.npy'.format(ds_name), matrix_metrics)
        else:
            matrix_metrics = get_matrix_metrics(joined['benchmark'], 
                                                (ds_name, data_label), metric)

        # some styling
        s = marker_size[ds_name]
        m = markers[ds_name]
        #l = legend_names[ds_name]

        # plot x vs y
        ax.scatter(matrix_metrics, relative_rec_times, marker=m, s=s) # , label=l

        # track for annotations (and trend line)
        all_xs.extend(matrix_metrics)
        all_ys.extend(relative_rec_times)
        all_names.extend(joined['benchmark'])

    # labels and formatting
    ax.hlines(1, 0, 1, colors=['grey'], linestyles='--')
    ax.set_xlabel(metric_labels[metric]) #, fontsize = ...
    if (data_label[-3:] == 'bdd'):
        ax.set_ylabel('time ReachBDD / saturation') #, fontsize = ...
    elif (data_label[-3:] == 'ldd'):
        ax.set_ylabel('time ReachMDD / saturation')
    ax.set_yscale('log')
    ax.legend(framealpha=1.0) # , prop={"size":16}
    plt.tight_layout()

    # plots without data-point lables
    info("Writing plots to {}".format(plots_folder))
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


def _plot_parallel(strat1, strat2, strat3, data_label, min_time, plot_legend=True, add_y_label=True):
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
    fig, ax = plt.subplots(figsize=(scaling*0.85, scaling*0.60*width)) # *0.70
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
    if (add_y_label):
        ax.set_ylabel('speedup')
    ax.tick_params(axis='x', which='both',length=0)
    if (plot_legend):
        ax.legend([bplot["boxes"][0], bplot["boxes"][1], bplot["boxes"][2]], 
                ['Parallel saturation', 'ReachBDD', 'ReachBDD-par'],
                loc='upper left', framealpha=1.0)
    plt.tight_layout()

    # plots without data-point lables
    info("Writing plots to {}".format(plots_folder))
    for fig_format in fig_formats:
        subfolder = plots_folder.format(fig_format)
        fig_name = '{}speedups_{}_{}_{}_on{}_mintime_{}_{}_cores.{}'.format(subfolder, 
                    strat1, strat2, strat3, data_label, min_time, max(n_workers), fig_format)
        fig.savefig(fig_name, dpi=300)
    plt.close(fig)


def _plot_parallel_scatter(x_cores, y_cores, strat, min_time, add_merge_time, 
                           write_speedups_only=False):

    scaling = 4.0 # default = ~6.0
    fig, ax = plt.subplots(figsize=(scaling, scaling*0.75))
    
    ax, meta = _plot_time_comp('cores', ax, 'sl-bdd', 'sl-bdd', 
                                x_cores, y_cores, strat, 
                                min_time=min_time, 
                                join_type='inner',
                                add_merge_time=add_merge_time)
    
    if (write_speedups_only):
        speedups = np.array(meta['xs']) / np.array(meta['ys'])
        print(f"\tP50   = {np.percentile(speedups, 50)}")
        print(f"\tP95   = {np.percentile(speedups, 95)}")
        print(f"\tP99   = {np.percentile(speedups, 99)}")
        print(f"\tP99.5 = {np.percentile(speedups, 99.5)}")
        return
    

    # plot diagonal lines
    ax = _plot_diagonal_lines(ax, meta['max_val'], meta['min_val'], at=[x_cores/y_cores])

    # set axis labels + legend
    _xc = 'core' if x_cores == 1 else 'cores'
    _yc = 'core' if y_cores == 1 else 'cores'
    xlabel = '{} time (s), {} {}'.format(axis_label[(strat,'bdd')], x_cores, _xc)
    ylabel = '{} time (s), {} {}'.format(axis_label[(strat,'bdd')], y_cores, _yc)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.legend(framealpha=1.0)
    plt.tight_layout()

    # plots without data-point lables
    for fig_format in fig_formats:
        subfolder = plots_folder.format(fig_format)
        fig_name = '{}reachtime_{}_{}cores_vs_{}cores_incMergeTime{}.{}'.format(subfolder,
                                                          strat, x_cores, y_cores,
                                                          add_merge_time,
                                                          fig_format)
        fig.savefig(fig_name, dpi=300)

    # add data-point labes and plot as pdf
    for i, bench_name in enumerate(meta['names']):
        ax.annotate(bench_name, (meta['xs'][i], meta['ys'][i]), fontsize=1.0)
    fig_name = '{}reachtime_{}_{}cores_vs_{}cores_incMergeTime{}.{}'.format(label_folder,
                                                      strat, x_cores, y_cores,
                                                      add_merge_time,
                                                      'pdf')
    fig.savefig(fig_name, dpi=300)
    plt.close(fig)


def _plot_parallel_scatter_sbs(x_cores, y_cores, strat0, strat1, min_time, add_merge_time, plot_half='both'):
    """
    plot_half = [top | bottom | both]
    """

    if (plot_half == 'both'):
        scaling = 4.8 # default = ~6.0
        w = 1.6 # relative width
    elif (plot_half == 'top'):
        scaling = 4.6
        w = 1.7
    elif (plot_half == 'bottom'):
        scaling = 4.6
        w = 1.7
    fig, axs = plt.subplots(1, 2, sharey=True, figsize=(w*scaling, scaling*0.75))

    # actually fill in the subplots
    axs[0], meta0 = _plot_time_comp('cores', axs[0], 'sl-bdd', 'sl-bdd', 
                                    x_cores, y_cores, strat0,
                                    min_time=min_time,
                                    join_type='inner',
                                    add_merge_time=add_merge_time)
    axs[1], meta1 = _plot_time_comp('cores', axs[1], 'sl-bdd', 'sl-bdd', 
                                    x_cores, y_cores, strat1,
                                    min_time=min_time,
                                    join_type='inner',
                                    add_merge_time=add_merge_time)
    
    # plot diagonal lines
    max_val = max(meta0['max_val'], meta1['max_val'])
    min_val = min(meta0['min_val'], meta1['min_val'])
    axs[0] = _plot_diagonal_lines(axs[0], max_val, min_val, at=[x_cores/y_cores])
    axs[1] = _plot_diagonal_lines(axs[1], max_val, min_val, at=[x_cores/y_cores])

    # set axis labels+ legend
    _xc = 'core' if x_cores == 1 else 'cores'
    _yc = 'core' if y_cores == 1 else 'cores'
    x_label = 'Reachability time (s), {} {}'.format(x_cores, _xc)
    y_label = 'Reachability time (s), {} {}'.format(y_cores, _yc)
    if (plot_half == 'top'):
        axs[0].set_xticks([])
        axs[1].set_xticks([])
    if (plot_half == 'bottom' or plot_half == 'both'):
        fig.text(0.54, 0.01, x_label, ha='center', fontsize=12)
    axs[0].set_ylabel(y_label, fontsize=12)
    axs[1].axes.yaxis.set_visible(False)
    if (plot_half == 'top' or plot_half == 'both'):
        axs[0].set_title(axis_label[(strat0, 'bdd')])
        axs[1].set_title(axis_label[(strat1, 'bdd')])
        axs[0].legend(framealpha=1.0, fontsize=11)
    if (plot_half == 'top'):
        plt.tight_layout(pad=0.2)
    elif (plot_half == 'bottom'):
        plt.tight_layout(pad=0.2)
    else:
        plt.tight_layout(pad=0.2)
    if (plot_half == 'bottom' or plot_half == 'both'):
        fig.subplots_adjust(bottom=0.15)

    # plots without data-point lables
    for fig_format in fig_formats:
        subfolder = plots_folder.format(fig_format)
        fig_name = '{}reachtime_{}_and_{}_{}cores_vs_{}cores_incMergeTime{}_{}.{}'.format(subfolder,
                                                          strat0, strat1, 
                                                          x_cores, y_cores,
                                                          add_merge_time,
                                                          plot_half,
                                                          fig_format)
        fig.savefig(fig_name, dpi=300)

def _plot_parallel_scatter_2x2(x_cores, y_cores0, y_cores1, strat0, strat1, min_time, add_merge_time):
    
    scaling = 5.5 # default = ~6.0
    w = 1.1
    fig, axs = plt.subplots(2, 2, sharey=True, sharex=True, figsize=(w*scaling, scaling))

    # actually fill in the subplots
    axs[0,0], meta00 = _plot_time_comp('cores', axs[0,0], 'sl-bdd', 'sl-bdd', 
                                       x_cores, y_cores0, strat0,
                                       min_time=min_time,
                                       join_type='inner',
                                       add_merge_time=add_merge_time)
    axs[0,1], meta01 = _plot_time_comp('cores', axs[0,1], 'sl-bdd', 'sl-bdd', 
                                       x_cores, y_cores0, strat1,
                                       min_time=min_time,
                                       join_type='inner',
                                       add_merge_time=add_merge_time)
    axs[1,0], meta10 = _plot_time_comp('cores', axs[1,0], 'sl-bdd', 'sl-bdd', 
                                       x_cores, y_cores1, strat0,
                                       min_time=min_time,
                                       join_type='inner',
                                       add_merge_time=add_merge_time)
    axs[1,1], meta11 = _plot_time_comp('cores', axs[1,1], 'sl-bdd', 'sl-bdd', 
                                       x_cores, y_cores1, strat1,
                                       min_time=min_time,
                                       join_type='inner',
                                       add_merge_time=add_merge_time)
    
    # plot diagonal lines
    max_val = max(meta00['max_val'], meta01['max_val'])
    min_val = min(meta00['min_val'], meta01['min_val'])
    axs[0,0] = _plot_diagonal_lines(axs[0,0], max_val, min_val, at=[x_cores/y_cores0])
    axs[0,1] = _plot_diagonal_lines(axs[0,1], max_val, min_val, at=[x_cores/y_cores0])
    axs[1,0] = _plot_diagonal_lines(axs[1,0], max_val, min_val, at=[x_cores/y_cores1])
    axs[1,1] = _plot_diagonal_lines(axs[1,1], max_val, min_val, at=[x_cores/y_cores1])
    
    # labels and styling
    # set axis labels+ legend
    _xc  = 'core' if x_cores == 1 else 'cores'
    _yc0 = 'core' if y_cores0 == 1 else 'cores'
    _yc1 = 'core' if y_cores1 == 1 else 'cores'
    x_label  = 'Reachability time (s), {} {}'.format(x_cores, _xc)
    y_label0 = 'time (s), {} {}'.format(y_cores0, _yc0)
    y_label1 = 'time (s), {} {}'.format(y_cores1, _yc1)
    fig.text(0.54, 0.01, x_label, ha='center', fontsize=11)
    axs[0,0].axes.xaxis.set_visible(False)
    axs[0,1].axes.xaxis.set_visible(False)
    axs[0,1].axes.yaxis.set_visible(False)
    axs[1,1].axes.yaxis.set_visible(False)
    axs[0,0].set_ylabel(y_label0, fontsize=11)
    axs[1,0].set_ylabel(y_label1, fontsize=11)
    axs[0,0].set_title(axis_label[(strat0, 'bdd')])
    axs[0,1].set_title(axis_label[(strat1, 'bdd')])
    axs[0,0].legend(framealpha=1.0, fontsize=10)
    plt.tight_layout(pad=0.2)
    plt.subplots_adjust(wspace=0.02, hspace=0.02)
    fig.subplots_adjust(bottom=0.09)

    # plots without data-point lables
    for fig_format in fig_formats:
        subfolder = plots_folder.format(fig_format)
        fig_name = '{}reachtime_par_2x2_cores_incMergeTime{}.{}'.format(subfolder,
                                                          add_merge_time,
                                                          fig_format)
        fig.savefig(fig_name, dpi=300)


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


def plot_its_vs_dd(data_folder, its_type):
    data_its = load_its_data(its_type)
    data_dd  = load_pnml_encode_data(data_folder)
    
    # select relevant subset
    data_its = data_its.loc[data_its['type'] == its_type]
    its_zero_time = data_its.loc[data_its['reach_time'] == 0]
    data_its = data_its.loc[data_its['reach_time'] != 0]

    # inner join
    data_its = data_its.set_index('benchmark')
    joined = data_dd.join(data_its, on='benchmark', how='outer',
                            lsuffix='_dd', rsuffix='_its')
    
    """
    if ('final_states_its' in joined):
        neq_states = joined['final_states_dd'] != joined['final_states_its']
        if (sum(neq_states) > 0):
            print("WARNING: ITS-tools and Sylvan disagree on # states for:")
            issue_cases = joined.loc[neq_states]
            print(issue_cases)
    """
    
    xs = joined['reach_time'].to_numpy() # NOTE: output time from its-tools, not real wall time
    ys = joined['encode_time'].to_numpy() + joined['ddmc_time'].to_numpy()
    ys[np.isinf(ys)] = np.nan

    # some styling
    scaling = 3.2 # default = ~6.0
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

    # at_least_one_finishes == all - both_timout
    at_least_one_finishes = len(ys) - np.sum(xs == ys)
    print(f"# of instances where at least one finishes: {at_least_one_finishes} ")
    print(f"REACH faster: {(np.sum(xs > ys) / at_least_one_finishes)*100}% of instances")
    print(f"ITS faster:   {(np.sum(xs < ys) / at_least_one_finishes)*100}% of instances")

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
    ax.set_ylabel("pnml-encode + ReachMDD")
    ax.set_xlabel("ITS-tools")
    ax.set_xlim([min_val-0.15*min_val, max_val+0.15*max_val])
    ax.set_ylim([min_val-0.15*min_val, max_val+0.15*max_val])
    ax.legend(framealpha=1.0)
    plt.tight_layout()

    info(f"Writing plots to {plots_folder}")
    for fig_format in fig_formats:
        subfolder = plots_folder.format(fig_format)
        fig_name = '{}its{}_vs_{}_{}.{}'.format(subfolder,
                                            its_type.replace('.','_'), 
                                            'ldd', 
                                            'rec',
                                            fig_format)
        fig.savefig(fig_name, dpi=300)
    
    # add data-point labels and plot as pdf
    for i, bench_name in enumerate(all_names):
        ax.annotate(bench_name, (xs[i], ys[i]), fontsize=1.0)
    fig_name = fig_name = '{}its{}_vs_{}_{}.{}'.format(label_folder,
                                            its_type.replace('.','_'), 
                                            'ldd', 
                                            'rec',
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

def plot_paper_plot_sat_vs_rec_copy(data_folder):
    load_data(data_folder)
    pre_process()
    assert_states_nodes()

    set_subfolder_name('all/Saturation vs REACH (Figure 4)')
    plot_comparison_sbs('sat', 'sl-bdd', 'rec', 'sl-bdd', 
                        'sat', 'sl-ldd', 'rec-copy', 'sl-ldd', 
                        'Saturation time (s)', 'ReachBDD/MDD time (s)', 
                        add_merge_time=True)

def plot_copy_nodes_test(subfolder):
    set_subfolder_name(subfolder + '/Copy Nodes Test')

    # all on '2lts-sym' LDDs
    # Saturation 'normal' vs REACH 'overapprox' (old setup)
    plot_comparison('sat', 'sl-ldd', 'rec', 'sl-ldd', True,
                    xlabel='Saturation on "normal" LDDs',
                    ylabel='REACH on "-2r,r2+,w2+" LDDs')
    plot_comparison('sat', 'sl-ldd', 'rec', 'sl-ldd', False,
                    xlabel='Saturation on "normal" LDDs',
                    ylabel='REACH on "-2r,r2+,w2+" LDDs')
    
    # Saturation 'normal' vs REACH 'normal' (extend w/ copy nodes in lddmc)
    plot_comparison('sat', 'sl-ldd', 'rec-copy', 'sl-ldd', True,
                    xlabel='Saturation on "normal" LDDs',
                    ylabel='REACH on "normal" LDDs (extended w/ copy nodes)')
    plot_comparison('sat', 'sl-ldd', 'rec-copy', 'sl-ldd', False,
                    xlabel='Saturation on "normal" LDDs',
                    ylabel='REACH on "normal" LDDs (extended w/ copy nodes)')
    
    # REACH 'overapprox' vs REACH 'normal' (extend w/ copy nodes in lddmc)
    plot_comparison('rec', 'sl-ldd', 'rec-copy', 'sl-ldd', True,
                    xlabel='REACH on "-2r,r2+,w2+" LDDs',
                    ylabel='REACH on "normal" LDDs (extended w/ copy nodes)')
    plot_comparison('rec', 'sl-ldd', 'rec-copy', 'sl-ldd', False,
                    xlabel='REACH on "-2r,r2+,w2+" LDDs',
                    ylabel='REACH on "normal" LDDs (extended w/ copy nodes)')

def plot_right_recursion_test(subfolder):
    set_subfolder_name(subfolder + '/Right Recursion Test')

    # BFS (w/ copy nodes) loop vs recursive
    plot_comparison('bfs-plain-copy', 'sl-ldd', 'bfs-plain-copy-rr', 'sl-ldd', True,
                    xlabel='BFS w/ copy nodes + IMG LOOP',
                    ylabel='BFS w/ copy nodes + IMG RECURSIVE')
    plot_comparison('bfs-plain-copy', 'sl-ldd', 'bfs-plain-copy-rr', 'sl-ldd', False,
                    xlabel='BFS w/ copy nodes + IMG LOOP',
                    ylabel='BFS w/ copy nodes + IMG RECURSIVE')

    # REACH (w/ copy nodes) loop vs recursive
    plot_comparison('rec-copy', 'sl-ldd', 'rec-copy-rr', 'sl-ldd', True,
                    xlabel='REACH w/ copy nodes, LOOP',
                    ylabel='REACH w/ copy nodes, RECURSIVE')
    plot_comparison('rec-copy', 'sl-ldd', 'rec-copy-rr', 'sl-ldd', False,
                    xlabel='REACH w/ copy nodes, LOOP',
                    ylabel='REACH w/ copy nodes, RECURSIVE')


def plot_static_vs_otf(data_folder):
    load_data(data_folder)
    pre_process()
    assert_states_nodes()
    
    set_subfolder_name('all/Static (with hmorphn nodes) vs On-The-Fly ')
    plot_comparison('rec-copy', 'sl-ldd', 'rec-copy', 'sl-static-ldd', True,
                    xlabel='REACH time (s) on LTSmin on-the-fly LDDs',
                    ylabel='REACH time (s) on pnml-encode')
    plot_comparison('rec-copy', 'sl-ldd', 'rec-copy', 'sl-static-ldd', False,
                    xlabel='REACH time (s) on LTSmin on-the-fly LDDs',
                    ylabel='REACH time (s) on pnml-encode LDDs')


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
    set_subfolder_name(subfolder + '/Locality metric comparison (Figure 6)')
    plot_rec_over_sat_vs_rel_metric('sl-ldd', 'rel-avg-bw', 'rec-copy', add_merge_time)
    plot_rec_over_sat_vs_rel_metric('sl-bdd', 'rel-avg-bw', 'rec', add_merge_time)
    #plot_rec_over_sat_vs_rel_metric('sl-ldd', 'rel-max-bw', 'rec-copy', add_merge_time)
    #plot_rec_over_sat_vs_rel_metric('sl-bdd', 'rel-max-bw', 'rec', add_merge_time)
    #plot_rec_over_sat_vs_rel_metric('sl-ldd', 'var-avg-bw', 'rec-copy', add_merge_time)
    #plot_rec_over_sat_vs_rel_metric('sl-bdd', 'var-avg-bw', 'rec', add_merge_time)
    #plot_rec_over_sat_vs_rel_metric('sl-ldd', 'var-max-bw', 'rec-copy', add_merge_time)
    #plot_rec_over_sat_vs_rel_metric('sl-bdd', 'var-max-bw', 'rec', add_merge_time)


def plot_paper_plot_parallel(subfolder, plot_legend=True, ylabel=True):
    set_subfolder_name(subfolder + '/Parallel speedups (Figure 7)')
    if subfolder == 'subset':
        _plot_parallel('sat', 'rec', 'rec-par', 'sl-bdd', 0, plot_legend, ylabel)
    else:
        _plot_parallel('sat', 'rec', 'rec-par', 'sl-bdd', 1, plot_legend, ylabel)


def plot_paper_plot_merge_overhead(subfolder):
    set_subfolder_name(subfolder + '/Merge overhead (New Figure)')
    plot_merge_overhead('sl-bdd')
    plot_merge_overhead('sl-ldd')


def plot_paper_plot_its_tools_vs_dds(data_folder):
    set_subfolder_name('all/ITS-Tools vs LDDs with hmorph nodes')
    plot_its_vs_dd(data_folder, '.gal')
    #plot_its_vs_dd('.img.gal')
    #plot_its_vs_dd_deadlocks('sl-bdd', 'rec', 'RD')


def plot_both_parallel(data_folder):
    if(load_data(data_folder, expected=3)):
        pre_process()
        assert_states_nodes()
        plot_paper_plot_parallel('all', plot_legend=False)
    else:
        print('no complete data found in ' + data_folder)
    
    # Plot parallel (Figure 11)
    data_folder = 'bench_data/all/par_96/'
    if(load_data(data_folder, expected=1)):
        pre_process()
        assert_states_nodes()
        plot_paper_plot_parallel('all', ylabel=False)
    else:
        print('no complete data found in ' + data_folder)


def plot_all_parallel_scatter(data_folder, plot_cores):
    load_data(data_folder)
    pre_process()
    assert_states_nodes()
    set_subfolder_name('all/Parallel speedups scatter')

    min_time = 0.1

    print(f"Writing plots to {plots_folder}")
    if len(plot_cores) == 2:
        _plot_parallel_scatter_2x2(1, plot_cores[1], plot_cores[0], 'sat', 'rec-par', min_time, False)
    elif len(plot_cores) == 1:
        #_plot_parallel_scatter_sbs(1, plot_cores[0], 'sat', 'rec-par', min_time, True)
        _plot_parallel_scatter_sbs(1, plot_cores[0], 'sat', 'rec-par', min_time, False)
        #_plot_parallel_scatter_sbs(1, plot_cores[0], 'sat', 'rec-par', min_time, False, 'top')
        #_plot_parallel_scatter_sbs(1, plot_cores[0], 'sat', 'rec-par', min_time, False, 'bottom')

    for cores in plot_cores:
        for alg in ['sat', 'rec-par']:
            print(f"Speedup for {alg} with {cores} cores at Percentiles: ")
            _plot_parallel_scatter(1, cores, alg, min_time, False, True)

def plot_all_locality_plots(data_folder):
    load_data(data_folder)
    pre_process()
    assert_states_nodes()
    plot_paper_plot_locality('all', add_merge_time=False)

def plot_paper_plots(subfolder='all'):   
    # Plot Fig 9, Fig 10, New Fig
    data_folder = 'bench_data/'+ subfolder + '/single_worker/10m/20220419_164504/'
    load_data(data_folder)
    pre_process()
    assert_states_nodes()

    # Plot saturation vs REACH on Sloan BDDs/LDDs (Figure 9)
    #plot_paper_plot_sat_vs_rec(subfolder, add_merge_time) # /single_worker/10m/20220419_164504/
#    plot_paper_plot_sat_vs_rec_copy(subfolder)

    #plot_pnml_encode_tests(subfolder) # '/single_worker/10m/pnml-encode/'
    #plot_copy_nodes_test(subfolder) # '/single_worker/10m/20220407_191756/'
    #plot_right_recursion_test(subfolder) # '/custom_image_test/20220411_210135/'
    # Plot locality metric correlation (Figure 10) (on same data)

    # Plot relative merge time overhead (New Figure)
    #plot_paper_plot_merge_overhead(subfolder)



if __name__ == '__main__':
    which_plot, data_folder, plot_cores = parse_args()
    if (which_plot == 'saturation'): # bench_data/all/single_worker/10m/bdds_and_ldds_w_copy/
        plot_paper_plot_sat_vs_rec_copy(data_folder)
    elif (which_plot == 'parallel'): # bench_data/all/par_8/2m/
        plot_both_parallel(data_folder)
    elif (which_plot == 'parallel-scatter'):
        plot_all_parallel_scatter(data_folder, plot_cores)
    elif (which_plot == 'locality'): # bench_data/all/single_worker/10m/bdds_and_ldds_w_copy/
        plot_all_locality_plots(data_folder)
    elif (which_plot == 'its'):
        plot_paper_plot_its_tools_vs_dds(data_folder)
    elif (which_plot == 'static'):
        plot_static_vs_otf(data_folder)
    else:
        print("First argument must be [saturation|parallel|parallel-scatter|locality|its|static]")
    

