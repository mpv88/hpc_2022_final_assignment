#!/usr/bin/env python3

import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


CSV_COLUMNS = ['evolution', 'grid_width', 'grid_height', 'steps', 'mpi_tasks', 'omp_threads', 'repetition', 'read_time', 'evolution_time', 'write_time', 'total_time']

def parse_arguments():
    '''parse command-line arguments'''
    parser = argparse.ArgumentParser(
        description='plot Game of Life HPC benchmark results'
    )
    parser.add_argument(
        'csv_files', nargs='+', type=Path,
        help='benchmark CSV files'
    )
    parser.add_argument(
        '--node', choices=['THIN', 'EPYC'],
        help='node type; if omitted, infer it from the CSV path'
    )
    return parser.parse_args()

def detect_node(csv_files, explicit_node):
    '''determine the node name used in plot titles'''
    if explicit_node is not None:
        return explicit_node

    text = ' '.join(str(path).lower() for path in csv_files)

    if re.search(r'(^|[^a-z])thin([^a-z]|$)', text):
        return 'THIN'
    if re.search(r'(^|[^a-z])epyc([^a-z]|$)', text):
        return 'EPYC'

    raise ValueError(
        'could not determine the node type from the CSV path; '
        'use --node THIN or --node EPYC'
    )

def load_csv(csv_files):
    '''load and validate benchmark CSV files'''
    frames = []

    for path in csv_files:
        if not path.is_file():
            raise FileNotFoundError(f'CSV file not found: {path}')

        frame = pd.read_csv(path)
        missing = [column for column in CSV_COLUMNS if column not in frame.columns]

        if missing:
            raise ValueError(
                f'{path}: missing columns: {", ".join(missing)}'
            )

        frames.append(frame)

    data = pd.concat(frames, ignore_index=True)

    numeric_columns = ['grid_width', 'grid_height', 'steps', 'mpi_tasks', 'omp_threads', 'repetition', 'read_time', 'evolution_time', 'write_time', 'total_time']

    for column in numeric_columns:
        data[column] = pd.to_numeric(data[column], errors='raise')

    return data

def aggregate(data):
    '''calculate mean and standard deviation over repetitions'''
    grouping = ['evolution', 'grid_width', 'grid_height', 'steps', 'mpi_tasks', 'omp_threads']

    result = (
        data.groupby(grouping, as_index=False)['evolution_time']
        .agg(['mean', 'std', 'count'])
        .reset_index()
    )
    result['std'] = result['std'].fillna(0.0)
    return result

def experiment_name(csv_files):
    '''get the experiment name from the CSV parent directory'''
    parents = {path.parent.name for path in csv_files}
    return next(iter(parents)) if len(parents) == 1 else 'combined'

def output_directory(csv_files):
    '''return exercise1/figs/<experiment>'''
    results_dir = Path(__file__).resolve().parent
    exercise_dir = results_dir.parent
    directory = exercise_dir / 'figs' / experiment_name(csv_files)
    directory.mkdir(parents=True, exist_ok=True)
    return directory

def plot_mean_band(ax, frame, x_column, y_column, std_column, label):
    '''plot a mean curve and its standard-deviation band'''
    frame = frame.sort_values(x_column)

    ax.plot(
        frame[x_column], frame[y_column],
        marker='o', label=label
    )
    ax.fill_between(
        frame[x_column],
        frame[y_column] - frame[std_column],
        frame[y_column] + frame[std_column],
        alpha=0.15
    )

def processor_label(x_column):
    '''return the human-readable processor-axis label'''
    return 'OMP threads' if x_column == 'omp_threads' else 'MPI tasks'

def plot_speedup(data, node, directory, x_column, scaling_type):
    '''plot speedup and ideal linear scaling'''
    grouped = aggregate(data)
    fig, ax = plt.subplots()

    for evolution, frame in grouped.groupby('evolution'):
        frame = frame.sort_values(x_column).copy()
        baseline = frame.iloc[0]['mean']
        frame['speedup'] = baseline / frame['mean']
        frame['speedup_std'] = (
                frame['speedup'] * frame['std'] / frame['mean']
        )
        plot_mean_band(
            ax, frame, x_column, 'speedup', 'speedup_std', evolution
        )

    x_min = grouped[x_column].min()
    x_max = grouped[x_column].max()
    ax.plot(
        [x_min, x_max], [1, x_max / x_min],
        linestyle='--', label='ideal'
    )

    ax.set_title(f'{scaling_type} scaling speedup — {node}')
    ax.set_xlabel(processor_label(x_column))
    ax.set_ylabel('Speedup')
    ax.grid(True, alpha=0.3)
    ax.legend()

    save_figure(
        fig, directory,
        f'{scaling_type}_speedup_{x_column}_{node.lower()}.png'
    )

def plot_time(data, node, directory, x_column, scaling_type):
    '''plot execution time'''
    grouped = aggregate(data)
    fig, ax = plt.subplots()

    for evolution, frame in grouped.groupby('evolution'):
        frame = frame.rename(
            columns={'mean': 'time', 'std': 'time_std'}
        )
        plot_mean_band(
            ax, frame, x_column, 'time', 'time_std', evolution
        )

    ax.set_title(f'{scaling_type} scaling execution time — {node}')
    ax.set_xlabel(processor_label(x_column))
    ax.set_ylabel('Evolution time (s)')
    ax.grid(True, alpha=0.3)
    ax.legend()

    save_figure(
        fig, directory,
        f'{scaling_type}_time_{x_column}_{node.lower()}.png'
    )

def plot_efficiency(data, node, directory, x_column, scaling_type):
    '''plot parallel efficiency'''
    grouped = aggregate(data)
    fig, ax = plt.subplots()

    for evolution, frame in grouped.groupby('evolution'):
        frame = frame.sort_values(x_column).copy()
        baseline = frame.iloc[0]['mean']

        if scaling_type == 'strong':
            frame['efficiency'] = baseline / (frame['mean'] * frame[x_column])
            frame['efficiency_std'] = (
                    frame['efficiency'] * frame['std'] / frame['mean']
            )
        else:  # weak scaling
            frame['efficiency'] = baseline / frame['mean']
            frame['efficiency_std'] = (
                    frame['efficiency'] * frame['std'] / frame['mean']
            )

        plot_mean_band(
            ax, frame, x_column,
            'efficiency', 'efficiency_std', evolution
        )

    ax.axhline(1.0, linestyle='--', label='ideal')
    ax.set_title(f'{scaling_type} scaling efficiency — {node}')
    ax.set_xlabel(processor_label(x_column))
    ax.set_ylabel('Parallel efficiency')
    ax.set_ylim(bottom=0)
    ax.grid(True, alpha=0.3)
    ax.legend()

    save_figure(
        fig, directory,
        f'{scaling_type}_efficiency_{x_column}_{node.lower()}.png'
    )

def save_figure(fig, directory, filename):
    '''save and close a figure'''
    fig.tight_layout()
    fig.savefig(directory / filename, dpi=200, bbox_inches='tight')
    plt.close(fig)

def main():
    '''load results and generate plots'''
    args = parse_arguments()
    node = detect_node(args.csv_files, args.node)
    data = load_csv(args.csv_files)
    directory = output_directory(args.csv_files)
    experiment = experiment_name(args.csv_files).lower()

    if 'omp' in experiment:
        plot_speedup(data, node, directory, 'omp_threads', 'strong')
        plot_time(data, node, directory, 'omp_threads', 'strong')
        plot_efficiency(data, node, directory, 'omp_threads', 'strong')
    elif 'weak' in experiment:
        plot_time(data, node, directory, 'mpi_tasks', 'weak')
        plot_efficiency(data, node, directory, 'mpi_tasks', 'weak')
    elif 'strong' in experiment or 'mpi' in experiment:
        plot_speedup(data, node, directory, 'mpi_tasks', 'strong')
        plot_time(data, node, directory, 'mpi_tasks', 'strong')
        plot_efficiency(data, node, directory, 'mpi_tasks', 'strong')
    else:
        raise ValueError(
            f'could not determine experiment type from directory "{experiment}"'
            'use a folder name containing omp, strong, or weak'
        )

    print(f'plots saved to: {directory}')

if __name__ == '__main__':
    main()