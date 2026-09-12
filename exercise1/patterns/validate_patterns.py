#!/usr/bin/env python3

from pathlib import Path
import hashlib
import re
import sys

# config
GRID_WIDTH = 100
GRID_HEIGHT = 100
MAXVAL = 255
RLE_DIR = Path('validate/rle')
PGM_DIR = Path('validate/pgm')
OUTPUT_DIR = Path('output')
RLE_BACK_DIR = Path('output/rle_from_pgm')


def parse_rle(filename):
    '''read a golly conway life rle file and return live-cell coordinates'''
    text = filename.read_text()
    position_match = re.search(r'#CXRLE\s+Pos=(-?\d+),(-?\d+)', text)
    position_x, position_y = (int(position_match.group(1)), int(position_match.group(2))) if position_match else (0, 0)
    header_match = re.search(r'x\s*=\s*(\d+)\s*,\s*y\s*=\s*(\d+).*?\n', text)

    if not header_match:
        raise ValueError(f'no rle header found in {filename}')

    body = text[header_match.end():]
    body = '\n'.join(line for line in body.splitlines() if not line.startswith('#'))

    if '!' not in body:
        raise ValueError(f'no \'!\' terminator found in {filename}')

    body = body[:body.index('!')]
    live_cells = []
    x = y = 0
    run_count = ''

    for character in body:
        if character.isdigit():
            run_count += character
            continue

        count = int(run_count) if run_count else 1
        run_count = ''

        if character == 'o':
            live_cells.extend((position_x + x + i, position_y + y) for i in range(count))
            x += count
        elif character == 'b':
            x += count
        elif character == '$':
            y += count
            x = 0
        elif character not in ' \t\r\n':
            raise ValueError(f'unexpected rle character {character!r} in {filename}')

    return live_cells


def parse_pgm(filename):
    '''read a binary PGM (P5) file and return live-cell coordinates in grid space'''
    data = filename.read_bytes()

    # minimal P5 parser
    idx = 0
    def skip_whitespace():
        nonlocal idx
        while idx < len(data) and data[idx:idx+1] in (b' ', b'\t', b'\n', b'\r'):
            idx += 1

    def read_token():
        nonlocal idx
        skip_whitespace()
        if idx >= len(data):
            return None
        if data[idx:idx+1] == b'#':
            while idx < len(data) and data[idx:idx+1] != b'\n':
                idx += 1
            return read_token()
        token = b''
        while idx < len(data) and data[idx:idx+1] not in (b' ', b'\t', b'\n', b'\r'):
            token += data[idx:idx+1]
            idx += 1
        return token.decode('ascii')

    magic = read_token()
    if magic != 'P5':
        raise ValueError(f'{filename} is not a binary PGM (expected P5, got {magic!r})')

    width = int(read_token())
    height = int(read_token())
    maxval = int(read_token())

    if width != GRID_WIDTH or height != GRID_HEIGHT:
        raise ValueError(f'{filename} size {width}x{height} does not match expected {GRID_WIDTH}x{GRID_HEIGHT}')

    # pixel data starts now
    pixels = data[idx:]
    if len(pixels) < width * height:
        raise ValueError(f'{filename} has insufficient pixel data')

    live_cells = []
    for py in range(height):
        for px in range(width):
            val = pixels[py * width + px]
            if val == 255:
                grid_x = px - GRID_WIDTH // 2
                grid_y = py - GRID_HEIGHT // 2
                live_cells.append((grid_x, grid_y))

    return live_cells


def write_rle(filename, live_cells, pos_x=0, pos_y=0):
    '''write live-cell coordinates to a Golly-style .rle file'''
    if not live_cells:
        content = f'#CXRLE Pos={pos_x},{pos_y}\nx = 0, y = 0, rule = B3/S23\n!\n'
        filename.write_text(content)
        return

    min_x = min(x for x, y in live_cells)
    min_y = min(y for x, y in live_cells)
    normalized = [(x - min_x, y - min_y) for x, y in live_cells]

    max_x = max(x for x, y in normalized)
    max_y = max(y for x, y in normalized)
    grid = [[False] * (max_x + 1) for _ in range(max_y + 1)]
    for x, y in normalized:
        grid[y][x] = True

    lines = []
    for y in range(max_y + 1):
        row = grid[y]
        x = 0
        while x <= max_x:
            b_count = 0
            while x <= max_x and not row[x]:
                b_count += 1
                x += 1
            if b_count:
                lines.append(f"{b_count}b" if b_count > 1 else "b")

            o_count = 0
            while x <= max_x and row[x]:
                o_count += 1
                x += 1
            if o_count:
                lines.append(f"{o_count}o" if o_count > 1 else "o")

        lines.append("$")

    if lines and lines[-1] == "$":
        lines.pop()
    body = "".join(lines) + "!"

    width = max_x + 1
    height = max_y + 1
    header = f"#CXRLE Pos={pos_x},{pos_y}\nx = {width}, y = {height}, rule = B3/S23\n"

    filename.write_text(header + body + "\n")


def convert_rle_files():
    '''convert all rle files to pgm files'''
    if not RLE_DIR.is_dir():
        print(f'error: directory not found: {RLE_DIR}')
        sys.exit(1)

    PGM_DIR.mkdir(parents=True, exist_ok=True)
    rle_files = sorted(RLE_DIR.glob('*.rle'))

    if not rle_files:
        print(f'error: no rle files found in {RLE_DIR}')
        sys.exit(1)

    for rle_file in rle_files:
        live_cells = parse_rle(rle_file)
        image = bytearray(GRID_WIDTH * GRID_HEIGHT)

        for x, y in live_cells:
            pixel_x = x + GRID_WIDTH // 2
            pixel_y = y + GRID_HEIGHT // 2

            if not (0 <= pixel_x < GRID_WIDTH and 0 <= pixel_y < GRID_HEIGHT):
                raise ValueError(f'live cell ({x},{y}) lies outside the {GRID_WIDTH}x{GRID_HEIGHT} grid')

            image[pixel_y * GRID_WIDTH + pixel_x] = 255

        header = f'P5\n# generated by gol\n{GRID_WIDTH} {GRID_HEIGHT}\n{MAXVAL}\n'.encode('ascii')
        pgm_file = PGM_DIR / f'{rle_file.stem}.pgm'
        pgm_file.write_bytes(header + image)

        print(f'[converted] {rle_file.name} -> {pgm_file.name}')

    print(f'\nconverted {len(rle_files)} rle files')


def convert_pgm_files():
    '''convert all pgm files to rle files'''
    if not OUTPUT_DIR.is_dir():
        print(f'error: directory not found: {OUTPUT_DIR}')
        sys.exit(1)

    RLE_BACK_DIR.mkdir(parents=True, exist_ok=True)
    pgm_files = sorted(OUTPUT_DIR.glob('*.pgm'))

    if not pgm_files:
        print(f'error: no pgm files found in {OUTPUT_DIR}')
        sys.exit(1)

    for pgm_file in pgm_files:
        live_cells = parse_pgm(pgm_file)
        rle_file = RLE_BACK_DIR / f'{pgm_file.stem}.rle'
        write_rle(rle_file, live_cells)

        print(f'[converted] {pgm_file.name} -> {rle_file.name}')

    print(f'\nconverted {len(pgm_files)} pgm files')


def sha256(filename):
    '''return the sha-256 digest of a file'''
    digest = hashlib.sha256()

    with filename.open('rb') as file:
        for block in iter(lambda: file.read(1024 * 1024), b''):
            digest.update(block)

    return digest.hexdigest()


def check_output_files():
    '''compare reference pgm with output pgm using sha256'''
    if not PGM_DIR.is_dir():
        print(f'error: directory not found: {PGM_DIR}')
        sys.exit(1)

    if not OUTPUT_DIR.is_dir():
        print(f'error: directory not found: {OUTPUT_DIR}')
        sys.exit(1)

    pgm_files = sorted(PGM_DIR.glob('*.pgm'))

    if not pgm_files:
        print(f'error: no pgm files found in {PGM_DIR}')
        sys.exit(1)

    for pgm_file in pgm_files:
        output_file = OUTPUT_DIR / pgm_file.name

        if not output_file.exists():
            print(f'[fail] {pgm_file.stem}: output file not found')
            sys.exit(1)

        reference_hash = sha256(pgm_file)
        output_hash = sha256(output_file)

        if reference_hash != output_hash:
            print(f'[fail] {pgm_file.stem}')
            print(f'       reference: {reference_hash}')
            print(f'       output:    {output_hash}')
            sys.exit(1)

        print(f'[ok] {pgm_file.stem}')

    print(f'\nsuccess: all {len(pgm_files)} files match')


if __name__ == "__main__":
    convert_pgm_files()
    #convert_rle_files()
    #check_output_files()