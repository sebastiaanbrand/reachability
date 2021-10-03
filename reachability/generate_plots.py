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
plots_folder = 'plots/{}/' # output in plots/fig_format/
label_folder = 'plots/labeled/' # for plots with labels for all data-points
data_folder  = 'bench_data/'

datamap = {} # map from ('dataset','type') -> dataframe

datasetnames = ['beem']#, 'ptri', 'prom']
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

"""
load data from predefined filenames if those file exist
"""
def load_data():
    try_load_data(('beem','vn'), data_folder + 'beem_vanilla_stats.csv')
    try_load_data(('beem','sl'), data_folder + 'beem_sloan_stats.csv')
    try_load_data(('ptri','vn'), data_folder + 'petrinets_vanilla_stats.csv')
    try_load_data(('ptri','sl'), data_folder + 'petrinets_sloan_stats.csv')
    try_load_data(('prom','vn'), data_folder + 'promela_vanilla_stats.csv')
    try_load_data(('prom','sl'), data_folder + 'promela_sloan_stats.csv')
    try_load_data(('beem','vn-ldd'), data_folder + 'beem_vanilla_stats_ldd.csv')
    try_load_data(('beem','sl-ldd'), data_folder + 'beem_sloan_stats_ldd.csv')


def pre_process():
    # make plots folders if necessary
    for fig_format in fig_formats:
        Path(plots_folder.format(fig_format)).mkdir(parents=True, exist_ok=True)
    Path(label_folder).mkdir(parents=True, exist_ok=True)

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

"""
strat in {'bfs', 'sat', 'rec'}
data in {'vn', 'ga'}
"""
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

def plot_things():
    #plot_comparison_shared_y('bfs', 'ga', 'sat', 'ga', 'rec', 'ga')
    #plot_comparison_sbs('sat', 'vn', 'sat', 'ga',
    #                    'rec', 'vn', 'rec', 'ga', 
    #                    'time with default grouping (s)',
    #                    'time with \'ga\' grouping (s)')
    #plot_comparison('bfs', 'vn', 'sat', 'vn')
    #plot_comparison('bfs', 'ga', 'sat', 'ga')
    #plot_comparison('bfs', 'vn', 'rec', 'vn')
    #plot_comparison('bfs', 'ga', 'rec', 'ga')
    #plot_comparison('sat', 'vn', 'rec', 'vn')
    #plot_comparison('sat', 'ga', 'rec', 'ga')
    #plot_comparison('bfs', 'vn', 'bfs', 'ga')
    #plot_comparison('sat', 'vn', 'sat', 'ga')
    #plot_comparison('rec', 'vn', 'rec', 'ga')
    #plot_selection('sat', 'ga', 'rec', 'ga')
    #plot_comparison_shared_y('bfs', 'vn-ldd', 'sat', 'vn-ldd', 'rec', 'vn-ldd')
    plot_comparison_shared_y('bfs', 'sl-ldd', 'sat', 'sl-ldd', 'rec', 'sl-ldd')

if __name__ == '__main__':
    load_data()
    pre_process()
    assert_states_nodes()
    plot_things()

