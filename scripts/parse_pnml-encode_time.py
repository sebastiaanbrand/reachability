import os
from pathlib import Path

look_for1 = "pnml-encode: Result symbolic LTS written to"
look_for2 = "real\t"

maxval = 6

input_folder = 'models/petrinets/static_ldds/sloan/maxval_{}/overapprox/'.format(maxval)
output_file = 'bench_data/pnml-encode/pnml_encode_time_ldd_overapprox_maxval_{}.csv'.format(maxval)

def write_header():
    with open(output_file, 'w') as f:
        f.write('benchmark, encode_time\n')

def write_line(data):
    with open(output_file, 'a') as f:
        f.write('{}, {}\n'.format(*data))

def parse_time(line):
    time_str = line.split()[1]
    time_str = time_str.split('m')
    time_min = float(time_str[0])
    time_sec = float(time_str[1][:-1])
    return time_min * 60 + time_sec

def parse_file(folder, filename):
    with open(folder + filename, 'r') as f:
        lines = f.readlines()
        wrote_dd = False
        for index, line in enumerate(lines):
            if (line.startswith(look_for1)):
                wrote_dd = True
            if (line.startswith(look_for2)):
                time = parse_time(line)
        if (not wrote_dd):
            time = 'DNF'
        data = [filename[:-4], time]
        write_line(data)

def process_data(folder):
    write_header()
    for filename in sorted(os.listdir(folder)):
        if (filename.endswith('.log')):
            parse_file(folder, filename)


if __name__ == '__main__':
    Path('bench_data/pnml-encode/').mkdir(parents=True, exist_ok=True)
    process_data(input_folder)