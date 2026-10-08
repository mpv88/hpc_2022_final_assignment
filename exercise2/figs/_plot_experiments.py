import os
import glob
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


COLORS = {
    ('blis', 'spread'): '#4C78A8',
    ('blis', 'close'): '#9ECAE1',
    ('openblas', 'spread'): '#E45756',
    ('openblas', 'close'): '#F28E8E',
    ('mkl', 'spread'): '#7A5195',
    ('mkl', 'close'): '#B79AC8',
}


CORE_TICKS = {
    'THIN': [1, 2, 4, 6, 8, 10, 12],
    'EPYC': [1, 8, 16, 24, 32, 40, 48, 56, 64],
}


def load_csv_files(results_dir):
    files = sorted(glob.glob(os.path.join(results_dir, 'gemm_*.csv')))

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


def calculate_statistics(data, experiment):
    data = data[data['experiment'] == experiment].copy()

    group_columns = [
        'experiment', 'library', 'precision', 'architecture',
        'm', 'k', 'n', 'cores', 'threads', 'affinity',
        'memory_policy', 'numa_nodes'
    ]

    statistics = (
        data.groupby(group_columns, dropna=False, as_index=False)
        .agg(
            avg_time_s=('time_s', 'mean'),
            std_time_s=('time_s', 'std'),
            avg_gflops=('gflops', 'mean'),
            std_gflops=('gflops', 'std'),
            repetitions=('repetition', 'count'),
        )
    )

    return statistics


def calculate_tpp(architecture, precision, cores):
    return TPP_PER_CORE[architecture][precision] * cores


def get_color(library, affinity):
    return COLORS[(library.lower(), affinity)]


def plot_size_scalability(data, output_dir):
    for (architecture, precision, affinity), group in data.groupby(
            ['architecture', 'precision', 'affinity']):

        cores = group['cores'].iloc[0]
        fig, ax = plt.subplots(figsize=(8, 5))

        for library, library_data in group.groupby('library'):
            library_data = library_data.sort_values('m')
            color = get_color(library, affinity)

            ax.errorbar(
                library_data['m'],
                library_data['avg_gflops'],
                yerr=library_data['std_gflops'],
                color=color,
                linestyle='-',
                marker=None,
                linewidth=1.8,
                elinewidth=1.0,
                capsize=3,
                capthick=1.0,
                label=library.upper()
            )

        if architecture in TPP_PER_CORE:
            tpp = calculate_tpp(architecture, precision, cores)
            ax.axhline(
                tpp,
                color='black',
                linestyle='--',
                linewidth=1.2,
                label=f'TPP ({tpp:.1f} GFLOPS)'
            )

        ax.set_ylim(bottom=0)
        ax.set_xticks(range(2000, 20001, 2000))

        ax.set_xlabel('Matrix size (M = K = N)')
        ax.set_ylabel('Performance (GFLOPS)')
        ax.set_title(
            f'GEMM Matrix-Size Scalability: {architecture} - '
            f'{precision.upper()} - {affinity} - {cores} Cores'
        )
        ax.legend()
        ax.grid(True, alpha=0.3)
        fig.tight_layout()

        filename = (
            f'size_{architecture}_{precision}_{affinity}_{cores}cores.png'
        )
        fig.savefig(os.path.join(output_dir, filename), dpi=300)
        plt.close(fig)


def plot_core_scalability(data, output_dir):
    for (architecture, precision, m, k, n, affinity), group in data.groupby(
            ['architecture', 'precision', 'm', 'k', 'n', 'affinity']):

        fig, ax = plt.subplots(figsize=(8, 5))

        for library, library_data in group.groupby('library'):
            library_data = library_data.sort_values('cores')
            color = get_color(library, affinity)

            ax.errorbar(
                library_data['cores'],
                library_data['avg_gflops'],
                yerr=library_data['std_gflops'],
                color=color,
                linestyle='-',
                marker=None,
                linewidth=1.8,
                elinewidth=1.0,
                capsize=3,
                capthick=1.0,
                label=library.upper()
            )

        if architecture in TPP_PER_CORE:
            cores = group['cores'].sort_values().unique()
            tpp = calculate_tpp(architecture, precision, cores)

            ax.plot(
                cores,
                tpp,
                color='black',
                linestyle='--',
                linewidth=1.2,
                label=f'TPP ({TPP_PER_CORE[architecture][precision]:.1f} GFLOPS/core)'
            )

        ax.set_ylim(bottom=0)
        ax.set_xlim(left=0)
        ax.set_xticks(CORE_TICKS[architecture])

        ax.set_xlabel('Number of cores')
        ax.set_ylabel('Performance (GFLOPS)')
        ax.set_title(
            f'GEMM Core Scalability: {architecture} - '
            f'{precision.upper()} - {affinity} - '
            f'M=K=N={int(m)}'
        )
        ax.legend()
        ax.grid(True, alpha=0.3)
        fig.tight_layout()

        filename = (
            f'core_{architecture}_{precision}_{affinity}_{int(m)}.png'
        )
        fig.savefig(os.path.join(output_dir, filename), dpi=300)
        plt.close(fig)


def plot_size_affinity_comparison(data, output_dir):
    for (architecture, precision), group in data.groupby(
            ['architecture', 'precision']):

        affinities = group['affinity'].unique()

        if len(affinities) < 2:
            continue

        cores = group['cores'].iloc[0]
        fig, ax = plt.subplots(figsize=(8, 5))

        for (library, affinity), library_data in group.groupby(
                ['library', 'affinity']):

            library_data = library_data.sort_values('m')
            color = get_color(library, affinity)

            ax.errorbar(
                library_data['m'],
                library_data['avg_gflops'],
                yerr=library_data['std_gflops'],
                color=color,
                linestyle='-',
                marker=None,
                linewidth=1.8,
                elinewidth=1.0,
                capsize=3,
                capthick=1.0,
                label=f'{library.upper()} — {affinity}'
            )

        if architecture in TPP_PER_CORE:
            tpp = calculate_tpp(architecture, precision, cores)
            ax.axhline(
                tpp,
                color='black',
                linestyle='--',
                linewidth=1.2,
                label=f'TPP ({tpp:.1f} GFLOPS)'
            )

        ax.set_ylim(bottom=0)
        ax.set_xticks(range(2000, 20001, 2000))

        ax.set_xlabel('Matrix size (M = K = N)')
        ax.set_ylabel('Performance (GFLOPS)')
        ax.set_title(
            f'GEMM Matrix-Size Scalability: {architecture} - '
            f'{precision.upper()} - Affinity Comparison - {cores} Cores'
        )
        ax.legend()
        ax.grid(True, alpha=0.3)
        fig.tight_layout()

        affinity_label = '_vs_'.join(sorted(affinities))
        filename = (
            f'size_{architecture}_{precision}_{affinity_label}_{cores}cores.png'
        )
        fig.savefig(os.path.join(output_dir, filename), dpi=300)
        plt.close(fig)


def plot_core_affinity_comparison(data, output_dir):
    for (architecture, precision, m, k, n), group in data.groupby(
            ['architecture', 'precision', 'm', 'k', 'n']):

        affinities = group['affinity'].unique()

        if len(affinities) < 2:
            continue

        fig, ax = plt.subplots(figsize=(8, 5))

        for (library, affinity), library_data in group.groupby(
                ['library', 'affinity']):

            library_data = library_data.sort_values('cores')
            color = get_color(library, affinity)

            ax.errorbar(
                library_data['cores'],
                library_data['avg_gflops'],
                yerr=library_data['std_gflops'],
                color=color,
                linestyle='-',
                marker=None,
                linewidth=1.8,
                elinewidth=1.0,
                capsize=3,
                capthick=1.0,
                label=f'{library.upper()} — {affinity}'
            )

        if architecture in TPP_PER_CORE:
            cores = group['cores'].sort_values().unique()
            tpp = calculate_tpp(architecture, precision, cores)

            ax.plot(
                cores,
                tpp,
                color='black',
                linestyle='--',
                linewidth=1.2,
                label=f'TPP ({TPP_PER_CORE[architecture][precision]:.1f} GFLOPS/core)'
            )

        ax.set_ylim(bottom=0)
        ax.set_xlim(left=0)
        ax.set_xticks(CORE_TICKS[architecture])

        ax.set_xlabel('Number of cores')
        ax.set_ylabel('Performance (GFLOPS)')
        ax.set_title(
            f'GEMM Core Scalability: {architecture} - '
            f'{precision.upper()} - Affinity Comparison - '
            f'M=K=N={int(m)}'
        )
        ax.legend()
        ax.grid(True, alpha=0.3)
        fig.tight_layout()

        affinity_label = '_vs_'.join(sorted(affinities))
        filename = (
            f'core_{architecture}_{precision}_{affinity_label}_{int(m)}.png'
        )
        fig.savefig(os.path.join(output_dir, filename), dpi=300)
        plt.close(fig)


def plot_core_speedup(data, output_dir):
    for (architecture, precision, m, k, n, affinity), group in data.groupby(
            ['architecture', 'precision', 'm', 'k', 'n', 'affinity']):

        fig, ax = plt.subplots(figsize=(8, 5))

        for library, library_data in group.groupby('library'):
            library_data = library_data.sort_values('cores')
            baseline = library_data[library_data['cores'] == 1]

            if baseline.empty:
                continue

            baseline_time = baseline['avg_time_s'].iloc[0]
            baseline_std = baseline['std_time_s'].iloc[0]

            speedup = baseline_time / library_data['avg_time_s']

            relative_error = (
                (baseline_std / baseline_time) ** 2
                + (
                    library_data['std_time_s']
                    / library_data['avg_time_s']
                ) ** 2
            ) ** 0.5

            speedup_std = speedup * relative_error
            color = get_color(library, affinity)

            ax.errorbar(
                library_data['cores'],
                speedup,
                yerr=speedup_std,
                color=color,
                linestyle='-',
                marker=None,
                linewidth=1.8,
                elinewidth=1.0,
                capsize=3,
                capthick=1.0,
                label=library.upper()
            )

        cores = group['cores'].sort_values().unique()

        ax.plot(
            cores,
            cores,
            color='black',
            linestyle='--',
            linewidth=1.2,
            label='Ideal speedup'
        )

        ax.set_ylim(bottom=0)
        ax.set_xlim(left=0)
        ax.set_xticks(CORE_TICKS[architecture])

        ax.set_xlabel('Number of cores')
        ax.set_ylabel('Speedup')
        ax.set_title(
            f'GEMM Core Scalability: Speedup - {architecture} - '
            f'{precision.upper()} - {affinity} - M=K=N={int(m)}'
        )
        ax.legend()
        ax.grid(True, alpha=0.3)
        fig.tight_layout()

        filename = (
            f'core_{architecture}_{precision}_{affinity}_{int(m)}_speedup.png'
        )
        fig.savefig(os.path.join(output_dir, filename), dpi=300)
        plt.close(fig)


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.dirname(script_dir)

    results_dir = os.path.join(project_dir, 'results')
    output_dir = script_dir

    data = load_csv_files(results_dir)

    size_statistics = calculate_statistics(data, 'size')
    core_statistics = calculate_statistics(data, 'core')

    size_file = os.path.join(results_dir, 'aggregated_size.csv')
    core_file = os.path.join(results_dir, 'aggregated_core.csv')

    size_statistics.to_csv(size_file, index=False)
    core_statistics.to_csv(core_file, index=False)

    print(f'loaded measurements: {len(data)}')
    print(f'size measurements: {len(data[data["experiment"] == "size"])}')
    print(f'core measurements: {len(data[data["experiment"] == "core"])}')
    print(f'aggregated size groups: {len(size_statistics)}')
    print(f'aggregated core groups: {len(core_statistics)}')

    plot_size_scalability(size_statistics, output_dir)
    plot_core_scalability(core_statistics, output_dir)
    plot_core_speedup(core_statistics, output_dir)
    plot_size_affinity_comparison(size_statistics, output_dir)
    plot_core_affinity_comparison(core_statistics, output_dir)

    print(f'aggregated size data: {size_file}')
    print(f'aggregated core data: {core_file}')
    print(f'plots saved to: {output_dir}')


if __name__ == '__main__':
    main()