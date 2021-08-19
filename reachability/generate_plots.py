import numpy as np
import pandas as pd
from pathlib import Path
import matplotlib.pyplot as plt

fig_formats = ['png', 'pdf', 'eps']
plots_folder = 'plots/{}/' # output in plots/fig_format/
label_folder = 'plots/labeled/' # for plots with labels for all data-points
data_folder  = 'bench_data/'

beem_vanilla = pd.read_csv(data_folder + 'beem_vanilla_stats.csv')
beem_ga      = pd.read_csv(data_folder + 'beem_ga_stats.csv')
ptri_vanilla = pd.read_csv(data_folder + 'petrinets_vanilla_stats.csv')
ptri_ga      = pd.read_csv(data_folder + 'petrinets_ga_stats.csv')
prom_vanilla = pd.read_csv(data_folder + 'promela_vanilla_stats.csv')
prom_ga      = pd.read_csv(data_folder + 'promela_ga_stats.csv')
datasetnames = ['beem', 'ptri', 'prom']
legend_names = {'beem' : 'dve', 'ptri' : 'petrinets', 'prom' : 'promela'}
datamap      = {('beem','vanilla') : beem_vanilla, ('beem', 'ga') : beem_ga, 
                ('ptri','vanilla') : ptri_vanilla, ('ptri', 'ga') : ptri_ga,
                ('prom','vanilla') : prom_vanilla, ('prom', 'ga') : prom_ga}
stratIDs     = {'bfs' : 0,
                'sat' : 2,
                'rec' : 4}
axis_label = {'bfs' : 'BFS',
              'sat' : 'saturation',
              'rec' : 'recursive'}

verbose = True

def info(str):
    if (verbose):
        print(str)

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
    n_states = {} # dict : benchmark -> number of final states
    for df in datamap.values():
        n_nodes  = {} # dict : benchmark -> number of final nodes
        for index, row in df.iterrows():
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
data in {'vanilla', 'ga'}
"""
def plot_comparison(x_strat, x_data_label, y_strat, y_data_label):
    info("plotting {} ({}) vs {} ({})".format(x_strat, x_data_label, 
                                              y_strat, y_data_label))

    scaling = 5.0 # default = ~6.0
    fig, ax = plt.subplots(figsize=(scaling, scaling*0.75))
    point_size = 8.0
    label_font_size = 1.0

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
        group_x.set_index('benchmark')
        group_y = group_y.set_index('benchmark')

        # inner join x and y
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
        max_val = max(max_val, max(np.max(xs), np.max(ys)))
        min_val = min(min_val, min(np.min(xs), np.min(ys)))

    # diagonal line
    ax.plot([min_val, max_val], [min_val, max_val], ls="--", c="gray")

    # labels and formatting
    ax.set_xlabel('{} time (s)'.format(axis_label[x_strat]))
    ax.set_ylabel('{} time (s)'.format(axis_label[y_strat]))
    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlim([min_val-0.15*min_val, max_val+0.15*max_val])
    ax.set_ylim([min_val-0.15*min_val, max_val+0.15*max_val])
    legend = ax.legend(framealpha=1.0)
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
        ax.annotate(bench_name, (all_xs[i], all_ys[i]), fontsize=label_font_size)
    fig_name = '{}reachtime_{}_{}_vs_{}_{}.{}'.format(label_folder,
                                                      x_strat, x_data_label,
                                                      y_strat, y_data_label,
                                                      'pdf')
    fig.savefig(fig_name, dpi=300)

def plot_things():
    plot_comparison('bfs', 'vanilla', 'sat', 'vanilla')
    plot_comparison('bfs', 'ga',      'sat', 'ga')
    plot_comparison('bfs', 'vanilla', 'rec', 'vanilla')
    plot_comparison('bfs', 'ga',      'rec', 'ga')
    plot_comparison('sat', 'vanilla', 'rec', 'vanilla')
    plot_comparison('sat', 'ga',      'rec', 'ga')
    plot_comparison('bfs', 'vanilla', 'bfs', 'ga')
    plot_comparison('sat', 'vanilla', 'sat', 'ga')
    plot_comparison('rec', 'vanilla', 'rec', 'ga')

if __name__ == '__main__':
    pre_process()
    assert_states_nodes()
    plot_things()

