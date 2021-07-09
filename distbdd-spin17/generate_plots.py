import numpy as np
import pandas as pd
import pathlib
import matplotlib.pyplot as plt

fig_formats = ['png', 'pdf']
plots_folder = 'plots/'

vanilla_data = pd.read_csv('beem_vanilla_stats.csv')
ga_data      = pd.read_csv('beem_ga_stats.csv')
lable2data = {'vanilla' : vanilla_data, 'ga' : ga_data}
stratIDs = {'bfs' : 0,
            'sat' : 2,
            'rec' : 4}

def pre_process():
    # make plots folder if necessary
    pathlib.Path(plots_folder).mkdir(parents=True, exist_ok=True)

    # strip whitespace from column names
    vanilla_data.columns = vanilla_data.columns.str.strip()
    ga_data.columns      = ga_data.columns.str.strip()

"""
strat in {'bfs', 'sat', 'rec'}
data in {'vanilla', 'ga'}
"""
def plot_comparison(x_strat, x_data_label, y_strat, y_data_label):
    # get the relevant data
    x_data = lable2data[x_data_label]
    y_data = lable2data[y_data_label]

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
    fig, ax = plt.subplots()
    ax.scatter(xs, ys, s=10)

    # diagonal line
    max_val = max(np.max(xs), np.max(ys))
    min_val = min(np.min(xs), np.min(ys)) # use minval instead of bc of log axes
    ax.plot([min_val, max_val], [min_val, max_val], ls="--", c="gray")

    # labels and formatting
    ax.set_xlabel('time (s) - {} on {} BDDs'.format(x_strat, x_data_label))
    ax.set_ylabel('time (s) - {} on {} BDDs'.format(y_strat, y_data_label))
    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_title('reachability runtime comparison')

    for fig_format in fig_formats:
        fig_name = '{}reachtime_{}_{}_vs_{}_{}.{}'.format(plots_folder,
                                                          x_strat, x_data_label,
                                                          y_strat, y_data_label,
                                                          fig_format)
        fig.savefig(fig_name, dpi=300)


if __name__ == '__main__':
    pre_process()
    plot_comparison('bfs', 'vanilla', 'sat', 'vanilla')
    plot_comparison('bfs', 'ga',      'sat', 'ga')
    plot_comparison('bfs', 'vanilla', 'rec', 'vanilla')
    plot_comparison('bfs', 'ga',      'rec', 'ga')
    plot_comparison('sat', 'vanilla', 'rec', 'vanilla')
    plot_comparison('sat', 'ga',      'rec', 'ga')
    plot_comparison('bfs', 'vanilla', 'bfs', 'ga')
    plot_comparison('sat', 'vanilla', 'sat', 'ga')
    plot_comparison('rec', 'vanilla', 'rec', 'ga')

