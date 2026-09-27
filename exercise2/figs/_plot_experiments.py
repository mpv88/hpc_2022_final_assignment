import os
import glob
from platform import node
import pandas as pd
import matplotlib.pyplot as plt


TPP_PER_CORE = {
    'EPYC': {
        'float': 83.2,
        'double': 41.6,
    },
    'THIN': {
        'float': 147.2,
        'double': 73.6,
    },
}


def load_csv_files(results_dir):
    files = sorted(glob.glob(os.path.join(results_dir, '*.csv')))

    if not files:
        raise FileNotFoundError(f'no CSV files found in {results_dir}')

    data = []

    for file in files:
        frame = pd.read_csv(file)
        experiment = classify_experiment(file)
        frame['experiment'] = experiment
        data.append(frame)

    return pd.concat(data, ignore_index=True)


def classify_experiment(filename):
    name = os.path.basename(filename)

    if name.startswith('gemm_size_'):
        return 'size'

    if name.startswith('gemm_core_'):
        return 'core'

    raise ValueError(f'unknown experiment type: {name}')


def calculate_statistics(data):
    group_columns = ['experiment','library','precision','node','m','k','n','cores','threads','affinity']

    statistics = (data.groupby(group_columns, as_index=False).agg(
        mean_gflops=('gflops', 'mean'),
        std_gflops=('gflops', 'std'),
        mean_time_s=('time_s', 'mean'),
        std_time_s=('time_s', 'std'),
        repetitions=('gflops', 'count')
        )
    )
    return statistics


def calculate_tpp(node, precision, cores):
    return TPP_PER_CORE[node][precision] * cores


def plot_size_scalability(data, output_dir):
    size_data = data[data['experiment'] == 'size']

    for (node, precision, affinity), group in size_data.groupby(
            ['node', 'precision', 'affinity']):

        cores = group['cores'].iloc[0]
        fig, ax = plt.subplots(figsize=(8, 5))

        for library, library_data in group.groupby('library'):
            library_data = library_data.sort_values('m')

            ax.errorbar(
                library_data['m'],
                library_data['mean_gflops'],
                yerr=library_data['std_gflops'],
                marker='o',
                capsize=3,
                label=library.upper()
            )

        if node in TPP_PER_CORE:
            tpp = calculate_tpp(node, precision, cores)
            ax.axhline(tpp, linestyle='--', label=f'TPP ({tpp:.1f} GFLOPS)')

        ax.set_xlabel('Matrix size (M = N = K)')
        ax.set_ylabel('Performance (GFLOPS)')
        ax.set_title(f'GEMM Matrix-Size Scalability: {node} - {precision.upper()} - {affinity} - {cores} Cores')
        ax.legend()
        ax.grid(True, alpha=0.3)
        fig.tight_layout()

        filename = (f'size_{node}_{precision}_{affinity}_{cores}cores.png')
        fig.savefig(os.path.join(output_dir, filename), dpi=300)
        plt.close(fig)


def plot_core_scalability(data, output_dir):
    core_data = data[data['experiment'] == 'core']

    for (node, precision, m, k, n, affinity), group in core_data.groupby(['node', 'precision', 'm', 'k', 'n', 'affinity']):
        fig, ax = plt.subplots(figsize=(8, 5))

        for library, library_data in group.groupby('library'):
            library_data = library_data.sort_values('cores')

            ax.errorbar(
                library_data['cores'],
                library_data['mean_gflops'],
                yerr=library_data['std_gflops'],
                marker='o',
                capsize=3,
                label=library.upper()
            )

        if node in TPP_PER_CORE:
            cores = group['cores'].sort_values().unique()
            tpp = calculate_tpp(node, precision, cores)
            ax.plot(cores, tpp, linestyle='--', label='TPP')

        ax.set_xlabel('Number of cores')
        ax.set_ylabel('Performance (GFLOPS)')
        ax.set_title(f'GEMM Core Scalability: {node} - {precision.upper()} - {affinity} - M=N=K={int(m)}')
        ax.legend()
        ax.grid(True, alpha=0.3)
        fig.tight_layout()

        filename = (f'core_{node}_{precision}_{affinity}_{int(m)}.png')
        fig.savefig(os.path.join(output_dir, filename), dpi=300)
        plt.close(fig)


def plot_size_affinity_comparison(data, output_dir):
    size_data = data[data['experiment'] == 'size']

    for (node, precision), group in size_data.groupby(['node', 'precision']):
        affinities = group['affinity'].unique()

        if len(affinities) < 2:
            continue

        cores = group['cores'].iloc[0]
        fig, ax = plt.subplots(figsize=(8, 5))

        for (library, affinity), library_data in group.groupby(
                ['library', 'affinity']):

            library_data = library_data.sort_values('m')

            ax.errorbar(
                library_data['m'],
                library_data['mean_gflops'],
                yerr=library_data['std_gflops'],
                marker='o',
                capsize=3,
                label=f'{library.upper()} — {affinity}'
            )

        if node in TPP_PER_CORE:
            tpp = calculate_tpp(node, precision, cores)
            ax.axhline(tpp, linestyle='--', label=f'TPP ({tpp:.1f} GFLOPS)')

        ax.set_xlabel('Matrix size (M = K = N)')
        ax.set_ylabel('Performance (GFLOPS)')
        ax.set_title(f'GEMM Matrix-Size Scalability: {node} - {precision.upper()} - Affinity Comparison - {cores} Cores')
        ax.legend()
        ax.grid(True, alpha=0.3)
        fig.tight_layout()

        affinity_label = '_vs_'.join(sorted(affinities))
        filename = (f'size_{node}_{precision}_{affinity_label}_{cores}cores.png')
        fig.savefig(os.path.join(output_dir, filename), dpi=300)
        plt.close(fig)


def plot_core_affinity_comparison(data, output_dir):
    core_data = data[data['experiment'] == 'core']

    for (node, precision, m, k, n), group in core_data.groupby(['node', 'precision', 'm', 'k', 'n']):
        affinities = group['affinity'].unique()

        if len(affinities) < 2:
            continue

        fig, ax = plt.subplots(figsize=(8, 5))

        for (library, affinity), library_data in group.groupby(['library', 'affinity']):
            library_data = library_data.sort_values('cores')

            ax.errorbar(
                library_data['cores'],
                library_data['mean_gflops'],
                yerr=library_data['std_gflops'],
                marker='o',
                capsize=3,
                label=f'{library.upper()} — {affinity}'
            )

        if node in TPP_PER_CORE:
            cores = group['cores'].sort_values().unique()
            tpp = calculate_tpp(node, precision, cores)
            ax.plot(cores, tpp, linestyle='--', label=f'TPP ({TPP_PER_CORE[node][precision]:.1f} GFLOPS/core)')

        ax.set_xlabel('Number of cores')
        ax.set_ylabel('Performance (GFLOPS)')
        ax.set_title(f'GEMM Core Scalability: {node} - {precision.upper()} - Affinity Comparison - M=N=K={int(m)}')
        ax.legend()
        ax.grid(True, alpha=0.3)
        fig.tight_layout()

        affinity_label = '_vs_'.join(sorted(affinities))
        filename = (f'core_{node}_{precision}_{affinity_label}_{int(m)}.png')
        fig.savefig(os.path.join(output_dir, filename), dpi=300)
        plt.close(fig)


def plot_core_speedup(data, output_dir):
    core_data = data[data['experiment'] == 'core']

    for (node, precision, m, k, n, affinity), group in core_data.groupby(['node', 'precision', 'm', 'k', 'n', 'affinity']):
        fig, ax = plt.subplots(figsize=(8, 5))

        for library, library_data in group.groupby('library'):
            library_data = library_data.sort_values('cores')
            baseline = library_data[library_data['cores'] == 1]

            if baseline.empty:
                continue
            baseline_time = baseline['mean_time_s'].iloc[0]
            baseline_std = baseline['std_time_s'].iloc[0]
            speedup = baseline_time / library_data['mean_time_s']
            relative_error = ((baseline_std / baseline_time) ** 2 + (library_data['std_time_s'] / library_data['mean_time_s']) ** 2) ** 0.5
            speedup_std = speedup * relative_error

            ax.errorbar(
                library_data['cores'],
                speedup,
                yerr=speedup_std,
                marker='o',
                capsize=3,
                label=library.upper()
            )

        cores = group['cores'].sort_values().unique()

        ax.plot(cores, cores, linestyle='--', label='Ideal speedup')
        ax.set_xlabel('Number of cores')
        ax.set_ylabel('Speedup')
        ax.set_title(f'GEMM Core Scalability: Speedup - {node} - {precision.upper()} - {affinity} - M=N=K={int(m)}')
        ax.legend()
        ax.grid(True, alpha=0.3)
        fig.tight_layout()

        filename = (
            f'core_{node}_{precision}_{affinity}_{int(m)}_speedup.png'
        )
        fig.savefig(os.path.join(output_dir, filename), dpi=300)
        plt.close(fig)


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.dirname(script_dir)

    results_dir = os.path.join(project_dir, 'results')
    output_dir = script_dir

    data = load_csv_files(results_dir)
    statistics = calculate_statistics(data)

    print(f'loaded measurements: {len(data)}')
    print(f'loaded experiments: {data["experiment"].unique().tolist()}')
    print(f'statistical groups: {len(statistics)}')

    plot_size_scalability(statistics, output_dir)
    plot_core_scalability(statistics, output_dir)
    plot_core_speedup(statistics, output_dir)
    plot_size_affinity_comparison(statistics, output_dir)
    plot_core_affinity_comparison(statistics, output_dir)

    print(f'plots saved to: {output_dir}')


if __name__ == '__main__':
    main()