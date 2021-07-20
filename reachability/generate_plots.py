import numpy as np
import pandas as pd
import pathlib
import matplotlib.pyplot as plt

fig_formats = ['png', 'pdf']
plots_folder = 'plots/'
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

def pre_process():
    # make plots folder if necessary
    pathlib.Path(plots_folder).mkdir(parents=True, exist_ok=True)

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

    fig, ax = plt.subplots()
    max_val = 0
    min_val = 1e9
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
        ax.scatter(xs, ys, s=10, label=legend_names[ds_name])

        # max and min for diagonal line
        max_val = max(max_val, max(np.max(xs), np.max(ys)))
        min_val = min(min_val, min(np.min(xs), np.min(ys)))

    # diagonal line
    ax.plot([min_val, max_val], [min_val, max_val], ls="--", c="gray")

    # labels and formatting
    ax.set_xlabel('time (s) - {} on {} BDDs'.format(x_strat, x_data_label))
    ax.set_ylabel('time (s) - {} on {} BDDs'.format(y_strat, y_data_label))
    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlim([min_val-0.15*min_val, max_val+0.15*max_val])
    ax.set_ylim([min_val-0.15*min_val, max_val+0.15*max_val])
    ax.legend()
    ax.set_title('reachability runtime comparison')

    for fig_format in fig_formats:
        fig_name = '{}reachtime_{}_{}_vs_{}_{}.{}'.format(plots_folder,
                                                          x_strat, x_data_label,
                                                          y_strat, y_data_label,
                                                          fig_format)
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

