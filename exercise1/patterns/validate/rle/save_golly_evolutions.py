import golly as g
import os

# config (set PATTERN & STEPS to desired values)
PATTERN = 'blinker' #'blinker', 'glider', 'r_pentomino'
STEPS = 10 # periods: 2 for blinker, 4 for glider, 1103 for r-pentomino

# load & save gen 0
g.open(PATTERN + '_00000.rle')
g.save(f'{PATTERN}_00000.rle', 'rle')

# generate and save subsequent steps
for generation in range(1, STEPS + 1):
    g.run(1)

    filename = f'{PATTERN}_{generation:05d}.rle'
    g.save(filename, 'rle')

    g.show(f'saved generation {generation}: {filename}')

g.show('done')